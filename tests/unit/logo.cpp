#include "game/logo.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
struct Calls final : sadx::logo::Calls {
    sadx::logo::State &state;
    std::vector<unsigned> displayed;
    unsigned object_frames = 0, releases = 0;
    bool fail_release = false;
    explicit Calls(sadx::logo::State &state) : state(state) {}
    void run_objects() override { ++object_frames; }
    void display(std::uint8_t first_texture) override { displayed.push_back(first_texture); }
    void release_textures() override {
        require(state.mode == sadx::logo::Mode::Inactive,
                "mode store precedes texture release");
        require(state.logic_entry_flag == 1, "logic flag clear follows release");
        ++releases;
        if (fail_release)
            throw std::runtime_error("release stopped");
        state.demo_stage = 2;
    }
};
void sequence() {
    using namespace sadx::logo;
    State state{Mode::SegaHold, 0, 0, 0};
    Calls calls(state);
    for (unsigned frame = 0; frame < 386; ++frame) {
        require(advance(state, calls) == Completion::Running, "logo sequence still running");
        require(calls.displayed.back() == (frame < 193 ? 0U : 12U),
                "first and second logos retain their complete display intervals");
    }
    require(state.mode == Mode::Release && state.frames == 0,
            "second fade queues release for the next frame");
    require(advance(state, calls) == Completion::Title,
            "completion reads the demo stage changed by release");
    require(calls.object_frames == 387 && calls.displayed.size() == 386 && calls.releases == 1,
            "whole sequence preserves object, draw and release order");
    require(state.logic_entry_flag == 0 && state.mode == Mode::Inactive,
            "release completes the state transition");
    require(advance(state, calls) == Completion::Running && calls.object_frames == 388 &&
                calls.displayed.size() == 386,
            "inactive logo still runs objects without drawing");
}
void failure_prefix() {
    using namespace sadx::logo;
    State state{Mode::Release, 0, 1, 6};
    Calls calls(state);
    calls.fail_release = true;
    bool stopped = false;
    try {
        (void)advance(state, calls);
    } catch (const std::runtime_error &error) {
        stopped = std::string(error.what()) == "release stopped";
    }
    require(stopped && state.mode == Mode::Inactive && state.logic_entry_flag == 1 &&
                state.demo_stage == 6,
            "failure keeps the source store prefix without inventing completion");
}
} // namespace
int main() {
    sequence();
    failure_prefix();
    std::cout << "Native logo update: complete 387-frame sequence and release failure prefix.\n";
}

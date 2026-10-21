#include "game/save_frame.hpp"
#include <algorithm>
#include <array>
#include <vector>
namespace {
struct Stop {};
struct Fixture final : sadx::save::FrameCalls {
    std::uint8_t ready, count, exec, pending;
    std::int32_t phase;
    std::uint32_t pause;
    sadx::menu_caption::Context message;
    const std::uint32_t *input;
    std::vector<std::array<std::uint32_t, 7>> trace;
    explicit Fixture(const std::uint32_t *v)
        : ready(v[1]), count(v[2]), exec(v[4]), pending(v[5]),
          phase(static_cast<std::int32_t>(v[3])), pause(v[6]), input(v) {}
    sadx::save::FrameStateView view() {
        return {ready, count, phase, exec, pending, pause, message};
    }
    void event(std::uint32_t id) {
        trace.push_back(
            {id, ready, count, static_cast<std::uint32_t>(phase), exec, pending, pause});
        if (trace.size() == input[7])
            throw Stop{};
        if ((input[8] & (1U << (trace.size() - 1))) != 0) {
            const auto *values = input + 16 + (trace.size() - 1) * 6;
            ready = static_cast<std::uint8_t>(values[0]);
            count = static_cast<std::uint8_t>(values[1]);
            phase = static_cast<std::int32_t>(values[2]);
            exec = static_cast<std::uint8_t>(values[3]);
            pending = static_cast<std::uint8_t>(values[4]);
            pause = values[5];
        }
    }
    void display_saving_message(sadx::menu_caption::Context &value) override {
        if (&value != &message)
            throw Stop{};
        event(1);
    }
    void close_saving_message(sadx::menu_caption::Context &value) override {
        if (&value != &message)
            throw Stop{};
        event(2);
    }
    void write_save_file() override { event(3); }
};
} // namespace
extern "C" int sadx_native_save_frame(const std::uint32_t *input, std::uint32_t *output) {
    Fixture fixture(input);
    std::fill_n(output, 80, 0);
    bool failed = false;
    try {
        if (input[0] != 1)
            sadx::save::before_frame_completion(fixture.view(), fixture);
        if (input[0] == 2)
            fixture.event(4); // Actual411d00 caller boundary, not a frame-timing implementation.
        if (input[0] != 0)
            sadx::save::after_frame_completion(fixture.view(), fixture);
    } catch (const Stop &) {
        failed = true;
    }
    output[0] = failed;
    output[1] = static_cast<std::uint32_t>(fixture.trace.size());
    const auto state = fixture.view();
    const std::uint32_t values[] = {
        state.ready, state.count,           static_cast<std::uint32_t>(state.now_saving),
        state.exec,  state.pending_message, state.pause_state};
    std::copy_n(values, 6, output + 2);
    for (std::size_t i = 0; i < fixture.trace.size(); ++i)
        std::copy(fixture.trace[i].begin(), fixture.trace[i].end(), output + 16 + i * 7);
    return 1;
}

#include "game/logo_startup.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace {
struct Stop {};
struct Calls final : sadx::logo::StartupCalls {
    sadx::logo::StartupState &state;
    const std::uint32_t *input;
    std::uint32_t *output;
    unsigned count = 0;
    Calls(sadx::logo::StartupState &state, const std::uint32_t *input, std::uint32_t *output)
        : state(state), input(input), output(output) {}
    std::array<std::uint32_t, 7> values() const {
        return {static_cast<std::uint32_t>(state.logo.mode),
                static_cast<std::uint32_t>(state.logo.frames), state.logo.logic_entry_flag,
                state.logo.demo_stage, static_cast<std::uint32_t>(state.demo_mode),
                static_cast<std::uint32_t>(state.demo_event),
                static_cast<std::uint32_t>(state.demo_reset)};
    }
    void call(unsigned kind, unsigned a = 0, unsigned b = 0, unsigned c = 0) {
        auto *row = output + 9 + count * 11;
        row[0] = kind;
        row[1] = a;
        row[2] = b;
        row[3] = c;
        const auto snapshot = values();
        std::copy(snapshot.begin(), snapshot.end(), row + 4);
        ++count;
        if (input[8] == count)
            throw Stop{};
        if (input[9] == count) {
            state.logo.mode = static_cast<sadx::logo::Mode>(input[10]);
            state.logo.frames = static_cast<std::int32_t>(input[11]);
            state.logo.logic_entry_flag = input[12];
            state.logo.demo_stage = input[13];
            state.demo_mode = static_cast<std::int32_t>(input[14]);
            state.demo_event = static_cast<std::int32_t>(input[15]);
            state.demo_reset = static_cast<std::int32_t>(input[16]);
        }
    }
    void initialize_heap() override { call(1); }
    void initialize_tasks() override { call(2); }
    void set_character_selection(std::int32_t value) override { call(3, value); }
    void set_background(unsigned a, unsigned b, unsigned c) override { call(4, a, b, c); }
    void load_textures(std::string_view name) override {
        if (name != "SEGALOGO_E")
            throw std::runtime_error("incorrect logo resource");
        call(5);
    }
    void set_frame_rate(std::int32_t a, std::int32_t b) override { call(6, a, b); }
    void set_wait_vsync(std::int32_t value) override { call(7, value); }
    void set_fade_color(unsigned value) override { call(8, value); }
    void initialize_texture_drawing() override { call(9); }
    void initialize_save_data() override { call(10); }
};
} // namespace

extern "C" void sadx_native_logo_startup(const std::uint32_t *input, std::uint32_t *output) {
    std::fill_n(output, 130, 0);
    sadx::logo::State logo{static_cast<sadx::logo::Mode>(input[0]),
                           static_cast<std::int32_t>(input[1]), input[2], input[3]};
    auto demo_mode = static_cast<std::int32_t>(input[4]);
    auto demo_event = static_cast<std::int32_t>(input[5]);
    auto demo_reset = static_cast<std::int32_t>(input[6]);
    sadx::logo::StartupState state{logo, demo_mode, demo_event, demo_reset};
    Calls calls(state, input, output);
    try {
        if (input[7])
            sadx::logo::initialize_sega(state, calls);
        else
            sadx::logo::load_sega_textures(logo, calls);
        output[7] = 1;
    } catch (Stop) {
    }
    const auto snapshot = calls.values();
    std::copy(snapshot.begin(), snapshot.end(), output);
    output[8] = calls.count;
}

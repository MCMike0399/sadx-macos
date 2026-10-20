#include "game/logo.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace {
struct Stop {};
struct Calls final : sadx::logo::Calls {
    sadx::logo::State &state;
    const std::uint32_t *input;
    std::uint32_t *output;
    std::uint32_t count = 0;
    Calls(sadx::logo::State &state, const std::uint32_t *input, std::uint32_t *output)
        : state(state), input(input), output(output) {}
    void call(std::uint32_t kind, std::uint32_t argument = 0) {
        output[6 + count * 6] = kind;
        output[7 + count * 6] = argument;
        output[8 + count * 6] = static_cast<std::uint32_t>(state.mode);
        output[9 + count * 6] = static_cast<std::uint32_t>(state.frames);
        output[10 + count * 6] = state.logic_entry_flag;
        output[11 + count * 6] = state.demo_stage;
        ++count;
        if (input[4] == count)
            throw Stop{};
        if (kind == 1 && input[5]) {
            state.mode = static_cast<sadx::logo::Mode>(input[6]);
            state.frames = static_cast<std::int32_t>(input[7]);
        }
        if (kind == 3 && input[8])
            state.demo_stage = input[9];
    }
    void run_objects() override { call(1); }
    void display(std::uint8_t first) override { call(2, first); }
    void release_textures() override { call(3); }
};
} // namespace
extern "C" void sadx_native_logo(const std::uint32_t *input, std::uint32_t *output) {
    std::fill_n(output, 24, 0);
    sadx::logo::State state{static_cast<sadx::logo::Mode>(input[0]),
                            static_cast<std::int32_t>(input[1]), input[2], input[3]};
    Calls calls(state, input, output);
    output[4] = 99;
    try {
        output[4] = static_cast<std::uint32_t>(sadx::logo::advance(state, calls));
    } catch (Stop) {
    }
    output[0] = static_cast<std::uint32_t>(state.mode);
    output[1] = static_cast<std::uint32_t>(state.frames);
    output[2] = state.logic_entry_flag;
    output[3] = state.demo_stage;
    output[5] = calls.count;
}

#include "player/gamma_input_buffer.hpp"
#include <algorithm>
#include <array>
#include <cstring>

namespace {
struct Stop {};
struct Memory final : sadx::gamma_input_buffer::Access {
    const std::uint32_t *input;
    std::uint32_t *output;
    unsigned step = 0;
    explicit Memory(const std::uint32_t *input, std::uint32_t *output)
        : input(input), output(output) {
        std::copy_n(input + 8, 1024, output);
    }
    void access() {
        ++step;
        output[1025] = step;
        if (input[3] == step && input[4] < 1024)
            output[input[4]] = input[5];
        if (input[2] == step) {
            output[1024] = step;
            throw Stop{};
        }
    }
    std::uint32_t &word(std::uint32_t index) {
        if (index >= 1024) {
            output[1024] = step;
            throw Stop{};
        }
        return output[index];
    }
    std::uint32_t destination(std::int32_t player, unsigned field) {
        // Reference-only original pointer arithmetic, including 32-bit wrap.
        return (0x7a0U + std::uint32_t(player) * 8U + field * 4U) / 4U;
    }
    std::int32_t buffered_angle() override {
        access();
        const auto bits = word(input[1]);
        std::int32_t result;
        std::memcpy(&result, &bits, sizeof(result));
        return result;
    }
    void store_angle(std::int32_t player, std::int32_t angle) override {
        access();
        word(destination(player, 0)) = static_cast<std::uint32_t>(angle);
    }
    std::uint32_t buffered_stroke_bits() override {
        access();
        return word(input[1] + 1);
    }
    void store_stroke_bits(std::int32_t player, std::uint32_t bits) override {
        access();
        word(destination(player, 1)) = bits;
    }
};
} // namespace
extern "C" int sadx_native_gamma_input_buffer(const std::uint32_t *input, std::uint32_t *output) {
    std::fill_n(output, 1026, 0);
    Memory memory(input, output);
    try {
        sadx::gamma_input_buffer::restore(memory, static_cast<std::int32_t>(input[0]));
    } catch (const Stop &) {
    }
    return 1;
}

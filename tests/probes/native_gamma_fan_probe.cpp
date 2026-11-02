#include "math/ninja_math.hpp"
#include "math/ninja_sine_table.hpp"
#include "player/gamma_fan.hpp"
#include <algorithm>
#include <cstring>

namespace {
struct Stop {};
struct Memory final : sadx::gamma_fan::Access {
    const std::uint32_t *input;
    std::uint32_t *output;
    sadx::X87Context &arithmetic;
    sadx::NinjaMath math;
    unsigned step = 0;
    Memory(const std::uint32_t *input, std::uint32_t *output, sadx::X87Context &arithmetic)
        : input(input), output(output), arithmetic(arithmetic),
          math(sadx::ninja_sine_table(), arithmetic) {
        std::copy_n(input + 8, 1024, output);
    }
    void access() {
        output[1025] = ++step;
        if (input[4] == step && input[5] < 1024)
            output[input[5]] = input[6];
        if (input[3] == step) {
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
    float speed(unsigned component) {
        access();
        const auto bits = word(input[1] + 14 + component);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    float lateral_speed() override { return speed(2); }
    float forward_speed() override { return speed(0); }
    std::uint32_t phase() override {
        access();
        return word(input[2] + 7);
    }
    void store_phase(std::uint32_t value) override {
        access();
        word(input[2] + 7) = value;
    }
    sadx::fidelity::ExactFloat sine(std::int32_t angle) override {
        access();
        return sadx::fidelity::ExactFloat::retained(arithmetic, math.sin_extended(angle));
    }
    void store_height(float value) override {
        access();
        std::uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        word(input[2] + 6) = bits;
    }
    void store_tilt(std::int32_t value) override {
        access();
        word(input[2] + 10) = static_cast<std::uint32_t>(value);
    }
};
} // namespace
extern "C" int sadx_native_gamma_fan(const std::uint32_t *input, std::uint32_t *output) {
    std::fill_n(output, 1027, 0);
    sadx::X87Context arithmetic(static_cast<std::uint16_t>(input[0]));
    Memory memory(input, output, arithmetic);
    try {
        sadx::gamma_fan::update(memory, arithmetic);
    } catch (const Stop &) {
    }
    output[1026] = arithmetic.exception_flags();
    return 1;
}

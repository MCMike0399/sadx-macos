#include "cpu/fidelity_float.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {
using sadx::X87Comparison;
using sadx::X87Context;
using sadx::fidelity::ExactFloat;

void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
float number(std::uint32_t bits) {
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

void retained_precision() {
    // A real binary32 spill loses the unit; a retained 53/64-bit result keeps it.
    for (const auto control : {0x007f, 0x027f, 0x037f}) {
        X87Context fp(control);
        const auto retained = ExactFloat(fp, 16777216.0f) + 1.0f;
        const auto result = (retained - 16777216.0f).rounded();
        require(result == (control == 0x007f ? 0.0f : 1.0f),
                "retained expression obeys selected precision without a hidden binary32 spill");
        const auto base = ExactFloat(fp, 16777216.0f);
        const auto unit = ExactFloat(fp, 1.0f);
        require(((base + unit) - base).rounded() == result,
                "loaded-operand expressions retain the same precision without a hidden spill");
        const auto spilled = retained.rounded();
        require((ExactFloat(fp, spilled) - 16777216.0f).rounded() == 0.0f,
                "explicit binary32 spill is observably different at wider precision");
    }
    // The retained exponent range stays wider even with precision control set to 24 bits.
    for (const auto precision : {0x0000, 0x0200, 0x0300}) {
        X87Context fp(precision | 0x7f);
        const auto value = ExactFloat(fp, std::numeric_limits<float>::max()) * 2.0f;
        require(fp.exception_flags() == 0 &&
                    (value / 2.0f).rounded() == std::numeric_limits<float>::max(),
                "retained overflow outside binary32 range must recover before spilling");
        require(std::isinf(value.rounded()) && (fp.exception_flags() & 0x28U) == 0x28U,
                "explicit overflowing spill records overflow and precision");
    }
}

void rounding_controls() {
    const float next = std::nextafter(1.0f, 2.0f);
    for (const auto precision : {0x0000, 0x0200, 0x0300}) {
        for (const auto rounding : {0x0000, 0x0400, 0x0800, 0x0c00}) {
            X87Context fp(precision | rounding | 0x7f);
            const auto positive = (ExactFloat(fp, 1.0f) + 0x1p-24f).rounded();
            const auto negative = (ExactFloat(fp, -1.0f) - 0x1p-24f).rounded();
            require(positive == (rounding == 0x0800 ? next : 1.0f),
                    "positive halfway result uses caller rounding control");
            require(negative == (rounding == 0x0400 ? -next : -1.0f),
                    "negative halfway result uses caller rounding control");
            require(fp.control_word() == (precision | rounding | 0x7f),
                    "value operations must preserve caller control word");
        }
    }
}

void memory_operand_identity() {
    require(bits(sadx::fidelity::absolute_float(number(0xff812345))) == 0x7f812345,
            "binary32 absolute value clears sign without quieting signaling NaN");
    require(bits(sadx::fidelity::absolute_float(-0.0f)) == 0,
            "binary32 absolute value clears negative zero sign");
    X87Context fp(0x037f);
    const auto quiet = ExactFloat(fp, number(0xffc00000));
    const float signaling = number(0x7f812345);
    require(bits((quiet + signaling).rounded()) == 0xffc00000 && (fp.exception_flags() & 1U) != 0,
            "memory signaling NaN keeps the existing quiet NaN and records invalid");
    fp.clear_exceptions();
    const auto loaded = ExactFloat(fp, signaling);
    require(bits((quiet + loaded).rounded()) == 0x7fc12345,
            "explicitly loaded signaling NaN selects its larger quiet payload");
    fp.clear_exceptions();
    require(bits((quiet * std::numeric_limits<float>::denorm_min()).rounded()) == 0xffc00000 &&
                fp.exception_flags() == 0,
            "quiet register NaN suppresses denormal memory-operand handling");
    require(quiet.compare(1.0f) == X87Comparison::unordered && (fp.exception_flags() & 1U) != 0,
            "FCOM-style unordered comparison records invalid for a quiet NaN");
}

void reverse_operands_and_registers() {
    X87Context fp(0x037f);
    const auto value = ExactFloat(fp, 2.0f);
    require((10.0f - value).rounded() == 8.0f && (10.0f / value).rounded() == 5.0f,
            "left memory subtraction and division retain operand order");
    require((value - 10.0f).rounded() == -8.0f && (value / 8.0f).rounded() == 0.25f,
            "right memory subtraction and division retain operand order");
    const auto other = ExactFloat(fp, 4.0f);
    require((value + other).rounded() == 6.0f && (value - other).rounded() == -2.0f &&
                (value * other).rounded() == 8.0f && (value / other).rounded() == 0.5f,
            "loaded operands support exact arithmetic without converting to float");
    require(value.compare(other) == X87Comparison::less &&
                value.compare(2.0f) == X87Comparison::equal &&
                other.compare(value) == X87Comparison::greater,
            "memory and loaded comparisons retain their ordered relation");
    require(bits((-ExactFloat(fp, 0.0f)).rounded()) == 0x80000000U,
            "sign change retains signed zero");
}

void explicit_failures() {
    X87Context first(0x037f), second(0x007f);
    const auto a = ExactFloat(first, 1.0f), b = ExactFloat(second, 2.0f);
    bool rejected = false;
    try {
        (void)(a + b);
    } catch (const std::logic_error &) {
        rejected = true;
    }
    require(rejected && first.exception_flags() == 0 && second.exception_flags() == 0,
            "mixed contexts fail before arithmetic or flag mutation");

    X87Context unmasked(0x0377); // Overflow unmasked, all other exception masks retained.
    const auto wide = ExactFloat(unmasked, std::numeric_limits<float>::max()) * 2.0f;
    float output = 123.0f;
    rejected = false;
    try {
        output = wide.rounded();
    } catch (const sadx::X87Error &error) {
        rejected = error.failure() == sadx::X87Failure::unmasked_exception &&
                   (error.exceptions() & 8U) != 0;
    }
    require(rejected && output == 123.0f,
            "unmasked store conversion stops before its destination changes");
}

static_assert(!std::is_convertible_v<ExactFloat, float>);
static_assert(!std::is_convertible_v<ExactFloat, double>);
static_assert(!std::is_constructible_v<ExactFloat, X87Context &, double>);
} // namespace

int main() {
    try {
        retained_precision();
        rounding_controls();
        memory_operand_identity();
        reverse_operands_and_registers();
        explicit_failures();
        std::cout << "Exact float: retained precision/exponent, 12 controls, memory NaNs, explicit "
                     "spills, reverse operands and failure prefixes passed.\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

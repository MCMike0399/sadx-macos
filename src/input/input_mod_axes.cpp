#include "input/input_mod_axes.hpp"
#include <algorithm>

namespace sadx {
namespace {
// NOTSA: decode one coefficient word without host byte-order or float conversion.
std::uint32_t read_coefficient_word(const std::uint8_t *bytes) {
    return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
           (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}

// NOTSA: widen before negation so the signed16 minimum has magnitude 32768.
std::int32_t absolute_axis(std::int16_t value) {
    const auto widened = static_cast<std::int32_t>(value);
    return widened < 0 ? -widened : widened;
}

// NOTSA: the source clips only the asymmetric signed16 minimum before measuring.
std::int32_t symmetric_axis(std::int16_t value) {
    return std::max(-32767, static_cast<std::int32_t>(value));
}

// NOTSA: retain the low word after CVTT, including integer-indefinite and wrap.
std::int16_t signed_low_word(std::int32_t value) {
    const auto low = static_cast<std::uint32_t>(value) & 65535U;
    const auto signed_value = low < 32768U ? static_cast<std::int32_t>(low)
                                         : static_cast<std::int32_t>(low) - 65536;
    return static_cast<std::int16_t>(signed_value);
}

struct Direction {
    Sse32 length;
    Sse32 x;
    Sse32 y;
};

// NOTSA: shared ordered length/direction calculation in the two recovered paths.
Direction measure_direction(InputModRawAxes raw, Sse32 threshold, SseContext &arithmetic) {
    const auto x = arithmetic.from_int32(symmetric_axis(raw.x));
    const auto y = arithmetic.from_int32(symmetric_axis(raw.y));
    const auto squared_y = arithmetic.multiply(y, y);
    const auto squared_x = arithmetic.multiply(x, x);
    const auto squared_length = arithmetic.add(squared_y, squared_x);
    const auto wide_squared_length = arithmetic.to_double(squared_length);
    // Signed16 inputs cannot reach the negative-sum CRT fallback. Preserve the
    // comparison status before the binary64 root and binary32 narrowing.
    (void)arithmetic.ucomisd(Sse64{0}, wide_squared_length);
    const auto length = arithmetic.to_single(arithmetic.sqrt(wide_squared_length));
    const auto comparison = arithmetic.comiss(threshold, length);
    Sse32 direction_x{0};
    Sse32 direction_y{0};
    if (comparison != SseComparison::greater) {
        direction_x = arithmetic.divide(x, length);
        direction_y = arithmetic.divide(y, length);
    }
    return {length, direction_x, direction_y};
}
} // namespace

std::optional<InputModAxesConstants>
decode_input_mod_axes_constants(const std::uint8_t *bytes, std::size_t size) {
    if (!bytes || size != 20)
        return std::nullopt;
    return InputModAxesConstants{{read_coefficient_word(bytes)},
                                {read_coefficient_word(bytes + 4)},
                                {read_coefficient_word(bytes + 8)},
                                {read_coefficient_word(bytes + 12)},
                                {read_coefficient_word(bytes + 16)}};
}

void convert_input_mod_261_axes(InputModRawAxes raw, std::int16_t deadzone, bool radial,
                                const InputModAxesConstants &constants, SseContext &arithmetic,
                                InputModAxesOutput &output) {
    if (absolute_axis(raw.x) < deadzone && absolute_axis(raw.y) < deadzone) {
        output.x = 0;
        output.y = 0;
        output.magnitude = Sse32{0};
        return;
    }
    const auto threshold = arithmetic.from_int32(deadzone);
    const auto direction = measure_direction(raw, threshold, arithmetic);
    const auto comparison = arithmetic.comiss(constants.maximum, direction.length);
    // COMISS/CMOVBE selects maximum for less/equal/unordered; MINSS differs.
    const auto limited_length =
        comparison == SseComparison::greater ? direction.length : constants.maximum;
    const auto remaining_length = arithmetic.subtract(limited_length, threshold);
    const auto available_range = arithmetic.from_int32(32767 - std::int32_t(deadzone));
    const auto magnitude = arithmetic.divide(remaining_length, available_range);
    const auto component = [&](Sse32 axis) {
        const auto directed = arithmetic.multiply(magnitude, axis);
        const auto scaled = arithmetic.multiply(directed, constants.controller_scale);
        const auto converted = signed_low_word(arithmetic.truncate_int32(scaled));
        return static_cast<std::int16_t>(std::clamp(static_cast<std::int32_t>(converted), -127, 127));
    };
    output.x = !radial && absolute_axis(raw.x) < deadzone ? std::int16_t{0} : component(direction.x);
    output.y = !radial && absolute_axis(raw.y) < deadzone ? std::int16_t{0} : component(direction.y);
    output.magnitude = magnitude;
}

void normalize_input_mod_261_keyboard(InputModRawAxes left, InputModRawAxes right,
                                     const InputModAxesConstants &constants, SseContext &arithmetic,
                                     InputModAxesOutput &left_output,
                                     InputModAxesOutput &right_output) {
    const auto left_direction = measure_direction(left, constants.epsilon, arithmetic);
    auto left_magnitude = arithmetic.divide(left_direction.length, constants.maximum);
    const auto scaled_left_y = arithmetic.multiply(left_direction.y, constants.keyboard_scale);
    left_magnitude = arithmetic.minimum(left_magnitude, constants.magnitude_cap);
    const auto scaled_left_x = arithmetic.multiply(left_direction.x, constants.keyboard_scale);
    // These three publications are deliberately separate: an X conversion fault
    // retains the already committed Y and magnitude, including aliased outputs.
    left_output.y = signed_low_word(arithmetic.truncate_int32(scaled_left_y));
    left_output.magnitude = left_magnitude;
    left_output.x = signed_low_word(arithmetic.truncate_int32(scaled_left_x));

    const auto right_direction = measure_direction(right, constants.epsilon, arithmetic);
    auto right_magnitude = arithmetic.divide(right_direction.length, constants.maximum);
    const auto scaled_right_x = arithmetic.multiply(right_direction.x, constants.keyboard_scale);
    const auto scaled_right_y = arithmetic.multiply(right_direction.y, constants.keyboard_scale);
    right_magnitude = arithmetic.minimum(right_magnitude, constants.magnitude_cap);
    // Both right conversions precede its first store, unlike the left path.
    const auto right_x = signed_low_word(arithmetic.truncate_int32(scaled_right_x));
    const auto right_y = signed_low_word(arithmetic.truncate_int32(scaled_right_y));
    right_output.x = right_x;
    right_output.magnitude = right_magnitude;
    right_output.y = right_y;
}
} // namespace sadx

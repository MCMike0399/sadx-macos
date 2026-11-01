#pragma once
#include "cpu/sse_math.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sadx {
struct InputModRawAxes {
    std::int16_t x;
    std::int16_t y;
};

// NOTSA: caller-supplied numeric content; this value owns no DLL memory.
struct InputModAxesConstants {
    Sse32 epsilon;
    Sse32 maximum;
    Sse32 keyboard_scale;
    Sse32 controller_scale;
    Sse32 magnitude_cap;
};

// NOTSA: five little-endian binary32 words, in declaration order, at the byte boundary.
std::optional<InputModAxesConstants> decode_input_mod_axes_constants(const std::uint8_t *bytes,
                                                                  std::size_t size);

// Disengaged fields have not been published. Unmasked failures retain earlier
// stores and leave later fields untouched, including previously engaged values.
struct InputModAxesOutput {
    std::optional<std::int16_t> x;
    std::optional<std::int16_t> y;
    std::optional<Sse32> magnitude;
};

// Deadzone is signed16; singular and negative settings keep their original arithmetic.
// FUNCTION: INPUT 0x10004400 convert_axes
void convert_input_mod_261_axes(InputModRawAxes raw, std::int16_t deadzone, bool radial,
                                const InputModAxesConstants &constants, SseContext &arithmetic,
                                InputModAxesOutput &output);

// Excludes event/ramp ownership, cursor, half-press, recenter and controller arbitration.
// NOTSA: the two scalar keyboard normalization slices share one native operation.
void normalize_input_mod_261_keyboard(InputModRawAxes left, InputModRawAxes right,
                                     const InputModAxesConstants &constants, SseContext &arithmetic,
                                     InputModAxesOutput &left_output,
                                     InputModAxesOutput &right_output);
} // namespace sadx

#pragma once
#include "cpu/sse_math.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
namespace sadx {
// Raw signed16 SDL/keyboard ramp values, before any native normalization.
struct InputModRawAxes { std::int16_t x, y; };
struct InputModAxesConstants {
    Sse32 epsilon; // DLL 0x1000bb6c
    Sse32 maximum; // DLL 0x1000bbb0
    Sse32 keyboard_scale; // DLL 0x1000bba0
    Sse32 controller_scale; // DLL 0x1000bba4
    Sse32 magnitude_cap; // DLL 0x1000bb7c
};
// Five little-endian f32 words in the declaration order; requires caller bytes.
std::optional<InputModAxesConstants> decode_input_mod_axes_constants(const std::uint8_t*,std::size_t);
struct InputModAxesOutput {
    std::optional<std::int16_t> x, y;
    std::optional<Sse32> magnitude;
};
// Actual convert_axes helper 0x10004400..45df: deadzone is signed16 in ABI.
// Earlier stores survive an unmasked arithmetic exception. No hidden deadzone.
void convert_input_mod_261_axes(InputModRawAxes, std::int16_t deadzone, bool radial,
    const InputModAxesConstants&, SseContext&, InputModAxesOutput&);
// Pure scalar keyboard normalization in poll routine, for both sticks. This
// excludes the Windows raw event/ramp owner, cursor, half-press, recenter and
// controller-priority phases. Earlier source stores survive later failures.
void normalize_input_mod_261_keyboard(InputModRawAxes left, InputModRawAxes right,
    const InputModAxesConstants&, SseContext&, InputModAxesOutput& left_output,
    InputModAxesOutput& right_output);
} // namespace sadx

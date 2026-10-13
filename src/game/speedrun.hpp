#pragma once

#include <cstdint>
#include <optional>

namespace sadx::speedrun {

// Original SDK/Dreamcast masks, not SDL's button enumeration.
inline constexpr std::uint32_t button_a = 0x4;
inline constexpr std::uint32_t button_start = 0x8;
inline constexpr std::uint32_t camera_reset_bits = 0x80000004;

// Source-level equivalent of CCEF at 0x434870 and 0x438330. The owning
// camera operation supplies its full flags; preserve all existing bits.
std::uint32_t reset_camera_flags(std::uint32_t existing_flags) noexcept;

struct SkipCheck {
    bool requested;
    std::uint32_t pressed_buttons;
};

// Equivalent to the Skipma check inside the original in-game skip path.
// The owning cutscene state machine must still enforce skip eligibility.
// The pressed Start bit is set even when Start is not held, as in the hook.
SkipCheck in_game_skip_check(std::uint32_t held_buttons,
                             std::uint32_t pressed_buttons) noexcept;
bool pre_rendered_skip_check(std::uint32_t held_buttons) noexcept;

// Speedrun 2.0 (sadx-speedrun-mod.dll SHA f3fc68a3...) frame-limiter math,
// from REA Ghidra Evidence ev_9c9c4f7b... (+0x10f0) and ev_5fc7b7a3... (+0x11c0).
// The cadence is the DLL global at +0x1001001c (60 in the file, 63 when the
// profile's FrameLimit is "63FPS").
inline constexpr std::int32_t cadence_63fps = 63;

// +0x11c0, used by the 411d1c hook: wrapped FrameMultiplier-1, floored at 0.
// INT_MIN-1 wraps to INT_MAX exactly as the 32-bit source does.
std::int32_t extra_game_ticks(std::int32_t frame_multiplier) noexcept;

// +0x10f0 interval: low64(sext(FrameMultiplier) * frequency) then __alldiv by
// sext(cadence), truncating toward zero. Division by zero faults in the
// source and is nullopt here; INT64_MIN / -1 wraps like the MSVC helper.
std::optional<std::int64_t> limiter_interval(std::int32_t frame_multiplier,
    std::int64_t frequency, std::int32_t cadence) noexcept;

// The +0x10f0 wait ends when the wrapped signed 64-bit (now-last) reaches the
// interval (signed high-word / unsigned low-word compare).
bool limiter_elapsed(std::int64_t now, std::int64_t last, std::int64_t interval) noexcept;

} // namespace sadx::speedrun

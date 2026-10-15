#pragma once
#include <functional>
#include <cstdint>
#include <optional>

namespace sadx {
enum class GameFrameControlFailure {none,unknown_frequency_high,unknown_frequency_low,
    unknown_render_factor,unknown_flags,unknown_save_phase,unknown_edge,unknown_mask};
struct GameFrameControlResult {
    GameFrameControlFailure failure=GameFrameControlFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    bool completed=false;
    explicit operator bool()const noexcept{return failure==GameFrameControlFailure::none&&completed;}
};
struct GameFrameInputState {
    std::optional<std::uint8_t>& reset_key_edge; // borrowed from the caller's input owner
    // GLOBAL: SONIC 0x3b0eaa0 SoftResetByte size=1
    std::optional<std::uint8_t> soft_reset_flags;
    // GLOBAL: SONIC 0x3b0e3b8 SoftResetInputMask size=1
    std::optional<std::uint8_t> reset_input_mask;
    std::optional<std::int32_t>& save_phase; // borrowed from the caller's save owner
};
using GameFrameInputCallback=GameFrameControlResult(*)(GameFrameInputState&);
struct FrameRateModeState {
    // GLOBAL: SONIC 0x3b11188 FrameMultiplier size=4
    std::optional<std::int32_t> frame_multiplier;
    // GLOBAL: SONIC 0x3b11178 loop_const size=4
    std::optional<std::int32_t> logic_increment;
    // GLOBAL: SONIC 0x3b1117c loop_count size=4
    std::optional<std::int32_t> pending_logic_ticks;
};
// Source LARGE_INTEGER words may be independently unavailable in reference
// fixtures. Keep those facts separate without exposing a source memory address.
struct CounterWords {std::optional<std::uint32_t> low,high;};
struct GameFrameRendererState {
    // GLOBAL: SONIC 0x3d08538 PerformanceFrequency size=8
    CounterWords counter_frequency;
    // GLOBAL: SONIC 0x389d7dc FrameIntervalMultiplier size=4
    std::optional<std::int32_t> frame_interval_multiplier;
    // GLOBAL: SONIC 0x89295c framerate_setting size=4
    std::optional<std::int32_t> configured_frame_interval;
    // GLOBAL: SONIC 0x3d084a8 FrameFrequency size=8
    CounterWords frame_interval;
    // Speedrun 2.0 limiter reference counter (DLL pointer +0x10010058 -> 3d08550).
    // GLOBAL: SONIC 0x3d08550 PerformanceCounter size=8
    std::optional<std::int64_t> previous_counter;
    // GLOBAL: SONIC 0x3d0fa34 FrameInputCallback size=4
    std::optional<GameFrameInputCallback> input_callback;
};
// Exact7899a0/77ed40 low64 multiply followed by signed64 division by60.
// Reads both frequency words BEFORE writing FrameIntervalMultiplier; no host clock or
// captured Windows frequency is silently selected for the native runtime.
GameFrameControlResult set_frame_interval_multiplier(std::int32_t,GameFrameRendererState&);
// Actual411e30. The input callback is a real native implementation of411e20,
// whose40f010 input-byte owner remains explicit about absent live facts.
GameFrameControlResult set_frame_rate_mode(std::int32_t,std::int32_t,
    FrameRateModeState&,GameFrameRendererState&);
GameFrameControlResult update_soft_reset_flags(GameFrameInputState&); //40f010
GameFrameControlResult run_frame_input(GameFrameInputState&); //411e20
// Speedrun 2.0 7899e0 hook (DLL +0x10f0): QPF into 3d08538/3c, interval from
// 3b11188 and the DLL cadence, spin on QPC until (now-last) >= interval, then
// last=now (no deadline accumulation). QPF failure stores one QPC into last.
// The clock is the caller's native counter owner; no host clock is implied.
struct SpeedrunLimiterClock {
    std::function<std::optional<std::int64_t>()> frequency; // QueryPerformanceFrequency
    std::function<std::optional<std::int64_t>()> counter;   // QueryPerformanceCounter
    // Optional host hint before re-reading the counter (remaining ticks > 0).
    // It may sleep; it changes only the number of counter reads, not stores.
    std::function<void(std::int64_t)> wait_hint={};
};
enum class SpeedrunLimiterFailure {none,missing_clock,counter_unavailable,unknown_frame_multiplier,
    divide_fault,unknown_last_counter};
struct SpeedrunLimiterResult {
    SpeedrunLimiterFailure failure=SpeedrunLimiterFailure::none;
    std::uint32_t source_boundary=0; // DLL VA
    std::optional<std::int64_t> interval;
    std::uint64_t counter_reads=0;
    bool frequency_available=false,completed=false;
    explicit operator bool()const noexcept{return failure==SpeedrunLimiterFailure::none&&completed;}
};
SpeedrunLimiterResult run_speedrun_frame_limiter(GameFrameRendererState&,const FrameRateModeState&,std::int32_t cadence,const SpeedrunLimiterClock&);
} // namespace sadx

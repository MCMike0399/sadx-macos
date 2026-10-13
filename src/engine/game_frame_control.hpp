#pragma once
#include <functional>
#include "engine/game_logic_support.hpp"

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
    GameLogicEdgeState& edges;
    std::optional<std::uint8_t> flags_at_3b0eaa0,mask_at_3b0e3b8;
    std::optional<std::int32_t>& save_phase_at_3b291a4; // borrowed: GameLoopState owns 3b291a4
};
using GameFrameInputCallback=GameFrameControlResult(*)(GameFrameInputState&);
struct GameFrameRendererState {
    std::optional<std::uint32_t> frequency_high_at_3d0853c,frequency_low_at_3d08538;
    std::optional<std::int32_t> frame_multiplier_at_389d7dc,render_factor_at_89295c;
    std::optional<std::uint32_t> interval_low_at_3d084a8,interval_high_at_3d084ac;
    // Speedrun 2.0 limiter reference counter (DLL pointer +0x10010058 -> 3d08550).
    std::optional<std::int64_t> last_counter_at_3d08550;
    std::optional<GameFrameInputCallback> input_callback_at_3d0fa34;
};
// Exact7899a0/77ed40 low64 multiply followed by signed64 division by60.
// Reads both frequency words BEFORE writing FrameMultiplier; no host clock or
// captured Windows frequency is silently selected for the native runtime.
GameFrameControlResult set_game_frame_multiplier(std::int32_t,GameFrameRendererState&);
// Actual411e30. The input callback is a real native implementation of411e20,
// whose40f010 input-byte owner remains explicit about absent live facts.
GameFrameControlResult set_game_logic_mode(std::int32_t,std::int32_t,
    GameLogicTimingState&,GameFrameRendererState&);
GameFrameControlResult update_game_frame_input_flags(GameFrameInputState&); //40f010
GameFrameControlResult run_game_frame_input(GameFrameInputState&); //411e20
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
SpeedrunLimiterResult run_speedrun_frame_limiter(GameFrameRendererState&,const GameLogicTimingState&,std::int32_t cadence,const SpeedrunLimiterClock&);
} // namespace sadx

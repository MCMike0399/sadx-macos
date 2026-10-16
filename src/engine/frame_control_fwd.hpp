#pragma once
#include "engine/game_frame_control.hpp"

namespace sadx {
// NOTSA: legacy ABI declarations only. Reference facts are defined by the portable adapter.
struct GameFrameInputState;
struct FrameRateModeState;
struct GameFrameRendererState;
struct GameFrameControlResult;
struct SpeedrunLimiterResult;
using GameFrameInputCallback = GameFrameControlResult (*)(GameFrameInputState &);
using SpeedrunLimiterClock = frame::LimiterClock;
GameFrameControlResult set_frame_interval_multiplier(std::int32_t, GameFrameRendererState &);
GameFrameControlResult set_frame_rate_mode(std::int32_t, std::int32_t, FrameRateModeState &,
                                           GameFrameRendererState &);
GameFrameControlResult update_soft_reset_flags(GameFrameInputState &);
GameFrameControlResult run_frame_input(GameFrameInputState &);
SpeedrunLimiterResult run_speedrun_frame_limiter(GameFrameRendererState &,
                                                 const FrameRateModeState &, std::int32_t,
                                                 const SpeedrunLimiterClock &);
} // namespace sadx

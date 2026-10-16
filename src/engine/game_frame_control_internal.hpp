#pragma once
#include "engine/game_frame_control.hpp"

namespace sadx::frame {
enum class ModeInterval { Normal, Double };
// NOTSA: typed state seams keep fixture availability and diagnostics outside game decisions.
std::int64_t counter_frequency(const RendererState &);
void store_counter_frequency(RendererState &, std::int64_t);
void store_interval_multiplier(RendererState &, std::int32_t);
void store_frame_interval(RendererState &, std::int64_t);
std::int32_t configured_frame_interval(const RendererState &, ModeInterval);
void apply_mode_interval(RendererState &, std::int32_t, ModeInterval);
void store_frame_multiplier(ModeState &, std::int32_t);
void store_logic_increment(ModeState &, std::int32_t);
void clear_pending_logic_ticks(ModeState &);
void store_input_callback(RendererState &, InputCallback);
std::uint8_t soft_reset_flags(const InputState &);
void store_soft_reset_flags(InputState &, std::uint8_t);
std::int32_t save_phase(const InputState &);
std::uint8_t reset_key_edge(const InputState &);
std::uint8_t reset_input_mask(const InputState &);
std::int32_t frame_multiplier(const ModeState &);
std::int64_t previous_counter(const RendererState &);
void store_previous_counter(RendererState &, std::int64_t);
// NOTSA: diagnostic progress observation; native implementation has no gameplay effect.
void report_limiter_progress(const LimiterResult &);
} // namespace sadx::frame

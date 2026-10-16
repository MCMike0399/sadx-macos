#include "engine/game_frame_control_internal.hpp"
namespace sadx::frame {
std::int64_t counter_frequency(const RendererState &s) { return s.counter_frequency; }
void store_counter_frequency(RendererState &s, std::int64_t v) { s.counter_frequency = v; }
void store_interval_multiplier(RendererState &s, std::int32_t v) {
    s.frame_interval_multiplier = v;
}
void store_frame_interval(RendererState &s, std::int64_t v) { s.frame_interval = v; }
std::int32_t configured_frame_interval(const RendererState &s, ModeInterval) {
    return s.configured_frame_interval;
}
void apply_mode_interval(RendererState &s, std::int32_t v, ModeInterval) {
    set_frame_interval_multiplier(v, s);
}
void store_frame_multiplier(ModeState &s, std::int32_t v) { s.frame_multiplier = v; }
void store_logic_increment(ModeState &s, std::int32_t v) { s.logic_increment = v; }
void clear_pending_logic_ticks(ModeState &s) { s.pending_logic_ticks = 0; }
void store_input_callback(RendererState &s, InputCallback callback) { s.input_callback = callback; }
std::uint8_t soft_reset_flags(const InputState &s) { return s.soft_reset_flags; }
void store_soft_reset_flags(InputState &s, std::uint8_t v) { s.soft_reset_flags = v; }
std::int32_t save_phase(const InputState &s) { return s.save_phase; }
std::uint8_t reset_key_edge(const InputState &s) { return s.reset_key_edge; }
std::uint8_t reset_input_mask(const InputState &s) { return s.reset_input_mask; }
std::int32_t frame_multiplier(const ModeState &s) { return s.frame_multiplier; }
std::int64_t previous_counter(const RendererState &s) { return s.previous_counter; }
void store_previous_counter(RendererState &s, std::int64_t v) { s.previous_counter = v; }
void report_limiter_progress(const LimiterResult &) {}
} // namespace sadx::frame

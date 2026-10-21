#include "engine/frame_loop_native.hpp"
#include "engine/speedrun.hpp"
#include <limits>

namespace sadx::frame_loop {
void complete_counter_state(State &state, const frame::ModeState &mode,
                             const frame::RendererState &renderer) {
    std::int32_t elapsed;
    if (state.resync != 0) {
        elapsed = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(renderer.configured_frame_interval) - 1U);
        state.resync = 0;
    } else {
        elapsed = speedrun::extra_game_ticks(mode.frame_multiplier);
    }
    const auto starting = state.loop_starting;
    state.elapsed_ticks = elapsed;
    state.previous_loop_starting = starting;
    state.loop_starting = 0;
}

TickFailure calculate_ticks(TickState &state) {
    if (state.loop_starting == 0) {
        state.mode.pending_logic_ticks = 0;
        state.loop_count = 1;
        return TickFailure::None;
    }
    const auto dividend = state.elapsed_ticks;
    const auto divisor = state.mode.frame_multiplier;
    if (divisor == 0) {
        return TickFailure::DivideByZero;
    }
    if (dividend == std::numeric_limits<std::int32_t>::min() && divisor == -1) {
        return TickFailure::DivideOverflow;
    }
    const auto quotient = dividend / divisor;
    const auto incremented = static_cast<std::uint32_t>(quotient) + 1U;
    const auto product = static_cast<std::int32_t>(
        incremented * static_cast<std::uint32_t>(state.mode.logic_increment));
    state.mode.pending_logic_ticks = product;
    state.loop_count = static_cast<std::uint32_t>(product);
    if (product > 6 && state.demo_playing == 0) {
        state.mode.pending_logic_ticks = 6;
        state.loop_count = 6;
    }
    return TickFailure::None;
}

} // namespace sadx::frame_loop

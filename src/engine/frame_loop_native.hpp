#pragma once
#include "engine/game_frame_control.hpp"
#include <cstdint>

namespace sadx::frame_loop {
// NOTSA: source zero-filled counter owners, shared by frame completion and menu/game logic.
struct State {
    std::uint8_t resync = 0;
    std::int32_t elapsed_ticks = 0;
    std::int32_t loop_starting = 0;
    std::int32_t previous_loop_starting = 0;
};
enum class TickFailure { None, DivideByZero, DivideOverflow };
struct TickState {
    frame::ModeState &mode;
    std::uint32_t &loop_count;
    const std::int32_t &elapsed_ticks;
    const std::int32_t &loop_starting;
    const std::int32_t &demo_playing;
};
// NOTSA: scalar phase of frame completion; rendering and pacing remain caller-owned.
void complete_counter_state(State &, const frame::ModeState &, const frame::RendererState &);
// FUNCTION: SONIC 0x413920 calcvsyncsyoriochi
TickFailure calculate_ticks(TickState &);
} // namespace sadx::frame_loop

#pragma once

#include "player/floor_angles.hpp"
#include "player/player_input.hpp"
#include "player/player_speed.hpp"
#include "player/player_task.hpp"

namespace sadx {

struct PlayerMovementResult {
    PlayerSpeedResult speed;
    std::uint32_t input_query_count = 0;
    std::optional<PlayerInputResult> input;
    std::optional<CoordinateResult> coordinates;
    std::optional<FloorAnglesResult> floor_angles;
    explicit operator bool() const noexcept { return static_cast<bool>(speed); }
};

// Binds actual recovered GetAnalog and coordinate code to the recovered speed
// routine. This consumes an explicit normalized-input snapshot; it does not
// manufacture original WriteAnalogs direction/magnitude, update the complete
// Sonic state machine, advance collision/position, or run a game tick.
PlayerMovementResult run_player_movement(
    const PlayerTaskState&, const PlayerInputRuntime&, MotionWork&, PlayerPhysicsState&,
    const PlayerParameters&, PlayerCoordinates&, const FloorAngleConstants&,
    const CrtAsinRuntime&, X87Context&);

} // namespace sadx

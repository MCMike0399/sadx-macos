#pragma once

#include "player/player_acceleration.hpp"
#include "player/player_rotation.hpp"

namespace sadx {

struct AirAccelerationConstants {
    float zero; // Source memory operand 0x7dcb2c, not an implicit state default.
};
struct AirAccelerationCallees {
    std::function<PlayerAccelerationResult(PlayerTaskState&,PlayerMotionState&,
        PlayerPhysicsState&,const PlayerParameters&,X87Context&)> event_acceleration; // 0x447b90
    std::function<CoordinateResult(const PlayerOrientation&,NinjaVector&,X87Context&)> world_to_player;
    std::function<PlayerRotationResult(PlayerTaskState&,PlayerMotionState&,
        PlayerPhysicsState&,std::int32_t,X87Context&)> yaw_regular; // 0x443c50
};
struct AirAccelerationResult {
    PlayerAccelerationResult acceleration;
    std::optional<PlayerRotationResult> rotation;
    explicit operator bool() const noexcept { return static_cast<bool>(acceleration); }
};

// Source 0x44b9c0..0x44bb54. Gravity/drag, real requested analog outputs,
// target-yaw stores, braking/control-gain selection and final acceleration
// stores occur in source order. Missing callees/facts retain earlier mutations.
// This does not advance position/collision or a complete character/game tick.
AirAccelerationResult get_player_acceleration_air(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,const PlayerParameters&,
    const AirAccelerationConstants&,const PlayerAccelerationRuntime&,
    const AirAccelerationCallees&,X87Context&);

} // namespace sadx

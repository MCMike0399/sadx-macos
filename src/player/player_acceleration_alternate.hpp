#pragma once

#include "player/player_acceleration_ground.hpp"
#include "player/player_projection.hpp"

namespace sadx {

struct EventAccelerationConstants {
    float target_stop_distance; // binary32 source0x7dcb10
    double angle_units;         // binary64 source0x7dccd0
    double positive_factor;     // binary64 source0x7dccc8
};
struct AlternateSurfaceRuntime {
    std::array<std::optional<std::array<std::int16_t,2>>,8> raw_axes;
    // Source camera task angles.x/y; captured through the camera's native owner.
    std::optional<std::array<std::int32_t,2>> camera_pitch_yaw;
};

// Actual surface-attribute0x4000 routine0x447010. Unlike ordinary ground
// acceleration, the source does not transform gravity before querying input,
// uses rush_speed for its longitudinal cap, and has distinct friction/store
// behavior. Required yaw callees use the same mutable task/physics owners.
PlayerAccelerationResult get_player_acceleration_attribute_4000(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,const PlayerParameters&,
    const GroundAccelerationConstants&,const PlayerAccelerationRuntime&,
    const GroundAccelerationCallees&,X87Context&);

// Event-ground0x447510 and event-air0x447b90 share mode dispatch, but retain
// their different target-speed/step, drag/friction and final-store semantics.
PlayerAccelerationResult get_player_acceleration_event_ground(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,const PlayerParameters&,
    const GroundAccelerationConstants&,const EventAccelerationConstants&,
    const PlayerAccelerationRuntime&,const GroundAccelerationCallees&,X87Context&);
PlayerAccelerationResult get_player_acceleration_event_air(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,const PlayerParameters&,
    const GroundAccelerationConstants&,const EventAccelerationConstants&,
    const PlayerAccelerationRuntime&,const GroundAccelerationCallees&,X87Context&);
PlayerAccelerationResult get_player_acceleration_attribute_20000(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,const PlayerParameters&,
    const GroundAccelerationConstants&,const EventAccelerationConstants&,
    const PlayerAccelerationRuntime&,const AlternateSurfaceRuntime&,
    const GroundAccelerationCallees&,PlayerCoordinates&,const CoordinateAxisConstants&,
    const ProjectionAngleConstants&,const CrtAsinRuntime&,X87Context&);

} // namespace sadx

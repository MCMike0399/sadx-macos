#pragma once

#include "player/player_coordinates.hpp"
#include "player/player_input.hpp"
#include "player/player_state.hpp"
#include "player/player_task.hpp"
#include "player/player_physics.hpp"
#include "player/player_projection.hpp"
#include "player/player_rotation_result.hpp"
#include "player/player_direction_reset.hpp"

namespace sadx {

struct PlayerMotionState {
    MotionWork motion;
    std::int32_t target_yaw; // motionwk2 +0x20
    float source_field_34;  // observed float output; semantic SDK name unestablished
    float source_field_38;  // friction spill, distinct from speed.x (+0x04)
    float friction_scale;  // motionwk2 +0x3c
    std::optional<std::int32_t> angle_x_at_1c = {},angle_z_at_24 = {};
};
struct PlayerAccelerationRuntime {
    std::optional<NinjaVector> world_gravity;
    PlayerInputRuntime input;
    ForcedInputAngleConversion forced_angle;
};
enum class PlayerAccelerationFailure {
    none, unavailable_gravity, missing_world_to_player, coordinate_unavailable,
    input_unavailable, unavailable_collision_vector, missing_callee, callee_unavailable,
    unavailable_event_mode, unavailable_event_vector, unavailable_event_target_speed,
    unavailable_event_acceleration_step, unavailable_task_position, unavailable_floor_normal,
    unavailable_raw_axes, unavailable_camera, missing_projection_owner, projection_unavailable,
    input_outputs_unavailable,
};
struct PlayerAccelerationResult {
    PlayerAccelerationFailure failure = PlayerAccelerationFailure::none;
    std::uint32_t source_boundary = 0;
    std::uint32_t callee_address = 0;
    std::optional<NinjaVector> temporary_acceleration = {};
    PlayerInputFailure input_failure = PlayerInputFailure::none;
    CoordinateFailure coordinate_failure = CoordinateFailure::none;
    PlayerAccelerationFailure callee_failure = PlayerAccelerationFailure::none;
    std::uint32_t nested_source_boundary = 0;
    std::optional<ProjectionAngleResult> projection = {};
    std::optional<FloorProjectionResult> floor_projection = {};
    std::optional<PlayerRotationResult> rotation = {};
    std::optional<PlayerDirectionResetResult> direction_reset = {};
    explicit operator bool() const noexcept { return failure == PlayerAccelerationFailure::none; }
};
} // namespace sadx

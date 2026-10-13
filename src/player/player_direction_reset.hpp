#pragma once
#include "player/player_task.hpp"
#include "player/player_physics.hpp"
#include "player/player_state.hpp"
#include "player/player_path.hpp"
#include "player/player_projection.hpp"
namespace sadx {
struct PlayerMotionState;
class PlayerCoordinates;
struct PlayerDirectionResetConstants {
    PathQueryConstants path;
    CrtFloorConstants floor;
    ProjectionAngleConstants angle;
    double plane_distance_scale; // source0x7e0b40
};
std::optional<PlayerDirectionResetConstants> decode_player_direction_reset_constants(const std::uint8_t*,std::size_t);
struct PlayerDirectionResetRuntime {
    PlayerInputRuntime input;
    ForcedInputAngleConversion forced_angle;
    CrtFloorRuntime floor;
    CrtAsinRuntime inverse;
};
enum class PlayerDirectionResetFailure { none, input_unavailable, unavailable_distance,
    unavailable_path_resource, null_path_resource, path_query_failed, unavailable_contact_work,
    unavailable_contact_direction, unavailable_task_position, unavailable_path_position,
    vector_angle_failed };
struct PlayerDirectionResetResult {
    PlayerDirectionResetFailure failure=PlayerDirectionResetFailure::none;
    std::uint32_t source_boundary=0;
    PlayerInputFailure input_failure=PlayerInputFailure::none;
    std::uint32_t nested_source_boundary=0;
    std::optional<PlayerPathStatus> path_status={};
    std::optional<PathQueryResult> query={};
    std::optional<NinjaVector> projected_displacement={},projected_velocity={};
    std::optional<ProjectionAngleResult> angle={};
    explicit operator bool() const noexcept {return failure==PlayerDirectionResetFailure::none;}
};
// Actual788d40's six-float plane input. Normalization is not a prerequisite:
// source projection/returned distance use explicit squared normal and spills.
X87Value project_point_onto_plane(const NinjaVector& input,const NinjaVector& plane_point,
    const NinjaVector& plane_normal,NinjaVector* output,double distance_scale,X87Context&);
PlayerDirectionResetResult reset_player_direction(PlayerTaskState&,PlayerMotionState&,
    PlayerPhysicsState&,const PlayerParameters&,const PlayerDirectionResetConstants&,
    const PlayerDirectionResetRuntime&,PlayerCoordinates&,X87Context&);
} // namespace sadx

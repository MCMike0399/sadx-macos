#pragma once
#include "player/player_position.hpp"
#include "player/player_plane.hpp"
#include "player/player_projection.hpp"
#include "player/player_speed.hpp"

namespace sadx {
struct PlayerReactionConstants {
    PlayerPlaneConstants plane;
    ProjectionAngleConstants angle;
};
std::optional<PlayerReactionConstants> decode_player_reaction_constants(const std::uint8_t*,std::size_t);
enum class PlayerReactionFailure { none,null_motion,unknown_collision_reference,
    null_collision_reference,unresolved_collision_work,unknown_collision_flags,
    missing_normal_owner,normal_owner_failed,unknown_normal,unknown_floor_normal,angle_failed };
struct PlayerReactionResult {
    PlayerReactionFailure failure=PlayerReactionFailure::none;
    std::uint32_t source_boundary=0;
    std::uint32_t nested_source_boundary=0;
    PlayerPositionFailure normal_failure=PlayerPositionFailure::none;
    std::optional<NinjaPlane> intersection={};
    std::optional<ProjectionAngleResult> angle={};
    explicit operator bool()const noexcept{return failure==PlayerReactionFailure::none;}
};
// Actual43c580. Shared task/world/player/scene owners are modified in source
// order. Normal query is the real41bed0 owner supplied by the collision scene;
// unknown output never becomes zero. Plane, angle and numeric callees are native.
PlayerReactionResult react_player_to_contact(PlayerTaskState&,PlayerMotionState*,PlayerPhysicsState&,
    PlayerPositionCollisionScene&,const PlayerReactionConstants&,const CrtAsinRuntime&,
    const decltype(PlayerPositionCallees::normal)&,X87Context&);
// Binds the actual body into PSetPosition while retaining complete lower
// diagnostics. Borrows the caller's live CRT runtime, which must outlive the
// binding; each invocation observes current policy/handler facts. Constants
// and the native normal callable are owned immutable composition inputs.
decltype(PlayerPositionCallees::reaction) bind_player_contact_reaction(
    PlayerReactionConstants,const CrtAsinRuntime&,decltype(PlayerPositionCallees::normal));
decltype(PlayerPositionCallees::reaction) bind_player_contact_reaction(
    PlayerReactionConstants,CrtAsinRuntime&&,decltype(PlayerPositionCallees::normal))=delete;
decltype(PlayerPositionCallees::reaction) bind_player_contact_reaction(
    PlayerReactionConstants,const CrtAsinRuntime&&,decltype(PlayerPositionCallees::normal))=delete;
} // namespace sadx

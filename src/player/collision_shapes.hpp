#pragma once
#include "player/player_position.hpp"
namespace sadx {
// Borrowed reads from actual native task owners. No copied pose or hidden null
// interpretation. Missing presence/read/value is unknown; false presence is a
// caller-observed null task pointer. Absolute CCL centers do not consume pose.
struct CollisionTaskView {
    std::optional<bool> present;
    std::function<std::optional<NinjaVector>()> position;
    std::function<std::optional<std::int32_t>(unsigned)> angle;
};
CollisionTaskView collision_task_view(const PlayerTaskState* observed_task);
using CollisionTaskLookup = std::function<std::optional<CollisionTaskView>(PositionResourceRef)>;

enum class CollisionShapeFailure {
    none, unavailable_attributes, unavailable_center, unavailable_task_presence,
    null_required_task, missing_pose_owner, unavailable_task_position,
    unavailable_task_angle, missing_matrix_owner, invalid_matrix_state,
    coordinate_unavailable,
};
struct CollisionShapeResult {
    CollisionShapeFailure failure = CollisionShapeFailure::none;
    std::uint32_t source_boundary = 0;
    std::uint32_t nested_source_boundary = 0;
    explicit operator bool() const noexcept { return failure==CollisionShapeFailure::none; }
};
// Actual source419270, including raw absolute MOV and translation-
// only paths, source rotation order flags, point transformation and popEx.
CollisionShapeResult collision_shape_center(const CollisionTaskView&,const PositionCollisionInfo&,
    NinjaVector& output,PlayerCoordinates*,X87Context&);
// Independent source419030 entry: successful arithmetic matches419270, but
// its unavailable-state boundaries retain its own instruction addresses.
CollisionShapeResult collision_shape_center_419030(const CollisionTaskView&,const PositionCollisionInfo&,
    NinjaVector& output,PlayerCoordinates*,X87Context&);
// Actual4193a0 uses supplied-unit push, translates even for flag8000 and only
// suppresses rotation there. It differs from419270 when capacity is exhausted.
CollisionShapeResult collision_shape_box_center(const CollisionTaskView&,const PositionCollisionInfo&,
    NinjaVector& output,PlayerCoordinates*,X87Context&);
// Actual419900: f32 difference spills, supplied-unit frame, optional full-int
// yaw test, CalcVector and Pop1. Inputs/outputs can alias through owned values.
CollisionShapeResult collision_relative_yaw(const NinjaVector& first,const NinjaVector& second,
    std::int32_t yaw,NinjaVector& output,PlayerCoordinates*,X87Context&);
} // namespace sadx

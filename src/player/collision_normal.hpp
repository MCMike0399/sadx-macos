#pragma once
#include "player/collision_shapes.hpp"
namespace sadx {
// The caller owns previous output facts. An AL0 return leaves every component
// untouched. Partial source stores remain represented when a later owner fails.
struct CollisionNormalOutput {
    std::optional<float> x,y,z;
    std::optional<NinjaVector> complete() const {
        if(!x||!y||!z)return {};return NinjaVector{*x,*y,*z};
    }
};
enum class CollisionNormalFailure {
    none, unavailable_reference, null_required_reference, unresolved_resource,
    unavailable_info_index, unavailable_info_count, unavailable_shape,
    unavailable_shape_field, unavailable_actor_task, shape_unavailable,
    unavailable_task_position, unavailable_task_yaw, missing_matrix_owner,
    invalid_matrix_state, coordinate_unavailable,
};
struct CollisionNormalResult {
    CollisionNormalFailure failure = CollisionNormalFailure::none;
    std::uint32_t source_boundary = 0;
    std::uint32_t nested_source_boundary = 0;
    std::optional<std::int8_t> contact; // actual signed AL from wrapper41bed0
    std::uint8_t output_stores = 0; // bit0x,bit1y,bit2z; prior values independent
    std::optional<CollisionShapeResult> shape;
    explicit operator bool() const noexcept {return failure==CollisionNormalFailure::none;}
};
// Actual41bc00 + wrapper41bed0. Lookup supplies borrowed live pose reads by
// native task-work identity; it is invoked only by paths which need the pose.
// Caller supplies existing matrix/table/normalizer owners and x87 context.
CollisionNormalResult collision_normal_query(const PlayerTaskState&,const PlayerPositionCollisionScene&,
    const CollisionTaskLookup&,const CoordinateAxisConstants&,PlayerCoordinates*,
    CollisionNormalOutput&,X87Context&);
// Bind the real backend to PSetPosition. Each call starts with unknown caller
// scratch, so a source AL0/no-write cannot invent a normal for its consumer.
std::function<PositionNormalResult(PlayerTaskState&,PlayerPositionCollisionScene&,X87Context&)>
    bind_collision_normal(const CollisionTaskLookup&,const CoordinateAxisConstants&,PlayerCoordinates*);
} // namespace sadx

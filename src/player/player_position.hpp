#pragma once
#include "player/player_rotation.hpp"
#include "cpu/crt_acos.hpp"
#include <functional>
#include <optional>
#include <unordered_map>
#include <memory>
namespace sadx {
struct PlayerReactionResult;
struct PlayerHoldResult;
struct PlayerScenePositionResult;

// IDs identify native scene-owned resources, never guest addresses. A missing
// outer optional is unknown; a known ref with no ID is an observed null pointer.
struct PositionResourceRef { std::optional<std::size_t> id; };
struct PositionCollisionInfo {
    std::optional<std::uint8_t> form;   // CCL_INFO +1
    std::optional<std::uint8_t> push;   // +2
    std::optional<std::uint8_t> damage; // +3
    std::optional<std::uint32_t> attributes; // +4
    std::optional<float> a; // +0x14
    std::optional<NinjaVector> center = {}; // CCL_INFO +8
    std::optional<float> c = {}; // +0x1c
    std::optional<std::int32_t> angle_y = {}; // +0x28
};
struct PositionInfoTable { std::unordered_map<std::int16_t,PositionCollisionInfo> records; };
struct PositionHitActor {
    std::optional<std::uint8_t> task_id; // hit owner taskwk +2, not player index
    std::optional<std::int32_t> yaw; // taskwk +0x18
    std::array<std::optional<std::uint8_t>,2> flags_bytes_at_4 = {};
    // Borrowed live owner, when supplied. Captured fields above then remain
    // historical facts and never substitute for an unknown live field.
    const PlayerTaskState* live_task = nullptr;
    std::optional<std::int32_t> angle_x_at_14 = {},angle_z_at_1c = {};
    std::optional<std::uint16_t> control_timer_at_6 = {};
    std::optional<std::int32_t> source_counter_at_c = {};
};
inline std::optional<std::uint8_t> position_actor_task_id(const PositionHitActor& actor) {
    return actor.live_task?actor.live_task->task_id_at_2:actor.task_id;
}
inline std::optional<std::int32_t> position_actor_yaw(const PositionHitActor& actor) {
    return actor.live_task?std::optional<std::int32_t>{actor.live_task->orientation.angles[1]}:actor.yaw;
}
inline std::optional<std::int32_t> position_actor_angle(const PositionHitActor& actor,std::size_t index) {
    if(index>=3)return {};
    if(actor.live_task)return actor.live_task->orientation.angles[index];
    return index==0?actor.angle_x_at_14:index==1?actor.yaw:actor.angle_z_at_1c;
}
inline std::optional<std::uint16_t> position_actor_control_timer(const PositionHitActor& actor) {
    return actor.live_task?actor.live_task->control_timer_at_6:actor.control_timer_at_6;
}
inline std::optional<std::int32_t> position_actor_source_counter(const PositionHitActor& actor) {
    return actor.live_task?actor.live_task->source_counter_at_c:actor.source_counter_at_c;
}
inline std::optional<std::uint8_t> position_actor_flag_byte(const PositionHitActor& actor,std::size_t index) {
    if(index>=2)return std::nullopt;
    if(actor.live_task)return static_cast<std::uint8_t>(actor.live_task->orientation.flags>>(8*index));
    return actor.flags_bytes_at_4[index];
}
struct PositionCollisionWork {
    std::optional<std::uint16_t> flags; // colliwk +4
    std::optional<std::int16_t> hit_count; // +2
    std::optional<std::int16_t> my_info_index; // +0xa2
    std::optional<PositionResourceRef> info; // +0xc
    std::optional<PositionResourceRef> hit_work; // +0xa4
    // Resolved mytask +0x9c -> taskwk +0x20, with each link explicit.
    std::optional<PositionResourceRef> owning_task;
    std::optional<std::uint16_t> info_count = {}; // colliwk +6
};
struct PositionTaskResource { std::optional<PositionResourceRef> work; };
// Caller owns the scene and supplies facts from its actual collision resources.
// References allow cycles/aliasing without owning pointer cycles. No geometry,
// contacts, identities or flags are manufactured by this local control owner.
struct PlayerPositionCollisionScene {
    std::optional<PositionResourceRef> task_collision;
    std::unordered_map<std::size_t,PositionCollisionWork> works;
    std::unordered_map<std::size_t,PositionInfoTable> info_tables;
    std::unordered_map<std::size_t,PositionHitActor> actors;
    std::unordered_map<std::size_t,PositionTaskResource> tasks;
};
struct PlayerPositionConstants {
    float speed_cap; // 0x7e012c
    float opposing_dot; // 0x7e01ac
    double angle_units; // 0x7dccd0
    double positive_factor; // 0x7dccc8
    double negative_factor; // 0x7dcce8
};
std::optional<PlayerPositionConstants> decode_player_position_constants(const std::uint8_t*,std::size_t);

enum class PlayerPositionFailure {
    none, unavailable_reference, null_required_reference, unresolved_resource,
    unavailable_collision_flags, unavailable_collision_index,
    unavailable_collision_info, unavailable_collision_field,
    unavailable_hit_count, unavailable_actor, unavailable_actor_field,
    null_required_motion, unavailable_stage, unavailable_act,
    missing_normal_owner, normal_owner_failed, unavailable_normal_output,
    missing_collision_owner, collision_owner_failed, unavailable_collision_result,
    missing_hold_owner, hold_owner_failed, unavailable_hold_result,
    missing_reaction_owner, reaction_owner_failed,
    missing_matrix_owner, coordinate_failed, unavailable_task_position,
    acos_unavailable,
};
struct PlayerPositionResult {
    PlayerPositionFailure failure = PlayerPositionFailure::none;
    std::uint32_t source_boundary = 0;
    std::uint32_t callee_address = 0;
    std::uint32_t nested_source_boundary = 0;
    std::optional<std::int32_t> outcome; // completed source return only
    std::int32_t provisional_outcome = 0; // retained source scratch accumulator
    std::optional<NinjaVector> temporary_vector;
    std::shared_ptr<const PlayerReactionResult> reaction_diagnostics = {};
    std::shared_ptr<const PlayerHoldResult> hold_diagnostics = {};
    std::shared_ptr<const PlayerScenePositionResult> scene_diagnostics = {};
    explicit operator bool() const noexcept { return failure==PlayerPositionFailure::none; }
};
struct PositionNormalResult {
    PlayerPositionFailure failure;
    std::optional<NinjaVector> normal;
    std::uint32_t source_boundary = 0;
};
struct PositionCollisionResult {
    PlayerPositionFailure failure;
    std::optional<std::int32_t> outcome;
    std::uint32_t source_boundary = 0;
    std::shared_ptr<const PlayerScenePositionResult> scene_diagnostics = {};
};
struct PositionHoldResult {
    PlayerPositionFailure failure;
    std::optional<std::uint32_t> flags;
    std::uint32_t source_boundary = 0;
    std::shared_ptr<const PlayerHoldResult> hold_diagnostics = {};
};
struct PositionEffectResult {
    PlayerPositionFailure failure;
    std::uint32_t source_boundary = 0;
    std::shared_ptr<const PlayerReactionResult> reaction_diagnostics = {};
};
struct PlayerPositionCallees {
    // 0x41bed0 ->0x41bc00: actual normal query and required native scene/global
    // query owners. Caller ignores AL but consumes actual initialized output.
    std::function<PositionNormalResult(PlayerTaskState&,PlayerPositionCollisionScene&,X87Context&)> normal;
    // 0x449500 and alternate0x445d10: mandatory stage/geometry/water/contact
    // owners. Null motion below is observed source nullness, not missing state.
    std::function<PositionCollisionResult(PlayerTaskState&,PlayerMotionState*,PlayerPhysicsState&,
        PlayerParameters&,PlayerPositionCollisionScene&,PlayerCoordinates*,X87Context&)> ordinary_collision, alternate_collision;
    std::function<PositionHoldResult(PlayerTaskState&,PlayerPhysicsState&,
        PlayerPositionCollisionScene&,X87Context&)> hold;
    std::function<PositionEffectResult(PlayerTaskState&,PlayerMotionState*,PlayerPhysicsState&,
        PlayerPositionCollisionScene&,X87Context&)> reaction;
};
struct PlayerPositionRuntime {
    std::optional<std::int16_t> stage, act;
    CrtAsinRuntime crt_math;
};

// Tiny actual0x41c1c0: index outside[0,16), known null task/cwp returns-1.
// Missing resources remain failures; invalid direct info pointers are not-1.
PlayerPositionResult position_collision_form(const PlayerPositionCollisionScene&,std::int32_t index,
                                             bool task_present);
// Full recovered local PSetPosition0x44cdf0. Required scene owners are explicit;
// missing callbacks never create a successful collision/position no-op. Earlier
// source stores and arithmetic/matrix status survive a later boundary failure.
PlayerPositionResult set_player_position(PlayerTaskState&,PlayerMotionState*,PlayerPhysicsState&,
    PlayerParameters&,PlayerPositionCollisionScene&,const PlayerPositionConstants&,
    const PlayerRotationConstants&,const PlayerPositionRuntime&,const PlayerPositionCallees&,
    PlayerCoordinates*,X87Context&);
} // namespace sadx

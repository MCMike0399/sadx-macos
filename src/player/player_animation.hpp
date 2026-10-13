#pragma once
#include "player/player_physics.hpp"
#include "player/player_scene_resources.hpp"
#include "player/player_animation_state.hpp"
#include <memory>
namespace sadx {
struct AnimationResourceReference { std::optional<std::size_t> id; };
struct PlayerAnimationAction { std::optional<SceneResourceReference> object; std::optional<AnimationResourceReference> motion; };
struct PlayerAnimationMotion {
    std::optional<std::uint32_t> frame_count;
    std::optional<std::uint16_t> type,input_function;
    std::optional<AnimationResourceReference> data = {};
};
// MDATA2/3 channels retain their pointer/null and count separately. Values are
// raw source words: position/scale use binary32; angles use signed32 integers.
struct PlayerAnimationKey {std::uint32_t frame;std::array<std::uint32_t,3> values;};
struct PlayerAnimationKeyBytes {
    std::shared_ptr<const std::vector<std::uint8_t>> bytes;
    std::size_t offset=0,byte_extent=0; // caller-observed contiguous backing, not a count clamp
};
struct PlayerAnimationNode {
    std::array<std::optional<AnimationResourceReference>,3> keys;
    std::array<std::optional<std::uint32_t>,3> counts;
    // Actual43f39c consumes this raw pointer word as a count for MDATA3.
    // Keep numerical source bits separate from the normalized resource ID.
    std::optional<std::uint32_t> scale_pointer_bits_at_8 = {};
};
struct PlayerAnimationRecord {
    std::optional<AnimationResourceReference> action;
    std::optional<std::uint8_t> object_count;
    std::optional<std::int8_t> mode;
    std::optional<std::uint16_t> next; // Source consumers deliberately read its LOW byte.
    std::optional<float> transition_step,frame_step;
};
struct PlayerAnimationTable { std::unordered_map<std::uint16_t,PlayerAnimationRecord> records; };
// The model hierarchy remains the scene/object resource owner's data. Static
// artifact motion/action identities never become guest or host pointer casts.
struct PlayerAnimationResources {
    std::unordered_map<std::size_t,PlayerAnimationAction> actions;
    std::unordered_map<std::size_t,PlayerAnimationMotion> motions;
    std::unordered_map<std::size_t,std::vector<PlayerAnimationNode>> node_arrays;
    std::unordered_map<std::size_t,PlayerAnimationKeyBytes> key_arrays;
    const std::unordered_map<std::size_t,SceneGeometryObject>* objects=nullptr;
};
// The actual startup allocation owns the138/13c aliases; read its arena
// references at the consuming instruction instead of storing another alias.
struct PlayerAnimationView { PlayerPhysicsState& player; PlayerAnimationTail& tail; TaskWorkHandle player_allocation; };
struct PlayerAnimationConstants {float zero,one,unsigned_correction;};
enum class PlayerAnimationFailure {
    none,unknown_field,null_resource,unresolved_resource,missing_arena,
    work_failed,missing_blend_owner,blend_failed,source_loop_no_progress,
    object_stack_overflow,unavailable_object_pose,unavailable_key,
};
struct PlayerAnimationResult {
    PlayerAnimationFailure failure=PlayerAnimationFailure::none;
    std::uint32_t source_boundary=0,callee_address=0,nested_source_boundary=0;
    std::optional<TaskWorkFailure> work_failure;
    bool completed=false;
    std::uint32_t wrap_iterations=0;
    explicit operator bool()const noexcept{return failure==PlayerAnimationFailure::none&&completed;}
};
struct PlayerAnimationRuntime {
    TaskWorkArena* arena;
    const PlayerAnimationResources& resources;
    const PlayerAnimationConstants& constants;
};
// Complete local44a800 dispatch, with explicit native resources and buffer
// allocation/release. Parent/model table initialization remains its own owner.
PlayerAnimationResult update_player_animation(PlayerAnimationView&,PlayerAnimationRuntime&,X87Context&);
PlayerAnimationResult reset_player_animation_frame(PlayerAnimationView&,const PlayerAnimationRuntime&,X87Context&); //43f230
PlayerAnimationResult next_player_animation(PlayerAnimationView&,const PlayerAnimationRuntime&,std::uint16_t,X87Context&); //446910
PlayerAnimationResult release_animation_keys(PlayerAnimationView&,PlayerAnimationRuntime&); //43f9f0
PlayerAnimationResult blend_player_animation(PlayerAnimationView&,PlayerAnimationRuntime&,X87Context&); //446950
PlayerAnimationResult build_player_animation_transition(PlayerAnimationView&,PlayerAnimationRuntime&,X87Context&); //43f2a0
std::int32_t animation_angle_delta(std::int32_t,std::int32_t) noexcept; //4383a0
std::int32_t animation_unwrap_angle(std::int32_t,std::int32_t) noexcept; //4383d0
}

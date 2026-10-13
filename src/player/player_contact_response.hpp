#pragma once
#include "player/player_mesh_query.hpp"
#include "player/player_plane.hpp"
#include "player/player_projection.hpp"
#include "player/player_task.hpp"
#include "player/player_physics.hpp"
#include "player/player_acceleration.hpp"

namespace sadx {
enum class PlayerContactResponseEntry { at_43cce0,at_43cf90,at_43d180,at_43d8b0,at_43d360 };
struct PlayerContactResponseConstants {
    ProjectionAngleConstants angle;
    PlayerPlaneConstants plane;
    float floor_dot,hard_impact,slow_factor;
};
std::optional<PlayerContactResponseConstants> decode_contact_response_constants(const std::uint8_t*,std::size_t);
struct PlayerContactResponseRuntime {
    const CrtAsinRuntime& inverse_math;
    const std::optional<NinjaVector>& world_gravity;
    const NinjaMath::SineTable* sine_table;
    // Actual source reads uninitialized local storage when selected=-1.
    // An explicit observed word can resolve that boundary; no default zero.
    std::optional<std::uint32_t> unselected_distance_word;
};
struct PlayerContactResponseActors {
    PlayerTaskState* task;
    PlayerMotionState* world;
    PlayerPhysicsState* player;
    const PlayerParameters* parameters;
    std::optional<std::uint32_t> attributes;
};
enum class PlayerContactResponseFailure {
    none,unavailable_contact_field,unavailable_response,projection_failed,
    unavailable_gravity,missing_sine_table,unavailable_attributes,
    null_motion,null_player,null_task,missing_parameters,
    unavailable_prior_attributes,unavailable_stack_word,invalid_entry
};
struct PlayerContactResponseResult {
    PlayerContactResponseFailure failure=PlayerContactResponseFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<std::int32_t> outcome;
    std::optional<ProjectionAngleResult> projection;
    bool selection_scratch_initialized=false;
    std::array<std::optional<float>,16> distances;
    std::array<std::optional<std::uint32_t>,16> classifications;
    std::int32_t candidates=0,selected=-1;
    float closest=1000.0F; // source immediate447a0000, not a native setting
    std::optional<NinjaVector> normal_sum; // source local exists only in43cce0/43d360
    explicit operator bool()const noexcept{return failure==PlayerContactResponseFailure::none&&outcome.has_value();}
};
// Five actual native source bodies. Consumes only selected branch facts;
// missing dependencies retain prior object stores, scratch facts and FP flags.
// No geometry query/collision generation is replaced by this local response.
PlayerContactResponseResult respond_player_contacts(PlayerContactResponseEntry,
    PlayerContactWork&,const PlayerMeshQueryState&,const PlayerContactResponseActors&,
    const PlayerContactResponseConstants&,const PlayerContactResponseRuntime&,X87Context&);
} // namespace sadx

#pragma once

#include "player/player_contact.hpp"
#include "player/player_coordinates.hpp"
#include "player/player_scene_resources.hpp"

namespace sadx {
enum class SceneQueryFailure {
    none, unavailable_field, null_required_reference, unresolved_resource,
    unavailable_registry_storage, cyclic_resource_links, invalid_matrix_owner,
};
struct SceneQueryResult {
    SceneQueryFailure failure = SceneQueryFailure::none;
    std::uint32_t source_boundary = 0;
    std::uint32_t nested_source_boundary = 0;
    std::optional<float> extent, extent_squared;
    CoordinateFailure coordinate_failure = CoordinateFailure::none;
    explicit operator bool() const noexcept { return failure==SceneQueryFailure::none; }
};
struct SceneBroadphaseConstants {
    float land_padding; // source7df1c8
    float node_distance_scale; // source7e0600
};
// Actual43acd0 selects source land/object records with exact bound arithmetic
// and fixed1024 capacity; source order and current matrix owner are retained.
SceneQueryResult collect_scene_land_candidates(PlayerSceneLandscape&,const NinjaVector& center,
    float radius,const SceneBroadphaseConstants&,PlayerCoordinates&,X87Context&);
// Actual450c10: runs the actual land query, then traverses registered bound
// groups and doubly-linked nodes; fixed128 output and partial stores retained.
SceneQueryResult query_player_scene_broadphase(PlayerSceneLandscape&,const PlayerContactWork&,
    const SceneBroadphaseConstants&,PlayerCoordinates&,X87Context&);
} // namespace sadx

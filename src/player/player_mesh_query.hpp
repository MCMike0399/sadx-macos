#pragma once

#include "player/player_contact.hpp"
#include "player/player_scene_resources.hpp"
#include "player/player_mesh_math.hpp"
#include "player/player_mesh_matrix.hpp"
#include "player/floor_angles.hpp"
#include "cpu/crt_atan2.hpp"
#include "cpu/crt_fabs.hpp"

namespace sadx {
struct MeshContactPayload {
    std::int32_t angle_x,angle_z; // source88-byte response +0,+4
    NinjaVector normal,oriented_normal; // +8,+0x14
    NinjaVector point_at_20,point_at_2c,vector_at_38,vector_at_44;
    float plane_start_y,plane_end_y; // +0x50,+0x54
};
static_assert(sizeof(MeshContactPayload)==88,"Source response has22 words");
struct MeshContactRecord {
    std::optional<std::uint32_t> untouched_word_at_0;
    std::optional<std::size_t> node;
    std::optional<SceneMeshBoundNode> node_snapshot;
    std::optional<MeshContactPayload> payload;
};
struct PlayerMeshQueryState {
    std::optional<std::uint32_t> contact_count; // source3b42ff4
    std::array<MeshContactRecord,16> records; // source3b47590, stride0x9c
    std::array<std::optional<MeshContactPayload>,16> responses; // source3b47008
};
struct PlayerMeshQueryConstants {
    float half_radius; // source7dcc98
    float three; // source7dccd8
    float negative_one; // source7dcb0c
    float triangle_expansion; // literal push3f851eb8
    double angle_units; // source7dccd0
    double angle_divisor; // source7e05f8, exact6.283184
};
struct PlayerMeshQueryRuntime {
    CrtAsinRuntime inverse_math;
    CrtAtan2Runtime planar_angle;
    CrtFabsRuntime absolute_math;
    // Caller resource budget is explicit and never converts exhaustion into a
    // successful no-hit. Absent means source walk with stagnation detection.
    std::optional<std::size_t> sample_budget;
};
enum class PlayerMeshQueryFailure {
    none, unavailable_contact_field, unavailable_group_selector,
    unresolved_group, cyclic_groups, unavailable_group_field,
    unavailable_selected_node, unresolved_node, unavailable_triangle,
    unavailable_material_flags, normalization_unavailable,
    normal_angles_unavailable, fabs_unavailable, atan2_unavailable,
    asin_unavailable, invalid_matrix_owner, sampling_stagnated,
    sample_budget_exhausted,
};
struct PlayerMeshQueryResult {
    PlayerMeshQueryFailure failure=PlayerMeshQueryFailure::none;
    std::uint32_t source_boundary=0;
    std::uint32_t nested_source_boundary=0;
    std::optional<std::int32_t> outcome;
    std::optional<std::size_t> node;
    std::optional<NinjaVector> sample;
    std::optional<float> traveled;
    std::optional<MeshNormalizationResult> normal_normalization,direction_normalization;
    std::optional<FloorAnglesResult> normal_angles;
    std::optional<CrtFabsResult> fabs;
    std::optional<CrtAtan2Result> atan2;
    std::optional<CrtAsinResult> asin;
    CoordinateFailure coordinate_failure=CoordinateFailure::none;
    explicit operator bool()const noexcept{return failure==PlayerMeshQueryFailure::none && outcome.has_value();}
};
// Actual453820, consumes the actual registered/selected triangle resources.
// Completedoutcome0 is the recovered source no-contact path, never a callback
// substitute. Missing fields/callees retain stores, buffers and matrix frames.
PlayerMeshQueryResult query_mesh_object_contacts(PlayerContactWork&,PlayerSceneLandscape&,
    const SceneResourceReference& object,PlayerMeshQueryState&,const PlayerMeshQueryConstants&,
    const FloorAngleConstants&,const CoordinateAxisConstants&,PlayerMeshQueryRuntime&,
    PlayerCoordinates&,X87Context&);
} // namespace sadx

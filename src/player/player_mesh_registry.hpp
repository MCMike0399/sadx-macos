#pragma once
#include "player/player_mesh_matrix.hpp"
#include "player/player_scene_registration.hpp"
#include "cpu/crt_fabs.hpp"
#include "player/player_mesh_query.hpp"

namespace sadx {
struct MeshRegistryBucket {
    std::optional<SceneResourceReference> head,tail;
};
// These are the original independently stored globals. The scene owns all
// group/node links and polygon data; this owner never caches a second list.
struct PlayerMeshRegistryState {
    std::optional<SceneResourceReference> free_groups,free_nodes,forced_object;
    std::optional<std::uint32_t> force_refresh;
    std::array<MeshRegistryBucket,128> buckets;
    std::optional<float> base_z,max_z;
    std::optional<std::uint32_t> source_word_at_3b42ff8;
};
enum class MeshRegistryFailure {
    none, unavailable_field, null_required_reference, unresolved_resource,
    unavailable_array_storage, invalid_pool_layout, invalid_matrix_owner,
    cyclic_object_graph, fabs_unavailable, registration_unavailable,
};
struct MeshRegistryResult {
    MeshRegistryFailure failure=MeshRegistryFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<std::uint32_t> outcome;
    std::optional<SceneResourceReference> returned_reference;
    std::optional<std::size_t> object,meshset,primitive,triangle,node;
    std::optional<CrtFabsResult> fabs;
    std::optional<SceneQueryResult> registration;
    CoordinateFailure coordinate_failure=CoordinateFailure::none;
    explicit operator bool()const noexcept{return failure==MeshRegistryFailure::none;}
};
// Source451030/4510e0 pools: the final slot remains an allocation sentinel.
// Slot identities are normalized at the loader boundary, never source casts.
struct MeshRegistryPools {
    std::array<std::size_t,256> groups;
    std::array<std::size_t,16384> nodes;
};
MeshRegistryResult initialize_mesh_registry(PlayerSceneLandscape&,PlayerMeshRegistryState&,PlayerMeshQueryState&,
    const MeshRegistryPools&);
void clear_mesh_registry_buckets(PlayerMeshRegistryState&); //4518d0
MeshRegistryResult allocate_mesh_group(PlayerSceneLandscape&,PlayerMeshRegistryState&); //451790
MeshRegistryResult release_mesh_group(PlayerSceneLandscape&,PlayerMeshRegistryState&,std::size_t); //451800
MeshRegistryResult allocate_mesh_node(PlayerSceneLandscape&,PlayerMeshRegistryState&,std::size_t bucket); //4521c0
MeshRegistryResult allocate_mesh_node_for_z(PlayerSceneLandscape&,PlayerMeshRegistryState&,float,X87Context&); //452160
MeshRegistryResult join_mesh_registry_buckets(PlayerSceneLandscape&,PlayerMeshRegistryState&,std::size_t group); //451910

struct MeshPrimitiveDescriptor {
    std::uint32_t kind;
    std::uint32_t vertex_count;
    std::size_t index_cursor;
    std::size_t model;
    std::array<std::optional<NinjaVector>,1024> transformed_vertices;
};
// Source452230 mutates count/cursor and transformed scratch in source order.
MeshRegistryResult transform_mesh_primitive(const PlayerSceneLandscape&,const SceneResourceReference& indices,
    MeshPrimitiveDescriptor&,PlayerCoordinates&,X87Context&);
MeshRegistryResult produce_mesh_object_sets(PlayerSceneLandscape&,PlayerMeshRegistryState&,std::size_t object,
    CrtFabsRuntime&,PlayerCoordinates&,X87Context&); //451ad0
MeshRegistryResult traverse_mesh_object(PlayerSceneLandscape&,PlayerMeshRegistryState&,std::size_t object,
    CrtFabsRuntime&,PlayerCoordinates&,X87Context&); //451a00

// Actual43b580. Dynamic records copy to3b32728 and3b2f720; static records
// append only to the shared3b2f720 work array, retaining the dynamic array tail.
MeshRegistryResult register_mesh_scene_without_culling(PlayerSceneLandscape&,
    const DecodedSceneLandTable&,const SceneRegistrationRuntime&);
struct MeshRegistryRefreshRuntime {
    const DecodedSceneLandTable& table;
    const SceneRegistrationRuntime& registration;
    const SceneRegistrationConstants& registration_constants;
    CrtFabsRuntime& absolute_math;
};
MeshRegistryResult refresh_mesh_registry(PlayerSceneLandscape&,PlayerMeshRegistryState&,
    const MeshRegistryRefreshRuntime&,PlayerCoordinates&,X87Context&); //4511d0
} // namespace sadx

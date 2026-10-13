#pragma once

#include "player/player_scene_resources.hpp"
#include <string>

namespace sadx {
enum class SceneDecodeFailure { none, invalid_pe_image, unavailable_source_range, unsupported_image,
    null_landtable, negative_land_count, unavailable_object, unavailable_model,
    invalid_vertex_count, unavailable_vertices, unavailable_normals,
    unavailable_meshsets, unavailable_index_stream, unavailable_materials };
struct SceneArtifactLocation {
    std::uint32_t source_virtual_address;
    std::size_t file_offset;
    std::size_t byte_size;
    std::string record_kind;
};
struct DecodedSceneLandTable {
    std::uint16_t source_flags; // LANDTABLE +4
    std::int16_t source_count;  // +0, signed source count
    std::vector<SceneStaticLandEntry> entries;
    std::unordered_map<std::size_t,SceneGeometryObject> objects;
    std::unordered_map<std::size_t,SceneBasicModel> models;
    std::unordered_map<std::size_t,std::vector<NinjaVector>> vector_arrays;
    std::unordered_map<std::size_t,std::vector<SceneMeshSet>> mesh_set_arrays;
    std::unordered_map<std::size_t,std::vector<std::uint16_t>> index_arrays;
    std::unordered_map<std::size_t,std::vector<SceneMaterial>> material_arrays;
    std::unordered_map<std::size_t,SceneArtifactLocation> vector_locations,meshset_locations,index_locations;
    std::unordered_map<std::size_t,SceneArtifactLocation> material_locations;
    std::unordered_map<std::size_t,SceneArtifactLocation> object_locations,model_locations;
    SceneArtifactLocation table_location;
    std::uint32_t source_land_array_address;
};
struct SceneDecodeResult {
    SceneDecodeFailure failure = SceneDecodeFailure::none;
    std::uint32_t source_boundary = 0;
    std::optional<DecodedSceneLandTable> table;
    std::uint32_t requested_source_address = 0;
    explicit operator bool() const noexcept { return failure==SceneDecodeFailure::none; }
};
// Format boundary: resolves original PE32 pointer words into native resource
// identities and preserves raw float bits. Never executes/casts source code.
// The caller keeps the artifact digest alongside this decoded resource owner.
SceneDecodeResult decode_scene_landtable(const std::uint8_t* pe_bytes,std::size_t size,
                                        std::uint32_t source_virtual_address);
// Actual GeoLists lookup at43b728..43b751: signed stage*8+signed act, source
// table97da08. Caller-selected stage/act are not an observed startup capture.
SceneDecodeResult decode_stage_scene_landtable(const std::uint8_t* pe_bytes,std::size_t size,
                                              std::int8_t stage,std::int8_t act);
} // namespace sadx

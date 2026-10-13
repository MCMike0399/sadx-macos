#pragma once

#include "math/ninja_math.hpp"
#include "engine/scene_resource_reference.hpp"
#include <optional>
#include <unordered_map>
#include <vector>

namespace sadx {
// Native resource identities assigned by the scene loader. A missing outer
// optional means unknown; a present reference with no ID is observed null.
// No source executable address is cast into a native pointer.
struct SceneMeshSet {
    std::optional<std::uint16_t> type_and_material; // source meshset +0
    std::optional<std::uint16_t> mesh_count;        // +2
    std::optional<SceneResourceReference> indices; // +4, preserves strip header words
};
struct SceneMaterial { std::array<std::uint32_t,5> words; }; // raw20-byte source material; flags=words[4]
struct SceneBasicModel {
    std::optional<NinjaVector> center; // source basic model +0x18
    std::optional<float> radius;       // +0x24
    std::optional<SceneResourceReference> vertices = {}; // model +0
    std::optional<SceneResourceReference> normals = {};  // +4, same shared vector owner
    std::optional<SceneResourceReference> meshsets = {}; // +0xc
    std::optional<std::int32_t> vertex_count = {};       // +8
    std::optional<std::uint16_t> meshset_count = {};      // +0x14
    std::optional<SceneResourceReference> materials = {}; // +0x10
    std::optional<std::uint16_t> material_count = {};     // +0x16
};
struct SceneGeometryObject {
    std::optional<SceneResourceReference> model; // NJS_OBJECT +4
    std::optional<NinjaVector> position;        // +8
    std::optional<std::array<std::int32_t,3>> angles; // +0x14
    std::optional<NinjaVector> scale = {}; // +0x20
    std::optional<std::uint32_t> evaluation_flags = {}; // +0
    std::optional<SceneResourceReference> child = {},sibling = {}; // +0x2c,+0x30
};
struct SceneLandEntry {
    std::optional<std::uint32_t> attributes;
    std::optional<SceneResourceReference> object;
    std::optional<SceneResourceReference> task;
};
struct SceneStaticLandEntry {
    std::optional<NinjaVector> center; // source36-byte record +0
    std::optional<float> radius;       // +0xc
    std::optional<SceneResourceReference> object; // +0x18
    std::optional<std::uint32_t> attributes; // +0x20
    std::optional<std::uint32_t> raw_at_10 = {},raw_at_14 = {},raw_at_1c = {};
};
struct SceneMeshBoundNode {
    std::optional<SceneResourceReference> next, previous; // +0,+4
    std::optional<NinjaVector> center; // +8
    std::optional<float> radius_squared; // +0x14
    std::optional<std::array<NinjaVector,3>> triangle_vertices = {}; // +18,+24,+30
    std::optional<std::uint32_t> raw_at_3c = {}; // source64-byte node tail
};
// Source intrusive group links encode a registry-head address as its bitwise
// complement. Keep that ownership fact distinct from a normalized group ID.
struct SceneMeshGroupBacklink {
    bool registry_head;
    std::optional<std::size_t> group;
};
struct SceneMeshBoundGroup {
    std::optional<SceneResourceReference> next; // +0
    std::optional<SceneResourceReference> object; // +8
    std::optional<SceneResourceReference> head,tail; // +0x10,+0x14
    std::optional<float> z_padding; // +0x28
    // Source +0x1c points into the fixed query output; store the native index.
    std::optional<std::size_t> selected_begin;
    std::uint32_t selected_count = 0; // +0x24, owned source writes
    std::optional<SceneMeshGroupBacklink> previous = {}; // +4
    std::optional<std::uint32_t> attributes_at_c = {},source_word_at_18 = {},source_word_at_2c = {};
    std::optional<NinjaVector> center_at_30 = {};
};
struct PlayerSceneLandscape {
    std::optional<std::int32_t> dynamic_count; // source3b36d38
    std::vector<SceneLandEntry> dynamic_land_entries; // source3b32728, dynamic_count prefix
    std::optional<std::int32_t> static_enabled; // source915460, only1 selects
    std::optional<SceneResourceReference> landtable; // source3b2f718
    std::optional<std::int16_t> static_count; // source3b36d3c
    std::vector<SceneStaticLandEntry> static_entries;
    std::unordered_map<std::size_t,SceneGeometryObject> objects;
    std::unordered_map<std::size_t,SceneBasicModel> models;
    std::unordered_map<std::size_t,std::vector<NinjaVector>> vector_arrays;
    std::unordered_map<std::size_t,std::vector<SceneMeshSet>> mesh_set_arrays;
    std::unordered_map<std::size_t,std::vector<std::uint16_t>> index_arrays;
    std::unordered_map<std::size_t,std::vector<SceneMaterial>> material_arrays;
    std::optional<SceneResourceReference> mesh_groups; // source3c48550
    std::unordered_map<std::size_t,SceneMeshBoundGroup> groups;
    std::unordered_map<std::size_t,SceneMeshBoundNode> nodes;
    // Source3b2f720 fixed1024 work records, retained after the logical end.
    // Registration and broadphase share these bytes; registry4511d0 reads them.
    std::array<std::optional<SceneLandEntry>,1024> candidates;
    std::int16_t candidate_count = 0; // source3b32724
    std::array<std::optional<std::size_t>,128> selected_nodes;
    std::uint32_t selected_count = 0; // source3b42ff0
};
} // namespace sadx

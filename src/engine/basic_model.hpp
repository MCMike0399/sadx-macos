#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/character_pack.hpp"
#include "engine/graphics_frame.hpp"
#include "math/d3dx_math.hpp"
#include <stdexcept>

namespace sadx::basic_model {
// Source NJD_CONTROL_3D and RenderFlags roles used by this renderer.
enum class Control : std::uint32_t {
    ConstantAttributes = 4,
    ConstantMaterial = 16,
    OffsetMaterial = 32,
};
enum class RenderFlag : std::uint32_t {
    Environment = 1,
    ConstantMaterial = 2,
    OffsetMaterial = 4,
    IgnoreVertexColors = 16,
};
enum class MaterialFlag : std::uint32_t {
    UClamp = 1U << 16,
    VClamp = 1U << 15,
    UFlip = 1U << 18,
    VFlip = 1U << 17,
    UseAlpha = 1U << 20,
    Texture = 1U << 21,
    Environment = 1U << 22,
    IgnoreLight = 1U << 25,
};
enum class RenderState : std::uint32_t {
    SourceBlend = 19,
    DestinationBlend = 20,
    CullMode = 22,
    AlphaReference = 24,
    AlphaBlendEnable = 27,
    Lighting = 137,
    DiffuseMaterialSource = 145,
    AmbientMaterialSource = 147,
};
enum class TextureState : std::uint32_t {
    AlphaOperation = 4,
    CoordinateIndex = 11,
    AddressU = 13,
    AddressV = 14,
    TransformFlags = 24
};
enum class TextureShading : std::uint32_t {
    Modulate = 0,
    DecalAlpha = 1,
    ModulateAlpha = 2,
    Decal = 3
};
enum class Primitive : std::uint16_t { Triangles, Quads, Polygon, Strip };
enum class VertexLayout : std::uint32_t {
    Position,
    PositionUv,
    PositionColor,
    PositionColorUv,
    PositionNormal,
    PositionNormalUv,
    PositionNormalColor,
    PositionNormalColorUv,
};

// Source renderer scratch only. Canonical control/material/view/texture owners
// are borrowed below or by Context; this value never owns those systems.
struct State {
    // GLOBAL: SONIC 0x3d08498 _RenderFlags size=4
    std::uint32_t render_flags = 0;
    // GLOBAL: SONIC 0x3d08488 _nj_texglobal_color_ size=4
    std::uint32_t constant_color = 0;
    // GLOBAL: SONIC 0x3d0848c PolyBuffVertexColor size=4
    std::uint32_t vertex_color = 0;
    // GLOBAL: SONIC 0x3d08490 Direct3D_CurrentCullMode size=4
    std::uint32_t cull_mode = 0;
    // GLOBAL: SONIC 0x3d08494 LandTableVertexColor size=4
    std::uint32_t landtable_color = 0;
    // GLOBAL: SONIC 0x3d12e00 _nj_current_matrix_no_view_ size=64
    D3dxMatrix current_no_view = {};
    // GLOBAL: SONIC 0x3d0fd40 _nj_current_matrix_no_view_inverse_ size=64
    D3dxMatrix inverse_current = {};
    // GLOBAL: SONIC 0x3d12900 WorldMatrix size=64
    D3dxMatrix world = {};
    // GLOBAL: SONIC 0x3d128c0 _nj_current_matrix_extracted_view_ size=64
    D3dxMatrix extracted = {};
};
// NOTSA: references into the existing system owners, with no mirrored globals.
struct Owners {
    State &state;
    const std::uint32_t &control;
    const std::uint32_t &attribute_mask;
    const std::uint32_t &attributes;
    const std::array<float, 4> &constant_material;
    const graphics::Matrix &current_matrix;
    const D3dxMatrix &inverse_view;
    const D3dxMatrix &reset_world;
};
// NJS_MATERIAL's two packed colors and raw floating exponent precede tex/flags.
struct Material {
    std::uint32_t diffuse, specular;
    float power;
    std::uint32_t texture, attributes;
};
// NOTSA: a borrowed mesh payload view. Offsets include the pack's count prefix;
// all resource identities still name the existing immutable CharacterPack.
struct Meshset {
    const character_pack::CharacterPack &pack;
    character_pack::ResourceId resource;
    std::uint32_t ordinal;
};

// NOTSA: integration borrows the actual device, texture cache, arithmetic
// contexts and callback owner here. It supplies ordinary link-time bindings.
struct Context;
using MaterialCallback = void (*)(Context &);
fidelity::ExactFloat exact(Context &, float);
D3dxMatrix multiply_matrices(Context &, const graphics::Matrix &, const D3dxMatrix &);
// The source ignores a singular inverse's NULL result: destination stays intact.
void invert_matrix(Context &, D3dxMatrix &destination, const graphics::Matrix &source);
void set_render_state(Context &, RenderState, std::uint32_t);
void set_texture_state(Context &, TextureState, std::uint32_t);
void set_device_material(Context &, const graphics::Material &);
void set_world_transform(Context &, const D3dxMatrix &);
void set_texture_transform(Context &, const graphics::Matrix &);
void clear_stream_source(Context &);
void clear_indices(Context &);
// Bind the exact shared texture_selection::set_texture_number, never select_index.
void set_texture_number(Context &, std::uint32_t);
void set_null_texture(Context &);
// FUNCTION: SONIC 0x791990 njTextureShadingMode
void set_texture_shading(Context &, TextureShading);
MaterialCallback snapshot_material_callback(Context &);

// NOTSA: integration maps these explicit unavailable leaves to the source call
// boundary. This is a failure tag, never a fabricated device HRESULT.
enum class Boundary {
    OffsetMaterial,
    EnvironmentMap,
    TextureReset,
    AlternateModel,
    Primitive,
    Buffer
};
class Unavailable : public std::runtime_error {
  public:
    explicit Unavailable(Boundary value)
        : std::runtime_error("basic model: unrecovered source callee"), boundary(value) {}
    Boundary boundary;
};

// FUNCTION: SONIC 0x781ec0 GetConstantMaterialBGRA
std::uint32_t offset_material_color(const Material &, Owners, Context &);
// FUNCTION: SONIC 0x7944b0 Direct3D_EnvironmentMap
void environment_map(Context &);
// FUNCTION: SONIC 0x794500 Direct3D_ResetTextureTransform
void reset_texture_transform(Context &);
// STUB: SONIC 0x784ae0 njDrawModel_ alternate material renderer
void draw_alternate_model(Context &, character_pack::ResourceId,
                          const character_pack::CharacterPack &, Owners);
// NOTSA: link-time handoff to the independently recovered polybuff::normal_uv_strip.
void draw_normal_uv_strip(Context &, const Meshset &, character_pack::ResourceId points,
                          character_pack::ResourceId normals);
// NOTSA: explicit boundary for other original table entries and buffered draws.
// These bindings must report the selected source boundary, never silently skip it.
void unsupported_primitive(Context &, VertexLayout, Primitive, const Meshset &,
                           character_pack::ResourceId points, character_pack::ResourceId normals);
void draw_buffered_mesh(Context &, const Meshset &);

// FUNCTION: SONIC 0x791a80 PrepareBasicModelMatrices
void prepare_matrices(Owners, Context &);
// FUNCTION: SONIC 0x791a50 ProjectToWorldSpace
void project_to_world_space(Owners, Context &);
// FUNCTION: SONIC 0x791ab0 Direct3D_SetWorldTransform
void set_world_transform(Owners, Context &);
// FUNCTION: SONIC 0x791ad0 Direct3D_ResetWorldTransform
void reset_world_transform(Owners, Context &);
// FUNCTION: SONIC 0x781cc0 CopyConstantMaterial
void copy_constant_material(Owners, Context &);
// FUNCTION: SONIC 0x781df0 RenderFlags_Reset
void reset_render_flags(State &, Context &);
// FUNCTION: SONIC 0x784850 Direct3D_ParseMaterial
void parse_material(const Material &, Owners, Context &);
// STUB: SONIC 0x77eda0 njDirectDrawModel buffered draw path unrecovered
void draw_direct_model(character_pack::ResourceId, const character_pack::CharacterPack &, Owners,
                       Context &);
// FUNCTION: SONIC 0x77ef70 njDrawModel
void draw_model(character_pack::ResourceId, const character_pack::CharacterPack &, Owners,
                Context &);
} // namespace sadx::basic_model

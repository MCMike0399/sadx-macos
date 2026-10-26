#include "game/game_mode.hpp"
#include "runtime/model_device.hpp"
#include "runtime/model_draw_context.hpp"
#include "runtime/object_draw_context.hpp"
#include <cstring>

namespace sadx::material_correction {
graphics::Material get_material(Context &context) { return context.device().material; }
void set_material(Context &context, const graphics::Material &material) {
    context.device().material = material;
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.display.arithmetic(), value};
}
float specular_power(Context &context) {
    return context.lighting.active_palette.upper_specular_power;
}
} // namespace sadx::material_correction

namespace sadx::basic_model {
namespace {
// NOTSA: lossless representation conversion at the existing maintained SSE math boundary.
D3dxMatrix words(const graphics::Matrix &matrix) {
    D3dxMatrix result;
    std::memcpy(result.data(), matrix.data(), sizeof(result));
    return result;
}
void correct_current_material(Context &context) {
    material_correction::Context material{context.display, context.environment.lighting};
    material_correction::correct_material(material);
}
constexpr std::size_t kMeshsetBytes = 28;
constexpr std::size_t kArrayHeaderBytes = 4;
constexpr std::size_t kBitsPerByte = 8;
// NOTSA: immutable imported mesh descriptors are read without host-layout casts.
std::uint32_t mesh_integer(const Meshset &mesh, std::size_t field, std::size_t width) {
    const auto &bytes = mesh.pack.record(mesh.resource).payload;
    const auto offset = kArrayHeaderBytes + mesh.ordinal * kMeshsetBytes + field;
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < width; ++index)
        value |= std::uint32_t(bytes.at(offset + index)) << (index * kBitsPerByte);
    return value;
}
} // namespace
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.display.arithmetic(), value};
}
D3dxMatrix multiply_matrices(Context &context, const graphics::Matrix &first,
                             const D3dxMatrix &second) {
    return d3dx_matrix_multiply_774310(words(first), second, context.display.sse());
}
void invert_matrix(Context &context, D3dxMatrix &destination, const graphics::Matrix &source) {
    const auto result = d3dx_matrix_inverse_7757e0(words(source), context.display.sse(),
                                                   sse_reciprocal_by_division);
    if (result.matrix)
        destination = *result.matrix;
}
void set_render_state(Context &context, RenderState state, std::uint32_t value) {
    context.device().render_states[static_cast<std::uint32_t>(state)] = value;
}
void set_texture_state(Context &context, TextureState state, std::uint32_t value) {
    context.device().stage0_states[static_cast<std::uint32_t>(state)] = value;
}
void set_device_material(Context &context, const graphics::Material &material) {
    context.device().material = material;
}
void set_world_transform(Context &context, const D3dxMatrix &matrix) {
    std::memcpy(context.device().world.data(), matrix.data(), sizeof(matrix));
}
void set_texture_transform(Context &context, const graphics::Matrix &matrix) {
    context.device().texture_transform = matrix;
}
void clear_stream_source(Context &context) {
    polybuff::Context device{context.display};
    polybuff::set_stream_source(device, nullptr, 0, 0);
}
void clear_indices(Context &context) { context.device().indices.clear(); }
void set_texture_number(Context &context, std::uint32_t index) {
    (void)context.display.output().set_texture_number(index);
}
void set_null_texture(Context &context) { context.display.clear_texture_binding(); }
MaterialCallback snapshot_material_callback(Context &context) {
    switch (context.environment.material_callback) {
    case bootstrap::MaterialCallback::None:
        return nullptr;
    case bootstrap::MaterialCallback::CorrectMaterial:
        return correct_current_material;
    }
    throw flow::UnrecoveredGameFlowCall("native_material_callback");
}
void draw_normal_uv_strip(Context &context, const Meshset &mesh, character_pack::ResourceId points,
                          character_pack::ResourceId normals) {
    const auto cull = context.state().cull_mode;
    const auto color = context.state().vertex_color;
    constexpr std::size_t kStripCountField = 2, kIndexField = 4, kUvField = 20;
    constexpr std::size_t kShortBytes = 2, kWordBytes = 4;
    const auto &geometry = context.environment.scene.geometry;
    const auto &indices = geometry.indices(mesh_integer(mesh, kIndexField, kWordBytes));
    const auto &uv = geometry.texture_coordinates(mesh_integer(mesh, kUvField, kWordBytes));
    const auto &positions = geometry.vector_words(points);
    const auto &directions = geometry.vector_words(normals);
    namespace pb = polybuff;
    const pb::Mesh source{
        static_cast<std::uint16_t>(mesh_integer(mesh, kStripCountField, kShortBytes)),
        {indices.data(), indices.size(), 0, pb::DataKind::Mesh},
        {uv.data(), uv.size(), 0, pb::DataKind::TextureCoordinates}};
    const pb::Vectors point_view{{positions.data(), positions.size(), 0, pb::DataKind::Points}};
    const pb::Vectors normal_view{{directions.data(), directions.size(), 0, pb::DataKind::Normals}};
    pb::Context device{context.display};
    pb::normal_uv_strip(device, native::normal_uv_buffer(context.display), source, point_view,
                        normal_view, color, cull);
}
void draw_alternate_model(Context &, character_pack::ResourceId,
                          const character_pack::CharacterPack &, Owners) {
    throw Unavailable(Boundary::AlternateModel);
}
void unsupported_primitive(Context &, VertexLayout, Primitive, const Meshset &,
                           character_pack::ResourceId, character_pack::ResourceId) {
    throw Unavailable(Boundary::Primitive);
}
void draw_buffered_mesh(Context &, const Meshset &) { throw Unavailable(Boundary::Buffer); }
} // namespace sadx::basic_model

namespace sadx::object_draw {
void draw_model(Context &context, ModelId id) {
    basic_model::Context model{context.display, context.environment};
    basic_model::draw_model(id, context.environment.pack, model.owners(), model);
}
} // namespace sadx::object_draw

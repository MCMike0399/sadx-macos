#pragma once
// Reference-only link-time bindings. Device/callee results below are explicit
// synthetic receipts; they are never linked into the native application.
#include "engine/basic_model.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace sadx::basic_model {
struct Context {
    X87Context arithmetic;
    State &state;
    const std::uint32_t *input;
    std::vector<std::uint32_t> events;
    MaterialCallback callback = nullptr;
    explicit Context(State &state, const std::uint32_t *input)
        : arithmetic(static_cast<std::uint16_t>(input[18])), state(state), input(input) {}
    void event(std::uint32_t tag, std::initializer_list<std::uint32_t> args = {}) {
        events.push_back(tag);
        events.push_back(static_cast<std::uint32_t>(args.size()));
        events.insert(events.end(), args.begin(), args.end());
    }
    void matrix_event(std::uint32_t tag, const D3dxMatrix &matrix) {
        events.push_back(tag);
        events.push_back(matrix.size());
        events.insert(events.end(), matrix.begin(), matrix.end());
    }
};
fidelity::ExactFloat exact(Context &context, float value) {
    return fidelity::ExactFloat(context.arithmetic, value);
}
D3dxMatrix multiply_matrices(Context &context, const graphics::Matrix &first,
                             const D3dxMatrix &second) {
    D3dxMatrix first_words;
    std::memcpy(first_words.data(), first.data(), sizeof(first_words));
    context.events.push_back(5);
    context.events.push_back(32);
    context.events.insert(context.events.end(), first_words.begin(), first_words.end());
    context.events.insert(context.events.end(), second.begin(), second.end());
    D3dxMatrix out;
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = first_words[i] ^ second[i];
    return out;
}
void invert_matrix(Context &context, D3dxMatrix &out, const graphics::Matrix &source) {
    D3dxMatrix source_words;
    std::memcpy(source_words.data(), source.data(), sizeof(source_words));
    context.matrix_event(6, source_words);
    if (!context.input[23])
        for (std::size_t i = 0; i < out.size(); ++i)
            out[i] = source_words[15 - i];
}
void set_render_state(Context &context, RenderState state, std::uint32_t value) {
    context.event(1, {static_cast<std::uint32_t>(state), value});
}
void set_texture_state(Context &context, TextureState state, std::uint32_t value) {
    context.event(2, {static_cast<std::uint32_t>(state), value});
    if (state == TextureState::TransformFlags)
        context.state.render_flags ^= context.input[20];
}
std::uint32_t probe_bits(float value) {
    std::uint32_t out;
    std::memcpy(&out, &value, 4);
    return out;
}
float probe_float(std::uint32_t value) {
    float out;
    std::memcpy(&out, &value, 4);
    return out;
}
void set_device_material(Context &context, const graphics::Material &value) {
    context.events.push_back(3);
    context.events.push_back(17);
    for (const auto *channels : {&value.diffuse, &value.ambient, &value.specular, &value.emissive})
        for (auto channel : *channels)
            context.events.push_back(probe_bits(channel));
    context.events.push_back(probe_bits(value.power));
}
void set_world_transform(Context &context, const D3dxMatrix &value) {
    context.matrix_event(4, value);
}
void set_texture_transform(Context &context, const graphics::Matrix &value) {
    context.events.push_back(21);
    context.events.push_back(16);
    for (auto channel : value)
        context.events.push_back(probe_bits(channel));
}
void clear_stream_source(Context &context) { context.event(19); }
void clear_indices(Context &context) { context.event(20); }
void set_texture_number(Context &context, std::uint32_t value) { context.event(8, {value}); }
void set_null_texture(Context &context) { context.event(7); }
MaterialCallback snapshot_material_callback(Context &context) { return context.callback; }
void draw_alternate_model(Context &context, character_pack::ResourceId,
                          const character_pack::CharacterPack &, Owners) {
    context.event(13);
    throw Unavailable(Boundary::AlternateModel);
}
void draw_normal_uv_strip(Context &context, const Meshset &mesh, character_pack::ResourceId points,
                          character_pack::ResourceId normals) {
    context.event(14, {mesh.ordinal, points, normals});
}
void unsupported_primitive(Context &context, VertexLayout layout, Primitive primitive,
                           const Meshset &mesh, character_pack::ResourceId points,
                           character_pack::ResourceId normals) {
    context.event(15, {static_cast<std::uint32_t>(layout), static_cast<std::uint32_t>(primitive),
                       mesh.ordinal, points, normals});
    throw Unavailable(Boundary::Primitive);
}
void draw_buffered_mesh(Context &context, const Meshset &mesh) {
    context.event(16, {mesh.ordinal});
    throw Unavailable(Boundary::Buffer);
}
void second_material_callback(Context &context) { context.event(18); }
void first_material_callback(Context &context) {
    context.event(17);
    context.state.render_flags ^= context.input[22];
    context.callback = second_material_callback;
}
} // namespace sadx::basic_model

#include "engine/sprite_immediate.hpp"
#include "cpu/fidelity_float.hpp"
#include <utility>

namespace sadx::sprite {
using fidelity::ExactFloat;
void draw_sprite(const Sprite *sprite, std::int32_t animation_index, float priority,
                 std::uint8_t attributes, const std::array<float, 4> &material_argb,
                 ImmediateServices &services, X87Context &fp) {
    if (!sprite || !sprite->textures)
        return;
    services.select_texture_list(*sprite->textures);
    const auto &frame = sprite->animation.at(static_cast<std::size_t>(animation_index));
    constexpr float uv_unit = 0.003921568859368563f;
    float u_begin = (ExactFloat::integer(fp, frame.u_begin) * uv_unit).rounded();
    float v_begin = (ExactFloat::integer(fp, frame.v_begin) * uv_unit).rounded();
    float u_end = (ExactFloat::integer(fp, frame.u_end) * uv_unit).rounded();
    float v_end = (ExactFloat::integer(fp, frame.v_end) * uv_unit).rounded();
    const auto left = ExactFloat::integer(fp, -static_cast<std::int32_t>(frame.center_x));
    const auto top = ExactFloat::integer(fp, -static_cast<std::int32_t>(frame.center_y));
    const auto right = ExactFloat::integer(fp, frame.width) + left;
    const auto bottom = (ExactFloat::integer(fp, frame.height) + top).rounded();
    const auto x_begin = (left * sprite->scale_x).rounded();
    const auto x_end = (right * sprite->scale_x).rounded();
    const auto y_begin = (top * sprite->scale_y).rounded();
    const auto y_end = (ExactFloat(fp, bottom) * sprite->scale_y).rounded();
    Quad quad{};
    if (attributes & 1) {
        services.push_unit_matrix();
        services.rotate_z(sprite->angle);
        const auto corner = services.transform_vector({x_begin, y_begin, 0});
        quad.origin[0] = (ExactFloat(fp, corner[0]) + sprite->position[0]).rounded();
        quad.origin[1] = (ExactFloat(fp, corner[1]) + sprite->position[1]).rounded();
        quad.origin[2] = (ExactFloat(fp, -1.0f) / priority).rounded();
        const auto horizontal = services.transform_vector({x_end, y_begin, 0});
        quad.horizontal[0] = (ExactFloat(fp, horizontal[0]) - corner[0]).rounded();
        quad.horizontal[1] = (ExactFloat(fp, horizontal[1]) - corner[1]).rounded();
        const auto vertical = services.transform_vector({x_begin, y_end, 0});
        quad.vertical[0] = (ExactFloat(fp, vertical[0]) - corner[0]).rounded();
        quad.vertical[1] = (ExactFloat(fp, vertical[1]) - corner[1]).rounded();
        services.pop_matrix();
    } else {
        quad.origin[0] = (ExactFloat(fp, x_begin) + sprite->position[0]).rounded();
        quad.origin[1] = (ExactFloat(fp, y_begin) + sprite->position[1]).rounded();
        quad.origin[2] = (ExactFloat(fp, -1.0f) / priority).rounded();
        quad.horizontal[0] = (ExactFloat(fp, x_end) - x_begin).rounded();
        quad.vertical[1] = (ExactFloat(fp, y_end) - y_begin).rounded();
    }
    if (attributes & 4)
        std::swap(u_begin, u_end);
    if (attributes & 8)
        std::swap(v_begin, v_end);
    quad.uv_origin = {u_begin, v_begin};
    quad.uv_horizontal[0] = (ExactFloat(fp, u_end) - u_begin).rounded();
    quad.uv_vertical[1] = (ExactFloat(fp, v_end) - v_begin).rounded();
    std::uint32_t color = 4294967295U;
    if (attributes & 2) {
        color = 0;
        for (unsigned component = 0; component < 4; ++component) {
            const auto value = (ExactFloat(fp, material_argb[3 - component]) * 255.0f)
                                   .truncated_integer();
            color |= (static_cast<std::uint32_t>(value) & 255U) << (component * 8);
        }
    }
    services.begin_quad((attributes & 32) != 0);
    services.set_hud_color(color);
    services.select_texture(frame.texture_index);
    services.draw_quad(quad);
    services.restore_linear_filter();
}
} // namespace sadx::sprite

namespace sadx::sprite {
void begin_quad(bool alpha, graphics::Draw &state) {
    // njAlphaMode, Direct3D_TextureFilterPoint, FVF XYZRHW|DIFFUSE|TEX1, clamp U/V.
    state.render_states[27] = alpha;
    state.stage0_states[4] = alpha ? 4 : 3;
    state.render_states[22] = 1;
    state.render_states[137] = 0;
    state.render_states[145] = 1;
    state.stage0_states[16] = 1;
    state.stage0_states[17] = 1;
    state.stage0_states[18] = 1;
    state.space = graphics::PositionSpace::Screen;
    state.texture_coordinates = 1;
    state.has_normals = false;
    state.has_specular = false;
    state.stage0_states[13] = 3;
    state.stage0_states[14] = 3;
}
void draw_quad(const Quad &quad, std::uint32_t color, std::array<ScreenVertex, 4> &vertices,
               QuadServices &services, X87Context &fp) {
    auto x = (ExactFloat(fp, quad.origin[0]) + 0.5f).rounded();
    auto y = (ExactFloat(fp, quad.origin[1]) + 0.5f).rounded();
    const auto depth = ExactFloat::retained(fp, services.projected_depth(quad.origin[2]));
    auto u = ExactFloat(fp, quad.uv_origin[0]);
    auto v = ExactFloat(fp, quad.uv_origin[1]);
    auto &first = vertices[0];
    first.depth = depth.rounded();
    first.x = x;
    first.u = u.rounded();
    first.y = y;
    first.v = v.rounded();
    first.color = color;
    first.reciprocal_w = 1;
    auto &second = vertices[1];
    second.x = (ExactFloat(fp, x) + quad.horizontal[0]).rounded();
    const auto horizontal_y = ExactFloat(fp, y) + quad.horizontal[1];
    second.reciprocal_w = 1;
    second.color = color;
    second.y = horizontal_y.rounded();
    second.depth = depth.rounded();
    second.u = (u + quad.uv_horizontal[0]).rounded();
    second.v = (v + quad.uv_horizontal[1]).rounded();
    x = (ExactFloat(fp, x) + quad.vertical[0]).rounded();
    y = (ExactFloat(fp, y) + quad.vertical[1]).rounded();
    const auto vertical_u = (u + quad.uv_vertical[0]).rounded();
    v = v + quad.uv_vertical[1];
    auto &third = vertices[2];
    third.x = x;
    third.depth = depth.rounded();
    third.y = y;
    third.v = v.rounded();
    third.reciprocal_w = 1;
    third.color = color;
    third.u = vertical_u;
    auto &fourth = vertices[3];
    fourth.x = (ExactFloat(fp, x) + quad.horizontal[0]).rounded();
    const auto final_y = ExactFloat(fp, y) + quad.horizontal[1];
    fourth.reciprocal_w = 1;
    fourth.color = color;
    fourth.y = final_y.rounded();
    fourth.depth = depth.rounded();
    fourth.u = (ExactFloat(fp, vertical_u) + quad.uv_horizontal[0]).rounded();
    fourth.v = (v + quad.uv_horizontal[1]).rounded();
    services.submit_strip(vertices);
}
} // namespace sadx::sprite

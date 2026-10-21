#include "engine/pvr_device.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::pvr {
void set_default_render_state(graphics::Draw &state) {
    state.render_states[14] = 1;
    state.render_states[15] = 1;
    state.render_states[24] = 0;
    state.render_states[25] = 7;
    state.render_states[27] = 0;
    state.render_states[147] = 0;
    state.render_states[145] = 0;
    state.render_states[146] = 0;
    state.render_states[148] = 0;
    state.render_states[29] = 0;
    state.render_states[9] = 2;
}
void set_default_texture_stage_state(graphics::Draw &state) {
    state.stage0_states[1] = 4;
    state.stage0_states[2] = 2;
    state.stage0_states[3] = 1;
    state.stage0_states[4] = 4;
    state.stage0_states[5] = 2;
    state.stage0_states[6] = 1;
    state.stage0_states[16] = 2;
    state.stage0_states[17] = 2;
    state.stage0_states[18] = 2;
}
namespace {
// NOTSA: njAlphaMode's device state, before texture binding/filter operations.
void begin_alpha(std::uint32_t alpha, graphics::Draw &state) {
    state.render_states[27] = alpha != 0;
    state.stage0_states[4] = alpha != 0 ? 4 : 3;
}
// NOTSA: source Direct3D_TextureFilterPoint also selects strip rasterization state.
void point_filter(graphics::Draw &state) {
    state.render_states[22] = 1;
    state.render_states[137] = 0;
    state.render_states[145] = 1;
    state.stage0_states[16] = 1;
    state.stage0_states[17] = 1;
    state.stage0_states[18] = 1;
}
// NOTSA: source Direct3D_TextureFilterLinear restores all three texture filters.
void end(graphics::Draw &state) {
    state.stage0_states[16] = 2;
    state.stage0_states[17] = 2;
    state.stage0_states[18] = 2;
}
// NOTSA: source XYZRHW|DIFFUSE conversion shared by the two strip loops.
graphics::Vertex vertex(float x, float y, float depth, std::uint32_t color,
                         X87Context &fp) {
    graphics::Vertex result;
    result.position = {(fidelity::ExactFloat(fp, x) + 0.5f).rounded(),
                       (fidelity::ExactFloat(fp, y) + 0.5f).rounded(), depth, 1};
    result.diffuse = {float((color >> 16) & 255) / 255,
                      float((color >> 8) & 255) / 255, float(color & 255) / 255,
                      float(color >> 24) / 255};
    return result;
}
}
void submit_texture(const std::array<Vertex, 4> &vertices,
                    std::shared_ptr<const graphics::Texture> texture,
                    std::uint32_t alpha, bool fog_enabled, ImmediateDevice &device,
                    X87Context &fp) {
    if (!texture || !device.projected_depth)
        throw std::runtime_error("PVR submission requires texture and native projected depth");
    if (fog_enabled)
        device.state.render_states[28] = 0;
    begin_alpha(alpha, device.state);
    point_filter(device.state);
    device.state.texture = std::move(texture);
    device.state.stage0_states[13] = 3;
    device.state.stage0_states[14] = 3;
    auto draw = device.state;
    draw.space = graphics::PositionSpace::Screen;
    draw.primitive_type = 5;
    draw.primitive_count = 2;
    draw.texture_coordinates = 1;
    draw.has_normals = false;
    draw.has_specular = false;
    draw.indices.clear();
    draw.vertices.clear();
    for (const auto &v : vertices) {
        const auto depth = device.projected_depth(v.reciprocal_depth);
        auto converted = vertex(v.x, v.y, depth, v.color, fp);
        converted.uv[0] = {v.u, v.v};
        draw.vertices.push_back(converted);
    }
    device.frame.commands.emplace_back(std::move(draw));
    end(device.state);
    if (fog_enabled)
        device.state.render_states[28] = 1;
}
void submit_box(const std::array<Point, 4> &points, std::uint32_t color, float priority,
                std::uint32_t flags, ImmediateDevice &device, X87Context &fp) {
    submit_colored_polygon(points, {color, color, color, color}, priority, flags, device, fp,
                           [&device] { device.state.texture.reset(); });
}
void submit_colored_polygon(const std::array<Point, 4> &points,
                            const std::array<std::uint32_t, 4> &colors, float priority,
                            std::uint32_t flags, ImmediateDevice &device, X87Context &fp,
                            const std::function<void()> &unbind_texture) {
    if (!device.projected_depth || !unbind_texture)
        throw std::runtime_error("PVR polygon requires native depth and texture-cache services");
    if ((flags & 2147483648U) != 0)
        throw std::runtime_error("PVR packed-coordinate polygon path is not recovered");
    // The source spills to binary32 before testing the clamp. An unmasked overflow
    // therefore faults before any device calls, even when the eventual clamp is one.
    auto argument = (fidelity::ExactFloat(fp, -1.0f) / priority).rounded();
    if (fidelity::ExactFloat(fp, argument).compare(1.0f) == X87Comparison::greater)
        argument = 1.0f;
    begin_alpha((flags >> 6) & 1, device.state);
    unbind_texture();
    point_filter(device.state);
    std::vector<graphics::Vertex> vertices;
    vertices.reserve(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto depth = device.projected_depth(argument);
        vertices.push_back(vertex(points[index].x, points[index].y, depth, colors[index], fp));
    }
    // The source strip converter clears again after depth projection and before submission.
    unbind_texture();
    auto draw = device.state;
    draw.space = graphics::PositionSpace::Screen;
    draw.primitive_type = 5;
    draw.primitive_count = 2;
    draw.texture_coordinates = 0;
    draw.has_diffuse = true;
    draw.has_normals = false;
    draw.has_specular = false;
    draw.indices.clear();
    draw.vertices = std::move(vertices);
    device.frame.commands.emplace_back(std::move(draw));
    end(device.state);
}
} // namespace sadx::pvr

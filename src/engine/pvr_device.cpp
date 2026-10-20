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
// NOTSA: shared source calls njAlphaMode then Direct3D_TextureFilterPoint.
void begin(std::uint32_t alpha, graphics::Draw &state) {
    state.render_states[27] = alpha != 0;
    state.stage0_states[4] = alpha != 0 ? 4 : 3;
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
    begin(alpha, device.state);
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
    if (!device.projected_depth)
        throw std::runtime_error("PVR box submission requires native projected depth");
    auto reciprocal = fidelity::ExactFloat(fp, -1.0f) / priority;
    if (reciprocal.compare(1.0f) == X87Comparison::greater)
        reciprocal = fidelity::ExactFloat(fp, 1.0f);
    const auto argument = reciprocal.rounded();
    begin((flags >> 6) & 1, device.state);
    device.state.texture.reset();
    auto draw = device.state;
    draw.space = graphics::PositionSpace::Screen;
    draw.primitive_type = 5;
    draw.primitive_count = 2;
    draw.texture_coordinates = 0;
    draw.has_normals = false;
    draw.has_specular = false;
    draw.indices.clear();
    draw.vertices.clear();
    for (const auto &point : points) {
        const auto depth = device.projected_depth(argument);
        draw.vertices.push_back(vertex(point.x, point.y, depth, color, fp));
    }
    device.frame.commands.emplace_back(std::move(draw));
    end(device.state);
}
} // namespace sadx::pvr

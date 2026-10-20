#include "engine/pvr_draw.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::pvr {
std::uint32_t blend_colors(std::uint32_t first, std::uint32_t second) {
    std::uint32_t result = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        const auto a = (first >> shift) & 255;
        const auto b = (second >> shift) & 255;
        result |= ((a * b) / 255) << shift;
    }
    return result;
}
void set_material(std::uint32_t color, State &state) {
    const auto blended = blend_colors(color, state.base_color);
    for (auto &vertex : state.vertices)
        vertex.color = blended;
    state.alpha_enabled = (blended & 4278190080U) != 4278190080U;
}
void set_base_color(std::uint32_t color, State &state) {
    state.base_color = color;
    std::uint32_t alpha = 4278190080U;
    for (auto &vertex : state.vertices) {
        vertex.color = blend_colors(vertex.color, color);
        alpha &= vertex.color;
    }
    state.alpha_enabled = alpha != 4278190080U;
}
void default_blending(const std::function<void(std::uint32_t, std::uint32_t)> &set_blend) {
    set_blend(0, 8);
    set_blend(1, 6);
}
void initialize(State &state) {
    for (auto &vertex : state.vertices)
        vertex.color = 4294967295U;
    state.vertices[0].u = state.vertices[0].v = 0.0f;
    state.vertices[1].u = 0.0f;
    state.vertices[1].v = 1.0f;
    state.vertices[2].u = 1.0f;
    state.vertices[2].v = 0.0f;
    state.vertices[3].u = state.vertices[3].v = 1.0f;
    state.base_color = 4294967295U;
    state.alpha_enabled = 0;
    state.texture_flags = 1;
    state.texture_shading = 0;
    state.scale = 1.0f;
    state.mode = 0;
}

void draw_texture(std::uint32_t index, float x, float y, float depth, State &state,
                  std::int32_t display_disabled, Calls &calls, X87Context &fp) {
    if (display_disabled != 0)
        return;
    const auto texture = calls.texture(index & 822083583U);
    const auto flip = index >> 30;
    for (unsigned i = 0; i < 4; ++i) {
        state.vertices[i].u = float((i >= 2) != bool(flip & 2));
        state.vertices[i].v = float(bool(i & 1) != bool(flip & 1));
    }
    using fidelity::ExactFloat;
    const auto anchor = (index >> 24) & 15U;
    if (anchor > 8)
        throw std::runtime_error("ghDrawPvrTexture invalid anchor uses uninitialized source locals");
    const auto horizontal = anchor % 3;
    const auto vertical = anchor / 3;
    // FILD is signed; the conditional addition rounds before the coordinate arithmetic.
    const auto dimension = [&](std::uint32_t value) {
        auto result = ExactFloat::integer(fp, static_cast<std::int32_t>(value));
        if (static_cast<std::int32_t>(value) < 0)
            result = result + 4294967296.0f;
        return result;
    };
    auto left = ExactFloat(fp, x), top = ExactFloat(fp, y);
    auto right = left, bottom = top;
    if (horizontal == 0)
        right = dimension(texture.width) + x;
    else if (horizontal == 1) {
        const auto half = ExactFloat::integer(fp, texture.width >> 1).rounded();
        left = left - half;
        right = ExactFloat(fp, half) + x;
    } else
        left = left - dimension(texture.width);
    if (vertical == 0)
        bottom = dimension(texture.height) + y;
    else if (vertical == 1) {
        const auto half = ExactFloat::integer(fp, texture.height >> 1).rounded();
        top = top - half;
        bottom = ExactFloat(fp, half) + y;
    } else
        top = top - dimension(texture.height);
    // Source spills right/bottom before the retained reciprocal and left/top stores.
    const auto bottom_value = bottom.rounded();
    const auto right_value = right.rounded();
    const auto reciprocal = ExactFloat(fp, 1.0f) / depth;
    state.vertices[1].y = bottom_value;
    state.vertices[2].x = right_value;
    state.vertices[3].x = right_value;
    const auto reciprocal_value = reciprocal.rounded();
    state.vertices[0].x = left.rounded();
    state.vertices[0].y = top.rounded();
    state.vertices[2].reciprocal_depth = reciprocal_value;
    state.vertices[3].y = bottom_value;
    state.vertices[0].reciprocal_depth = reciprocal.rounded();
    state.vertices[3].reciprocal_depth = reciprocal_value;
    state.vertices[1].x = left.rounded();
    state.vertices[1].reciprocal_depth = reciprocal.rounded();
    state.vertices[2].y = top.rounded();
    // Profile GVR_Init clears the format mask's immediate byte in the source.
    // The profile clears the format mask's immediate byte, so alpha is always enabled.
    calls.draw_texture(state.vertices, texture.global_index, 1);
}

void draw_box_fill_2d(float left, float top, float right, float bottom, float priority,
                      std::uint32_t color, std::int32_t &display_disabled,
                      std::uint8_t &fog_emulation, std::uint8_t &fog_enabled,
                      float &queued_depth_offset, Calls &calls, X87Context &fp) {
    if ((fog_emulation & 3) == 3 && fog_enabled == 0) {
        fog_emulation &= 253;
        queued_depth_offset = -47952.0f;
        calls.queue_box(left, top, right, bottom, priority, color);
        fog_emulation |= 2;
    }
    if (display_disabled != 0)
        return;
    const std::array<Point, 4> points{{{left, top}, {left, bottom},
                                      {right, top}, {right, bottom}}};
    const auto flags = (color & 4278190080U) == 4278190080U ? 34U : 98U;
    if (fidelity::ExactFloat(fp, priority).compare(-1.0f) == X87Comparison::greater)
        priority = -1.0f;
    calls.draw_polygon(points, color, priority, flags);
}
} // namespace sadx::pvr

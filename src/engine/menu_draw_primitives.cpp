#include "engine/menu_draw_primitives.hpp"
#include "cpu/fidelity_float.hpp"
#include <cstring>
#include <optional>
#include <stdexcept>

namespace sadx::menu_draw {
void set_vertex_colors(const std::array<std::uint32_t, 4> &colors, pvr::State &state) {
    const auto base = state.base_color;
    const auto first = pvr::blend_colors(colors[0], base);
    const auto second = pvr::blend_colors(colors[1], base);
    const auto third = pvr::blend_colors(colors[2], base);
    state.vertices[3].color = pvr::blend_colors(colors[3], base);
    state.vertices[0].color = first;
    state.vertices[2].color = third;
    state.vertices[1].color = second;
    state.alpha_enabled =
        (first & second & third & state.vertices[3].color & 4278190080U) != 4278190080U;
}
void draw_sized_texture(std::uint32_t index, float x, float y, float depth, float width,
                        float height, pvr::State &state, std::int32_t disabled, pvr::Calls &calls,
                        X87Context &fp) {
    if (disabled != 0)
        return;
    const auto texture = calls.texture(index & 822083583U);
    const auto flip = index >> 30;
    for (unsigned vertex = 0; vertex < 4; ++vertex) {
        state.vertices[vertex].u = float((vertex >= 2) != bool(flip & 2));
        state.vertices[vertex].v = float(bool(vertex & 1) != bool(flip & 1));
    }
    using fidelity::ExactFloat;
    // NOTSA: defer source stack values until their branch actually loads them,
    // preserving the failure prefix for unmasked floating-point exceptions.
    std::optional<ExactFloat> left, top;
    float right = x, bottom = y;
    switch ((index >> 24) & 15U) {
    case 0:
        left.emplace(fp, x);
        top.emplace(fp, y);
        right = (ExactFloat(fp, x) + width).rounded();
        bottom = (ExactFloat(fp, y) + height).rounded();
        break;
    case 1: {
        const auto half = (ExactFloat(fp, width) * 0.5f).rounded();
        left = ExactFloat(fp, x) - half;
        top.emplace(fp, y);
        right = (ExactFloat(fp, half) + x).rounded();
        bottom = (ExactFloat(fp, y) + height).rounded();
        break;
    }
    case 2:
        left = ExactFloat(fp, x) - width;
        top.emplace(fp, y);
        bottom = (ExactFloat(fp, y) + height).rounded();
        break;
    case 3: {
        const auto half = (ExactFloat(fp, height) * 0.5f).rounded();
        left.emplace(fp, x);
        top = ExactFloat(fp, y) - half;
        right = (ExactFloat(fp, x) + width).rounded();
        bottom = (ExactFloat(fp, half) + y).rounded();
        break;
    }
    case 4: {
        const auto half_width = (ExactFloat(fp, width) * 0.5f).rounded();
        const auto half_height = (ExactFloat(fp, height) * 0.5f).rounded();
        left = ExactFloat(fp, x) - half_width;
        top = ExactFloat(fp, y) - half_height;
        right = (ExactFloat(fp, half_width) + x).rounded();
        bottom = (ExactFloat(fp, half_height) + y).rounded();
        break;
    }
    case 5: {
        const auto half = (ExactFloat(fp, height) * 0.5f).rounded();
        left = ExactFloat(fp, x) - width;
        top = ExactFloat(fp, y) - half;
        bottom = (ExactFloat(fp, half) + y).rounded();
        break;
    }
    case 6:
        left.emplace(fp, x);
        top = ExactFloat(fp, y) - height;
        right = (ExactFloat(fp, x) + width).rounded();
        break;
    case 7: {
        const auto half = (ExactFloat(fp, width) * 0.5f).rounded();
        left = ExactFloat(fp, x) - half;
        top = ExactFloat(fp, y) - height;
        right = (ExactFloat(fp, half) + x).rounded();
        break;
    }
    case 8:
        left = ExactFloat(fp, x) - width;
        top = ExactFloat(fp, y) - height;
        break;
    default:
        throw std::runtime_error(
            "ghDrawPvrTextureWH invalid anchor uses indeterminate source stack");
    }
    const auto reciprocal = ExactFloat(fp, 1.0f) / depth;
    state.vertices[1].y = bottom;
    state.vertices[2].x = right;
    state.vertices[3].x = right;
    const auto reciprocal_value = reciprocal.rounded();
    state.vertices[0].x = left->rounded();
    state.vertices[0].y = top->rounded();
    state.vertices[2].reciprocal_depth = reciprocal_value;
    state.vertices[3].y = bottom;
    state.vertices[0].reciprocal_depth = reciprocal.rounded();
    state.vertices[3].reciprocal_depth = reciprocal_value;
    state.vertices[1].x = left->rounded();
    state.vertices[1].reciprocal_depth = reciprocal.rounded();
    state.vertices[2].y = top->rounded();
    // Pinned GVR_Init clears the format-mask byte used by source alpha selection.
    calls.draw_texture(state.vertices, texture.global_index, 1);
}
} // namespace sadx::menu_draw

namespace sadx::menu_draw {
void draw_title_back(float x, float y, float depth, float width, float height, pvr::State &state,
                     const std::int32_t &disabled, Calls &calls, X87Context &fp) {
    const auto base = state.base_color;
    const auto solid = pvr::blend_colors(4278218772U, base);
    const auto transparent = pvr::blend_colors(28692U, base);
    const auto bottom = fidelity::ExactFloat(fp, y) + height;
    const auto bottom_value = bottom.rounded();
    const auto right = fidelity::ExactFloat(fp, x) + width;
    const auto right_value = right.rounded();
    // Both source FST/FSTP stores use the retained ext80 right/bottom results.
    const std::array<pvr::Point, 4> points{
        {{x, y}, {x, bottom_value}, {right_value, y}, {right.rounded(), bottom.rounded()}}};
    const auto priority = (-fidelity::ExactFloat(fp, depth)).rounded();
    if (disabled != 0)
        return;
    calls.set_depth_compare(7);
    calls.set_depth_write(0);
    calls.draw_colored_polygon(points, {solid, solid, transparent, transparent}, priority, 96);
    calls.set_depth_write(1);
    calls.restore_depth_compare();
}
font::Info current_font(const font::State &state) { return state.current; }
void set_current_font(const font::Info &info, font::State &state) { state.current = info; }
namespace {
std::string_view ascii_text(std::string_view text) {
    const auto end = text.find('\0');
    if (end != std::string_view::npos)
        text = text.substr(0, end);
    for (const auto byte : text)
        if (static_cast<unsigned char>(byte) >= 128)
            throw std::runtime_error(
                "font text uses an unrecovered negative source character index");
    return text;
}
const std::array<std::uint8_t, 128> &proportions(const font::State &font) {
    if (!font.current.proportions)
        throw std::runtime_error("font text requires source proportions resource");
    return *font.current.proportions;
}
} // namespace
std::int32_t text_width(std::string_view text, const font::State &state) {
    std::uint32_t width = 0;
    for (const auto byte : ascii_text(text)) {
        const auto advance = proportions(state)[static_cast<unsigned char>(byte)];
        width += advance ? static_cast<std::uint32_t>(advance) + 1 : 4;
    }
    return static_cast<std::int32_t>(width);
}
std::int32_t draw_glyph(std::uint8_t glyph, float x, float y, float depth, font::State &state,
                        Calls &calls, X87Context &fp) {
    using fidelity::ExactFloat;
    std::array<pvr::Vertex, 4> vertices{};
    const auto absolute_depth = fidelity::absolute_float(depth);
    const auto reciprocal = ExactFloat(fp, 1.0f) / absolute_depth;
    const auto column = static_cast<std::int32_t>(glyph & 15U);
    const auto row = static_cast<std::int32_t>(static_cast<std::int8_t>(glyph)) / 16;
    const auto column_value = ExactFloat::integer(fp, column).rounded();
    const auto row_value = ExactFloat::integer(fp, row).rounded();
    const auto inverse_width = ExactFloat(fp, 1.0f) / state.current.tex_width;
    const auto rounded_inverse_width = inverse_width.rounded();
    const auto u = inverse_width * state.width * column_value;
    const auto right_u =
        ((ExactFloat(fp, column_value) + 1.0f) * rounded_inverse_width * state.width).rounded();
    const auto inverse_height = ExactFloat(fp, 1.0f) / state.current.tex_height;
    const auto rounded_inverse_height = inverse_height.rounded();
    const auto v = inverse_height * state.height * row_value;
    const auto bottom_v =
        (ExactFloat(fp, row_value) + 1.0f) * rounded_inverse_height * state.height;
    const auto bottom_v_value = bottom_v.rounded();
    const auto reciprocal_value = reciprocal.rounded();
    const auto u_value = u.rounded();
    const auto v_value = v.rounded();
    const auto bottom = (ExactFloat(fp, state.current.char_height) + y).rounded();
    const auto right = (ExactFloat(fp, state.current.char_width) + x).rounded();
    vertices[0] = {x, y, reciprocal_value, u_value, v_value, state.current.colors[0]};
    vertices[1] = {
        x, bottom, reciprocal.rounded(), u.rounded(), bottom_v.rounded(), state.current.colors[1]};
    vertices[2] = {right, y, reciprocal.rounded(), right_u, v.rounded(), state.current.colors[2]};
    vertices[3] = {right,   bottom,         reciprocal.rounded(),
                   right_u, bottom_v_value, state.current.colors[3]};
    if (!state.current.textures)
        throw std::runtime_error("font glyph requires source texture list");
    const auto global_index = calls.texture_global_index(*state.current.textures, 0);
    calls.draw_texture(vertices, global_index, 1);
    if (state.current.proportions) {
        if (glyph >= 128)
            throw std::runtime_error("font glyph result uses negative source proportions index");
        return (*state.current.proportions)[glyph] + 2;
    }
    return static_cast<std::int32_t>(ExactFloat(fp, state.current.char_width).truncated_integer());
}
void draw_text(std::string_view text, float x, float y, float depth, font::State &state,
               Calls &calls, X87Context &fp) {
    using fidelity::ExactFloat;
    for (const auto byte : ascii_text(text)) {
        const auto width = proportions(state)[static_cast<unsigned char>(byte)];
        if (width == 0) {
            x = (ExactFloat(fp, x) + 4.0f).rounded();
        } else {
            const auto half =
                (ExactFloat(fp, state.current.char_width) - ExactFloat::integer(fp, width)) * 0.5f;
            const auto offset = half.truncated_integer();
            const auto glyph_x = (ExactFloat(fp, x) - ExactFloat::integer(fp, offset)).rounded();
            draw_glyph(static_cast<std::uint8_t>(byte), glyph_x, y, depth, state, calls, fp);
            x = (ExactFloat::integer(fp, width + 1) + x).rounded();
        }
    }
}
} // namespace sadx::menu_draw

namespace sadx::menu_draw {
std::uint32_t interpolate_color(std::uint32_t first, std::uint32_t second, float progress,
                                X87Context &fp) {
    using fidelity::ExactFloat;
    const auto inverse = (ExactFloat(fp, 1.0f) - progress).rounded();
    std::array<std::uint32_t, 4> values{};
    constexpr std::array<unsigned, 4> shifts{24, 16, 8, 0};
    for (unsigned i = 0; i < 4; ++i) {
        const auto a = ExactFloat::integer(fp, (first >> shifts[i]) & 255U) * inverse;
        const auto b = ExactFloat::integer(fp, (second >> shifts[i]) & 255U) * progress;
        const auto sum = a + b;
        const auto value = i == 2 ? sum : ExactFloat(fp, sum.rounded());
        values[i] = static_cast<std::uint32_t>(value.truncated_integer()) & 255U;
    }
    return (values[0] << 24) | (values[1] << 16) | (values[2] << 8) | values[3];
}
void draw_shadow_window(float x, float y, float depth, float width, float height, pvr::State &state,
                        const std::int32_t &disabled, Calls &calls, X87Context &fp) {
    using fidelity::ExactFloat;
    calls.set_blend(0, 8);
    calls.set_blend(1, 6);
    calls.reset_material();
    const auto top_color = state.vertices[0].color;
    const auto bottom_color = state.vertices[1].color;
    // Source78c5a0 is an unchanged RET; it has no native work to perform.
    calls.select_standard_texture(StandardTexture::Window);
    pvr::set_material(3229646976U, state);
    const auto reciprocal = ExactFloat(fp, 1.0f) / height;
    const auto rounded_reciprocal = reciprocal.rounded();
    const auto first_progress = (reciprocal * 16.0f).rounded();
    const auto first_color = interpolate_color(top_color, bottom_color, first_progress, fp);
    const auto second_progress = ((ExactFloat(fp, height) - 16.0f) * rounded_reciprocal).rounded();
    const auto second_color = interpolate_color(top_color, bottom_color, second_progress, fp);
    set_vertex_colors({top_color, first_color, top_color, first_color}, state);
    pvr::draw_texture(0, x, y, depth, state, disabled, calls, fp);
    const auto right = (ExactFloat(fp, x) + width).rounded();
    pvr::draw_texture(2181038080U, right, y, depth, state, disabled, calls, fp);
    set_vertex_colors({second_color, bottom_color, second_color, bottom_color}, state);
    const auto bottom = (ExactFloat(fp, y) + height).rounded();
    pvr::draw_texture(1174405120U, x, bottom, depth, state, disabled, calls, fp);
    pvr::draw_texture(3355443200U, right, bottom, depth, state, disabled, calls, fp);
    set_vertex_colors({first_color, second_color, first_color, second_color}, state);
    const auto inner_height = (ExactFloat(fp, height) - 32.0f).rounded();
    const auto inner_y = (ExactFloat(fp, y) + 16.0f).rounded();
    draw_sized_texture(1, x, inner_y, depth, 16.0f, inner_height, state, disabled, calls, fp);
    const auto inner_right = (ExactFloat(fp, right) - 16.0f).rounded();
    draw_sized_texture(1, inner_right, inner_y, depth, 16.0f, inner_height, state, disabled, calls,
                       fp);
    set_vertex_colors({top_color, bottom_color, top_color, bottom_color}, state);
    const auto inner_width = (ExactFloat(fp, width) - 32.0f).rounded();
    const auto inner_x = (ExactFloat(fp, x) + 16.0f).rounded();
    draw_sized_texture(1, inner_x, y, depth, inner_width, height, state, disabled, calls, fp);
    calls.set_blend(0, 8);
    calls.set_blend(1, 6);
    calls.reset_material();
}
} // namespace sadx::menu_draw

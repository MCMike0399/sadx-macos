#pragma once
#include "cpu/x87_math.hpp"
#include "engine/graphics_frame.hpp"
#include "engine/native_texture_lifetime.hpp"
#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace sadx::sprite {
struct AnimationFrame {
    std::int16_t width, height, center_x, center_y;
    std::int16_t u_begin, v_begin, u_end, v_end;
    std::int16_t texture_index, attributes;
};
using TextureList = texture_lifetime::List;
struct Sprite {
    std::array<float, 3> position;
    float scale_x, scale_y;
    std::int32_t angle;
    const TextureList *textures;
    const std::vector<AnimationFrame> &animation;
};
// NOTSA: source-used sprite fields only. References preserve live reloads after
// renderer callbacks; no unused source position.z or unknown angle is invented.
struct ScreenSpriteView {
    const float &x, &y, &scale_x, &scale_y;
    const TextureList *const &textures;
    std::function<const AnimationFrame &(std::int32_t)> frame_at;
    std::function<std::int32_t()> angle;
};
struct Quad {
    std::array<float, 3> origin;
    std::array<float, 2> horizontal, vertical;
    std::array<float, 2> uv_origin, uv_horizontal, uv_vertical;
};
struct ImmediateServices {
    std::function<void(const TextureList &)> select_texture_list;
    std::function<void()> push_unit_matrix;
    std::function<void(std::int32_t)> rotate_z;
    std::function<std::array<float, 3>(const std::array<float, 3> &)> transform_vector;
    std::function<void()> pop_matrix;
    std::function<void(bool)> begin_quad;
    std::function<void(std::uint32_t)> set_hud_color;
    std::function<void(std::int16_t)> select_texture;
    std::function<void(const Quad &)> draw_quad;
    std::function<void()> restore_linear_filter;
};
// NOTSA: defined-field adapter for source stack sprites whose angle is genuinely
// uninitialized when attributes bit0 is clear. The original entry below shares
// this same readable body; a selected angle branch requires an actual getter.
void draw_screen_sprite(const ScreenSpriteView *, std::int32_t animation_index, float priority,
                        std::uint8_t attributes, const std::array<float, 4> &material_argb,
                        ImmediateServices &, X87Context &);
// FUNCTION: SONIC 0x77e050 njDrawSprite2D
void draw_sprite(const Sprite *, std::int32_t animation_index, float priority,
                 std::uint8_t attributes, const std::array<float, 4> &material_argb,
                 ImmediateServices &, X87Context &);
} // namespace sadx::sprite

namespace sadx::sprite {
struct ScreenVertex {
    float x, y, depth, reciprocal_w;
    std::uint32_t color;
    float u, v;
};
struct QuadServices {
    std::function<X87Value(float)> projected_depth;
    std::function<void(const std::array<ScreenVertex, 4> &)> submit_strip;
};
// FUNCTION: SONIC 0x77de10 njDrawQuadTextureEx
void draw_quad(const Quad &, std::uint32_t hud_color, std::array<ScreenVertex, 4> &scratch,
               QuadServices &, X87Context &);
// FUNCTION: SONIC 0x77dd90 njQuadTextureStart
void begin_quad(bool alpha, graphics::Draw &device_state);
} // namespace sadx::sprite

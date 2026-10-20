#pragma once
#include "cpu/x87_math.hpp"
#include <array>
#include <cstdint>
#include <functional>

namespace sadx::pvr {
struct Vertex {
    float x = 0, y = 0, reciprocal_depth = 0, u = 0, v = 0;
    std::uint32_t color = 0;
};
// NOTSA: semantic storage supplied by the session; reference owners retain original layout.
struct State {
    std::array<Vertex, 4> vertices{};
    std::uint32_t base_color = 0, alpha_enabled = 0;
    std::uint32_t texture_shading = 0, texture_flags = 0;
    float scale = 0;
    std::uint32_t mode = 0;
};
struct TextureInfo {
    std::uint32_t global_index, surface_flags, width, height;
};
struct Point {
    float x, y;
};
// NOTSA: immediate and queued submission boundaries keep actual renderer services explicit.
struct Calls {
    virtual ~Calls() = default;
    virtual TextureInfo texture(std::uint32_t index) = 0;
    virtual void draw_texture(const std::array<Vertex, 4> &, std::uint32_t global_index,
                              std::uint32_t alpha_enabled) = 0;
    virtual void draw_polygon(const std::array<Point, 4> &, std::uint32_t color,
                              float priority, std::uint32_t flags) = 0;
    virtual void queue_box(float left, float top, float right, float bottom,
                           float priority, std::uint32_t color) = 0;
};
// FUNCTION: SONIC 0x432ea0 ghInitPvrTexture
void initialize(State &);
// FUNCTION: SONIC 0x432df0 BlendColors
std::uint32_t blend_colors(std::uint32_t first, std::uint32_t second);
// FUNCTION: SONIC 0x432f40 ghSetPvrTexMaterial
void set_material(std::uint32_t color, State &);
// FUNCTION: SONIC 0x433010 ghSetPvrTexBaseColor
void set_base_color(std::uint32_t color, State &);
// FUNCTION: SONIC 0x433170 ghDefaultBlendingMode
void default_blending(const std::function<void(std::uint32_t, std::uint32_t)> &set_blend);
// FUNCTION: SONIC 0x4338d0 ghDrawPvrTexture
void draw_texture(std::uint32_t index, float x, float y, float depth, State &,
                  std::int32_t display_disabled, Calls &, X87Context &);
// FUNCTION: SONIC 0x4071c0 ds_DrawBoxFill2D
void draw_box_fill_2d(float left, float top, float right, float bottom, float priority,
                      std::uint32_t color, std::int32_t &display_disabled,
                      std::uint8_t &fog_emulation, std::uint8_t &fog_enabled,
                      float &queued_depth_offset, Calls &, X87Context &);
} // namespace sadx::pvr

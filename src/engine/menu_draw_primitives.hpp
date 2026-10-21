#pragma once
#include "engine/font_native.hpp"
#include "engine/pvr_draw.hpp"
#include <string_view>

namespace sadx::menu_draw {
enum class StandardTexture { Square, Window };
struct Calls : pvr::Calls {
    virtual void select_standard_texture(StandardTexture) = 0;
    virtual void set_blend(std::uint32_t target, std::uint32_t mode) = 0;
    virtual void reset_material() = 0;
    virtual void set_depth_compare(std::uint32_t) = 0;
    virtual void set_depth_write(std::uint32_t) = 0;
    virtual void restore_depth_compare() = 0;
    virtual void draw_colored_polygon(const std::array<pvr::Point, 4> &,
                                      const std::array<std::uint32_t, 4> &, float priority,
                                      std::uint32_t flags) = 0;
    // This source query does not select or modify the current Ninja texture list.
    virtual std::uint32_t texture_global_index(const texture_lifetime::List &, std::uint32_t) = 0;
};
// FUNCTION: SONIC 0x433190 InterpolatePvrColor
std::uint32_t interpolate_color(std::uint32_t first, std::uint32_t second, float progress,
                                X87Context &);
// FUNCTION: SONIC 0x4334f0 DrawTitleBack
void draw_title_back(float x, float y, float depth, float width, float height, pvr::State &,
                     const std::int32_t &display_disabled, Calls &, X87Context &);
// FUNCTION: SONIC 0x4343e0 DrawShadowWindow
void draw_shadow_window(float x, float y, float depth, float width, float height, pvr::State &,
                        const std::int32_t &display_disabled, Calls &, X87Context &);
// FUNCTION: SONIC 0x420d30 ghGetFont
font::Info current_font(const font::State &);
// FUNCTION: SONIC 0x420d50 ghSetFont
void set_current_font(const font::Info &, font::State &);
// FUNCTION: SONIC 0x420cf0 ghGetStringWidth
std::int32_t text_width(std::string_view, const font::State &);
// FUNCTION: SONIC 0x420aa0 DrawStringCharacter
std::int32_t draw_glyph(std::uint8_t, float x, float y, float depth, font::State &, Calls &,
                        X87Context &);
// FUNCTION: SONIC 0x420c50 ghFontPuts
void draw_text(std::string_view, float x, float y, float depth, font::State &, Calls &,
               X87Context &);

// FUNCTION: SONIC 0x432f90 ghSetPvrTexVertexColor
void set_vertex_colors(const std::array<std::uint32_t, 4> &, pvr::State &);
// FUNCTION: SONIC 0x433c90 ghDrawPvrTextureWH
void draw_sized_texture(std::uint32_t index, float x, float y, float depth, float width,
                        float height, pvr::State &, std::int32_t display_disabled, pvr::Calls &,
                        X87Context &);
} // namespace sadx::menu_draw

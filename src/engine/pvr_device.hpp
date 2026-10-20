#pragma once
#include "engine/pvr_draw.hpp"
#include "engine/graphics_frame.hpp"
#include <functional>

namespace sadx::pvr {
// NOTSA: borrowed current device state and the recovered Ninja depth projection service.
struct ImmediateDevice {
    graphics::Draw &state;
    graphics::Frame &frame;
    std::function<float(float)> projected_depth;
};
// FUNCTION: SONIC 0x791860 Direct3D_SetDefaultRenderState
void set_default_render_state(graphics::Draw &);
// FUNCTION: SONIC 0x7943f0 Direct3D_SetDefaultTextureStageState
void set_default_texture_stage_state(graphics::Draw &);
// NOTSA: composes njDrawTexture and its screen-space triangle strip submission.
void submit_texture(const std::array<Vertex, 4> &, std::shared_ptr<const graphics::Texture>,
                    std::uint32_t alpha_enabled, bool fog_enabled, ImmediateDevice &,
                    X87Context &);
// NOTSA: composes ___SAnjDrawPolygon2D's non-textured strip path for ds_DrawBoxFill2D.
void submit_box(const std::array<Point, 4> &, std::uint32_t color, float priority,
                std::uint32_t flags, ImmediateDevice &, X87Context &);
} // namespace sadx::pvr

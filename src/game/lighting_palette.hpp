#pragma once
#include "engine/graphics_frame.hpp"
#include "engine/ninja_matrix.hpp"
#include "game/stage_lighting.hpp"

namespace sadx::lighting {
// NOTSA: source scratch light and the direction shared by stage and palette rendering.
struct DeviceState {
    // GLOBAL: SONIC 0x3abdb50 CurrentDeviceLight size=104
    graphics::Light light = {static_cast<graphics::LightType>(0), {}, {}, {}, {}, {}, 0, 0, {}, 0, 0};
    // GLOBAL: SONIC 0x9008a8 CurrentStageLight_Direction size=12
    std::array<float, 3> stage_direction = {0, -1, 0};
};
// NOTSA: the source device receives these commands immediately. Missing native
// device services remain an explicit composition error, never skipped writes.
struct DeviceServices {
    std::function<void(std::uint32_t, const graphics::Light &)> set_light;
    std::function<void(std::uint32_t, bool)> enable_light;
    std::function<void(std::uint32_t, std::uint32_t)> render_state;
};
// NOTSA: all existing queue, stage, camera and lighting owners are borrowed.
struct PaletteView {
    State &scene;
    StageView stage;
    const std::int32_t &loop_count;
    std::int32_t &palette_number;
    std::uint8_t &palette_copy;
    DeviceState &device;
    NinjaMatrixPrimitives &matrices;
};
class DirectionNormalizationUnavailable : public std::runtime_error {
  public:
    DirectionNormalizationUnavailable()
        : std::runtime_error("D3DX stage light direction normalization is not recovered") {}
};
// STUB: SONIC 0x775630 D3DXVec3Normalize_SSE source stage-direction normalization
std::array<float, 3> normalize_stage_direction(const std::array<float, 3> &);
// FUNCTION: SONIC 0x401440 SetCurrentLightType_Copy
void copy_palette_type(std::int32_t, std::uint8_t &);
// FUNCTION: SONIC 0x40a630 ApplyLSPalette
void apply_palette(PaletteView, const DeviceServices &, X87Context &);
// FUNCTION: SONIC 0x412420 ___dsSetPalette
void set_palette(std::int32_t, PaletteView, TimeServices &, const DeviceServices &, X87Context &);
} // namespace sadx::lighting

#pragma once
#include "engine/ninja_view_native.hpp"
#include "runtime/logo_renderer.hpp"
#include <memory>

namespace sadx::native {
// NOTSA: native presentation owners initialized by the recovered camera/device operations.
class Display {
  public:
    explicit Display(const std::filesystem::path &system_directory);
    LogoRenderer &output() { return *output_; }
    const LogoRenderer &output() const { return *output_; }
    LogoFrame frame_state();
    X87Context &arithmetic() { return arithmetic_; }

  private:
    X87Context arithmetic_{127};
    SseContext sse_{8064};
    graphics::Draw device_state_;
    view::State view_;
    NinjaMath::SineTable sine_;
    NinjaMatrixStack matrices_;
    D3dxMatrix screen_transform_;
    std::unique_ptr<LogoRenderer> output_;
    float horizontal_scale_ = 1, vertical_scale_ = 1, queued_depth_ = 0;
    std::int32_t display_disabled_ = 0;
    std::uint8_t fog_emulation_ = 0, fog_enabled_ = 0;
};
} // namespace sadx::native

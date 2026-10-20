#pragma once
#include "engine/ninja_view_native.hpp"
#include "engine/sprite_immediate.hpp"
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
    SseContext &sse() { return sse_; }
    void set_camera();
    void set_clip_range(float near_distance, float far_distance);
    void draw_sprite(const sprite::Sprite &, std::int32_t frame, float priority,
                     std::uint8_t attributes);
    void set_constant_material(const std::array<float, 4> &argb) { material_argb_ = argb; }
    void set_depth_comparison(std::uint32_t mode);
    void set_depth_write(bool enabled);
    void restore_depth_comparison();
    void release_all_textures();

  private:
    X87Context arithmetic_{127};
    SseContext sse_{8064};
    graphics::Draw device_state_;
    graphics::Frame frame_;
    view::State view_;
    NinjaMath::SineTable sine_;
    NinjaMatrixStack matrices_;
    std::unique_ptr<NinjaMatrixPrimitives> coordinates_;
    D3dxMatrix screen_transform_;
    std::unique_ptr<LogoRenderer> output_;
    std::array<float, 4> material_argb_{};
    std::array<sprite::ScreenVertex, 4> sprite_vertices_{};
    const sprite::TextureList *sprite_textures_ = nullptr;
    std::uint32_t hud_color_ = 0;
    std::uint32_t default_depth_comparison_ = 1;
    float horizontal_scale_ = 1, vertical_scale_ = 1, queued_depth_ = 0;
    std::int32_t display_disabled_ = 0;
    std::uint8_t fog_emulation_ = 0, fog_enabled_ = 0;
    view::Services view_services();
    void submit_sprite_quad(const sprite::Quad &);
};
} // namespace sadx::native

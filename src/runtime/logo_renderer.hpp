#pragma once
#include "engine/ninja_projected_depth.hpp"
#include "engine/pvr_device.hpp"
#include "runtime/logo_scene.hpp"
#include <filesystem>

namespace sadx::native {
// NOTSA: assets-only logo resource owner and shared-device command submission.
class LogoRenderer final : public LogoOutput {
  public:
    LogoRenderer(std::filesystem::path system_directory, graphics::Draw &device_state,
                 NinjaDepthProjection &, X87Context &, SseContext &,
                 SseReciprocalEstimate);
    void begin_frame();
    const graphics::Frame &frame() const { return frame_; }
    std::size_t loaded_textures() const { return textures_.size(); }
    void load_logo_archive(std::string_view name) override;
    void select_logo_archive() override;
    void release_logo_archive() override;
    void set_background(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void set_blending(std::uint32_t, std::uint32_t) override;
    pvr::TextureInfo texture(std::uint32_t index) override;
    void draw_texture(const std::array<pvr::Vertex, 4> &, std::uint32_t global_index,
                      std::uint32_t alpha) override;
    void draw_polygon(const std::array<pvr::Point, 4> &, std::uint32_t color,
                      float priority, std::uint32_t flags) override;
    void queue_box(float, float, float, float, float, std::uint32_t) override;

  private:
    struct TextureSlot {
        std::uint32_t global_index;
        std::shared_ptr<const graphics::Texture> texture;
    };
    std::filesystem::path system_directory_;
    graphics::Draw &state_;
    NinjaDepthProjection &projection_;
    X87Context &arithmetic_;
    SseContext &sse_;
    SseReciprocalEstimate reciprocal_;
    graphics::Frame frame_;
    std::vector<TextureSlot> textures_;
    bool selected_ = false;
    std::uint32_t background_ = 4278190080U;
    pvr::ImmediateDevice immediate_device();
};
} // namespace sadx::native

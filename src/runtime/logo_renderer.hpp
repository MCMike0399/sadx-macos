#pragma once
#include "engine/ninja_projected_depth.hpp"
#include "engine/pvr_device.hpp"
#include "engine/native_texture_lifetime.hpp"
#include "runtime/logo_scene.hpp"
#include <filesystem>

namespace sadx::native {
// NOTSA: native immediate renderer and raw archive loader, including startup's logo list.
class LogoRenderer final : public LogoOutput {
  public:
    LogoRenderer(std::filesystem::path system_directory, graphics::Draw &device_state,
                 graphics::Frame &frame,
                 NinjaDepthProjection &, X87Context &, SseContext &,
                 SseReciprocalEstimate, texture_lifetime::State);
    void begin_frame();
    const graphics::Frame &frame() const { return frame_; }
    std::size_t loaded_textures() const;
    std::size_t texture_slots_in_use() const;
    void load_logo_archive(std::string_view name) override;
    std::vector<texture_lifetime::Memory *> load_pvm_archive(std::string_view);
    std::int32_t load_individual_textures(texture_lifetime::List &);
    void select_texture_list(const texture_lifetime::List &);
    using ArchiveLoader = std::function<void(std::string_view, texture_lifetime::List &)>;
    void bind_archive_loader(ArchiveLoader loader) { archive_loader_ = std::move(loader); }
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
        texture_lifetime::Memory *memory;
    };
    std::filesystem::path system_directory_;
    graphics::Draw &state_;
    NinjaDepthProjection &projection_;
    X87Context &arithmetic_;
    SseContext &sse_;
    SseReciprocalEstimate reciprocal_;
    graphics::Frame &frame_;
    texture_lifetime::State texture_state_;
    texture_lifetime::List texture_list_;
    ArchiveLoader archive_loader_;
    std::vector<TextureSlot> textures_;
    std::uint32_t background_ = 4278190080U;
    pvr::ImmediateDevice immediate_device();
    texture_lifetime::Memory *load_record(const std::shared_ptr<NinjaTextureArchive> &,
                                          std::size_t, std::uint32_t);
};
} // namespace sadx::native

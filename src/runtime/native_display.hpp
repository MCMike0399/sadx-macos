#pragma once
#include "engine/ninja_view_native.hpp"
#include "engine/sprite_immediate.hpp"
#include "engine/ninja_queue_native.hpp"
#include "engine/ninja_fog.hpp"
#include "game/game_view_native.hpp"
#include "runtime/logo_renderer.hpp"
#include "game/menu_caption.hpp"
#include "game/message_texture.hpp"
#include "game/saved_fov.hpp"
#include "engine/native_message_texture.hpp"
#include "engine/queue_consumer_native.hpp"
#include "engine/message_sprite_queue.hpp"
#include "engine/screen_sprite.hpp"
#include "engine/message_sprite_profile.hpp"
#include <memory>

namespace sadx::native {
// NOTSA: native presentation owners initialized by the recovered camera/device operations.
class Display {
  public:
    // Native configuration defaults to the source's absent CTRLFLAG0 setting.
    Display(const std::filesystem::path &system_directory, frame::ModeState &,
            std::uint32_t renderer_control_flags = 0);
    LogoRenderer &output() { return *output_; }
    const LogoRenderer &output() const { return *output_; }
    LogoFrame frame_state();
    const D3dxViewport &viewport() const { return view_.depth.viewport; }
    X87Context &arithmetic() { return arithmetic_; }
    SseContext &sse() { return sse_; }
    void set_camera();
    void set_perspective(std::int32_t, std::uint32_t &pause_state);
    void set_view_angle(std::int32_t, std::uint32_t &pause_state);
    void set_lighting(bool enabled);
    void set_clip_range(float near_distance, float far_distance);
    ui_scale::State &ui_scaling() { return ui_scaling_; }
    void draw_screen_sprite(const sprite::ScreenSprite &, std::int32_t, float, std::uint8_t);
    void draw_sprite(const sprite::Sprite &, std::int32_t frame, float priority,
                     std::uint8_t attributes);
    void set_constant_material(const std::array<float, 4> &argb) { material_argb_ = argb; }
    void set_depth_comparison(std::uint32_t mode);
    void set_depth_write(bool enabled);
    void restore_depth_comparison();
    void release_all_textures();
    void clear_texture_binding();
    void release_texture_list(sprite::TextureList &);
    void release_texture_list(sprite::TextureList *);
    void release_movie_texture_list(sprite::TextureList &);
    texture_lifetime::Memory *create_argb_texture(
        std::uint32_t global_index, std::shared_ptr<const std::vector<std::uint8_t>> pixels,
        std::shared_ptr<const graphics::Texture>);
    std::int32_t load_message_texture(texture_lifetime::List &, const message_texture::Image &);
    void reload_message_texture(std::uint32_t, const std::uint16_t *, std::uint32_t, std::uint32_t);
    void consume_draw_queue();
    menu_caption::BitmapView message_bitmap_from_end(std::uint32_t);
    queue::AnimationAllocation allocate_message_animation(std::uint32_t);
    void set_message_material(const std::array<float, 4> &);
    void publish_message_sprite(const sprite::ScreenSprite &);
    void set_message_sprite_y(float value) { message_sprite_.value.y = value; }
    void submit_message_sprite(std::int32_t, float, std::uint32_t, std::uint32_t);
    void reset_message_queue();
    void skip_queue_execution();
    std::uint8_t &reset_pending() { return reset_pending_; }
    std::int32_t &saved_fov() { return saved_fov_.value; }
    std::uint32_t current_fov() const { return static_cast<std::uint32_t>(perspective_copy_); }
    void begin_game_frame();
    void end_game_frame();
    void setup_screen(std::uint32_t &pause_state);
    void set_blank_queue_mode();
    void set_depth_compare_mode(std::uint32_t);
    void enable_fog();
    void apply_fog_emulation_setting(std::int32_t value);
    void disable_fog();
    void reset_material();
    void set_alpha_test_enabled(bool enabled);
    menu_caption::DeferredRelease &message_texture_releases() { return message_texture_releases_; }
    const menu_caption::DeferredRelease &message_texture_releases() const {
        return message_texture_releases_;
    }

  private:
    frame::ModeState &frame_mode_;
    X87Context arithmetic_{127};
    SseContext sse_{8064};
    graphics::Draw device_state_;
    graphics::Frame frame_;
    view::State view_;
    NinjaMath::SineTable sine_;
    NinjaMatrixStack matrices_;
    std::unique_ptr<NinjaMatrixPrimitives> coordinates_;
    D3dxMatrix screen_transform_;
    std::array<float, 4> material_argb_{};
    std::array<sprite::ScreenVertex, 4> sprite_vertices_{};
    std::vector<texture_lifetime::Memory> texture_pool_ =
        std::vector<texture_lifetime::Memory>(2048);
    std::int32_t cached_texture_global_index_ = -1;
    texture_lifetime::Memory *active_texture_memory_ = nullptr;
    const sprite::TextureList *current_texture_list_ = nullptr;
    std::array<std::uint32_t, 8> temporary_texture_info_{};
    std::uint32_t registered_texture_list_count_ = 0;
    // NOTSA: the message release queue borrows the existing canonical count.
    menu_caption::DeferredRelease message_texture_releases_{registered_texture_list_count_, {}};
    std::unique_ptr<LogoRenderer> output_;
    texture_lifetime::State texture_state();
    texture_lifetime::Services texture_services();
    std::uint32_t hud_color_ = 0;
    std::uint32_t default_depth_comparison_ = 1;
    float horizontal_scale_ = 1, vertical_scale_ = 1, queued_depth_ = 0;
    std::uint8_t reset_pending_ = 0;
    std::int32_t animation_queue_state_ = 0;
    GameViewInitialScreen screen_descriptor_;
    float projection_center_x_ = 0, projection_center_y_ = 0;
    std::int32_t perspective_copy_ = 0;
    view::SavedFovState saved_fov_;
    std::int8_t camera_type_ = 0;
    std::uint8_t interpolate_camera_angle_ = 0;
    std::int32_t previous_camera_angle_ = 0;
    std::array<float, 4> reset_material_{}, view_material_{};
    ui_scale::State ui_scaling_{horizontal_scale_, vertical_scale_};
    sprite::ImmediateServices sprite_services();
    queue::Storage queue_storage_;
    queue::State queue_state_;
    texture_palette::State texture_palettes_;
    sprite::ScreenSpriteScratch message_sprite_;
    // Native world storage: SONIC0x3abd950, separate from the selected device texture list.
    const texture_lifetime::List *queued_texture_list_ = nullptr;
    std::uint32_t control_3d_ = 0, constant_attribute_mask_ = 0, constant_attributes_ = 0;
    queue::Owners queue_owners();
    queue::Services queue_services();
    screen::Services screen_services();
    fog::State fog_;
    std::uint32_t renderer_control_flags_ = 0;
    fog::Runtime fog_runtime();
    view::Services view_services();
    void submit_sprite_quad(const sprite::Quad &);
};
} // namespace sadx::native

#include "runtime/native_title_scene.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"
#include "game/message_frame.hpp"
#include "game/saving_message.hpp"
#include "game/message_texture.hpp"
#include <cstring>

namespace sadx::native {
// NOTSA: all message producers, uploads and the save scheduler borrow the same
// native-world contexts, queue arena, texture pool and source frame owners.
struct TitleScene::MessageServices final : message_frame::Calls, message_texture::Calls,
                                          saving_message::Calls, save::FrameCalls {
    explicit MessageServices(TitleScene &owner) : scene(owner) {}
    TitleScene &scene;
    queue::AnimationAllocation animation{};
    menu_caption::Context &resolve(const cleanup::Message *entry) override {
        if (entry == &scene.session_.menu_caption_.message)
            return scene.session_.menu_caption_.message;
        if (entry == &scene.session_.saving_message_)
            return scene.session_.saving_message_;
        throw flow::UnrecoveredGameFlowCall("native_unknown_message_context");
    }
    std::uint32_t current_fov() override { return scene.display_.current_fov(); }
    void set_fov(std::uint32_t value) override {
        std::int32_t angle;
        std::memcpy(&angle, &value, sizeof(angle));
        scene.display_.set_perspective(angle, scene.session_.flow_.pause_state);
    }
    void set_depth_comparison(std::uint32_t value) override {
        scene.display_.set_depth_comparison(value);
    }
    void set_depth_write(bool value) override { scene.display_.set_depth_write(value); }
    void default_blending() override { scene.default_blending(); }
    void set_blend(std::uint32_t side, std::uint32_t mode) override { scene.set_blend(side, mode); }
    void set_panel_color(std::uint32_t color) override { scene.set_vertex_colors({color, color, color, color}); }
    void draw_panel(float x, float y, float z, float width, float height) override {
        scene.draw_shadow_window(x, y, z, width, height);
    }
    void draw_box(float left, float top, float right, float bottom, float priority,
                  std::uint32_t color) override {
        const auto frame = scene.display_.frame_state();
        pvr::draw_box_fill_2d(left, top, right, bottom, priority, color, frame.display_disabled,
            frame.fog_emulation, frame.fog_enabled, frame.queued_depth_offset,
            scene.display_.output(), scene.display_.arithmetic());
    }
    void upload_texture(menu_caption::Context &context) override {
        message_texture::upload(context, scene.session_.skip_message_mipmaps_, *this);
    }
    sprite::AnimationFrame &allocate_animation(std::uint32_t bytes) override {
        animation = scene.display_.allocate_message_animation(bytes);
        return *animation.frame;
    }
    void set_material(const std::array<float, 4> &value) override {
        scene.display_.set_message_material(value);
    }
    void publish_centered_sprite(const message_frame::SpriteInput &value) override {
        sprite::ScreenSprite source;
        source.x = value.x;
        source.y = value.y;
        source.scale_x = value.scale_x;
        source.scale_y = value.scale_y;
        source.textures = value.textures;
        source.animation = value.animation;
        source.animation_count = 1;
        source.animation_owner = animation.storage.owner;
        scene.display_.publish_message_sprite(source);
    }
    void set_sprite_y(float y) override { scene.display_.set_message_sprite_y(y); }
    void submit_sprite(std::int32_t frame, float priority, std::uint32_t attributes,
                       std::uint32_t flags) override {
        scene.display_.submit_message_sprite(frame, priority, attributes, flags);
    }
    void complete_caption_frame() override { scene.complete_caption_frame(); }
    std::int32_t load_image(texture_lifetime::List &list, const message_texture::Image &image) override {
        return scene.display_.load_message_texture(list, image);
    }
    void reload_image(std::uint32_t id, const std::uint16_t *pixels, std::uint32_t attrs,
                      std::uint32_t palette) override {
        scene.display_.reload_message_texture(id, pixels, attrs, palette);
    }
    std::shared_ptr<menu_caption::Buffer> allocate_buffer(std::size_t bytes) override {
        return scene.allocate_buffer(bytes);
    }
    void free_buffer(std::shared_ptr<menu_caption::Buffer> buffer) override {
        scene.free_buffer(std::move(buffer));
    }
    void wait_for_cooperative_thread() override { scene.wait_for_cooperative_thread(); }
    void append_message(menu_caption::Context &context, const std::string &text) override {
        scene.append_message(context, text);
    }
    void load_message_texture(menu_caption::Context &context) override {
        scene.load_message_texture(context);
    }
    void draw_message(menu_caption::Context &context) override { scene.draw_message(context); }
    void release_texture_list(texture_lifetime::List &list) override { scene.release_texture_list(list); }
    void release_queue_overflow() override { scene.release_queue_overflow(); }
    menu_caption::BitmapView bitmap_from_arena_end(std::uint32_t bytes) override {
        return scene.display_.message_bitmap_from_end(bytes);
    }
    void disable_fog() override { scene.display_.disable_fog(); }
    void draw_black_box(float right, float bottom, float priority, std::uint32_t color) override {
        draw_box(0, 0, right, bottom, priority, color);
    }
    void reset_draw_queue() override { scene.display_.reset_message_queue(); }
    void skip_queue_execution() override { scene.display_.skip_queue_execution(); }
    void display_saving_message(menu_caption::Context &context) override {
        if (&context != &scene.session_.saving_message_)
            throw std::runtime_error("native saving context identity changed");
        if (!scene.session_.message_font_assets_)
            throw flow::UnrecoveredGameFlowCall("native_saving_text_asset");
        const auto frame = scene.display_.frame_state();
        saving_message::display({scene.session_.saving_message_pending_, context,
            scene.session_.flow_.pause_state, scene.display_.reset_pending(), frame.horizontal_scale,
            frame.vertical_scale, scene.session_.text_language_, scene.session_.message_fonts_,
            scene.session_.message_font_assets_->saving_text}, *this, scene.display_.arithmetic());
    }
    void close_saving_message(menu_caption::Context &context) override {
        menu_caption::close_message(context, scene.display_.message_texture_releases(), *this);
    }
    void write_save_file() override { scene.session_.write_live_save(); }
    save::FrameStateView frame_state() {
        auto &s = scene.session_;
        return {s.flow_.save_ready, s.save_count_, s.flow_.save_phase, s.save_exec_,
                s.saving_message_pending_, s.flow_.pause_state, s.saving_message_};
    }
};
void TitleScene::render_messages() {
    MessageServices services(*this);
    const auto frame = display_.frame_state();
    message_frame::render({session_.messages_, display_.saved_fov(), frame.horizontal_scale,
                           frame.vertical_scale}, services, display_.arithmetic());
}
void TitleScene::before_save_frame_completion() {
    MessageServices services(*this);
    save::before_frame_completion(services.frame_state(), services);
}
void TitleScene::after_save_frame_completion() {
    MessageServices services(*this);
    save::after_frame_completion(services.frame_state(), services);
}
void Session::attach_message_fonts(MessageFontAssets assets) {
    if (message_font_assets_) throw std::runtime_error("native message fonts already attached");
    message_font_assets_ = std::make_unique<MessageFontAssets>(std::move(assets));
    message_fonts_.directory.languages = message_font_assets_->languages;
    struct Loader final : message_text::LoadCalls {
        explicit Loader(const MessageFontAssets &assets) : assets(assets) {}
        const MessageFontAssets &assets;
        std::shared_ptr<const message_text::FontData> load(message_text::FontFamily family) override {
            return family == message_text::FontFamily::Wide ? assets.wide : assets.single;
        }
    } loader(*message_font_assets_);
    message_text::initialize(message_fonts_, text_language_, loader);
    if (message_fonts_.widths != message_font_assets_->expected_widths)
        throw std::runtime_error("native message font widths differ from imported source");
}
} // namespace sadx::native

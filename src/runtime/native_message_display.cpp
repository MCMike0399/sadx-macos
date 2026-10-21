#include "runtime/native_display.hpp"
#include "game/game_mode.hpp"
#include "engine/queue_blank_frame.hpp"
#include <cstring>

namespace sadx::native {
std::int32_t Display::load_message_texture(texture_lifetime::List &list,
                                           const message_texture::Image &image) {
    if (image.type != 2306 || image.attributes != 1082130432)
        throw std::runtime_error("native message loader requires source ARGB4444 memory image");
    auto state = texture_state();
    return message_image::load(list, image.global_index, image.pixels, image.width, image.height,
                               state, texture_palettes_);
}
void Display::reload_message_texture(std::uint32_t global_index, const std::uint16_t *pixels,
                                      std::uint32_t attributes, std::uint32_t palette) {
    (void)attributes;
    (void)palette;
    auto state = texture_state();
    (void)message_image::reload(global_index, pixels, state, texture_palettes_);
}
void Display::consume_draw_queue() {
    queue::ConsumerView view{queue_owners(), control_3d_, constant_attribute_mask_,
        constant_attributes_, fog_.special_flags, fog_.active, queued_texture_list_};
    queue::ConsumerServices services{queue_services(),
        [this] { return coordinates_->current_matrix(); },
        [this](const NinjaMatrix &matrix) { coordinates_->current_matrix() = matrix; },
        [this](bool enabled) {
            if (enabled)
                enable_fog();
            else
                disable_fog();
        },
        [this] {
            queue::draw_blank_frame({horizontal_scale_, vertical_scale_, frame_mode_.pending_logic_ticks,
                fog_.special_flags, fog_.active, queued_depth_}, *output_,
                [this] { restore_depth_comparison(); }, arithmetic_);
        }};
    const auto result = queue::consume(view, services, arithmetic_);
    if (!result.completed)
        throw flow::UnrecoveredGameFlowCall("native_draw_queue_record_or_mode");
}
menu_caption::BitmapView Display::message_bitmap_from_end(std::uint32_t bytes) {
    const auto allocation = queue::borrow_from_end(queue_storage_, bytes);
    return {allocation.bytes, allocation.size};
}
queue::AnimationAllocation Display::allocate_message_animation(std::uint32_t bytes) {
    return queue::allocate_animation(queue_storage_, bytes, {});
}
void Display::set_message_material(const std::array<float, 4> &value) {
    queue::set_material(value, queue_state_, queue_services(), arithmetic_);
}
void Display::publish_message_sprite(const sprite::ScreenSprite &source) {
    sprite::center_screen_sprite(message_sprite_, source, horizontal_scale_, vertical_scale_, arithmetic_);
}
void Display::reset_message_queue() {
    auto owners = queue_owners();
    queue::reset(owners, queue_services(), arithmetic_);
}
void Display::skip_queue_execution() {
    queue::skip_execution(queue_state_);
}
void Display::submit_message_sprite(std::int32_t frame, float priority, std::uint32_t attributes,
                                    std::uint32_t flags) {
    queue::LateSpriteServices services{queue_services(),
        []() -> const std::array<float, 4> & {
            throw flow::UnrecoveredGameFlowCall("native_secondary_queue_material");
        },
        [this](const std::array<float, 4> &value) { material_argb_ = value; },
        [this](const sprite::ScreenSprite &value, std::int32_t index, float depth, std::uint32_t attrs) {
            draw_screen_sprite(value, index, depth, static_cast<std::uint8_t>(attrs));
        },
        [this] {
            restore_depth_comparison();
        }};
    const auto result = ui_scale::draw_message_sprite(ui_scaling_, message_sprite_, frame, priority,
        attributes, flags, {queue_owners()}, services, arithmetic_);
    if (!result.completed)
        throw flow::UnrecoveredGameFlowCall("native_message_sprite_type2_queue_record");
}
} // namespace sadx::native

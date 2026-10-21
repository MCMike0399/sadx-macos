#include "engine/message_sprite_profile.hpp"
namespace sadx::ui_scale {
queue::LateResult draw_message_sprite(const State &state,
                                      const sprite::ScreenSpriteScratch &scratch,
                                      std::int32_t frame, float priority, std::uint32_t attributes,
                                      std::uint32_t queue_flags, queue::LateSpriteView view,
                                      const queue::LateSpriteServices &services, X87Context &fp) {
    require_message_forwarding(state);
    return queue::draw_late_sprite(&scratch.value, frame, priority, attributes, queue_flags, view,
                                   services, fp);
}
} // namespace sadx::ui_scale

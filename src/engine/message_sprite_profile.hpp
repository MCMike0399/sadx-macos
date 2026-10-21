#pragma once
#include "engine/message_sprite_queue.hpp"
#include "engine/ui_scale_profile.hpp"
namespace sadx::ui_scale {
// NOTSA: bounded invocation of loader njDrawSprite2D_Queue_r for the actual
// message scratch. This typed identity is source38f6ce4, not the lens-flare
// resource9bf3b0. Active scaling remains unsupported; this is not the full hook.
queue::LateResult draw_message_sprite(const State &, const sprite::ScreenSpriteScratch &,
                                      std::int32_t frame, float priority, std::uint32_t attributes,
                                      std::uint32_t queue_flags, queue::LateSpriteView,
                                      const queue::LateSpriteServices &, X87Context &);
} // namespace sadx::ui_scale

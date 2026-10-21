#pragma once
#include "cpu/x87_math.hpp"
#include "game/message_text.hpp"
#include <string_view>
namespace sadx::saving_message {
// NOTSA: all fields borrow the canonical SaveFrame/Display/text-language
// owners.
struct StateView {
    std::uint8_t &pending_message;
    menu_caption::Context &message;
    std::uint32_t &pause_state;
    std::uint8_t &reset_pending;
    const float &horizontal_scale, &vertical_scale;
    const std::int32_t &language;
    message_text::State &fonts;
    std::string_view text;
};
struct Calls : menu_caption::Calls {
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual menu_caption::BitmapView bitmap_from_arena_end(std::uint32_t bytes) = 0;
    virtual void disable_fog() = 0;
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual void draw_black_box(float right, float bottom, float priority, std::uint32_t color) = 0;
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual void reset_draw_queue() = 0;
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual void skip_queue_execution() = 0;
};
// FUNCTION: SONIC 0x40be40 DisplaySavingMessage
void display(StateView, Calls &, X87Context &);
} // namespace sadx::saving_message

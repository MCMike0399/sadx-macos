#pragma once
#include "game/message_text.hpp"
#include <array>
#include <memory>
#include <string>

namespace sadx::native {
// NOTSA: immutable imported font resources; live source bindings remain in message_text::State.
struct MessageFontAssets {
    std::shared_ptr<const message_text::FontData> single, wide;
    std::array<message_text::LanguageRule, 5> languages{};
    std::array<std::int8_t, 224> expected_widths{};
    std::string saving_text;
};
} // namespace sadx::native

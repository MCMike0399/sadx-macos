#pragma once
#include <optional>
#include "ninja/ninja_keyboard.hpp"
namespace sadx {
// Debug-text owners from exact instructions.
//   793e70 (781070 site 78115d): 77ffc0(78c5a0), then EAX = 0. 78c5a0 is a bare RET,
//          so the frame callback it registers does nothing when 780030 runs it.
// 793c70 (the font texture and queue setup) and 793cc0 (the EndScene flush) are owners.
constexpr std::uint32_t kNinjaReturnOnly=0x78c5a0;
NinjaKeyboardResult run_debug_text_register_793e70(NinjaCallbackList&);
} // namespace sadx

#pragma once
#include <array>
#include <cstdint>

namespace sadx {
// NOTSA: named components of the original NJS_ARGB values, without pointer layout.
struct GameViewInitialColor {
    float alpha = 0, red = 0, green = 0, blue = 0;
};
// NOTSA: numeric NJS_SCREEN load-time values, before view setup computes dimensions.
struct GameViewInitialScreen {
    float distance = 0, width = 0, height = 0, center_x = 0, center_y = 0;
};
// NOTSA: maintained game-view and shared queue-color initialization values.
// Live pause/frame controls, device facts and projection owners stay separate.
struct GameViewInitialState {
    float stretch_x = 1.0F, stretch_y = 1.0F; // ScreenRaitoX/Y
    // QueueCharacterAnimations; the wider meaning of nonzero values is not recovered here.
    std::int32_t animation_queue_state = 0;
    GameViewInitialScreen screen{};
    std::uint8_t reset_pending = 0;
    float projection_center_x = 0, projection_center_y = 0;
    std::int32_t perspective = 0, perspective_copy = 0, last_perspective = 0;
    std::array<std::uint32_t, 4> clear_colors{};
    GameViewInitialColor view_color{}, reset_color{}, sprite_color{};
};
// NOTSA: native load-time defaults verified against file bytes and mapped zero fill.
GameViewInitialState game_view_initial_state();
} // namespace sadx

#include "game/game_view_initial_state.hpp"
#include "engine/ninja_queue_consumer.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    auto values = sadx::game_view_initial_state();
    const auto color = [](const sadx::GameViewInitialColor &value) {
        return std::array<float, 4>{value.alpha, value.red, value.green, value.blue};
    };
    std::optional<std::array<float, 4>> current = color(values.sprite_color), projection;
    sadx::NinjaQueueColorOwners owners{current, projection, 1.0F, -1.0F};
    sadx::NinjaQueueColorResult partial{};
    sadx::X87Context fp(0x27f);
    std::uint32_t site = 0;
    if (!sadx::ninja_queue_color_402f40(color(values.view_color), owners, partial, fp, site) ||
        current != std::array<float, 4>{0, 0, 0, 0} || projection != current)
        throw std::runtime_error(
            "load-time transparent color reaches the queue and projection owners");
    values.view_color.red = 2;
    if (!sadx::ninja_queue_color_402f40(color(values.view_color), owners, partial, fp, site) ||
        current != std::array<float, 4>{0, 1, 0, 0} || projection != current)
        throw std::runtime_error("view color passes through the existing material clamp");
    if (values.reset_color.red || values.sprite_color.red ||
        sadx::game_view_initial_state().view_color.red)
        throw std::runtime_error("load-time colors and sessions own independent values");
    std::cout << "View defaults: transparent material, clamping and independent colors passed.\n";
}

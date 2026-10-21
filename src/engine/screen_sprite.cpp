#include "engine/screen_sprite.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>
namespace sadx::sprite {
void center_screen_sprite(ScreenSpriteScratch &scratch, const ScreenSprite &source,
                          const float &horizontal_scale, const float &vertical_scale,
                          X87Context &fp) {
    scratch.value = source;
    auto &target = scratch.value;
    const auto horizontal_delta = fidelity::ExactFloat(fp, target.x) - 320.0f;
    const auto horizontal_center = fidelity::ExactFloat(fp, horizontal_scale) * 320.0f;
    target.x = (horizontal_delta + horizontal_center).rounded();
    const auto vertical_delta = fidelity::ExactFloat(fp, target.y) - 240.0f;
    const auto vertical_center = fidelity::ExactFloat(fp, vertical_scale) * 240.0f;
    target.y = (vertical_delta + vertical_center).rounded();
}
ScreenSpriteView screen_view(const ScreenSprite &sprite) {
    return {sprite.x,
            sprite.y,
            sprite.scale_x,
            sprite.scale_y,
            sprite.textures,
            [&sprite](std::int32_t index) -> const AnimationFrame & {
                if (!sprite.animation || index < 0 ||
                    static_cast<std::size_t>(index) >= sprite.animation_count)
                    throw std::out_of_range(
                        "screen sprite animation is outside its actual backing");
                return sprite.animation[index];
            },
            [&sprite] {
                if (!sprite.angle)
                    throw std::invalid_argument(
                        "selected sprite rotation has indeterminate source angle");
                return *sprite.angle;
            }};
}
} // namespace sadx::sprite

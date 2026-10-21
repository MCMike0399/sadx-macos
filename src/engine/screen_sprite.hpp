#pragma once
#include "engine/sprite_immediate.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
namespace sadx::sprite {
// NOTSA: semantic32-byte source sprite packet. Position.z is copied but never
// loaded by77e050; its bits and angle may be genuinely indeterminate stack data.
// Only a selected rotation branch requires an actual observed angle.
struct ScreenSprite {
    float x = 0, y = 0, scale_x = 0, scale_y = 0;
    std::optional<std::uint32_t> copied_z_bits;
    std::optional<std::int32_t> angle;
    const TextureList *textures = nullptr;
    const AnimationFrame *animation = nullptr;
    std::size_t animation_count = 0;
    // NOTSA: actual native backing, e.g. the SAME canonical queue arena.
    std::shared_ptr<const void> animation_owner;
};
struct ScreenSpriteScratch {
    ScreenSpriteScratch() {
        value.copied_z_bits = 0;
        value.angle = 0;
    }
    // GLOBAL: SONIC 0x38f6ce4 MessageScreenSprite size=32
    ScreenSprite value = {};
};
// FUNCTION: SONIC 0x401070 FUN_00401070
void center_screen_sprite(ScreenSpriteScratch &, const ScreenSprite &,
                          const float &horizontal_scale, const float &vertical_scale, X87Context &);
// NOTSA: view creation keeps every numeric/list/animation read live through
// actualrenderer callbacks, rather than snapshotting source fields early.
ScreenSpriteView screen_view(const ScreenSprite &);
} // namespace sadx::sprite

#pragma once

#include "engine/sprite_immediate.hpp"

namespace sadx::sprite_registry {
// Source definition consumed by SpLoopOnlyDisplay. Texture ownership stays with
// the character texture system; the animation table is immutable imported data.
struct Definition {
    const texture_lifetime::List *textures = nullptr;
    std::vector<sprite::AnimationFrame> animation;
    std::uint16_t animation_count = 0;
    std::uint16_t count_padding = 0;
    std::uint32_t source_blend = 0;
    std::uint32_t destination_blend = 0;
};
} // namespace sadx::sprite_registry

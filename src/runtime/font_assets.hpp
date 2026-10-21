#pragma once
#include "runtime/regular_texture_assets.hpp"
#include <array>
#include <cstdint>

namespace sadx::native {
// NOTSA: pointer-free imported font declaration; dimensions retain exact source float words.
struct FontAssets {
    std::array<std::uint32_t, 4> dimension_words;
    std::array<std::uint8_t, 128> proportions;
    std::array<std::uint32_t, 4> colors;
    RegularTextureListAsset texture_list;
};
} // namespace sadx::native

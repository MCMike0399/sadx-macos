#pragma once
#include "runtime/regular_texture_assets.hpp"
#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace sadx::native {
// NOTSA: imported declarations retain source list identities across languages.
// A regular-list alias borrows that existing owner instead of making a new list.
struct MenuTextureListAsset {
    RegularTextureListAsset declaration;
    std::optional<std::size_t> regular_texture_list;
};
using MenuTextureCatalog = std::array<std::array<regular_textures::Binding, 30>, 5>;
struct MenuTextureAssets {
    std::array<std::array<RegularTextureBindingAsset, 30>, 5> languages;
    std::vector<MenuTextureListAsset> lists;
    RegularTextureBindingAsset japanese_title;
    std::vector<std::string> unavailable_archives;
};
} // namespace sadx::native

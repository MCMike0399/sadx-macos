#pragma once
#include "game/regular_textures.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sadx::native {
// NOTSA: pointer-free imported declarations. Equal indices preserve original
// NJS_TEXLIST and regular-group identities; runtime binding supplies native lists.
struct RegularTextureListAsset {
    std::size_t texture_count;
    std::vector<std::optional<std::string>> filenames;
    std::vector<std::uint32_t> attributes;
};
struct RegularTextureBindingAsset {
    std::optional<std::string> archive;
    std::optional<std::size_t> texture_list;
};
struct RegularTextureAssets {
    std::array<std::optional<std::size_t>, 3> entries;
    std::vector<std::vector<RegularTextureBindingAsset>> groups;
    std::vector<RegularTextureListAsset> lists;
    regular_textures::Localizations localizations;
};
// NOTSA: construct the native equivalent of an imported cold NJS_TEXLIST.
inline texture_lifetime::List bind_texture_list(const RegularTextureListAsset &declaration) {
    if (declaration.filenames.size() != declaration.texture_count ||
        declaration.attributes.size() != declaration.texture_count)
        throw std::runtime_error("texture declaration members have different counts");
    texture_lifetime::List result;
    result.entries.resize(declaration.texture_count);
    result.attributes = declaration.attributes;
    for (const auto &name : declaration.filenames)
        result.filenames.push_back(name ? std::make_shared<const std::string>(*name) : nullptr);
    return result;
}
} // namespace sadx::native

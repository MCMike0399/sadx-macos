#pragma once
#include "game/character_textures.hpp"
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace sadx::native {
// NOTSA: imported PVM declarations, separate from currently loaded texture handles.
struct CharacterTextureArchive {
    std::string archive;
    std::string resource_name;
    std::size_t texture_count;
};
using CharacterTextureAssets = std::array<std::vector<CharacterTextureArchive>, 9>;
// NOTSA: invocation-local descriptors borrow Session's actual strings/lists. No
// texlist or loaded byte is copied; nonnull empty source tables retain identity.
struct CharacterTextureViews {
    std::array<std::vector<regular_textures::CharacterBinding>, 9> entries;
    std::array<regular_textures::CharacterTable, 9> tables{};
    regular_textures::CharacterBinding empty{std::nullopt, nullptr};
    CharacterTextureViews(const CharacterTextureAssets &assets,
                          std::array<std::vector<regular_textures::List>, 9> &lists) {
        for (std::size_t character = 0; character < entries.size(); ++character) {
            if (assets[character].size() != lists[character].size())
                throw std::runtime_error("native character texture declarations disagree with list owners");
            auto &bindings = entries[character];
            bindings.reserve(assets[character].size());
            for (std::size_t index = 0; index < assets[character].size(); ++index)
                bindings.push_back({std::string_view(assets[character][index].archive),
                                    &lists[character][index]});
            // The importer proves all nine source table pointers nonnull, even
            // when their first descriptor is the null-list sentinel.
            tables[character] = {bindings.empty() ? &empty : bindings.data(), bindings.size()};
        }
    }
    CharacterTextureViews(const CharacterTextureViews &) = delete;
    CharacterTextureViews &operator=(const CharacterTextureViews &) = delete;
    regular_textures::CharacterCatalog catalog(std::array<std::int8_t, 9> &loaded) const {
        return {tables.data(), loaded.data(), loaded.size()};
    }
};

} // namespace sadx::native

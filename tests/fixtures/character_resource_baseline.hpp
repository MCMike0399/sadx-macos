#pragma once
// Frozen native Session character binding loop at92dbd07; synthetic assets only.
#include "runtime/character_texture_assets.hpp"
#include "game/game_cleanup.hpp"
#include <stdexcept>
namespace sadx::test {
struct FrozenCharacterResources {
    explicit FrozenCharacterResources(const native::CharacterTextureAssets &assets)
        : texture_assets_(assets) {}
    void initialize() {
    for (std::size_t character = 0; character < texture_assets_.size(); ++character) {
        const auto &declarations = texture_assets_[character];
        if (declarations.empty())
            throw std::runtime_error("missing imported character texture declarations");
        auto &lists = character_textures_[character];
        lists.resize(declarations.size());
        for (std::size_t index = 0; index < lists.size(); ++index) {
            if (declarations[index].texture_count == 0)
                throw std::runtime_error("empty imported character texture declaration");
            lists[index].entries.resize(declarations[index].texture_count);
            character_texture_lists_[character].lists.push_back(&lists[index]);
        }
        character_texture_bindings_[character] = &character_texture_lists_[character];
    }
    }
    const auto &assets() const { return texture_assets_; }
    auto &lists() { return character_textures_; }
    const auto &bindings() const { return character_texture_bindings_; }
    auto &loaded() { return character_textures_loaded_; }
    const auto &loaded() const { return character_textures_loaded_; }
    native::CharacterTextureAssets texture_assets_;
    std::array<std::vector<sprite::TextureList>, 9> character_textures_;
    std::array<cleanup::CharacterTextureLists, 9> character_texture_lists_;
    std::array<const cleanup::CharacterTextureLists *, 9> character_texture_bindings_{};
    std::array<std::int8_t, 9> character_textures_loaded_{};
};
} // namespace sadx::test

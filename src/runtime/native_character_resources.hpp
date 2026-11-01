#pragma once
#include "runtime/character_texture_assets.hpp"
#include "game/game_cleanup.hpp"

namespace sadx::native {
// NOTSA: fixed-identity owner of imported character declarations, live lists,
// cleanup bindings and signed loaded counts. GPU memory remains Display-owned.
class CharacterResources {
  public:
    explicit CharacterResources(const CharacterTextureAssets &assets) : texture_assets_(assets) {}
    CharacterResources(const CharacterResources &) = delete;
    CharacterResources &operator=(const CharacterResources &) = delete;
    CharacterResources(CharacterResources &&) = delete;
    CharacterResources &operator=(CharacterResources &&) = delete;
    // Called once at the Session constructor-body boundary, after its later members exist.
    void initialize();
    const CharacterTextureAssets &assets() const { return texture_assets_; }
    auto &lists() { return character_textures_; }
    const auto &bindings() const { return character_texture_bindings_; }
    auto &loaded() { return character_textures_loaded_; }
    const auto &loaded() const { return character_textures_loaded_; }

  private:
    CharacterTextureAssets texture_assets_;
    std::array<std::vector<sprite::TextureList>, 9> character_textures_;
    std::array<cleanup::CharacterTextureLists, 9> character_texture_lists_;
    std::array<const cleanup::CharacterTextureLists *, 9> character_texture_bindings_{};
    std::array<std::int8_t, 9> character_textures_loaded_{};
};
} // namespace sadx::native

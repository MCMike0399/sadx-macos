#pragma once
#include "runtime/menu_texture_assets.hpp"
#include "engine/sprite_immediate.hpp"

namespace sadx::native {
// NOTSA: owns native menu list declarations and their stable language/regular aliases.
// GPU texture allocations remain with Display; regular lists are borrowed.
class MenuTextureBindings {
  public:
    MenuTextureBindings() = default;
    MenuTextureBindings(const MenuTextureBindings &) = delete;
    MenuTextureBindings &operator=(const MenuTextureBindings &) = delete;
    MenuTextureBindings(MenuTextureBindings &&) = delete;
    MenuTextureBindings &operator=(MenuTextureBindings &&) = delete;
    void attach(const MenuTextureAssets &, std::vector<sprite::TextureList> &regular);
    const regular_textures::Binding &binding(std::int32_t language, std::uint32_t index) const;
    bool attached() const { return attached_; }
    const MenuTextureCatalog &catalog() const { return catalog_; }
    const regular_textures::Binding &japanese() const { return japanese_; }

  private:
    std::vector<sprite::TextureList> lists_;
    std::vector<sprite::TextureList *> bindings_;
    MenuTextureCatalog catalog_{};
    regular_textures::Binding japanese_{};
    bool attached_ = false;
};
} // namespace sadx::native

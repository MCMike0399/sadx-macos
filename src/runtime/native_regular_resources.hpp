#pragma once
#include "runtime/regular_texture_assets.hpp"
#include "engine/sprite_immediate.hpp"

namespace sadx::native {
// NOTSA: owns imported regular texture declarations, stable bindings and live load state.
// Menu lists borrow this storage; GPU allocations remain owned by Display.
class RegularResources {
  public:
    RegularResources() = default;
    RegularResources(const RegularResources &) = delete;
    RegularResources &operator=(const RegularResources &) = delete;
    RegularResources(RegularResources &&) = delete;
    RegularResources &operator=(RegularResources &&) = delete;
    void attach(RegularTextureAssets, std::uint8_t &resync, const std::int32_t &language);
    sprite::TextureList *&common() { return common_textures_; }
    std::array<sprite::TextureList *, 32> &regular() { return regular_textures_; }
    std::vector<sprite::TextureList> &lists() { return regular_resource_lists_; }
    const regular_textures::Catalog &catalog() const { return regular_catalog_; }
    regular_textures::State *state() { return regular_texture_state_.get(); }
    const regular_textures::State *state() const { return regular_texture_state_.get(); }

  private:
    sprite::TextureList *common_textures_ = nullptr;
    std::array<sprite::TextureList *, 32> regular_textures_{};
    RegularTextureAssets regular_assets_{};
    std::vector<sprite::TextureList> regular_resource_lists_;
    std::vector<regular_textures::Group> regular_groups_;
    regular_textures::Catalog regular_catalog_{};
    std::unique_ptr<regular_textures::State> regular_texture_state_;
};
} // namespace sadx::native

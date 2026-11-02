#pragma once
#include "engine/character_pack.hpp"
#include "player/animation/animation.hpp"

namespace sadx::native {
// NOTSA: export identity is retained even when its payload is outside the imported closure.
// Zero denotes a source NULL; nonzero values identify the first aliased export plus one.
struct SonicActionDeclaration {
    std::uint16_t identity = 0;
    animation::ActionResource action{animation::ResourceStatus::Unavailable, 0, {}};
};
// NOTSA: one immutable imported Sonic table and reached payload closure. Players retain
// the shared table; animation runtimes borrow this owner's resources for the session lifetime.
class SonicAnimationAssets {
  public:
    using Declarations = std::array<SonicActionDeclaration, 149>;
    SonicAnimationAssets(const character_pack::CharacterPack &, Declarations);
    SonicAnimationAssets(const SonicAnimationAssets &) = delete;
    SonicAnimationAssets &operator=(const SonicAnimationAssets &) = delete;
    const Declarations &declarations() const { return declarations_; }
    const std::shared_ptr<const animation::TableStorage> &table() const { return table_; }
    const animation::Resources &resources() const { return resources_; }
    const std::array<ActionHandle, 3> &preview() const { return preview_; }

  private:
    Declarations declarations_;
    std::shared_ptr<const animation::TableStorage> table_;
    animation::Resources resources_;
    std::array<ActionHandle, 3> preview_{};
};
} // namespace sadx::native

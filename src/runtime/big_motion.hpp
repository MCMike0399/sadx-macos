#pragma once
#include "runtime/animation_resources.hpp"
#include "runtime/big_actor_assets.hpp"

namespace sadx::native {
// NOTSA: imported Big action resources, composed by the canonical animation update.
class BigMotionAssets {
  public:
    BigMotionAssets(const character_pack::CharacterPack &, const BigActorAssets &);
    BigMotionAssets(const BigMotionAssets &) = delete;
    BigMotionAssets &operator=(const BigMotionAssets &) = delete;
    const animation::Resources &resources() const { return resources_; }

  private:
    animation::Resources resources_;
};
} // namespace sadx::native

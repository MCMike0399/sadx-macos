#pragma once
#include "runtime/amy_actor_assets.hpp"
#include "runtime/animation_resources.hpp"

namespace sadx::native {
// NOTSA: imported Amy action resources, composed by the canonical animation update.
class AmyMotionAssets {
  public:
    AmyMotionAssets(const character_pack::CharacterPack &, const AmyActorAssets &);
    AmyMotionAssets(const AmyMotionAssets &) = delete;
    AmyMotionAssets &operator=(const AmyMotionAssets &) = delete;
    const animation::Resources &resources() const { return resources_; }

  private:
    animation::Resources resources_;
};
} // namespace sadx::native

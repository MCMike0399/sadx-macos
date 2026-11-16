#pragma once
#include "runtime/animation_resources.hpp"
#include "runtime/miles_actor_assets.hpp"

namespace sadx::native {
// NOTSA: imported Miles action resources, composed by the canonical animation update.
class MilesMotionAssets {
  public:
    MilesMotionAssets(const character_pack::CharacterPack &, const MilesActorAssets &);
    MilesMotionAssets(const MilesMotionAssets &) = delete;
    MilesMotionAssets &operator=(const MilesMotionAssets &) = delete;
    const animation::Resources &resources() const { return resources_; }

  private:
    animation::Resources resources_;
};
} // namespace sadx::native

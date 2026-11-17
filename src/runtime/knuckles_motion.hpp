#pragma once
#include "runtime/animation_resources.hpp"
#include "runtime/knuckles_actor_assets.hpp"

namespace sadx::native {
// NOTSA: imported Knuckles action resources, composed by the canonical animation update.
class KnucklesMotionAssets {
  public:
    KnucklesMotionAssets(const character_pack::CharacterPack &, const KnucklesActorAssets &);
    KnucklesMotionAssets(const KnucklesMotionAssets &) = delete;
    KnucklesMotionAssets &operator=(const KnucklesMotionAssets &) = delete;
    const animation::Resources &resources() const { return resources_; }

  private:
    animation::Resources resources_;
};
} // namespace sadx::native

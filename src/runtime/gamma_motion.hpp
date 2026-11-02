#pragma once
#include "runtime/animation_resources.hpp"
#include "runtime/gamma_actor_assets.hpp"

namespace sadx::native {
// NOTSA: immutable reached payload closure. The actor keeps the canonical table;
// invocations borrow Session's scene and the actual initialized player allocation.
class GammaMotionAssets {
  public:
    GammaMotionAssets(const character_pack::CharacterPack &, const GammaActorAssets &);
    GammaMotionAssets(const GammaMotionAssets &) = delete;
    GammaMotionAssets &operator=(const GammaMotionAssets &) = delete;
    const animation::Resources &resources() const { return resources_; }

  private:
    animation::Resources resources_;
};
// NOTSA: compose canonical animation without a second actor-specific state machine.
animation::Outcome update_gamma_motion(animation::View, TaskWorkArena &, const GammaMotionAssets &,
                                       const character_pack::ObjectState &, fidelity::Context &);
} // namespace sadx::native

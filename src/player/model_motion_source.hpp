#pragma once

#include "player/model_motion_matrices.hpp"
#include "player/animation/animation.hpp"
#include "player/player_animation_state.hpp"

namespace sadx::model_matrices {
// NOTSA: reads either an imported action or the actual generated action/motion
// and arena references. All scene poses and keys stay in their existing owners.
class NativeMotionSource final : public MotionSource {
  public:
    NativeMotionSource(const animation::Action &, const animation::Resources &,
                       const character_pack::ObjectState &);
    NativeMotionSource(const PlayerAnimationAllocatedAction &, TaskWorkArena &,
                       const character_pack::ObjectState &);
    std::uint32_t frame_count() override;
    std::uint16_t type() override;
    ObjectId root() override;
    const character_pack::ObjectResource &object(ObjectId) override;
    bool has_keys(unsigned node, Channel) override;
    std::uint32_t key_word(unsigned node, Channel, unsigned key, unsigned word) override;

  private:
    const animation::Action *imported_ = nullptr;
    const animation::Resources *resources_ = nullptr;
    const PlayerAnimationAllocatedAction *generated_ = nullptr;
    TaskWorkArena *arena_ = nullptr;
    const character_pack::ObjectState &objects_;
    const animation::Motion &imported_motion() const;
    const PlayerAnimationAllocatedMotion &generated_motion() const;
    const animation::Node &imported_node(unsigned) const;
    TaskWorkHandle generated_keys(unsigned, Channel) const;
};
} // namespace sadx::model_matrices

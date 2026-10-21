#pragma once
#include "engine/ninja_queue_native.hpp"
#include "engine/ninja_sprite_queue.hpp"
#include "engine/screen_sprite.hpp"
namespace sadx::queue {
struct ArenaBytes {
    std::shared_ptr<NinjaRenderQueueArena> owner;
    std::uint8_t *bytes;
    std::size_t size;
};
struct AnimationAllocation {
    ArenaBytes storage;
    sprite::AnimationFrame *frame;
};
// NOTSA: typed use of actual403d30(size20). No zero-fill/default ANIM values;
// source caller writes all ten fields before publishing its sprite.
AnimationAllocation allocate_animation(Storage &, std::uint32_t bytes, const NinjaQueueLogger &);
// FUNCTION: SONIC 0x403db0 late_alloca_from_end
ArenaBytes borrow_from_end(Storage &, std::uint32_t size);
struct LateSpriteView {
    Owners owners;
};
struct LateSpriteServices {
    Services queue;
    // Required only for source queue-flags bit1; no guessed unselected color.
    std::function<const std::array<float, 4> &()> secondary_material;
    // Raw77ff90 material copy, no clamp or secondcurrent-material owner.
    std::function<void(const std::array<float, 4> &)> copy_device_material;
    std::function<void(const sprite::ScreenSprite &, std::int32_t, float, std::uint32_t)> draw;
    std::function<void()> restore_default_depth;
};
enum class LateRoute { NotDrawn, Immediate, NeedsType2Record };
struct LateResult {
    LateRoute route = LateRoute::NotDrawn;
    bool completed = false;
};
// FUNCTION: SONIC 0x404290 FUN_00404290
float prepare_immediate_priority(std::uint8_t flags, float priority, LateSpriteView,
                                 const LateSpriteServices &, X87Context &);
// FUNCTION: SONIC 0x404660 FUN_00404660
LateResult draw_late_sprite(const sprite::ScreenSprite *, std::int32_t frame, float priority,
                            std::uint32_t attributes, std::uint32_t queue_flags, LateSpriteView,
                            const LateSpriteServices &, X87Context &);
} // namespace sadx::queue

#pragma once

#include "cpu/fidelity_float.hpp"
#include "player/player_scene_position_helpers.hpp"

namespace sadx::scene {

// NOTSA: lazy access preserves partial writes without putting fixture facts in game state.
struct NudgeContext {
    NudgeContext(X87Context &fp, const MotionConstants &tuning)
        : arithmetic(fp), constants(tuning) {}
    X87Context &arithmetic;
    const MotionConstants &constants;
    virtual ~NudgeContext() = default;
    virtual const NinjaVector &normal() = 0;
    virtual NinjaVector &position() = 0;
    virtual MotionWork &world_motion() = 0;
};

enum class PositionUse { IdlePlatform, SaveCallback };
enum class CenterUse { IdlePlatform, Callback, Displacement };
// NOTSA: lazy access to live platform owners and callback effects.
struct PlatformContext {
    PlatformContext(X87Context &fp, const MotionConstants &tuning)
        : arithmetic(fp), constants(tuning) {}
    X87Context &arithmetic;
    const MotionConstants &constants;
    virtual ~PlatformContext() = default;
    virtual bool has_force_work() = 0;
    virtual std::uint8_t player_index() = 0;
    virtual void select_row(std::size_t) = 0;
    virtual const NinjaVector &speed() = 0;
    virtual const NinjaVector &position(PositionUse) = 0;
    virtual void position(const NinjaVector &) = 0;
    virtual NinjaVector &center(CenterUse) = 0;
    virtual bool has_callback() = 0;
    virtual void save_position(const NinjaVector &) {}
    virtual void run_callback() = 0;
    virtual float displacement(std::size_t) = 0;
    virtual std::int32_t angle_delta(std::size_t) = 0;
    virtual PlayerOrientation &orientation() = 0;
};

// NOTSA: lazy access to floor facts and the matrix owner.
struct FloorContext {
    FloorContext(X87Context &fp, const MotionConstants &tuning)
        : arithmetic(fp), constants(tuning) {}
    X87Context &arithmetic;
    const MotionConstants &constants;
    virtual ~FloorContext() = default;
    virtual const NinjaVector &gravity() = 0;
    virtual const NinjaVector &direction() = 0;
    virtual const NinjaVector &normal() = 0;
    virtual std::uint16_t flags() = 0;
    virtual const NinjaVector &speed() = 0;
    virtual const PlayerParameters &parameters() = 0;
    virtual PlayerOrientation &orientation() = 0;
    virtual std::int32_t pitch(bool approaching) = 0;
    virtual std::int32_t roll(bool approaching) = 0;
    virtual void world_to_player(NinjaVector &) = 0;
};

void nudge_scene_normal(NudgeContext &);
void move_scene_platform(PlatformContext &, const NinjaVector &offset);
void rotate_scene_floor(FloorContext &);

} // namespace sadx::scene

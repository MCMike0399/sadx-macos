#include "player/player_motion.hpp"

#if defined(__FAST_MATH__)
#error Recovered player motion must be compiled without fast-math.
#endif

namespace sadx {

namespace {

float gravity_component(float gravity, float weight, float acceleration, X87Context& context) {
    // Original fmuls/fadds then fstps. Compile with -ffp-contract=off.
    const auto force = context.multiply_float_memory(context.from_float(gravity),weight);
    return context.to_float(context.add_float_memory(force,acceleration));
}

float limited_component(float speed, float limit, X87Context& context) {
    if (context.compare_float_memory(context.from_float(limit),speed) == X87Comparison::less) {
        speed = limit;
    }
    const auto negative_limit = context.from_float(limit).negated();
    if (context.compare_float_memory(negative_limit,speed) == X87Comparison::greater) {
        speed = context.to_float(negative_limit);
    }
    return speed;
}

} // namespace

void clear_speed(MotionWork* motion, PlayerMotion* player) noexcept {
    if (player) {
        // Instruction order: +0x40, +0x3c, +0x38.
        player->speed.z = 0.0F;
        player->speed.y = 0.0F;
        player->speed.x = 0.0F;
    }
    if (motion) {
        // Instruction order: +0x0c, +0x08, +0x04.
        motion->speed.z = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.x = 0.0F;
    }
}

PlayerMotionResult get_gravity(const PlayerOrientation& orientation,
                              const MotionWork& motion, PlayerMotion& player,
                              const NinjaVector& world_gravity, float weight,
                              const WorldToPlayerTransform& transform,
                              X87Context& context) {
    if ((orientation.flags & 0x4000U) != 0) {
        return {};
    }
    if (!transform) {
        return {PlayerMotionFailure::missing_world_to_player_transform};
    }
    NinjaVector temporary = motion.acceleration;
    temporary.x = gravity_component(world_gravity.x, weight, temporary.x, context);
    temporary.y = gravity_component(world_gravity.y, weight, temporary.y, context);
    temporary.z = gravity_component(world_gravity.z, weight, temporary.z, context);
    const auto transformed = transform(orientation, temporary);
    if (!transformed) {
        return transformed;
    }
    player.acceleration = temporary;
    return {};
}

void limit_motion_speed(MotionWork& motion, const MotionLimits& limits, X87Context& context) {
    motion.speed.x = limited_component(motion.speed.x, limits.horizontal, context);
    motion.speed.z = limited_component(motion.speed.z, limits.horizontal, context);
    motion.speed.y = limited_component(motion.speed.y, limits.vertical, context);
}

} // namespace sadx

#include "player/ground_acceleration_internal.hpp"
#include "player/ground_arithmetic.hpp"
#include "player/player_heading.hpp"

namespace sadx::physics {
namespace {
using fidelity::ExactFloat;
// NOTSA: recovered slope adjustment retains lateral acceleration until drag consumes it.
ExactFloat adjust_slope(GroundContext &context, const GroundConstants &tuning,
                        const NinjaVector &cross) {
    auto &state = context.motion;
    const auto &p = context.parameters;
    auto &fp = context.fp;
    auto &gravity = context.acceleration;
    const auto exact = [&](float value) {
        return ExactFloat(fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto zero = tuning.zero;
    auto retained_z = exact(gravity.z);
    if (less(compare(state.alignment, tuning.cross_slope_alignment)) &&
        greater(compare(fidelity::absolute_float(cross.y), tuning.cross_slope_threshold)) &&
        greater(compare(state.player.speed.x, tuning.slide_speed))) {
        gravity.x = 0;
        gravity.y = (-exact(p.weight)).rounded();
        retained_z = exact(zero);
    } else if (less(compare(state.alignment, tuning.steep_negative_alignment)) &&
               greater(compare(state.player.speed.x, tuning.slide_speed))) {
        gravity.y =
            (gravity.y - (exact(p.weight) * tuning.steep_slope_weight_multiplier)).rounded();
    } else if (less(compare(state.alignment, tuning.medium_negative_alignment)) &&
               greater(compare(state.player.speed.x, tuning.slide_speed))) {
        gravity.y = (gravity.y - (exact(p.weight) * tuning.turn_reduction)).rounded();
    } else if (less(compare(state.alignment, tuning.shallow_negative_alignment)) &&
               greater(compare(state.player.speed.x, tuning.slide_speed))) {
        gravity.y = (gravity.y - (exact(p.weight) * tuning.low_input_reduction)).rounded();
    } else if (less(compare(state.alignment, tuning.half)) &&
               less(compare(state.player.speed.x, p.run_speed)) &&
               less((-exact(p.run_speed)).compare(state.player.speed.x))) {
        gravity.x = (exact(gravity.x) * tuning.side_slope_multiplier).rounded();
        retained_z = (exact(gravity.z) * tuning.side_slope_multiplier);
    } else if (less(compare(state.alignment, tuning.medium_input_threshold)) &&
               less(compare(state.player.speed.x, p.run_speed)) &&
               less((-exact(p.run_speed)).compare(state.player.speed.x))) {
        retained_z = (exact(gravity.z) + exact(gravity.z));
    } else if (less(compare(state.alignment, tuning.shallow_slope_alignment)) &&
               less(compare(state.player.speed.x, p.jog_speed)) &&
               less((-exact(p.run_speed)).compare(state.player.speed.x))) {
        retained_z = (exact(gravity.z) * tuning.moderate_slope_side_multiplier);
    }

    return retained_z;
}

// NOTSA: drag consumes the retained lateral value after its longitudinal and vertical stores.
void apply_drag(GroundContext &context, const GroundConstants &tuning, bool active,
                ExactFloat retained_z) {
    auto &state = context.motion;
    const auto &p = context.parameters;
    auto &fp = context.fp;
    auto &gravity = context.acceleration;
    const auto exact = [&](float value) {
        return ExactFloat(fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto zero = tuning.zero;
    // Longitudinal drag: instructions may compare an unrounded intermediate
    // and always store the resulting acceleration separately.
    if (active && greater(compare(state.player.speed.x, p.max_x_spd)) &&
        greater(compare(state.alignment, tuning.overspeed_alignment))) {
        gravity.x = ((((exact(state.player.speed.x) - p.max_x_spd) * p.air_resist) *
                      tuning.overspeed_drag_multiplier) +
                     gravity.x)
                        .rounded();
    } else if (!active && greater(compare(state.player.speed.x, p.run_speed))) {
        gravity.x = ((exact(p.air_resist) * state.player.speed.x) + gravity.x).rounded();
    } else if (greater(compare(state.player.speed.x, p.max_x_spd))) {
        gravity.x =
            (((exact(state.player.speed.x) - p.max_x_spd) * p.air_resist) + gravity.x).rounded();
    } else if (less(compare(state.player.speed.x, zero))) {
        gravity.x = ((exact(p.air_resist) * state.player.speed.x) + gravity.x).rounded();
    }
    gravity.y = ((exact(p.air_resist_y) * state.player.speed.y) + gravity.y).rounded();
    gravity.z = ((exact(p.air_resist_z) * state.player.speed.z) + retained_z).rounded();
}

// NOTSA: low-speed input gain preserves the source's magnitude/threshold selection.
float jog_drive(GroundContext &context, const GroundConstants &tuning) {
    const auto &p = context.parameters;
    const auto exact = [&](float value) {
        return ExactFloat(context.fp, value);
    };
    const float magnitude = ground_input_magnitude(context, InputGainBand::Jog);
    if (greater(exact(magnitude).compare(tuning.half)))
        return (exact(magnitude) * p.run_accel).rounded();
    const auto threshold = exact(p.jog_speed) * tuning.low_input_reduction;
    if (greater(threshold.compare(context.motion.player.speed.x)))
        return (exact(magnitude) * p.run_accel).rounded();
    return 0.0f;
}

// NOTSA: run-speed input gain keeps the doubled run speed in the retained value domain.
float run_drive(GroundContext &context, const GroundConstants &tuning) {
    const auto &p = context.parameters;
    const auto exact = [&](float value) {
        return ExactFloat(context.fp, value);
    };
    const float magnitude = ground_input_magnitude(context, InputGainBand::Run);
    if (greater(exact(magnitude).compare(tuning.medium_input_threshold)))
        return (exact(magnitude) * p.run_accel).rounded();
    const auto run_speed = exact(p.run_speed);
    const auto threshold = (run_speed + run_speed) * tuning.half;
    if (greater(threshold.compare(context.motion.player.speed.x)))
        return (exact(magnitude) * p.run_accel).rounded();
    return 0.0f;
}

// NOTSA: the source chooses the magnitude load site after speed/alignment comparisons.
float input_drive(GroundContext &context, const GroundConstants &tuning) {
    auto &state = context.motion;
    const auto &p = context.parameters;
    const auto exact = [&](float value) {
        return ExactFloat(context.fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    if (less(compare(state.player.speed.x, p.max_x_spd))) {
        if (less(compare(state.player.speed.x, p.jog_speed)))
            return jog_drive(context, tuning);
        if (less(compare(state.player.speed.x, p.run_speed)))
            return run_drive(context, tuning);
        const auto rush = compare(state.player.speed.x, p.rush_speed);
        const float magnitude = ground_input_magnitude(context, InputGainBand::Fast);
        const bool reduce_input =
            less(rush) && !greater(compare(magnitude, tuning.high_input_threshold));
        auto gain = exact(magnitude) * p.run_accel;
        if (reduce_input)
            gain = gain * tuning.high_speed_input_reduction;
        return gain.rounded();
    }
    const auto speed_comparison = compare(state.player.speed.x, p.max_x_spd);
    const bool reduce = speed_comparison == X87Comparison::less ||
                        speed_comparison == X87Comparison::unordered ||
                        !less(compare(state.alignment, tuning.zero));
    const auto band =
        reduce ? InputGainBand::AboveMaximumOther : InputGainBand::AboveMaximumNegative;
    const float magnitude = ground_input_magnitude(context, band);
    auto gain = exact(magnitude) * p.run_accel;
    if (reduce)
        gain = gain * tuning.low_input_reduction;
    return gain.rounded();
}

// NOTSA: selects the original yaw owner, keeping target stores before owner invocation.
float turn_for_input(GroundContext &context, const GroundConstants &tuning, std::int32_t angle,
                     float magnitude) {
    auto &state = context.motion;
    const auto &p = context.parameters;
    const auto exact = [&](float value) {
        return ExactFloat(context.fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto distance = angle_distance(state.orientation.angles[1], angle);
    auto yaw = GroundYaw::Regular;
    if (equal(compare(state.player.speed.x, tuning.zero)) && distance > 4096) {
        state.target_yaw = angle;
        magnitude = 0;
        yaw = GroundYaw::Fast;
    } else if (!greater_or_unordered(((exact(p.run_speed) + p.jog_speed) * tuning.half)
                                         .compare(state.player.speed.x)) &&
               distance > 4096) {
        state.target_yaw = angle;
        magnitude = p.slow_down;
    } else if ((compare(state.player.speed.x, p.jog_speed) != X87Comparison::less &&
                compare(state.player.speed.x, p.jog_speed) != X87Comparison::unordered &&
                distance < 4096) ||
               (compare(state.player.speed.x, p.dash_speed) != X87Comparison::less &&
                compare(state.player.speed.x, p.dash_speed) != X87Comparison::unordered &&
                (state.attributes & kAttributeRotation) != 0)) {
        state.target_yaw = angle;
        yaw = distance < 4096 ? GroundYaw::BlendedSmallTurn : GroundYaw::BlendedAttribute;
    } else {
        if (compare(state.player.speed.x, p.jog_speed) != X87Comparison::less &&
            compare(state.player.speed.x, p.jog_speed) != X87Comparison::unordered &&
            !greater_or_unordered(compare(state.player.speed.x, p.rush_speed)) && distance > 8192)
            magnitude = (exact(magnitude) * tuning.turn_reduction).rounded();
        state.target_yaw = angle;
    }
    turn_ground_yaw(context, angle, yaw);
    state.force = magnitude;
    return magnitude;
}

// NOTSA: active input may replace its angle via reset before that scratch value is consumed.
float active_drive(GroundContext &context, const GroundConstants &tuning, float &extra_y) {
    auto &state = context.motion;
    std::int32_t angle;
    if ((state.orientation.flags & kResetDirection) != 0) {
        reset_ground_direction(context);
        angle = state.target_yaw;
        extra_y = -0.8f;
    } else {
        if (less(ExactFloat(context.fp, state.alignment).compare(tuning.attribute_alignment)) &&
            (state.attributes & kAttributeRotation) != 0)
            rotate_ground_attribute(context);
        angle = ground_input_angle(context);
    }
    const float magnitude = input_drive(context, tuning);
    return turn_for_input(context, tuning, angle, magnitude);
}

// NOTSA: inactive input retains the source's passive rotation and slowdown order.
float inactive_drive(GroundContext &context, const GroundConstants &tuning) {
    auto &state = context.motion;
    const auto &p = context.parameters;
    const auto exact = [&](float value) {
        return ExactFloat(context.fp, value);
    };
    float drive = 0;
    if (less(exact(state.alignment).compare(tuning.slide_alignment)))
        passive_ground_rotation(context);
    else if (greater(exact(state.player.speed.x).compare(tuning.zero)))
        drive = p.slow_down;
    else if (less(exact(state.player.speed.x).compare(tuning.zero)))
        drive = (-exact(p.slow_down)).rounded();
    state.force = 0;
    return drive;
}
} // namespace

void calculate_ground_acceleration(GroundMotion &state, const PlayerParameters &parameters,
                                   const GroundEnvironment &environment, X87Context &fp) {
    GroundContext context{state, parameters, environment, fp, {}, {}};
    if ((state.orientation.flags & kEventMove) != 0) {
        event_ground_acceleration(context);
        return;
    }
    if ((state.attributes & kAttributeAcceleration) != 0) {
        attribute_ground_acceleration(context);
        return;
    }
    if ((state.attributes & kAlternateAcceleration) != 0) {
        alternate_ground_acceleration(context);
        return;
    }
    auto &gravity = context.acceleration;
    const auto &world_gravity = read_world_gravity(context);
    const auto exact = [&](float value) {
        return ExactFloat(fp, value);
    };
    gravity.x = (exact(world_gravity.x) * parameters.weight + state.world.acceleration.x).rounded();
    gravity.y = (exact(world_gravity.y) * parameters.weight + state.world.acceleration.y).rounded();
    gravity.z = (exact(world_gravity.z) * parameters.weight + state.world.acceleration.z).rounded();
    report_ground_acceleration(context);
    convert_gravity_to_player(context);
    const bool active = query_ground_input(context);
    const auto direction = ground_contact_direction(context);
    NinjaVector cross;
    (void)cross_product_length_extended(direction, state.world.speed, cross, fp);
    const auto &tuning = ground_constants(context);
    const auto retained_z = adjust_slope(context, tuning, cross);
    apply_drag(context, tuning, active, retained_z);
    float extra_y = 0;
    const float drive =
        active ? active_drive(context, tuning, extra_y) : inactive_drive(context, tuning);
    auto friction = GroundFriction::Apply;
    if (less(exact(state.alignment).compare(tuning.slide_alignment)) &&
        less(exact(state.player.speed.x).compare(parameters.jog_speed)) &&
        less((-exact(parameters.jog_speed)).compare(state.player.speed.x)) && !active) {
        gravity.x = (exact(gravity.x) * tuning.inactive_slide_multiplier).rounded();
        gravity.z = (exact(gravity.z) * tuning.inactive_slide_multiplier).rounded();
        friction = GroundFriction::SkipLongitudinal;
    }
    finish_ground_friction(
        gravity, drive, extra_y, active,
        {state.player, state.attributes, state.friction_overflow, state.friction_scale}, parameters,
        tuning, friction, fp);
    report_ground_acceleration(context);
}
} // namespace sadx::physics

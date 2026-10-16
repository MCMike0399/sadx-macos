#include "player/ground_acceleration_internal.hpp"
#include "player/ground_arithmetic.hpp"

namespace sadx::physics {
namespace {
// NOTSA: braking drive follows the source's traction and overshoot cases.
void apply_negative_drive(NinjaVector &gravity, float drive, bool active, GroundFrictionState state,
                          const PlayerParameters &p, const GroundConstants &tuning,
                          X87Context &fp) {
    const auto exact = [&](float value) {
        return fidelity::ExactFloat(fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto zero = tuning.zero;
    const auto friction = [&](float coefficient) {
        return exact(coefficient) * state.scale * gravity.y;
    };
    float threshold = 0;

    if ((state.attributes & kGroundTraction) != 0) {
        bool clamped = false;
        if (less(compare(gravity.y, zero))) {
            const auto wide = friction(p.grd_frict);
            threshold = wide.rounded();
            if (greater(wide.compare(zero)) && greater((-exact(drive)).compare(threshold))) {
                auto sum = (exact(gravity.x) + drive);
                const auto negative = (-exact(threshold)).rounded();
                if (less(sum.compare(zero)) && less(sum.compare(negative))) {
                    state.overflow = (sum - threshold).rounded();
                    sum = exact(negative);
                } else if (greater(sum.compare(zero)) && greater(sum.compare(threshold))) {
                    state.overflow = (sum - threshold).rounded();
                    sum = exact(threshold);
                }
                gravity.x = sum.rounded();
                clamped = true;
            }
        }
        if (!clamped) {
            if (!active && !greater_or_unordered(compare(state.motion.speed.x, p.jog_speed)) &&
                less(compare(gravity.x, tuning.stop_maximum)) &&
                greater(compare(gravity.x, tuning.stop_minimum)))
                gravity.x = 0;
            else
                gravity.x = (exact(gravity.x) + drive).rounded();
        }
    } else if (greater(compare(state.motion.speed.x, zero))) {
        auto sum = (exact(gravity.x) + drive);
        if (less((sum * drive).compare(zero)))
            sum = exact(zero);
        gravity.x = sum.rounded();
    } else if (!active && !greater_or_unordered(compare(state.motion.speed.x, p.jog_speed)) &&
               less(compare(gravity.x, tuning.stop_maximum)) &&
               greater(compare(gravity.x, tuning.stop_minimum)))
        gravity.x = 0;
    else {
        auto sum = (friction(p.lim_frict) + drive);
        if (!less(sum.compare(zero)) && sum.compare(zero) != X87Comparison::unordered)
            sum = exact(zero);
        gravity.x = sum.rounded();
    }
}

// NOTSA: nonnegative drive uses the retained friction limit before the final spill.
void apply_positive_drive(NinjaVector &gravity, float drive, bool active, GroundFrictionState state,
                          const PlayerParameters &p, const GroundConstants &tuning,
                          X87Context &fp) {
    const auto exact = [&](float value) {
        return fidelity::ExactFloat(fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto zero = tuning.zero;
    const auto friction = [&](float coefficient) {
        return exact(coefficient) * state.scale * gravity.y;
    };
    float threshold = 0;

    threshold = friction(p.lim_frict).rounded();
    if (less(compare(state.motion.speed.x, zero)) &&
        less(compare(gravity.x, tuning.stop_maximum)) &&
        greater(compare(gravity.x, tuning.stop_minimum))) {
        auto sum = (exact(gravity.x) + drive);
        gravity.x = sum.rounded();
        if (less((sum * drive).compare(zero)))
            gravity.x = 0;
    } else if (greater(compare(threshold, zero)) && less(compare(threshold, drive))) {
        auto sum = (exact(gravity.x) + drive);
        if (greater(sum.compare(threshold))) {
            state.overflow = (sum - threshold).rounded();
            sum = exact(threshold);
        }
        gravity.x = sum.rounded();
    } else if (!active && !greater_or_unordered(compare(state.motion.speed.x, p.jog_speed)) &&
               less(compare(gravity.x, tuning.stop_maximum)) &&
               greater(compare(gravity.x, tuning.stop_minimum)))
        gravity.x = 0;
    else
        gravity.x = (exact(gravity.x) + drive).rounded();
}

// NOTSA: the common longitudinal stage delegates only after its original comparisons.
void apply_longitudinal_friction(NinjaVector &gravity, float drive, bool active,
                                 GroundFrictionState state, const PlayerParameters &p,
                                 const GroundConstants &tuning, X87Context &fp) {
    const auto exact = [&](float value) {
        return fidelity::ExactFloat(fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto zero = tuning.zero;
    const auto friction = [&](float coefficient) {
        return exact(coefficient) * state.scale * gravity.y;
    };
    float threshold = 0;
    if (equal(compare(state.motion.speed.x, zero))) {

        threshold = friction(p.lim_frict).rounded();
        gravity.x = (exact(gravity.x) + drive).rounded();
        if (!active) {
            const bool contained =
                less(compare(gravity.x, threshold)) && less((-exact(threshold)).compare(gravity.x));
            const bool small = less(compare(gravity.x, tuning.stop_maximum)) &&
                               greater(compare(gravity.x, tuning.stop_minimum));
            if (contained || small)
                gravity.x = 0;
        }
    } else if (less(compare(drive, zero))) {
        apply_negative_drive(gravity, drive, active, state, p, tuning, fp);
    } else {
        apply_positive_drive(gravity, drive, active, state, p, tuning, fp);
    }
}

// NOTSA: lateral friction retains its independent limit and sign-crossing checks.
void apply_lateral_friction(NinjaVector &gravity, GroundFrictionState state,
                            const PlayerParameters &p, const GroundConstants &tuning,
                            X87Context &fp) {
    const auto exact = [&](float value) {
        return fidelity::ExactFloat(fp, value);
    };
    const auto compare = [&](float first, float second) {
        return exact(first).compare(second);
    };
    const auto zero = tuning.zero;
    const auto friction = [&](float coefficient) {
        return exact(coefficient) * state.scale * gravity.y;
    };
    float threshold = 0;
    float extra_z = 0;
    if (equal(compare(state.motion.speed.z, zero))) {
        const auto wide = friction(p.lim_frict);
        if (less((exact(gravity.z)).compare(wide)) && less((-wide).compare(gravity.z)))
            gravity.z = 0;
    } else {
        threshold = less(compare(gravity.y, zero)) ? friction(p.grd_frict_z).rounded() : 0;
        auto z = exact(gravity.z);
        if ((state.attributes & kGroundTraction) != 0) {
            if (greater(z.compare(zero))) {
                z = (z - threshold);
                if (less(z.compare(zero)))
                    z = exact(zero);
            } else if (less(z.compare(zero))) {
                z = (z + threshold);
                if (greater(z.compare(zero)))
                    z = exact(zero);
            }
            gravity.z = z.rounded();
        } else {
            if (greater(z.compare(zero)))
                extra_z = (-exact(threshold)).rounded();
            else if (less(compare(gravity.z, zero)))
                extra_z = threshold;
            const auto summed = (exact(gravity.z) + extra_z);
            if (!equal(compare(gravity.z, zero)) && !equal(compare(extra_z, zero)) &&
                less((summed * gravity.z).compare(zero)))
                gravity.z = (exact(zero)).rounded();
            else
                gravity.z = summed.rounded();
        }
    }
}
} // namespace

void finish_ground_friction(NinjaVector &gravity, float drive, float extra_y, bool active,
                            GroundFrictionState state, const PlayerParameters &p,
                            const GroundConstants &tuning, GroundFriction mode, X87Context &fp) {
    if (mode == GroundFriction::Apply)
        apply_longitudinal_friction(gravity, drive, active, state, p, tuning, fp);
    apply_lateral_friction(gravity, state, p, tuning, fp);
    // Compute Y before storing X; its binary32 spill follows X's store.
    // Arithmetic failure preserves X, while a failed Y spill retains the new X.
    const auto final_y = fidelity::ExactFloat(fp, gravity.y) + extra_y;
    state.motion.acceleration.x = gravity.x;
    gravity.y = final_y.rounded();
    state.motion.acceleration.y = gravity.y;
    state.motion.acceleration.z = gravity.z;
}
} // namespace sadx::physics

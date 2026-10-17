#include "player/player_heading.hpp"
#include "player/player_scene_position_helpers_internal.hpp"
#include <algorithm>

namespace sadx::scene {
namespace {
using fidelity::ExactFloat;

// NOTSA: expression form of njInnerProduct's retained Z/Y/X sum.
ExactFloat dot(const NinjaVector &first, const NinjaVector &second, X87Context &fp) {
    const auto z = ExactFloat(fp, first.z) * second.z;
    const auto y = ExactFloat(fp, first.y) * second.y;
    const auto zy = z + y;
    const auto x = ExactFloat(fp, first.x) * second.x;
    return zy + x;
}

// NOTSA: njScalor retains loaded operands, spills the squared sum, then uses signed sqrt.
ExactFloat length(const NinjaVector &vector, X87Context &fp) {
    const auto x = ExactFloat(fp, vector.x);
    const auto xx = x * x;
    const auto y = ExactFloat(fp, vector.y);
    const auto yy = y * y;
    const auto z = ExactFloat(fp, vector.z);
    const auto zz = z * z;
    const auto zy = zz + yy;
    const auto squared = (zy + xx).rounded();
    return ExactFloat::signed_square_root(fp, squared);
}

// NOTSA: the carry test includes unordered comparisons.
bool less_or_unordered(X87Comparison comparison) {
    return comparison == X87Comparison::less || comparison == X87Comparison::unordered;
}
} // namespace

// FUNCTION: SONIC 0x43cb40 NudgeSceneNormal
void nudge_scene_normal(NudgeContext &context) {
    auto &fp = context.arithmetic;
    const auto &normal = context.normal();
    auto &position = context.position();
    const auto distance = context.constants.push_distance;
    position.x = ((ExactFloat(fp, normal.x) * distance) + position.x).rounded();
    position.y = ((ExactFloat(fp, normal.y) * distance) + position.y).rounded();
    position.z = ((ExactFloat(fp, normal.z) * distance) + position.z).rounded();

    // The source dereferences world motion only after publishing all three position stores.
    auto &world = context.world_motion();
    const auto x = ExactFloat(fp, normal.x);
    world.speed.x = ((x + x) + world.speed.x).rounded();
    const auto z = ExactFloat(fp, normal.z);
    world.speed.z = ((z + z) + world.speed.z).rounded();
}

// FUNCTION: SONIC 0x43ca40 MoveScenePlatform
void move_scene_platform(PlatformContext &context, const NinjaVector &offset) {
    if (!context.has_force_work())
        return;

    // Bind once: a callback may change player_index, but the row remains the selected row.
    context.select_row(context.player_index() == 0 ? 0 : 1);
    auto &fp = context.arithmetic;
    if (length(context.speed(), fp).compare(context.constants.zero) == X87Comparison::equal) {
        const auto &position = context.position(PositionUse::IdlePlatform);
        auto &center = context.center(CenterUse::IdlePlatform);
        center.x = (ExactFloat(fp, position.x) + offset.x).rounded();
        center.y = (ExactFloat(fp, position.y) + offset.y).rounded();
        center.z = (ExactFloat(fp, position.z) + offset.z).rounded();
    }

    if (context.has_callback()) {
        const auto saved = context.position(PositionUse::SaveCallback);
        context.save_position(saved);
        context.position(context.center(CenterUse::Callback));
        context.run_callback();
        // A failed callback leaves the temporary position installed, as in the reference.
        context.position(saved);
    }

    auto &center = context.center(CenterUse::Displacement);
    center.x = (ExactFloat(fp, context.displacement(0)) + center.x).rounded();
    center.y = (ExactFloat(fp, context.displacement(1)) + center.y).rounded();
    center.z = (ExactFloat(fp, context.displacement(2)) + center.z).rounded();
    auto &angles = context.orientation().angles;
    constexpr std::uint32_t kAngleMask = 65535;
    for (std::size_t i = 0; i < angles.size(); ++i) {
        angles[i] = static_cast<std::int32_t>((static_cast<std::uint32_t>(context.angle_delta(i)) +
                                               static_cast<std::uint32_t>(angles[i])) &
                                              kAngleMask);
    }
}

// FUNCTION: SONIC 0x444f10 RotateSceneFloor
void rotate_scene_floor(FloorContext &context) {
    auto &fp = context.arithmetic;
    const auto &gravity = context.gravity();
    const auto &direction = context.direction();
    const auto &normal = context.normal();
    const float old_alignment = dot(gravity, direction, fp).rounded();
    const auto alignment = dot(gravity, normal, fp).compare(old_alignment);

    // Ordered less copies immediately; unordered still consumes the player's flags.
    bool approach = false;
    constexpr std::uint16_t kGrounded = 1;
    constexpr std::uint16_t kSnapFloor = 8192;
    if (alignment != X87Comparison::less) {
        const auto flags = context.flags();
        approach = (flags & kGrounded) != 0 && (flags & kSnapFloor) == 0;
    }
    if (!approach) {
        context.orientation().angles[0] = context.pitch(false);
        context.orientation().angles[2] = context.roll(false);
        return;
    }

    auto local = normal;
    context.world_to_player(local);
    const auto x_squared = ExactFloat(fp, local.x) * local.x;
    const auto z_squared = ExactFloat(fp, local.z) * local.z;
    const auto squared = (x_squared + z_squared).rounded();
    local.x = ExactFloat::signed_square_root(fp, squared).rounded();
    local.z = 0;
    const auto horizontal = ExactFloat(fp, local.x);
    const auto vertical = ExactFloat(fp, local.y);
    auto angle = ExactFloat::arctangent(horizontal, vertical);
    angle = angle.multiply_double(context.constants.angle_units);
    angle = angle.multiply_double(context.constants.radians_to_turns);
    constexpr std::uint32_t kFullTurn = 65536;
    constexpr std::uint32_t kHalfTurn = kFullTurn / 2;
    auto step = static_cast<std::uint32_t>(angle.truncated_integer()) & (kFullTurn - 1);
    if (step > kHalfTurn)
        step = kFullTurn - step;

    auto &orientation = context.orientation();
    const auto &parameters = context.parameters();
    const auto &speed = context.speed();
    const auto below = [&](float threshold) {
        return less_or_unordered(ExactFloat(fp, speed.x).compare(threshold));
    };
    if ((orientation.flags & kSnapFloor) == 0 && below(parameters.dash_speed)) {
        if (!below(parameters.rush_speed))
            step = std::min(step, std::uint32_t{8192});
        else if (!below(parameters.run_speed))
            step = std::min(step, std::uint32_t{5376});
        else if (!below(parameters.jog_speed))
            step = std::min(step, std::uint32_t{4096});
        else
            step = 1024;
    }
    orientation.angles[0] =
        adjust_angle(orientation.angles[0], context.pitch(true), static_cast<std::int32_t>(step));
    orientation.angles[2] =
        adjust_angle(orientation.angles[2], context.roll(true), static_cast<std::int32_t>(step));
}

} // namespace sadx::scene

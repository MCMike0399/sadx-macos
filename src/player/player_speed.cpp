#include "player/player_speed.hpp"

namespace sadx {

namespace {

bool even_parity(std::uint8_t value) {
    value ^= static_cast<std::uint8_t>(value >> 4U);
    value ^= static_cast<std::uint8_t>(value >> 2U);
    value ^= static_cast<std::uint8_t>(value >> 1U);
    return (value & 1U) == 0;
}

std::uint8_t compare_high(X87Context& fp, X87Value first, float second) {
    return static_cast<std::uint8_t>(x87_comparison_status(fp.compare_float_memory(first,second)) >> 8U);
}

std::uint8_t compare_floats(X87Context& fp, float first, float second) {
    return compare_high(fp, fp.from_float(first), second);
}

PlayerInputGateResult input_gate(const PlayerSpeedCallees& callees,
                                 const PlayerOrientation& orientation, X87Context& fp) {
    if (!callees.input_gate) return {PlayerSpeedFailure::missing_input_gate, {}};
    auto result = callees.input_gate(orientation, fp);
    if (result.failure == PlayerSpeedFailure::none && !result.active) {
        result.failure = PlayerSpeedFailure::input_gate_unavailable;
    }
    return result;
}

} // namespace

PlayerSpeedResult get_player_speed(const PlayerOrientation& orientation,
                                   MotionWork& world_motion, PlayerPhysicsState& player,
                                   const PlayerParameters& parameters,
                                   const PlayerSpeedCallees& callees, X87Context& fp) {
    auto& speed = player.motion.speed;
    const auto& acceleration = player.motion.acceleration;
    const float old_x = speed.x;
    const auto new_x = fp.add_float_memory(fp.from_float(acceleration.x),old_x);
    speed.x = fp.to_float(new_x); // FST retains new_x in the x87 register
    float threshold = fp.to_float(fp.multiply_float_memory(fp.from_float(parameters.jog_speed),0.87F));

    if ((compare_floats(fp, old_x, 0) & 0x41U) == 0) {
        if (!even_parity(compare_high(fp, new_x, 0) & 0x05U)) {
            speed.x = 0;
        } else {
            const auto gate = input_gate(callees, orientation, fp);
            if (gate.failure != PlayerSpeedFailure::none) return {gate.failure, 0x443fa9, {}};
            if (!*gate.active && !even_parity(compare_floats(fp, speed.x, threshold) & 0x05U) &&
                (compare_floats(fp, player.dotp, 0.7F) & 0x41U) == 0) speed.x = 0;
        }
    } else if (!even_parity(compare_floats(fp, old_x, 0) & 0x05U)) {
        if ((compare_high(fp, new_x, 0) & 0x41U) == 0) {
            speed.x = 0;
        } else {
            const auto gate = input_gate(callees, orientation, fp);
            if (gate.failure != PlayerSpeedFailure::none) return {gate.failure, 0x443ffe, {}};
            if (!*gate.active && !even_parity(compare_high(fp, fp.from_float(threshold).negated(), speed.x) & 0x05U) &&
                (compare_floats(fp, player.dotp, 0.7F) & 0x41U) == 0) speed.x = 0;
        }
    }

    speed.y = fp.to_float(fp.add_float_memory(fp.from_float(acceleration.y),speed.y));
    const float old_z_source=speed.z;
    const auto old_z = fp.from_float(speed.z);
    speed.z = fp.to_float(fp.add_float_memory(fp.from_float(acceleration.z),old_z_source));
    if ((compare_high(fp, old_z, 0) & 0x41U) == 0) {
        if (!even_parity(compare_high(fp, old_z, threshold) & 0x05U) &&
            !even_parity(compare_high(fp, old_z, speed.z) & 0x44U)) {
            speed.z = 0;
        } else if ((compare_high(fp, old_z, threshold) & 0x01U) == 0 &&
                   !even_parity(compare_floats(fp, speed.z, threshold) & 0x05U)) {
            speed.z = 0;
        }
    } else if (!even_parity(compare_high(fp, old_z, 0) & 0x05U)) {
        threshold = fp.to_float(fp.from_float(threshold).negated());
        if ((compare_high(fp, old_z, threshold) & 0x41U) == 0 &&
            !even_parity(compare_high(fp, old_z, speed.z) & 0x44U)) {
            speed.z = 0;
        } else if (!even_parity(compare_high(fp, old_z, threshold) & 0x41U) &&
                   (compare_floats(fp, speed.z, threshold) & 0x41U) == 0) {
            speed.z = 0;
        }
    }

    if ((player.shadow_attributes_top & 0x400000U) != 0) {
        const auto braking_threshold = fp.multiply_float_memory(fp.from_float(parameters.jog_speed),0.8F);
        if (!even_parity(compare_high(fp, braking_threshold, speed.x) & 0x41U)) {
            speed.x = fp.to_float(fp.add_float_memory(fp.from_float(parameters.run_break),speed.x));
        }
    } else if ((player.shadow_attributes_top & 0x400002U) != 0) {
        const auto braking_threshold = fp.multiply_float_memory(
            fp.add_float_memory(fp.from_float(parameters.run_speed),parameters.jog_speed),0.5F);
        if (!even_parity(compare_high(fp, braking_threshold, speed.x) & 0x41U)) {
            speed.x = fp.to_float(fp.add_float_memory(fp.from_float(parameters.run_break),speed.x));
        }
    }

    NinjaVector temporary = speed;
    if ((player.attributes & 0x4000U) != 0) {
        if (!callees.floor_to_world) return {PlayerSpeedFailure::missing_floor_to_world_transform, 0x444155, temporary};
        if (!player.floor_normal) return {PlayerSpeedFailure::unavailable_floor_normal, 0x444155, temporary};
        const auto result = callees.floor_to_world(*player.floor_normal, temporary, orientation.angles[1], fp);
        if (result != PlayerSpeedFailure::none) return {result, 0x444155, temporary};
    } else {
        if (!callees.task_to_world) return {PlayerSpeedFailure::missing_task_to_world_transform, 0x444165, temporary};
        const auto result = callees.task_to_world(orientation, temporary, fp);
        if (result != PlayerSpeedFailure::none) return {result, 0x444165, temporary};
    }
    world_motion.speed = temporary;
    // njAddVector 0x785170: preserve X destination+source, Y/Z source+destination.
    world_motion.speed.x = fp.to_float(fp.add_float_memory(fp.from_float(world_motion.speed.x),player.effect.x));
    world_motion.speed.y = fp.to_float(fp.add_float_memory(fp.from_float(player.effect.y),world_motion.speed.y));
    world_motion.speed.z = fp.to_float(fp.add_float_memory(fp.from_float(player.effect.z),world_motion.speed.z));
    // One owner for the 0x43c380 signaling-FCOM limiter.
    limit_motion_speed(world_motion, {parameters.lim_h_spd, parameters.lim_v_spd}, fp);

    // 0x43c4c0 checks input again, even if the earlier X gate was reached.
    const auto cleanup_gate = input_gate(callees, orientation, fp);
    if (cleanup_gate.failure != PlayerSpeedFailure::none) return {cleanup_gate.failure, 0x43c4c5, {}};
    if (!*cleanup_gate.active) {
        for (float* component : {&world_motion.speed.x, &world_motion.speed.y, &world_motion.speed.z}) {
            if ((compare_floats(fp, 0.02F, *component) & 0x41U) == 0 &&
                !even_parity(compare_floats(fp, -0.02F, *component) & 0x05U)) *component = 0;
        }
    }
    return {};
}

} // namespace sadx

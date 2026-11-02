#pragma once
#include "player/player_initialization.hpp"

namespace sadx::sonic::native_fields {
// NOTSA: semantic references into the canonical native aggregates. Structured
// bindings make schema additions a compilation error, without encoding guest
// layouts or introducing a second value owner. These views perform no reads or
// availability checks; each consuming native binding checks only its own field.
struct Primary {
    std::optional<std::uint8_t> &id, &mode, &submode;
    std::optional<std::uint16_t> &control_timer;
    std::optional<std::int32_t> &counter;
};
// NOTSA: borrow canonical fields without eager reads.
inline Primary primary(PlayerTaskState &state) {
    auto &[orientation, index, character, event, position, path, id, scale, mode, submode,
           control_timer, counter] = state;
    return {id, mode, submode, control_timer, counter};
}
struct Player {
    std::optional<TaskHandle> &held, &moving;
    std::optional<std::uint16_t> &equipment, &flags, &timer, &requested_animation,
        &current_animation, &previous_animation, &idle_timer;
    std::optional<std::int16_t> &release_timer, &release_phase, &jump_timer;
    std::optional<float> &charge;
};
// NOTSA: borrow canonical fields without eager reads.
inline Player player(PlayerPhysicsState &state) {
    auto &[motion, alignment, effect, normal, attributes, shadow_attributes, contact, scratch,
           previous_attributes, path_distance, held, equipment, island, animation_mode, join_mode,
           shadow_scale, item_flags, no_control_timer, moving, land_object, flags, timer,
           requested_animation, current_animation, previous_animation, charge, release_timer,
           release_phase, jump_timer, idle_timer, animation] = state;
    return {held,
            moving,
            equipment,
            flags,
            timer,
            requested_animation,
            current_animation,
            previous_animation,
            idle_timer,
            release_timer,
            release_phase,
            jump_timer,
            charge};
}
// NOTSA: borrow canonical fields without eager reads.
inline std::optional<std::int32_t> &motion_angle(PlayerMotionState &state, unsigned axis) {
    auto &[motion, yaw, force, overflow, friction_scale, pitch, roll] = state;
    return axis == 0 ? pitch : roll;
}
// NOTSA: borrow canonical fields without eager reads.
inline auto &contact_direction(PlayerContactWork &state) {
    auto &[direction, radius, center, displacement, pitch, roll, count, selected, first, second,
           third, fourth, fifth, sixth] = state;
    return direction;
}
// NOTSA: borrow canonical fields without eager reads.
inline auto &animation_table(PlayerAnimationTail &state) {
    auto &[next, timer, flags, frame, start, table, buffer, join] = state;
    return table;
}
} // namespace sadx::sonic::native_fields

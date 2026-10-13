#pragma once

#include "player/player_motion.hpp"
#include "player/player_contact.hpp"
#include "engine/task.hpp"
#include "engine/scene_resource_reference.hpp"
#include "player/player_animation_state.hpp"
#include <optional>

namespace sadx {

// One native owner shared by acceleration, heading and speed. Source routines
// consume only their own fields; an unavailable optional collision/floor fact
// is never silently supplied by a zero vector or another independent snapshot.
struct PlayerPhysicsState {
    PlayerMotion motion;
    float dotp;                            // playerwk +0x1c
    NinjaVector effect;                    // +0x20
    std::optional<NinjaVector> floor_normal; // +0x50
    std::uint32_t attributes;               // +0x5c
    std::uint32_t shadow_attributes_top;     // +0x1e4
    std::optional<PlayerContactWork> contact_work = {}; // playerwk +0x64, owned contact fields
    std::optional<NinjaVector> source_vector_at_44 = {}; // source position/collision scratch +0x44
    std::optional<std::uint32_t> prior_attributes = {}; // +0x60, copied before current attrs clear
    std::optional<float> path_distance = {}; // source playerwk+0x18
    std::optional<TaskHandle> held_task_at_68 = {}; // unknown outer, observed-null handle
    std::optional<std::uint16_t> equipment_at_4 = {};
    std::optional<std::uint16_t> island_number_at_12 = {};
    std::optional<std::int16_t> animation_mode_at_120 = {},join_vertex_mode_at_122 = {};
    std::optional<float> shadow_scale_at_1fc = {};
    std::optional<std::uint16_t> flags_at_10 = {};
    std::optional<std::int16_t> no_control_time_at_a = {};
    std::optional<TaskHandle> moving_task_at_6c = {};
    std::optional<SceneResourceReference> cached_land_object_at_74 = {};
    std::optional<std::uint16_t> source_flags_at_6 = {},timer_at_7e = {};
    std::optional<std::uint16_t> requested_animation_at_124 = {},current_animation_at_126 = {},previous_animation_at_128 = {};
    std::optional<float> source_charge_at_0 = {}; // source work union float+0
    std::optional<std::int16_t> timer_at_80 = {},timer_at_82 = {},jump_timer_at_8 = {};
    std::optional<std::uint16_t> idle_timer_at_e = {};
    PlayerAnimationTail animation_tail = {}; // unknown fields until source calloc establishes availability

};

inline const NinjaVector* player_contact_direction(const PlayerPhysicsState& player) noexcept {
    if(!player.contact_work || !player.contact_work->direction_at_1c)return nullptr;
    return &*player.contact_work->direction_at_1c;
}

} // namespace sadx

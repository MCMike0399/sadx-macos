#pragma once

#include "player/player_motion.hpp"
#include "player/player_input.hpp"
#include "engine/task.hpp"
#include <optional>
#include <memory>

namespace sadx {

struct PlayerPathResource;

// One owner for task facts consumed by input, acceleration and speed. Each
// source routine derives its view at the call boundary, so a yaw/flag mutation
// cannot leave a second task snapshot stale within a movement operation.
struct PlayerEventActionListReference { std::optional<std::size_t> id; };
struct PlayerEventMoveData {
    std::optional<std::uint8_t> mode;           // eventwk +0x24
    std::optional<NinjaVector> vector_at_28;    // +0x28, meaning depends on mode
    std::optional<float> target_speed;          // +0x34
    std::optional<float> acceleration_step;     // +0x38
    std::optional<PlayerEventActionListReference> action_list_at_4c = {}; // eventwk.action.list
};

struct PlayerTaskState {
    PlayerOrientation orientation;
    std::uint8_t player_index;
    std::uint8_t character_id;
    std::optional<PlayerEventMoveData> event_move;
    std::optional<NinjaVector> position = {};   // taskwk +0x20
    // Absent is unknown; present null is the source null-pointer boundary.
    // Native owned data, never a cast of the source taskwk+0x10 address.
    std::optional<std::shared_ptr<const PlayerPathResource>> path_resource = {};
    std::optional<std::uint8_t> task_id_at_2 = {}; // taskwk ID, distinct from counter character +9
    std::optional<NinjaVector> scale_at_2c = {}; // source taskwk scale
    std::optional<std::uint8_t> action_at_0 = {};
    std::optional<std::uint8_t> secondary_mode_at_1 = {};
    std::optional<std::uint16_t> control_timer_at_6 = {};
    std::optional<std::int32_t> source_counter_at_c = {}; // raw taskwk+0x0c, distinct from player index
};

inline PlayerInputTask input_task_view(const PlayerTaskState& task) {
    return {task.orientation.flags,task.player_index,task.character_id,
            task.orientation.angles[1],task.event_move ? task.event_move->mode : std::nullopt};
}

// Installs the scheduler's child-state factory (0x40b960..0x40b9a3 for a parent with a live
// PlayerTaskState): the child gets zeroed live fields, then the parent's position x,y,z and angles
// 0,1,2 in source order. An unobserved parent position fails at 0x40b98d with the child state kept.
void install_player_child_state(TaskScheduler&);
std::optional<std::uint32_t> copy_player_child_state(const Task& parent, Task& child);

} // namespace sadx

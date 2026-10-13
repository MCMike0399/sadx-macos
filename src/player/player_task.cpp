#include "player/player_task.hpp"

namespace sadx {

std::optional<std::uint32_t> copy_player_child_state(const Task& parent, Task& task) {
    task.primary_state = std::make_shared<PlayerTaskState>();
    auto& child = *task.primary_state;
    child.position = NinjaVector{0,0,0};
    child.scale_at_2c = NinjaVector{0,0,0};
    child.task_id_at_2 = 0;
    child.action_at_0 = 0;
    child.secondary_mode_at_1 = 0;
    child.control_timer_at_6 = 0;
    child.source_counter_at_c = 0;
    child.path_resource = std::shared_ptr<const PlayerPathResource>{};
    // event_move denotes decoded facts, not a pointer-nullness fact.
    // There is no decoded event owner in newly zeroed task work.
    if (!parent.primary_state->position) return 0x40b98d;
    child.position->x = parent.primary_state->position->x;
    child.position->y = parent.primary_state->position->y;
    child.position->z = parent.primary_state->position->z;
    child.orientation.angles[0] = parent.primary_state->orientation.angles[0];
    child.orientation.angles[1] = parent.primary_state->orientation.angles[1];
    child.orientation.angles[2] = parent.primary_state->orientation.angles[2];
    // These live fields are authoritative. Arena pose bytes are not
    // read or rewritten into a second mutable task-state snapshot.
    return std::nullopt;
}

void install_player_child_state(TaskScheduler& scheduler) {
    scheduler.set_child_state_factory(copy_player_child_state);
}

} // namespace sadx

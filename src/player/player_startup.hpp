#pragma once

#include "player/player_acceleration.hpp"
#include "game/task.hpp"
#include <array>
#include <memory>

namespace sadx {

using PlayerParameterRecord = std::array<std::uint8_t,132>;
using PlayerStartupParameterTable = std::array<PlayerParameterRecord,8>;

enum class PlayerStartupStage {
    entry, player_allocated, parameters_copied, motion_bound, object_bound,
    contact_bound, registration_task,registration_primary,registered,
    shadow_stored, event_flags, speed_cleared,
    task_id, auxiliary_work, task_scale, auxiliary_task, complete,
};
enum class PlayerStartupFailure {
    none, missing_task, missing_arena, missing_primary_work, work_failure,
    missing_parameters, unsupported_character_row, missing_motion_work,
    missing_live_motion, missing_events, unsupported_registration_slot,
    missing_task_id, missing_auxiliary_effect_callback, task_creation_failed,
};

// Joins allocated native resources to the shared state used by gameplay.
// Mutable numeric state is authoritative here/Task.primary_state; arena bytes
// are initialization/storage provenance, not an independently updated player.
struct PlayerStartupPlayer {
    TaskHandle task;
    TaskWorkHandle player_work,contact_work,auxiliary_work;
    std::shared_ptr<PlayerMotionState> motion;
    std::shared_ptr<PlayerPhysicsState> player;
    std::optional<PlayerParameters> parameters;
    TaskHandle auxiliary_task;
    PlayerStartupStage stage = PlayerStartupStage::entry;
    NinjaVector* speed_at_138() noexcept { return player ? &player->motion.speed : nullptr; }
    PlayerPhysicsState* self_at_13c() noexcept { return player.get(); }
};

struct PlayerStartupRegistry {
    static constexpr std::size_t observed_slots = 8;
    // Outer omission is unknown; an observed null shared pointer is explicit.
    std::array<std::optional<TaskHandle>,observed_slots> tasks; // 0x3b42e30
    std::array<std::optional<std::shared_ptr<PlayerTaskState>>,observed_slots> task_work; // 0x3b42e10
    std::array<std::optional<std::shared_ptr<PlayerMotionState>>,observed_slots> motion_work; // 0x3b36dd0
    std::array<std::optional<std::shared_ptr<PlayerStartupPlayer>>,observed_slots> player_work; // 0x3b3cdf0
    std::optional<const PlayerPhysicsState*> lookup_player(std::uint8_t index) const;
    std::shared_ptr<PlayerStartupPlayer> lookup_player_work(const TaskWorkHandle&) const;
    // Allocation identity owns live views even before registration or after a
    // slot is overwritten. Source cleanup is required to release these; this
    // registry does not invent player destruction when a task is retired.
    std::unordered_map<const TaskWork*,std::shared_ptr<PlayerStartupPlayer>> allocations;
};

struct PlayerStartupRuntime {
    // Borrow the live source table/save facts; absence never means zero bytes.
    const PlayerStartupParameterTable* parameters = nullptr;
    const PlayerEventFlags* events = nullptr;
    // Exact source function identity 0x4a2b60. Required native owner, invoked
    // later by the scheduler; PInitialize only stores it in a new task.
    TaskCallback auxiliary_effect_initialize_4a2b60 = nullptr;
};
struct PlayerStartupResult {
    PlayerStartupFailure failure = PlayerStartupFailure::none;
    std::uint32_t source_boundary = 0,callee_boundary = 0;
    TaskWorkFailure work_failure = TaskWorkFailure::none;
    std::optional<TaskResult> task_failure;
    std::shared_ptr<PlayerStartupPlayer> partial;
    explicit operator bool() const noexcept { return failure==PlayerStartupFailure::none; }
};

// Bind generic owned work once to its authoritative native task-state view.
// Existing typed views are retained, including their explicit unknown fields.
TaskWorkResult bind_player_task_primary(TaskScheduler&,const TaskHandle&);

// Actual PInitialize0x442750. Caller already owns the main task and live world
// motion. No main-task creation or start-position lookup is inferred here.
// Failures retain registrations/resources/stores at their original boundaries.
PlayerStartupResult initialize_player_startup(std::int32_t player_index,
    const TaskHandle&,std::shared_ptr<PlayerMotionState>,TaskScheduler&,
    PlayerStartupRegistry&,const PlayerStartupRuntime&,X87Context&);

} // namespace sadx

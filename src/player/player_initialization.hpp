#pragma once
#include "engine/task.hpp"
#include "player/player_acceleration.hpp"
#include <array>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace sadx::player_initialization {
using ParameterTable = std::array<PlayerParameters, 8>;
enum class Stage {
    Entry,
    PlayerAllocated,
    ParametersCopied,
    MotionBound,
    ObjectBound,
    ContactBound,
    RegistrationTask,
    RegistrationPrimary,
    Registered,
    ShadowStored,
    EventFlags,
    SpeedCleared,
    TaskId,
    AuxiliaryWork,
    TaskScale,
    AuxiliaryTask,
    Complete
};
enum class Boundary {
    Task,
    Arena,
    Primary,
    PlayerAllocation,
    ParameterRow,
    Parameters,
    MotionWork,
    LiveMotion,
    Events,
    RegistrationSlot,
    TaskId,
    AuxiliaryCallback,
    TaskCreation,
    ObjectReference,
    ObjectStore,
    ObjectInitialization,
    ParameterStore,
    MotionBinding,
    ContactBinding,
    SpeedAlias,
    SelfAlias,
    AuxiliaryBinding,
    AuxiliaryIndex,
    PrimaryMode,
    PrimaryPosition,
    PrimaryScale,
    PrimaryNotZero,
    PrimaryConflict,
    MotionState,
    MotionNotZero
};
class Error : public std::runtime_error {
  public:
    explicit Error(Boundary boundary, TaskWorkFailure work = TaskWorkFailure::none)
        : std::runtime_error("player initialization dependency is unavailable"), boundary(boundary),
          work_failure(work) {}
    Boundary boundary;
    TaskWorkFailure work_failure;
    TaskResult task_failure{};
};
// NOTSA: references into the existing canonical PlayerTaskState; no pose copy.
struct PrimaryView {
    std::uint8_t &mode, &id, &player_index, &character;
    std::uint16_t &status;
    std::array<std::int32_t, 3> &rotation;
    NinjaVector &position, &scale;
};
// NOTSA: the native adapter checks old optional availability at this boundary.
PrimaryView bind_primary_view(const TaskHandle &);
// NOTSA: binds only a proven source-zero newly allocated primary, preserving an
// existing canonical owner. Call before actual factory stores, never after
// guessed reset.
void establish_zeroed_primary(TaskScheduler &, const TaskHandle &);
// NOTSA: actual motion allocation ownership, bound before preview aim stores.
void establish_zeroed_motion(TaskScheduler &, const TaskHandle &);
std::shared_ptr<PlayerMotionState> bind_existing_motion(const TaskHandle &);

struct Player {
    TaskHandle task, auxiliary_task;
    TaskWorkHandle allocation, contact, auxiliary;
    std::shared_ptr<PlayerMotionState> motion;
    std::shared_ptr<PlayerPhysicsState> physics;
    PlayerParameters parameters{};
    Stage stage = Stage::Entry;
};
using PlayerHandle = std::shared_ptr<Player>;
// NOTSA: native consumers borrow the initialized legacy fields through semantic names.
// Missing initialization is rejected here, before a game callback consumes the value.
struct InitializedPlayerView {
    std::uint16_t &timer, &flags, &requested_animation, &current_animation;
    TaskHandle &held_task;
    std::shared_ptr<const PlayerAnimationTable> &animation_table;
    PlayerJoinTable *&join_table;
};
InitializedPlayerView bind_initialized_player(const PlayerHandle &);

struct Registry {
    // GLOBAL: SONIC 0x3b42e30 playertp size=32
    std::array<TaskHandle, 8> tasks = {};
    // GLOBAL: SONIC 0x3b42e10 playertwp size=32
    std::array<std::shared_ptr<PlayerTaskState>, 8> primary = {};
    // GLOBAL: SONIC 0x3b36dd0 playermwp size=32
    std::array<std::shared_ptr<PlayerMotionState>, 8> motion = {};
    // GLOBAL: SONIC 0x3b3cdf0 playerpwp size=32
    std::array<PlayerHandle, 8> players = {};
    // NOTSA: allocation identity retains source allocations before registration
    // and after a slot is overwritten; retiring a task does not invent cleanup.
    std::unordered_map<const TaskWork *, PlayerHandle> allocations;
};
// NOTSA: per-consumption access separates missing reference facts from game
// behavior. Native dependencies borrow real owners; missing callbacks fail
// loudly.
struct Access {
    virtual ~Access() = default;
    virtual std::shared_ptr<PlayerTaskState> primary(const TaskHandle &) = 0;
    virtual const PlayerParameters &parameters(std::uint8_t character) = 0;
    virtual bool event_flag(std::uint16_t index) = 0;
    virtual std::shared_ptr<PlayerMotionState>
    live_motion(const TaskHandle &, const std::shared_ptr<PlayerMotionState> &) = 0;
    virtual std::uint8_t &task_id(const TaskHandle &) = 0;
    virtual void store_scale(const TaskHandle &, NinjaVector) = 0;
    virtual TaskCallback auxiliary_initializer() = 0;
};
struct NativeAccess final : Access {
    const ParameterTable &table;
    const PlayerEventFlags &events;
    TaskCallback auxiliary;
    NativeAccess(const ParameterTable &table, const PlayerEventFlags &events,
                 TaskCallback auxiliary)
        : table(table), events(events), auxiliary(auxiliary) {}
    std::shared_ptr<PlayerTaskState> primary(const TaskHandle &) override;
    const PlayerParameters &parameters(std::uint8_t) override;
    bool event_flag(std::uint16_t) override;
    std::shared_ptr<PlayerMotionState>
    live_motion(const TaskHandle &, const std::shared_ptr<PlayerMotionState> &) override;
    std::uint8_t &task_id(const TaskHandle &) override;
    void store_scale(const TaskHandle &, NinjaVector) override;
    TaskCallback auxiliary_initializer() override;
};
// The caller owns the main task and live motion. Partial stores/allocations
// remain in the supplied record if a dependency throws; no start position is
// inferred.
// FUNCTION: SONIC 0x442750 PInitialize
void initialize(std::int32_t player_index, const TaskHandle &, std::shared_ptr<PlayerMotionState>,
                TaskScheduler &, Registry &, Access &, X87Context &, PlayerHandle &record);
} // namespace sadx::player_initialization

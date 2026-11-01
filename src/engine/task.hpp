#pragma once

#include "engine/task_work.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace sadx {

struct TaskResourceUnavailable {};

class TaskScheduler;
class Task;
struct PlayerTaskState;
struct PlayerMotionState;
struct PlayerAuxiliaryEffectTaskWork;
struct TaskCallbackContext {
    virtual ~TaskCallbackContext() = default;
};
struct TaskCallbackDiagnostic {
    virtual ~TaskCallbackDiagnostic() = default;
};
class TaskSourceProbe; // test-only structural serialization, never a guest interpreter
using TaskHandle = std::shared_ptr<Task>;
using TaskCallback = void (*)(TaskScheduler &, const TaskHandle &);

class Task {
  public:
    TaskCallback exec = nullptr;
    // Native owned cooperative-thread resource (task.thp); its layout stays opaque.
    std::shared_ptr<void> cooperative_thread;
    TaskCallback display = nullptr;
    TaskCallback destructor = nullptr;
    TaskWorkHandle primary_work;
    // When attached, this is the authoritative live primary-work gameplay
    // view. Arena bytes then retain allocation/provenance, not another pose.
    std::shared_ptr<PlayerTaskState> primary_state;
    std::shared_ptr<PlayerAuxiliaryEffectTaskWork> auxiliary_effect_work;
    TaskWorkHandle motion_work;
    TaskWorkHandle force_work;
    TaskWorkHandle auxiliary_work;
    // NOTSA: typed native auxiliary data. Arena storage retains allocation identity;
    // menu and other native callbacks use this object as the authoritative work.
    std::shared_ptr<void> auxiliary_native_work;
    struct ObjectControl {
        std::uint8_t status_flags = 0;
        TaskHandle linked_task;
    };
    std::shared_ptr<ObjectControl> object_control;
    std::optional<std::uint16_t> source_slot() const noexcept { return source_slot_; }
    bool retired() const noexcept { return retired_; }

  private:
    friend class TaskScheduler;
    friend class TaskSourceProbe;
    Task() = default;
    TaskScheduler *owner_ = nullptr;
    Task *next_ = nullptr;
    Task *previous_ = nullptr;
    Task *parent_ = nullptr;
    Task *first_child_ = nullptr;
    std::size_t list_ = 0;
    bool retired_ = false;
    bool linked_ = false;
    std::optional<std::uint16_t> source_slot_;
    std::optional<std::uint16_t> detached_free_next_slot_;
    enum class DestructionStage {
        Live,
        Children,
        Collision,
        Object,
        ObjectFirstChain,
        ObjectExtra,
        ObjectFirstCounter,
        ObjectSecondChain,
        ObjectSecondCounter,
        ObjectRelease,
        Primary,
        Force,
        Motion,
        Auxiliary,
        Control,
        Recycle
    };
    DestructionStage destruction_stage_ = DestructionStage::Live;
    TaskHandle destruction_previous_, destruction_next_;
    TaskWorkHandle cleanup_object_, cleanup_cursor_;
    bool destructor_active_ = false;

  public:
    // NOTSA: one canonical native motion owner, bound from actual source calloc.
    // Appended so existing fields retain their layout in private compatibility tests.
    std::shared_ptr<PlayerMotionState> motion_state;
};

enum class TaskFailure {
    None,
    InvalidListIndex,
    ForeignTask,
    RetiredTask,
    NullChildCallback,
    DeletedSnapshotNext,
    SourcePoolUninitialized,
    SourcePoolExhausted,
    SourcePoolAlreadyInitialized,
    SourcePoolRequiresEmptyScheduler,
    InactiveTask,
    WorkFailure,
    DefaultResourceUnavailable,
    ChildStateFactoryMissing, // a live parent primary_state with no player-layer factory
                                 // installed
    DetachedCreationRollbackRejected,
    InactiveChildrenCorruption,
    CallbackMutationInvalidatedDestructor,
    SourceTaskRequiresDestructor,
    DetachedSnapshotNext,
    CallbackFailed,
    InvalidSourceGraph,
};

// NOTSA: semantic operation at an interrupted boundary; reference adapters map it to a PC.
enum class TaskStep {
    None,
    PoolAlreadyInitialized,
    PoolRequiresEmptyScheduler,
    PoolAllocation,
    PoolClear,
    AllocationLevel,
    PoolRequired,
    PoolExhaustion,
    PrimaryInitialization,
    ForceInitialization,
    MotionInitialization,
    AuxiliaryInitialization,
    CreationRollback,
    DefaultResource,
    ChildTransformRead,
    ChildTransformWrite,
    Membership,
    DestructionEntry,
    DestructionGeneration,
    InactiveChildSibling,
    InactiveChildParent,
    ChildrenDestroyed,
    CleanupResume,
    CollisionReference,
    CollisionFlags,
    CollisionExtraReference,
    CollisionExtraRelease,
    CollisionExtraClear,
    CollisionRelease,
    CollisionClear,
    ObjectReference,
    ObjectFirstChain,
    ObjectFirstNext,
    ObjectFirstRelease,
    ObjectFirstClear,
    ObjectExtraReference,
    ObjectExtraRelease,
    ObjectExtraClear,
    ObjectCounterClear,
    ObjectSecondChain,
    ObjectSecondNext,
    ObjectSecondRelease,
    ObjectSecondClear,
    ObjectSecondCounterClear,
    ObjectRelease,
    ObjectClear,
    PrimaryRelease,
    ForceRelease,
    MotionRelease,
    AuxiliaryRelease,
    ChildDispatchNext,
    IndexedDispatchNext,
    ExecutionCallback,
    CallbackDependency,
};

struct TaskResult {
    TaskFailure failure = TaskFailure::None;
    TaskHandle task;
    TaskWorkFailure work_failure = TaskWorkFailure::None;
    TaskStep step = TaskStep::None;
    std::shared_ptr<const TaskCallbackDiagnostic> callback_diagnostic = {};
    explicit operator bool() const noexcept { return failure == TaskFailure::None; }
};

struct TaskContext {
    // GLOBAL: SONIC 0x3abdbf4 TaskCurrentFunction size=4
    TaskCallback current_function = nullptr;
    // GLOBAL: SONIC 0x3abdbf8 TaskCurrentThread size=4
    std::shared_ptr<void> cooperative_thread;
};
// Native structural interchange: slot indices and owned references, never guest pointers.
struct TaskGraphNode {
    std::uint16_t slot = 0;
    std::optional<std::uint16_t> next, previous, parent, child;
    bool linked = false;
    TaskCallback exec = nullptr, display = nullptr, destructor = nullptr;
    std::shared_ptr<void> cooperative_thread;
    TaskWorkHandle primary, motion, force, auxiliary;
    std::shared_ptr<PlayerTaskState> primary_state;
    std::shared_ptr<PlayerAuxiliaryEffectTaskWork> auxiliary_effect_work;
    std::shared_ptr<void> auxiliary_native_work{};
    // NOTSA: structural interchange preserves the canonical native motion owner.
    std::shared_ptr<PlayerMotionState> motion_state{};
    std::shared_ptr<Task::ObjectControl> control;
};
struct TaskGraphState {
    TaskFailure failure = TaskFailure::None;
    std::vector<TaskGraphNode> nodes;
    std::vector<std::uint16_t> free_slots; // allocation pops the last entry
    std::array<std::optional<std::uint16_t>, 8> heads{};
    TaskContext context;
};
struct TaskDispatchState {
    TaskHandle next;
    TaskResult failure;
    bool children = false, awaiting_callback = false;
};
struct TaskDispatchCall {
    TaskResult result;
    TaskHandle task;
    TaskCallback callback = nullptr;
    bool completed = false;
};
enum class TaskLifetimeCallee { None, Destructor, FreeChildren, DestroyChild, RunChildren };
enum class TaskLifetimePhase { Begin, CallbackComplete, ChildrenComplete, Complete };
struct TaskLifetimeState {
    TaskHandle task, previous, next;
    TaskResult failure;
    TaskLifetimePhase phase = TaskLifetimePhase::Begin;
    bool children = false;
};
struct TaskLifetimeCall {
    TaskResult result;
    TaskHandle task;
    TaskCallback callback = nullptr;
    TaskLifetimeCallee callee = TaskLifetimeCallee::None;
    bool completed = false;
};

enum class TaskCallbackContextFailure { None, NullCallback, NullContext, ConflictingOwner };
struct TaskCallbackContextResult {
    TaskCallbackContextFailure failure = TaskCallbackContextFailure::None;
    explicit operator bool() const noexcept { return failure == TaskCallbackContextFailure::None; }
};

// Recovered dispatch prerequisite, not a complete task/game engine.
// Only native registered function pointers are accepted, never guest addresses.
// Host append/retire are explicit composition operations. Source allocation and
// destruction share this graph and native identities; unsupported cleanup stops
// with retained resources at its actual source call boundary.
class TaskScheduler {
  public:
    TaskScheduler() = default;
    TaskScheduler(const TaskScheduler &) = delete;
    TaskScheduler &operator=(const TaskScheduler &) = delete;
    TaskScheduler(TaskScheduler &&) = delete;
    TaskScheduler &operator=(TaskScheduler &&) = delete;

    TaskResult append_task(int list_index, TaskCallback callback,
                           std::shared_ptr<void> cooperative_thread = {});
    TaskResult append_child(const TaskHandle &parent, TaskCallback callback,
                            std::shared_ptr<void> cooperative_thread = {});

    // Reserves the actual task pool after the external master-object initializer.
    // That initializer remains outside this owner; no behavior is inferred for it.
    TaskResult initialize_source_pool(std::shared_ptr<TaskWorkArena> arena,
                                      std::shared_ptr<void> default_cooperative_thread);
    void set_default_cooperative_thread(std::shared_ptr<void> value) {
        default_cooperative_thread_ = std::move(value);
        default_resource_reader_ = {};
    }
    // NOTSA: a narrow lazy resource read for external composition. It may throw
    // TaskResourceUnavailable; creation retains all work published before this read.
    void set_default_resource_reader(std::function<std::shared_ptr<void>()> reader) {
        default_resource_reader_ = std::move(reader);
    }
    // Copies the live parent's transform through the player-layer factory. A failed read
    // retains the child and every store already made; no factory is an explicit failure.
    using ChildStateFactory = std::function<TaskStep(const Task &parent, Task &child)>;
    void set_child_state_factory(ChildStateFactory factory) {
        child_state_factory_ = std::move(factory);
    }
    // FUNCTION: SONIC 0x40b030 AllocateObjectMaster
    TaskResult allocate_task(int level, TaskCallback callback);
    // FUNCTION: SONIC 0x40b860 CreateElementalTask
    TaskResult create_element(std::uint32_t flags, int level, TaskCallback callback);
    // FUNCTION: SONIC 0x40b940 CreateChildTask
    TaskResult create_child(std::uint32_t flags, TaskCallback callback, const TaskHandle &parent);
    // FUNCTION: SONIC 0x40b570 DestroyTask
    TaskResult destroy_task(const TaskHandle &task);
    TaskLifetimeState begin_destruction(const TaskHandle &);
    TaskLifetimeState begin_free_children(const TaskHandle &);
    TaskLifetimeCall advance_lifetime(TaskLifetimeState &);
    // Explicit native continuation of an interrupted destruction, not another
    // call to source DestroyTask (which rejects the now-unlinked node).
    TaskResult resume_destruction(const TaskHandle &task);
    // FUNCTION: SONIC 0x40b7e0 FreeTaskC
    TaskResult free_children(const TaskHandle &task);
    // Recovered default callback is a RET.
    static void source_no_op(TaskScheduler &, const TaskHandle &);
    static void destroy_callback(TaskScheduler &, const TaskHandle &);
    static void free_children_callback(TaskScheduler &, const TaskHandle &);
    const std::shared_ptr<TaskWorkArena> &work_arena() const noexcept { return arena_; }
    std::size_t source_free_slots() const noexcept { return free_slots_.size(); }
    TaskHandle list_head(int index) const;
    TaskHandle first_child(const TaskHandle &task) const;
    TaskHandle next_task(const TaskHandle &task) const;
    TaskHandle source_task(std::uint16_t slot) const;
    const TaskWorkHandle &source_pool_work() const noexcept { return source_pool_work_; }
    TaskGraphState source_graph() const;
    // Reconcile external graph changes between source calls; requires an initialized
    // pool, no active native walker and no retained destruction continuation.
    // Validates references before replacing state.
    TaskResult restore_source_graph(const TaskGraphState &);
    bool active_member(const TaskHandle &task) const;

    // Lists 0..7, with index 8 aliasing 7.
    // Top-level null exec is skipped; next is captured before calling exec.
    // FUNCTION: SONIC 0x40b0c0 RunObjectIndex
    TaskResult run_index(int list_index);
    // One native dispatch transition at a time; callbacks may run externally.
    // The saved next identity precedes the callback. Reconcile the graph, then
    // advance again to apply source context cleanup and resolve pool-slot reuse.
    TaskDispatchState begin_index_dispatch(int list_index);
    TaskDispatchState begin_child_dispatch(const TaskHandle &parent);
    TaskDispatchCall advance_dispatch(TaskDispatchState &);
    // The special index (alias 7) runs first, then indices 0 through 6.
    // FUNCTION: SONIC 0x40b170 RunAllObjects
    TaskResult run_game_tasks();
    // Direct children only. A null child exec models the
    // original invalid indirect call as a tagged failure, not a silent skip.
    // FUNCTION: SONIC 0x40b420 LoopTaskC
    TaskResult run_children(const TaskHandle &parent);

    // Display traversal leaves the execution callback context untouched.
    // FUNCTION: SONIC 0x40b4f0 DisplayObjectIndex
    TaskResult display_index(int list_index);
    // FUNCTION: SONIC 0x40b130 DisplayObjectChildren
    TaskResult display_children(const TaskHandle &parent);
    // FUNCTION: SONIC 0x40b540 DisplayTask
    TaskResult display_game_tasks(const std::function<void()> &reset_material);
    // FUNCTION: SONIC 0x40b730 DeleteAllObjects
    TaskResult delete_all_tasks();

    // Replaces exec with the recovered destructor for members.
    // FUNCTION: SONIC 0x40b6c0 FreeTask
    TaskResult free_task(const TaskHandle &task);
    // Explicit host lifetime operation, not an implementation of guest FreeTask.
    // Unlinks a subtree. Handles and active walkers keep retired storage alive.
    TaskResult retire(const TaskHandle &task);

    const TaskContext &context() const noexcept { return context_; }
    TaskCallbackContextResult register_callback_context(TaskCallback,
                                                        std::shared_ptr<TaskCallbackContext>);
    std::shared_ptr<TaskCallbackContext> callback_context(TaskCallback) const;
    // Expected source-native module failure keeps context/partial resources;
    // unrelated host exceptions retain their existing exception behavior.
    [[noreturn]] static void raise_callback_failure(TaskResult);

  private:
    friend class TaskSourceProbe;
    class WalkGuard;
    std::array<Task *, 8> heads_{};
    std::unordered_map<Task *, TaskHandle> storage_;
    TaskContext context_;
    std::unordered_map<TaskCallback, std::shared_ptr<TaskCallbackContext>> callback_contexts_;
    std::size_t walk_depth_ = 0;
    std::shared_ptr<TaskWorkArena> arena_;
    TaskWorkHandle source_pool_work_;
    std::array<TaskHandle, 4096> source_slots_{};
    std::vector<std::uint16_t> free_slots_;
    std::shared_ptr<void> default_cooperative_thread_;
    std::function<std::shared_ptr<void>()> default_resource_reader_;
    ChildStateFactory child_state_factory_;

    // NOTSA: validation and publication are separate named steps of graph reconciliation.
    struct GraphRestorePlan;
    TaskResult validate_graph_allocations(const TaskGraphState &, GraphRestorePlan &) const;
    TaskResult validate_graph_links(const TaskGraphState &, GraphRestorePlan &) const;
    void publish_graph(const TaskGraphState &, const GraphRestorePlan &);

    static int canonical_index(int list_index) noexcept;
    TaskResult validate(const TaskHandle &task) const;
    TaskHandle retain(Task *task) const;
    void append_link(Task *&head, const TaskHandle &task);
    void retire_subtree(const TaskHandle &task);
    void reclaim_retired();
    TaskResult finish_destruction(const TaskHandle &task);
    // NOTSA: named stages of the existing lazy, retryable destruction sequence.
    TaskResult unlink_destroyed_task(const TaskHandle &task);
    TaskResult release_collision_work(const TaskHandle &task);
    TaskResult release_first_object_resources(const TaskHandle &task);
    TaskResult release_second_object_resources(const TaskHandle &task);
    TaskResult release_owned_task_work(const TaskHandle &task);
    // NOTSA: extracted child-lifetime transition shared by native continuation paths.
    TaskLifetimeCall advance_child_lifetime(TaskLifetimeState &, const TaskHandle &);
    TaskResult source_work_failure(const TaskHandle &, const TaskWorkResult &, TaskStep step) const;
    TaskHandle resolve_snapshot(const TaskHandle &) const;
    // NOTSA: common sibling loop shared by indexed and recursive child display.
    TaskResult display_chain(TaskHandle next);
};

} // namespace sadx

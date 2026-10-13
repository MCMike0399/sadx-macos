#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <vector>
#include "engine/task_work.hpp"

namespace sadx {

class TaskScheduler;
class Task;
struct PlayerTaskState;
struct PlayerAuxiliaryEffectTaskWork;
struct TaskCallbackContext { virtual ~TaskCallbackContext() = default; };
struct TaskCallbackDiagnostic { virtual ~TaskCallbackDiagnostic() = default; };
class TaskSourceProbe; // test-only structural serialization, never a guest interpreter
using TaskHandle = std::shared_ptr<Task>;
using TaskCallback = void (*)(TaskScheduler&, const TaskHandle&);

class Task {
public:
    TaskCallback exec = nullptr;
    // Native owned resource corresponding to the opaque guest field at +0x30.
    // No guest pointer or guessed resource layout is accepted by this API.
    std::shared_ptr<void> field30;
    TaskCallback display = nullptr;       // +0x14
    TaskCallback destructor = nullptr;    // +0x18
    TaskWorkHandle primary_work;           // +0x20
    // When attached, this is the authoritative live primary-work gameplay
    // view. Arena bytes then retain allocation/provenance, not another pose.
    std::shared_ptr<PlayerTaskState> primary_state;
    std::shared_ptr<PlayerAuxiliaryEffectTaskWork> auxiliary_effect_work;
    TaskWorkHandle motion_work;            // +0x24
    TaskWorkHandle force_work;             // +0x28
    TaskWorkHandle auxiliary_work;         // +0x2c
    struct ObjectControl {
        std::uint8_t flags_at_2 = 0;
        TaskHandle task_at_4;
    };
    std::shared_ptr<ObjectControl> object_control; // +0x1c, borrowed source linkage
    std::optional<std::uint16_t> source_slot() const noexcept { return source_slot_; }
    bool retired() const noexcept { return retired_; }

private:
    friend class TaskScheduler;
    friend class TaskSourceProbe;
    Task() = default;
    TaskScheduler* owner_ = nullptr;
    Task* next_ = nullptr;
    Task* previous_ = nullptr;
    Task* parent_ = nullptr;
    Task* first_child_ = nullptr;
    std::size_t list_ = 0;
    bool retired_ = false;
    bool linked_ = false;
    std::optional<std::uint16_t> source_slot_;
    std::optional<std::uint16_t> detached_free_next_slot_;
    enum class DestructionStage { live, children, collision, object, object_chain40,
                                  object_block44, object_clear3c, object_chain4c,
                                  object_clear48, object_release, primary, force,
                                  motion, auxiliary, control, recycle };
    DestructionStage destruction_stage_ = DestructionStage::live;
    TaskHandle destruction_previous_, destruction_next_;
    TaskWorkHandle cleanup_object_, cleanup_cursor_;
    bool destructor_active_ = false;
};

enum class TaskFailure {
    none,
    invalid_list_index,
    foreign_task,
    retired_task,
    null_child_callback,
    deleted_snapshot_next,
    source_pool_uninitialized,
    source_pool_exhausted,
    source_pool_already_initialized,
    source_pool_requires_empty_scheduler,
    inactive_task,
    work_failure,
    default_field30_unobserved,
    detached_creation_rollback_rejected,
    inactive_children_corruption,
    callback_mutation_invalidated_destructor,
    source_task_requires_destructor,
    detached_snapshot_next,
    callback_failed,
};

struct TaskResult {
    TaskFailure failure = TaskFailure::none;
    TaskHandle task;
    TaskWorkFailure work_failure = TaskWorkFailure::none;
    std::uint32_t source_boundary = 0;
    std::shared_ptr<const TaskCallbackDiagnostic> callback_diagnostic = {};
    explicit operator bool() const noexcept { return failure == TaskFailure::none; }
};

struct TaskContext {
    TaskCallback current_function = nullptr; // guest global 0x3abdbf4
    std::shared_ptr<void> field30;            // guest global 0x3abdbf8
};

enum class TaskCallbackContextFailure { none,null_callback,null_context,conflicting_owner };
struct TaskCallbackContextResult {
    TaskCallbackContextFailure failure=TaskCallbackContextFailure::none;
    explicit operator bool() const noexcept{return failure==TaskCallbackContextFailure::none;}
};

// Recovered dispatch prerequisite, not a complete task/game engine.
// Only native registered function pointers are accepted, never guest addresses.
// Host append/retire are explicit composition operations. Source allocation and
// destruction share this graph and native identities; unsupported cleanup stops
// with retained resources at its actual source call boundary.
class TaskScheduler {
public:
    TaskScheduler() = default;
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;

    TaskResult append_task(int list_index, TaskCallback callback,
                           std::shared_ptr<void> field30 = {});
    TaskResult append_child(const TaskHandle& parent, TaskCallback callback,
                            std::shared_ptr<void> field30 = {});

    // Begins at 0x40b466, AFTER the external 0x51a740 initializer. Its behavior
    // is not inferred by this owner. Reserve the actual 4096*0x34 arena bytes.
    TaskResult initialize_source_pool(std::shared_ptr<TaskWorkArena> arena,
        std::optional<std::shared_ptr<void>> default_field30 = {});
    void set_default_field30(std::optional<std::shared_ptr<void>> value) {
        default_field30_ = std::move(value);
    }
    TaskResult allocate_task(int level, TaskCallback callback); // 0x40b030
    TaskResult create_element(std::uint32_t flags, int level, TaskCallback callback);
    TaskResult create_child(std::uint32_t flags, TaskCallback callback,
                            const TaskHandle& parent);
    TaskResult destroy_task(const TaskHandle& task); // 0x40b570
    // Explicit native continuation of an interrupted destruction, not another
    // call to source DestroyTask (which rejects the now-unlinked node).
    TaskResult resume_destruction(const TaskHandle& task);
    TaskResult free_children(const TaskHandle& task); // 0x40b7e0
    static void source_no_op(TaskScheduler&, const TaskHandle&); // 0x51f680 RET
    static void destroy_callback(TaskScheduler&, const TaskHandle&);
    static void free_children_callback(TaskScheduler&, const TaskHandle&);
    const std::shared_ptr<TaskWorkArena>& work_arena() const noexcept { return arena_; }
    std::size_t source_free_slots() const noexcept { return free_slots_.size(); }
    TaskHandle list_head(int index) const;
    TaskHandle first_child(const TaskHandle& task) const;
    TaskHandle next_task(const TaskHandle& task) const;
    bool active_member(const TaskHandle& task) const;

    // RunObjectIndex 0x40b0c0: lists 0..7, with index 8 aliasing 7.
    // Top-level null exec is skipped; next is captured before calling exec.
    TaskResult run_index(int list_index);
    // RunTask0x40b170: source index8 (alias7) first, then indices0..6.
    TaskResult run_game_tasks();
    // LoopTaskC 0x40b420: direct children only. A null child exec models the
    // original invalid indirect call as a tagged failure, not a silent skip.
    TaskResult run_children(const TaskHandle& parent);

    // FreeTask 0x40b6c0 replaces exec with the recovered destructor for members.
    TaskResult free_task(const TaskHandle& task);
    // Explicit host lifetime operation, not an implementation of guest FreeTask.
    // Unlinks a subtree. Handles and active walkers keep retired storage alive.
    TaskResult retire(const TaskHandle& task);

    const TaskContext& context() const noexcept { return context_; }
    TaskCallbackContextResult register_callback_context(TaskCallback,
        std::shared_ptr<TaskCallbackContext>);
    std::shared_ptr<TaskCallbackContext> callback_context(TaskCallback) const;
    // Expected source-native module failure keeps context/partial resources;
    // unrelated host exceptions retain their existing exception behavior.
    [[noreturn]] static void raise_callback_failure(TaskResult);

private:
    friend class TaskSourceProbe;
    class WalkGuard;
    std::array<Task*, 8> heads_{};
    std::unordered_map<Task*, TaskHandle> storage_;
    TaskContext context_;
    std::unordered_map<TaskCallback,std::shared_ptr<TaskCallbackContext>> callback_contexts_;
    std::size_t walk_depth_ = 0;
    std::shared_ptr<TaskWorkArena> arena_;
    TaskWorkHandle source_pool_work_;
    std::array<TaskHandle, 4096> source_slots_{};
    std::vector<std::uint16_t> free_slots_;
    std::optional<std::shared_ptr<void>> default_field30_;

    static int canonical_index(int list_index) noexcept;
    TaskResult validate(const TaskHandle& task) const;
    TaskHandle retain(Task* task) const;
    void append_link(Task*& head, const TaskHandle& task);
    void retire_subtree(const TaskHandle& task);
    void reclaim_retired();
    TaskResult finish_destruction(const TaskHandle& task);
    TaskResult source_work_failure(const TaskHandle&, const TaskWorkResult&,
                                  std::uint32_t boundary) const;
    TaskHandle resolve_snapshot(const TaskHandle&) const;
};

} // namespace sadx

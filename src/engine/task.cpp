#include "engine/task.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace sadx {

namespace {
constexpr std::uint32_t kTaskPoolBytes = 4096 * 52;
constexpr std::uint32_t kCollisionReference = 56;
constexpr std::uint8_t kCollisionOwnsExtra = 0x80;
constexpr std::uint32_t kObjectReference = 60;
constexpr std::uint32_t kObjectFirstChain = 64;
constexpr std::uint32_t kObjectExtra = 68;
constexpr std::uint32_t kObjectFirstCounter = 60;
constexpr std::uint32_t kObjectSecondChain = 76;
constexpr std::uint32_t kObjectSecondCounter = 72;
constexpr std::uint32_t kFirstChainNext = 16;
constexpr std::uint32_t kSecondChainNext = 12;
constexpr std::uint8_t kClearControlLinked = 0xfe;

// Only recovered built-in callbacks throw this private control transfer. It
// preserves source partial state; arbitrary host callback exceptions still pass
// through the existing native exception boundary.
struct SourceTaskFailure {
    TaskResult result;
};
} // namespace

class TaskScheduler::WalkGuard {
  public:
    explicit WalkGuard(TaskScheduler &scheduler) : scheduler_(scheduler) {
        ++scheduler_.walk_depth_;
    }
    ~WalkGuard() {
        if (--scheduler_.walk_depth_ == 0) {
            scheduler_.reclaim_retired();
        }
    }

  private:
    TaskScheduler &scheduler_;
};

int TaskScheduler::canonical_index(int list_index) noexcept {
    if (list_index == 8) {
        return 7;
    }
    return list_index >= 0 && list_index < 8 ? list_index : -1;
}

TaskCallbackContextResult
TaskScheduler::register_callback_context(TaskCallback callback,
                                         std::shared_ptr<TaskCallbackContext> owner) {
    if (!callback)
        return {TaskCallbackContextFailure::null_callback};
    if (!owner)
        return {TaskCallbackContextFailure::null_context};
    const auto found = callback_contexts_.find(callback);
    if (found != callback_contexts_.end())
        return {found->second == owner ? TaskCallbackContextFailure::none
                                       : TaskCallbackContextFailure::conflicting_owner};
    callback_contexts_.emplace(callback, std::move(owner));
    return {};
}

std::shared_ptr<TaskCallbackContext> TaskScheduler::callback_context(TaskCallback callback) const {
    const auto found = callback_contexts_.find(callback);
    return found == callback_contexts_.end() ? std::shared_ptr<TaskCallbackContext>{}
                                             : found->second;
}

[[noreturn]] void TaskScheduler::raise_callback_failure(TaskResult result) {
    throw SourceTaskFailure{std::move(result)};
}

TaskResult TaskScheduler::validate(const TaskHandle &task) const {
    if (!task || task->owner_ != this) {
        return {TaskFailure::foreign_task, task};
    }
    if (task->retired_) {
        return {TaskFailure::retired_task, task};
    }
    if (storage_.find(task.get()) == storage_.end()) {
        return {TaskFailure::foreign_task, task};
    }
    return {};
}

TaskHandle TaskScheduler::retain(Task *task) const {
    if (!task) {
        return {};
    }
    const auto found = storage_.find(task);
    return found == storage_.end() ? TaskHandle{} : found->second;
}

void TaskScheduler::append_link(Task *&head, const TaskHandle &task) {
    if (!head) {
        head = task.get();
        return;
    }
    auto *tail = head;
    while (tail->next_) {
        tail = tail->next_;
    }
    tail->next_ = task.get();
    task->previous_ = tail;
}

TaskResult TaskScheduler::append_task(int list_index, TaskCallback callback,
                                      std::shared_ptr<void> cooperative_thread) {
    const int index = canonical_index(list_index);
    if (index < 0) {
        return {TaskFailure::invalid_list_index, {}};
    }
    TaskHandle task(new Task);
    task->owner_ = this;
    task->list_ = static_cast<std::size_t>(index);
    task->exec = callback;
    task->cooperative_thread = std::move(cooperative_thread);
    task->linked_ = true;
    storage_.emplace(task.get(), task);
    append_link(heads_[task->list_], task);
    return {TaskFailure::none, task};
}

TaskResult TaskScheduler::append_child(const TaskHandle &parent, TaskCallback callback,
                                       std::shared_ptr<void> cooperative_thread) {
    const auto accepted = validate(parent);
    if (!accepted) {
        return accepted;
    }
    TaskHandle task(new Task);
    task->owner_ = this;
    task->parent_ = parent.get();
    task->exec = callback;
    task->cooperative_thread = std::move(cooperative_thread);
    task->linked_ = true;
    storage_.emplace(task.get(), task);
    append_link(parent->first_child_, task);
    return {TaskFailure::none, task};
}

TaskDispatchState TaskScheduler::begin_index_dispatch(int list_index) {
    TaskDispatchState state;
    const auto index = canonical_index(list_index);
    if (index < 0)
        state.failure = {TaskFailure::invalid_list_index, {}};
    else
        state.next = retain(heads_[static_cast<std::size_t>(index)]);
    return state;
}
TaskDispatchState TaskScheduler::begin_child_dispatch(const TaskHandle &parent) {
    TaskDispatchState state;
    state.children = true;
    state.failure = validate(parent);
    if (state.failure)
        state.next = retain(parent->first_child_);
    return state;
}
TaskDispatchCall TaskScheduler::advance_dispatch(TaskDispatchState &state) {
    if (!state.failure)
        return {state.failure, {}, nullptr, false};
    if (state.awaiting_callback) {
        context_.cooperative_thread.reset();
        if (!state.children)
            context_.current_function = nullptr;
        state.awaiting_callback = false;
    }
    while (state.next) {
        auto task = state.next;
        // External graph reconciliation replaces native handles. The source
        // snapshot is a pool address, so resolve its current slot generation.
        if (task->source_slot_) {
            const auto current = source_slots_[*task->source_slot_];
            if (current)
                task = current;
            else
                return {{TaskFailure::deleted_snapshot_next,
                         task,
                         {},
                         state.children ? TaskStep::child_dispatch_next
                                        : TaskStep::indexed_dispatch_next},
                        task,
                        nullptr,
                        false};
        }
        if (task->retired_)
            return {{TaskFailure::deleted_snapshot_next, task}, task, nullptr, false};
        if (task->source_slot_ && !task->linked_)
            return {
                {TaskFailure::detached_snapshot_next,
                 task,
                 {},
                 state.children ? TaskStep::child_dispatch_next : TaskStep::indexed_dispatch_next},
                task,
                nullptr,
                false};
        state.next = retain(task->next_);
        const auto callback = task->exec;
        if (!state.children)
            context_.current_function = callback;
        if (callback || state.children)
            context_.cooperative_thread = task->cooperative_thread;
        if (!callback) {
            if (state.children)
                return {{TaskFailure::null_child_callback, task}, task, nullptr, false};
            context_.current_function = nullptr;
            continue;
        }
        state.awaiting_callback = true;
        return {{}, task, callback, false};
    }
    return {{}, {}, nullptr, true};
}
TaskResult TaskScheduler::run_index(int list_index) {
    auto state = begin_index_dispatch(list_index);
    if (!state.failure)
        return state.failure;
    WalkGuard guard(*this);
    for (;;) {
        const auto call = advance_dispatch(state);
        if (!call.result)
            return call.result;
        if (call.completed)
            return {};
        try {
            call.callback(*this, call.task);
        } catch (const SourceTaskFailure &failed) {
            return failed.result;
        } catch (...) {
            context_.cooperative_thread.reset();
            context_.current_function = nullptr;
            throw;
        }
    }
}

TaskResult TaskScheduler::run_game_tasks() {
    auto result = run_index(8);
    if (!result)
        return result;
    for (int index = 0; index < 7; ++index) {
        result = run_index(index);
        if (!result)
            return result;
    }
    return {};
}

TaskResult TaskScheduler::run_children(const TaskHandle &parent) {
    auto state = begin_child_dispatch(parent);
    if (!state.failure)
        return state.failure;
    WalkGuard guard(*this);
    for (;;) {
        const auto call = advance_dispatch(state);
        if (!call.result)
            return call.result;
        if (call.completed)
            return {};
        try {
            call.callback(*this, call.task);
        } catch (const SourceTaskFailure &failed) {
            return failed.result;
        } catch (...) {
            context_.cooperative_thread.reset();
            throw;
        }
    }
}

TaskResult TaskScheduler::display_chain(TaskHandle next) {
    while (next) {
        auto task = resolve_snapshot(next);
        if (!task || task->retired_)
            return {TaskFailure::deleted_snapshot_next, next};
        if (task->source_slot_ && !task->linked_)
            return {TaskFailure::detached_snapshot_next, task};
        const auto callback = task->display;
        next = retain(task->next_);
        if (callback != nullptr) {
            try {
                callback(*this, task);
            } catch (const SourceTaskFailure &failed) {
                return failed.result;
            }
        }
        // A callback can replace this pool slot. The source reloads the child
        // pointer through that slot after returning, rather than caching it.
        task = resolve_snapshot(task);
        if (task && !task->retired_ && task->first_child_ != nullptr) {
            const auto result = display_children(task);
            if (!result)
                return result;
        }
    }
    return {};
}

TaskResult TaskScheduler::display_index(int list_index) {
    const auto index = canonical_index(list_index);
    if (index < 0)
        return {TaskFailure::invalid_list_index, {}};
    WalkGuard guard(*this);
    return display_chain(retain(heads_[static_cast<std::size_t>(index)]));
}

TaskResult TaskScheduler::display_children(const TaskHandle &parent) {
    const auto accepted = validate(parent);
    if (!accepted)
        return accepted;
    WalkGuard guard(*this);
    return display_chain(retain(parent->first_child_));
}

TaskResult TaskScheduler::display_game_tasks(const std::function<void()> &reset_material) {
    try {
        reset_material();
    } catch (const SourceTaskFailure &failed) {
        return failed.result;
    }
    auto result = display_index(8);
    if (!result)
        return result;
    for (int index = 0; index < 7; ++index) {
        result = display_index(index);
        if (!result)
            return result;
    }
    return {};
}

TaskResult TaskScheduler::delete_all_tasks() {
    WalkGuard guard(*this);
    for (std::size_t index = 0; index < heads_.size(); ++index) {
        auto next = retain(heads_[index]);
        while (next) {
            auto task = resolve_snapshot(next);
            if (!task || task->retired_)
                return {TaskFailure::deleted_snapshot_next, next};
            if (task->source_slot_ && !task->linked_)
                return {TaskFailure::detached_snapshot_next, task};
            next = retain(task->next_);
            // The source leaves exec-null top-level tasks and their descendants alive.
            if (task->exec != nullptr) {
                try {
                    const auto result = destroy_task(task);
                    if (!result)
                        return result;
                } catch (const SourceTaskFailure &failed) {
                    return failed.result;
                }
            }
        }
    }
    return {};
}

TaskResult TaskScheduler::free_task(const TaskHandle &task) {
    const auto accepted = validate(task);
    if (!accepted) {
        return accepted;
    }
    if (!active_member(task))
        return {TaskFailure::inactive_task, task, {}, TaskStep::membership};
    task->exec = &destroy_callback;
    return {};
}

void TaskScheduler::retire_subtree(const TaskHandle &task) {
    while (task->first_child_) {
        retire_subtree(retain(task->first_child_));
    }
    if (task->linked_) {
        Task *&head = task->parent_ ? task->parent_->first_child_ : heads_[task->list_];
        if (task->previous_)
            task->previous_->next_ = task->next_;
        else
            head = task->next_;
        if (task->next_)
            task->next_->previous_ = task->previous_;
    }
    task->linked_ = false;
    task->exec = nullptr;
    task->retired_ = true;
    task->next_ = nullptr;
    task->previous_ = nullptr;
    task->parent_ = nullptr;
}

TaskHandle TaskScheduler::resolve_snapshot(const TaskHandle &task) const {
    if (task && task->source_slot_ && task->retired_) {
        const auto &current = source_slots_[*task->source_slot_];
        // The source cached a pool address, so a same-slot new generation is
        // observed on its next iteration. Unreused freed slots stay a tagged
        // unsupported freed-list traversal instead of host dangling pointers.
        if (current && !current->retired_)
            return current;
    }
    return task;
}

TaskHandle TaskScheduler::list_head(int index) const {
    const auto actual = canonical_index(index);
    return actual < 0 ? TaskHandle{} : retain(heads_[static_cast<std::size_t>(actual)]);
}
TaskHandle TaskScheduler::first_child(const TaskHandle &task) const {
    return task && task->owner_ == this ? retain(task->first_child_) : TaskHandle{};
}
TaskHandle TaskScheduler::next_task(const TaskHandle &task) const {
    return task && task->owner_ == this ? retain(task->next_) : TaskHandle{};
}

bool TaskScheduler::active_member(const TaskHandle &sought) const {
    if (!validate(sought))
        return false;
    std::vector<Task *> pending;
    for (auto *head : heads_)
        if (head)
            pending.push_back(head);
    std::unordered_set<Task *> seen;
    while (!pending.empty()) {
        auto *current = pending.back();
        pending.pop_back();
        if (!seen.insert(current).second)
            continue;
        if (current == sought.get())
            return true;
        if (current->next_)
            pending.push_back(current->next_);
        if (current->first_child_)
            pending.push_back(current->first_child_);
    }
    return false;
}

TaskResult TaskScheduler::source_work_failure(const TaskHandle &task, const TaskWorkResult &failure,
                                              TaskStep step) const {
    return {TaskFailure::work_failure, task, failure.failure, step};
}

TaskResult TaskScheduler::initialize_source_pool(std::shared_ptr<TaskWorkArena> arena,
                                                 std::shared_ptr<void> default_cooperative_thread) {
    if (arena_)
        return {TaskFailure::source_pool_already_initialized,
                {},
                {},
                TaskStep::pool_already_initialized};
    if (!storage_.empty())
        return {TaskFailure::source_pool_requires_empty_scheduler,
                {},
                {},
                TaskStep::pool_requires_empty_scheduler};
    if (!arena)
        return {TaskFailure::source_pool_uninitialized, {}, {}, TaskStep::pool_allocation};
    const auto reserved = arena->allocate(kTaskPoolBytes);
    if (!reserved)
        return source_work_failure({}, reserved, TaskStep::pool_allocation);
    arena_ = std::move(arena);
    source_pool_work_ = reserved.work;
    default_cooperative_thread_ = std::move(default_cooperative_thread);
    context_.cooperative_thread.reset();
    // The source pool is zeroed after allocation, with only free-list linkage
    // nonzero. Those opaque links are represented by native slot indices.
    const auto zeroed = arena_->fill(source_pool_work_, 0, 0, kTaskPoolBytes);
    if (!zeroed)
        return source_work_failure({}, zeroed, TaskStep::pool_clear);
    for (std::size_t i = 4096; i != 0; --i)
        free_slots_.push_back(static_cast<std::uint16_t>(i - 1));
    return {};
}

void TaskScheduler::source_no_op(TaskScheduler &, const TaskHandle &) {}

TaskHandle TaskScheduler::source_task(std::uint16_t slot) const {
    return slot < source_slots_.size() ? source_slots_[slot] : TaskHandle{};
}
TaskGraphState TaskScheduler::source_graph() const {
    TaskGraphState out;
    out.free_slots = free_slots_;
    out.context = context_;
    if (!arena_)
        out.failure = TaskFailure::source_pool_uninitialized;
    for (const auto &[p, t] : storage_) {
        (void)p;
        if (!t->source_slot_)
            out.failure = TaskFailure::invalid_source_graph;
    }
    const auto slot = [](Task *t) -> std::optional<std::uint16_t> {
        return t ? t->source_slot_ : std::nullopt;
    };
    for (unsigned k = 0; k < 8; ++k)
        out.heads[k] = slot(heads_[k]);
    for (const auto &t : source_slots_)
        if (t && !t->retired_) {
            TaskGraphNode node{};
            node.slot = *t->source_slot_;
            node.next = t->linked_ ? slot(t->next_) : t->detached_free_next_slot_;
            node.previous = slot(t->previous_);
            node.parent = slot(t->parent_);
            node.child = slot(t->first_child_);
            node.linked = t->linked_;
            node.exec = t->exec;
            node.display = t->display;
            node.destructor = t->destructor;
            node.cooperative_thread = t->cooperative_thread;
            node.primary = t->primary_work;
            node.motion = t->motion_work;
            node.force = t->force_work;
            node.auxiliary = t->auxiliary_work;
            node.control = t->object_control;
            node.primary_state = t->primary_state;
            node.auxiliary_effect_work = t->auxiliary_effect_work;
            node.auxiliary_native_work = t->auxiliary_native_work;
            node.motion_state = t->motion_state;
            out.nodes.push_back(std::move(node));
        }
    return out;
}
// NOTSA: temporary validated interchange plan, never another graph owner.
struct TaskScheduler::GraphRestorePlan {
    std::array<bool, 4096> free{}, used{};
    std::array<const TaskGraphNode *, 4096> nodes_by_slot{};
    std::array<std::size_t, 4096> lists{};
    std::map<Task::ObjectControl *, std::optional<std::uint16_t>> control_targets;
};

TaskResult TaskScheduler::validate_graph_allocations(const TaskGraphState &state,
                                                     GraphRestorePlan &plan) const {
    const auto bad = [] { return TaskResult{TaskFailure::invalid_source_graph, {}}; };
    auto &free = plan.free;
    auto &used = plan.used;
    // NOTSA: index the validated snapshot once; list traversal must not scan
    // every live task again for each link at every guest callback boundary.
    auto &nodes_by_slot = plan.nodes_by_slot;
    for (auto slot : state.free_slots) {
        if (slot >= 4096 || free[slot])
            return bad();
        free[slot] = true;
    }
    for (const auto &node : state.nodes) {
        if (node.slot >= nodes_by_slot.size() || free[node.slot] || used[node.slot])
            return bad();
        used[node.slot] = true;
        nodes_by_slot[node.slot] = &node;
    }
    const auto valid = [&](std::optional<std::uint16_t> v, bool permit_free = false) {
        return !v || (*v < 4096 && (used[*v] || (permit_free && free[*v])));
    };
    for (unsigned k = 0; k < 4096; ++k)
        if (!used[k] && !free[k])
            return bad();
    for (const auto &n : state.nodes) {
        if (!valid(n.next, !n.linked) || !valid(n.previous) || !valid(n.parent) || !valid(n.child))
            return bad();
        for (const auto &work : {n.primary, n.motion, n.force, n.auxiliary})
            if (work && !arena_->read(work, 0, nullptr, 0))
                return bad();
    }
    // NOTSA: raw-only imports cannot silently discard or replace a canonical
    // numeric owner while retaining its same actual motion allocation.
    for (const auto &node : state.nodes) {
        if (node.motion_state && !node.motion)
            return bad();
        const auto &current = source_slots_[node.slot];
        if (current && current->motion_state && current->motion_work == node.motion &&
            current->motion_state != node.motion_state)
            return bad();
    }
    return {};
}

TaskResult TaskScheduler::validate_graph_links(const TaskGraphState &state,
                                               GraphRestorePlan &plan) const {
    const auto bad = [] { return TaskResult{TaskFailure::invalid_source_graph, {}}; };
    const auto &free = plan.free;
    const auto &used = plan.used;
    const auto &nodes_by_slot = plan.nodes_by_slot;
    const auto valid = [&](std::optional<std::uint16_t> v, bool permit_free = false) {
        return !v || (*v < 4096 && (used[*v] || (permit_free && free[*v])));
    };
    for (const auto &h : state.heads)
        if (!valid(h))
            return bad();
    std::array<unsigned, 4096> visits{};
    auto &lists = plan.lists;
    std::function<bool(std::optional<std::uint16_t>, std::optional<std::uint16_t>, unsigned)> walk;
    walk = [&](std::optional<std::uint16_t> head, std::optional<std::uint16_t> parent,
               unsigned list) {
        std::optional<std::uint16_t> previous;
        while (head) {
            if (++visits[*head] != 1)
                return false;
            const auto *n = nodes_by_slot[*head];
            if (!n || !n->linked || n->previous != previous || n->parent != parent)
                return false;
            lists[*head] = list;
            if (!walk(n->child, head, list))
                return false;
            previous = head;
            head = n->next;
        }
        return true;
    };
    for (unsigned k = 0; k < 8; ++k)
        if (!walk(state.heads[k], {}, k))
            return bad();
    for (const auto &n : state.nodes)
        if (!n.linked && !n.parent && !walk(n.child, n.slot, 0))
            return bad();
    for (const auto &n : state.nodes)
        if (n.linked != (visits[n.slot] != 0))
            return bad();
    auto &control_targets = plan.control_targets;
    for (const auto &n : state.nodes)
        if (n.control) {
            const auto &target = n.control->linked_task;
            if (target &&
                (target->owner_ != this || !target->source_slot_ || !used[*target->source_slot_]))
                return bad();
            control_targets[n.control.get()] = target ? target->source_slot_ : std::nullopt;
        }
    return {};
}

void TaskScheduler::publish_graph(const TaskGraphState &state, const GraphRestorePlan &plan) {
    const auto &lists = plan.lists;
    const auto &control_targets = plan.control_targets;
    for (auto &[p, t] : storage_) {
        (void)p;
        if (t->object_control && t->object_control->linked_task &&
            t->object_control->linked_task->owner_ == this)
            t->object_control->linked_task.reset();
    }
    storage_.clear();
    source_slots_.fill({});
    heads_.fill(nullptr);
    free_slots_ = state.free_slots;
    context_ = state.context;
    for (const auto &n : state.nodes) {
        TaskHandle t(new Task);
        t->owner_ = this;
        t->source_slot_ = n.slot;
        t->linked_ = n.linked;
        t->list_ = lists[n.slot];
        t->exec = n.exec;
        t->display = n.display;
        t->destructor = n.destructor;
        t->cooperative_thread = n.cooperative_thread;
        t->primary_work = n.primary;
        t->motion_work = n.motion;
        t->force_work = n.force;
        t->auxiliary_work = n.auxiliary;
        t->object_control = n.control;
        t->primary_state = n.primary_state;
        t->auxiliary_effect_work = n.auxiliary_effect_work;
        t->auxiliary_native_work = n.auxiliary_native_work;
        t->motion_state = n.motion_state;
        if (!n.linked)
            t->detached_free_next_slot_ = n.next;
        source_slots_[n.slot] = t;
        storage_[t.get()] = t;
    }
    const auto pointer = [&](std::optional<std::uint16_t> v) {
        return v ? source_slots_[*v].get() : nullptr;
    };
    for (const auto &n : state.nodes) {
        auto &t = source_slots_[n.slot];
        if (n.linked)
            t->next_ = pointer(n.next);
        t->previous_ = pointer(n.previous);
        t->parent_ = pointer(n.parent);
        t->first_child_ = pointer(n.child);
    }
    for (unsigned k = 0; k < 8; ++k)
        heads_[k] = pointer(state.heads[k]);
    for (const auto &[control, target] : control_targets)
        control->linked_task = target ? source_slots_[*target] : TaskHandle{};
}

TaskResult TaskScheduler::restore_source_graph(const TaskGraphState &state) {
    const auto bad = []() {
        return TaskResult{TaskFailure::invalid_source_graph, {}, {}, TaskStep::none};
    };
    if (!arena_ || walk_depth_ || state.failure != TaskFailure::none)
        return bad();
    for (const auto &[p, t] : storage_) {
        (void)p;
        if (!t->retired_ &&
            (t->destructor_active_ || t->destruction_stage_ != Task::DestructionStage::live))
            return bad();
    }
    GraphRestorePlan plan;
    if (const auto result = validate_graph_allocations(state, plan); !result)
        return result;
    if (const auto result = validate_graph_links(state, plan); !result)
        return result;
    publish_graph(state, plan);
    return {};
}

TaskResult TaskScheduler::allocate_task(int level, TaskCallback callback) {
    if (level < 0 || level >= 9)
        return {TaskFailure::invalid_list_index, {}, {}, TaskStep::allocation_level};
    if (!arena_)
        return {TaskFailure::source_pool_uninitialized, {}, {}, TaskStep::pool_required};
    if (free_slots_.empty())
        return {TaskFailure::source_pool_exhausted, {}, {}, TaskStep::pool_exhaustion};
    const auto slot = free_slots_.back();
    free_slots_.pop_back();
    TaskHandle task(new Task);
    task->owner_ = this;
    task->source_slot_ = slot;
    task->exec = callback ? callback : &source_no_op;
    source_slots_[slot] = task;
    storage_.emplace(task.get(), task);
    if (level == 7 && !free_slots_.empty())
        task->detached_free_next_slot_ = free_slots_.back();
    if (level != 7) {
        task->list_ = static_cast<std::size_t>(canonical_index(level));
        task->next_ = heads_[task->list_];
        if (task->next_)
            task->next_->previous_ = task.get();
        heads_[task->list_] = task.get();
        task->linked_ = true;
    }
    return {TaskFailure::none, task};
}

TaskResult TaskScheduler::create_element(std::uint32_t flags, int level, TaskCallback callback) {
    auto created = allocate_task(level, callback);
    if (!created)
        return created;
    const auto task = created.task;
    struct Field {
        std::uint8_t flag;
        std::uint32_t bytes;
        TaskWorkHandle Task::*member;
        TaskStep step;
    };
    const Field fields[] = {{2, 64, &Task::primary_work, TaskStep::primary_initialization},
                            {4, 56, &Task::force_work, TaskStep::force_initialization},
                            {1, 64, &Task::motion_work, TaskStep::motion_initialization},
                            {8, 16, &Task::auxiliary_work, TaskStep::auxiliary_initialization}};
    for (const auto &field : fields) {
        if (!(static_cast<std::uint8_t>(flags) & field.flag))
            continue;
        const auto allocated = arena_->allocate(field.bytes);
        if (!allocated) {
            const auto rollback = destroy_task(task);
            if (!rollback)
                return {TaskFailure::detached_creation_rollback_rejected, task, allocated.failure,
                        TaskStep::creation_rollback};
            return source_work_failure(task, allocated, field.step);
        }
        task.get()->*(field.member) = allocated.work;
        const auto zeroed = arena_->fill(allocated.work, 0, 0, field.bytes);
        if (!zeroed)
            return source_work_failure(task, zeroed, field.step);
    }
    if (context_.cooperative_thread)
        task->cooperative_thread = context_.cooperative_thread;
    else {
        try {
            task->cooperative_thread =
                default_resource_reader_ ? default_resource_reader_() : default_cooperative_thread_;
        } catch (const TaskResourceUnavailable &) {
            return {
                TaskFailure::default_resource_unavailable, task, {}, TaskStep::default_resource};
        }
    }
    return created;
}

TaskResult TaskScheduler::create_child(std::uint32_t flags, TaskCallback callback,
                                       const TaskHandle &parent) {
    const auto accepted = validate(parent);
    if (!accepted)
        return accepted;
    auto created = create_element(flags, 7, callback);
    if (!created)
        return created;
    const auto task = created.task;
    task->next_ = parent->first_child_;
    if (task->next_)
        task->next_->previous_ = task.get();
    parent->first_child_ = task.get();
    task->parent_ = parent.get();
    task->linked_ = true;
    if (task->primary_work && parent->primary_work) {
        if (parent->primary_state) {
            if (!child_state_factory_)
                return {TaskFailure::child_state_factory_missing,
                        task,
                        {},
                        TaskStep::child_transform_read};
            if (const auto step = child_state_factory_(*parent, *task); step != TaskStep::none)
                return {TaskFailure::work_failure, task, TaskWorkFailure::unobserved_live_field,
                        step};
        } else {
            // Source loads/stores each raw word, preserving NaN payloads and angle
            // bits. Position comes before angles; failure keeps already copied words.
            for (const auto at : {32u, 36u, 40u, 20u, 24u, 28u}) {
                std::uint32_t word = 0;
                auto result = arena_->read(parent->primary_work, at, &word, 4);
                if (!result)
                    return source_work_failure(task, result, TaskStep::child_transform_read);
                result = arena_->write(task->primary_work, at, &word, 4);
                if (!result)
                    return source_work_failure(task, result, TaskStep::child_transform_write);
            }
        }
    }
    task->cooperative_thread = parent->cooperative_thread;
    return created;
}

void TaskScheduler::destroy_callback(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto result = scheduler.destroy_task(task);
    if (!result)
        throw SourceTaskFailure{result};
}
void TaskScheduler::free_children_callback(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto result = scheduler.free_children(task);
    if (!result)
        throw SourceTaskFailure{result};
}

TaskLifetimeState TaskScheduler::begin_destruction(const TaskHandle &task) {
    TaskLifetimeState state;
    state.task = task;
    state.failure = validate(task);
    if (state.failure && !active_member(task))
        state.failure = {TaskFailure::inactive_task, task, {}, TaskStep::membership};
    if (state.failure &&
        (task->destructor_active_ || task->destruction_stage_ != Task::DestructionStage::live))
        state.failure = {TaskFailure::callback_mutation_invalidated_destructor,
                         task,
                         {},
                         TaskStep::destruction_entry};
    return state;
}
TaskLifetimeState TaskScheduler::begin_free_children(const TaskHandle &task) {
    TaskLifetimeState state;
    state.task = task;
    state.children = true;
    state.failure = validate(task);
    return state;
}
TaskLifetimeCall TaskScheduler::advance_child_lifetime(TaskLifetimeState &state,
                                                       const TaskHandle &task) {
    if (state.phase == TaskLifetimePhase::begin && task->exec == &free_children_callback) {
        state.phase = TaskLifetimePhase::callback_complete;
        return {{}, task, nullptr, TaskLifetimeCallee::run_children, false};
    }
    if (state.phase == TaskLifetimePhase::callback_complete) {
        if (!task->first_child_)
            task->exec = &destroy_callback;
        return {{}, task, nullptr, TaskLifetimeCallee::none, true};
    }
    if (!task->first_child_)
        return {{}, task, nullptr, TaskLifetimeCallee::none, true};
    if (!active_member(task)) {
        if (task->first_child_->next_)
            return {{TaskFailure::inactive_children_corruption,
                     task,
                     {},
                     TaskStep::inactive_child_sibling},
                    task,
                    nullptr,
                    TaskLifetimeCallee::none,
                    false};
        const auto child = retain(task->first_child_);
        if (!child || child->parent_ != task.get())
            return {{TaskFailure::inactive_children_corruption,
                     task,
                     {},
                     TaskStep::inactive_child_parent},
                    task,
                    nullptr,
                    TaskLifetimeCallee::none,
                    false};
        task->first_child_ = nullptr;
        child->linked_ = false;
        return {{}, task, nullptr, TaskLifetimeCallee::none, true};
    }
    return {{}, retain(task->first_child_), nullptr, TaskLifetimeCallee::destroy_child, false};
}

TaskLifetimeCall TaskScheduler::advance_lifetime(TaskLifetimeState &state) {
    if (!state.failure)
        return {state.failure, state.task, nullptr, TaskLifetimeCallee::none, false};
    const auto resolve = [&](const TaskHandle &value) {
        if (value && value->source_slot_) {
            const auto current = source_slots_[*value->source_slot_];
            return current ? current : TaskHandle{};
        }
        return value;
    };
    const auto task = resolve(state.task);
    if (!task || task->retired_)
        return {{TaskFailure::callback_mutation_invalidated_destructor,
                 state.task,
                 {},
                 TaskStep::destruction_generation},
                state.task,
                nullptr,
                TaskLifetimeCallee::none,
                false};
    state.task = task;
    if (state.children)
        return advance_child_lifetime(state, task);

    if (state.phase == TaskLifetimePhase::begin) {
        state.phase = TaskLifetimePhase::callback_complete;
        if (task->destructor)
            return {{}, task, task->destructor, TaskLifetimeCallee::destructor, false};
    }
    if (state.phase == TaskLifetimePhase::callback_complete) {
        state.previous = retain(task->previous_);
        state.next = retain(task->next_);
        task->exec = task->display = task->destructor = nullptr;
        state.phase = TaskLifetimePhase::children_complete;
        return {{}, task, nullptr, TaskLifetimeCallee::free_children, false};
    }
    if (state.phase == TaskLifetimePhase::children_complete) {
        if (task->first_child_)
            return {{TaskFailure::callback_mutation_invalidated_destructor,
                     task,
                     {},
                     TaskStep::children_destroyed},
                    task,
                    nullptr,
                    TaskLifetimeCallee::none,
                    false};
        const auto previous = resolve(state.previous), next = resolve(state.next);
        if ((state.previous && !previous) || (state.next && !next))
            return {{TaskFailure::callback_mutation_invalidated_destructor,
                     task,
                     {},
                     TaskStep::children_destroyed},
                    task,
                    nullptr,
                    TaskLifetimeCallee::none,
                    false};
        task->destruction_previous_ = previous;
        task->destruction_next_ = next;
        task->destruction_stage_ = Task::DestructionStage::children;
        state.phase = TaskLifetimePhase::complete;
        const auto result = finish_destruction(task);
        return {result, task, nullptr, TaskLifetimeCallee::none, bool(result)};
    }
    return {{}, task, nullptr, TaskLifetimeCallee::none, true};
}
TaskResult TaskScheduler::free_children(const TaskHandle &parent) {
    auto state = begin_free_children(parent);
    for (;;) {
        const auto call = advance_lifetime(state);
        if (!call.result)
            return call.result;
        if (call.completed)
            return {};
        const auto result = call.callee == TaskLifetimeCallee::run_children
                                ? run_children(call.task)
                                : destroy_task(call.task);
        if (!result)
            return result;
    }
}
TaskResult TaskScheduler::destroy_task(const TaskHandle &requested_task) {
    // NOTSA: the source takes its task pointer by value. A destructor may clear or
    // replace the canonical owner passed by reference without changing this call.
    const TaskHandle task = requested_task;
    auto state = begin_destruction(task);
    for (;;) {
        const auto call = advance_lifetime(state);
        if (!call.result)
            return call.result;
        if (call.completed)
            return {};
        if (call.callee == TaskLifetimeCallee::destructor) {
            task->destructor_active_ = true;
            try {
                call.callback(*this, call.task);
            } catch (...) {
                task->destructor_active_ = false;
                throw;
            }
            task->destructor_active_ = false;
        } else {
            const auto result = free_children(call.task);
            if (!result)
                return result;
        }
    }
}

TaskResult TaskScheduler::resume_destruction(const TaskHandle &task) {
    const auto accepted = validate(task);
    if (!accepted)
        return accepted;
    if (task->destruction_stage_ == Task::DestructionStage::live)
        return {TaskFailure::inactive_task, task, {}, TaskStep::cleanup_resume};
    return finish_destruction(task);
}

TaskResult TaskScheduler::unlink_destroyed_task(const TaskHandle &task) {
    using Stage = Task::DestructionStage;
    if (task->destruction_stage_ == Stage::children) {
        const auto children = free_children(task);
        if (!children)
            return children;
        const auto previous = task->destruction_previous_;
        const auto next = task->destruction_next_;
        if ((previous && previous->retired_) || (next && next->retired_))
            return {TaskFailure::callback_mutation_invalidated_destructor,
                    task,
                    {},
                    TaskStep::children_destroyed};
        if (previous)
            previous->next_ = next.get();
        else if (task->parent_)
            task->parent_->first_child_ = next.get();
        else {
            for (auto &head : heads_)
                if (head == task.get()) {
                    head = next.get();
                    break;
                }
        }
        if (next)
            next->previous_ = previous.get();
        task->linked_ = false;
        task->destruction_stage_ = Stage::collision;
    }
    return {};
}

TaskResult TaskScheduler::release_collision_work(const TaskHandle &task) {
    using Stage = Task::DestructionStage;
    if (task->destruction_stage_ == Stage::collision) {
        if (task->primary_work) {
            const auto collision = arena_
                                       ? arena_->reference(task->primary_work, kCollisionReference)
                                       : TaskWorkResult{TaskWorkFailure::foreign_work, {}};
            if (!collision)
                return source_work_failure(task, collision, TaskStep::collision_reference);
            if (collision.work) {
                std::uint8_t flags = 0;
                auto result = arena_->read(collision.work, 5, &flags, 1);
                if (!result)
                    return source_work_failure(task, result, TaskStep::collision_flags);
                if (flags & kCollisionOwnsExtra) {
                    const auto nested = arena_->reference(collision.work, 12);
                    if (!nested)
                        return source_work_failure(task, nested,
                                                   TaskStep::collision_extra_reference);
                    result = arena_->release(nested.work);
                    if (!result)
                        return source_work_failure(task, result, TaskStep::collision_extra_release);
                    result = arena_->set_reference(collision.work, 12, {});
                    if (!result)
                        return source_work_failure(task, result, TaskStep::collision_extra_clear);
                }
                result = arena_->release(collision.work);
                if (!result)
                    return source_work_failure(task, result, TaskStep::collision_release);
                result = arena_->set_reference(task->primary_work, kCollisionReference, {});
                if (!result)
                    return source_work_failure(task, result, TaskStep::collision_clear);
            }
        }
        task->destruction_stage_ = Stage::object;
    }
    return {};
}

TaskResult TaskScheduler::release_first_object_resources(const TaskHandle &task) {
    using Stage = Task::DestructionStage;
    if (task->destruction_stage_ == Stage::object) {
        if (task->primary_work) {
            const auto object = arena_->reference(task->primary_work, kObjectReference);
            if (!object)
                return source_work_failure(task, object, TaskStep::object_reference);
            if (object.work) {
                task->cleanup_object_ = object.work;
                const auto cursor = arena_->reference(object.work, kObjectFirstChain);
                if (!cursor)
                    return source_work_failure(task, cursor, TaskStep::object_first_chain);
                task->cleanup_cursor_ = cursor.work;
                task->destruction_stage_ = Stage::object_first_chain;
            }
        }
        if (task->destruction_stage_ == Stage::object)
            task->destruction_stage_ = Stage::primary;
    }
    if (task->destruction_stage_ == Stage::object_first_chain) {
        while (task->cleanup_cursor_) {
            const auto next = arena_->reference(task->cleanup_cursor_, kFirstChainNext);
            if (!next)
                return source_work_failure(task, next, TaskStep::object_first_next);
            const auto released = arena_->release(task->cleanup_cursor_);
            if (!released)
                return source_work_failure(task, released, TaskStep::object_first_release);
            task->cleanup_cursor_ = next.work; // source snapshot before free
        }
        const auto cleared = arena_->set_reference(task->cleanup_object_, kObjectFirstChain, {});
        if (!cleared)
            return source_work_failure(task, cleared, TaskStep::object_first_clear);
        task->destruction_stage_ = Stage::object_extra;
    }
    if (task->destruction_stage_ == Stage::object_extra) {
        const auto extra = arena_->reference(task->cleanup_object_, kObjectExtra);
        if (!extra)
            return source_work_failure(task, extra, TaskStep::object_extra_reference);
        const auto released = arena_->release(extra.work);
        if (!released)
            return source_work_failure(task, released, TaskStep::object_extra_release);
        if (extra.work) {
            const auto cleared = arena_->set_reference(task->cleanup_object_, kObjectExtra, {});
            if (!cleared)
                return source_work_failure(task, cleared, TaskStep::object_extra_clear);
        }
        task->destruction_stage_ = Stage::object_first_counter;
    }
    return {};
}

TaskResult TaskScheduler::release_second_object_resources(const TaskHandle &task) {
    using Stage = Task::DestructionStage;
    if (task->destruction_stage_ == Stage::object_first_counter) {
        const auto cleared = arena_->fill(task->cleanup_object_, kObjectFirstCounter, 0, 4);
        if (!cleared)
            return source_work_failure(task, cleared, TaskStep::object_counter_clear);
        const auto cursor = arena_->reference(task->cleanup_object_, kObjectSecondChain);
        if (!cursor)
            return source_work_failure(task, cursor, TaskStep::object_second_chain);
        task->cleanup_cursor_ = cursor.work;
        task->destruction_stage_ = Stage::object_second_chain;
    }
    if (task->destruction_stage_ == Stage::object_second_chain) {
        while (task->cleanup_cursor_) {
            const auto next = arena_->reference(task->cleanup_cursor_, kSecondChainNext);
            if (!next)
                return source_work_failure(task, next, TaskStep::object_second_next);
            const auto released = arena_->release(task->cleanup_cursor_);
            if (!released)
                return source_work_failure(task, released, TaskStep::object_second_release);
            task->cleanup_cursor_ = next.work;
        }
        const auto cleared = arena_->set_reference(task->cleanup_object_, kObjectSecondChain, {});
        if (!cleared)
            return source_work_failure(task, cleared, TaskStep::object_second_clear);
        task->destruction_stage_ = Stage::object_second_counter;
    }
    if (task->destruction_stage_ == Stage::object_second_counter) {
        const auto cleared = arena_->fill(task->cleanup_object_, kObjectSecondCounter, 0, 4);
        if (!cleared)
            return source_work_failure(task, cleared, TaskStep::object_second_counter_clear);
        task->destruction_stage_ = Stage::object_release;
    }
    if (task->destruction_stage_ == Stage::object_release) {
        const auto released = arena_->release(task->cleanup_object_);
        if (!released)
            return source_work_failure(task, released, TaskStep::object_release);
        const auto cleared = arena_->set_reference(task->primary_work, kObjectReference, {});
        if (!cleared)
            return source_work_failure(task, cleared, TaskStep::object_clear);
        task->cleanup_object_.reset();
        task->destruction_stage_ = Stage::primary;
    }
    return {};
}

TaskResult TaskScheduler::release_owned_task_work(const TaskHandle &task) {
    using Stage = Task::DestructionStage;
    struct Field {
        Stage stage;
        Stage next;
        TaskWorkHandle Task::*member;
        TaskStep step;
    };
    const Field fields[] = {
        {Stage::primary, Stage::force, &Task::primary_work, TaskStep::primary_release},
        {Stage::force, Stage::motion, &Task::force_work, TaskStep::force_release},
        {Stage::motion, Stage::auxiliary, &Task::motion_work, TaskStep::motion_release},
        {Stage::auxiliary, Stage::control, &Task::auxiliary_work, TaskStep::auxiliary_release}};
    for (const auto &field : fields) {
        if (task->destruction_stage_ != field.stage)
            continue;
        auto &work = task.get()->*(field.member);
        if (work) {
            const auto released = arena_ ? arena_->release(work)
                                         : TaskWorkResult{TaskWorkFailure::foreign_work, work};
            if (!released)
                return source_work_failure(task, released, field.step);
            work.reset();
        }
        if (field.stage == Stage::auxiliary)
            task->auxiliary_native_work.reset();
        task->destruction_stage_ = field.next;
    }
    if (task->object_control) {
        task->object_control->linked_task.reset();
        task->object_control->status_flags &= kClearControlLinked;
        task->object_control.reset();
    }
    task->destruction_stage_ = Stage::recycle;
    task->retired_ = true;
    task->next_ = task->previous_ = task->parent_ = task->first_child_ = nullptr;
    task->destruction_previous_.reset();
    task->destruction_next_.reset();
    if (task->source_slot_) {
        source_slots_[*task->source_slot_].reset();
        free_slots_.push_back(*task->source_slot_);
    }
    if (!walk_depth_)
        reclaim_retired();
    return {};
}

TaskResult TaskScheduler::finish_destruction(const TaskHandle &task) {
    if (const auto result = unlink_destroyed_task(task); !result)
        return result;
    if (const auto result = release_collision_work(task); !result)
        return result;
    if (const auto result = release_first_object_resources(task); !result)
        return result;
    if (const auto result = release_second_object_resources(task); !result)
        return result;
    if (const auto result = release_owned_task_work(task); !result)
        return result;
    return {};
}

TaskResult TaskScheduler::retire(const TaskHandle &task) {
    const auto accepted = validate(task);
    if (!accepted) {
        return accepted;
    }
    std::vector<Task *> pending{task.get()};
    while (!pending.empty()) {
        auto *current = pending.back();
        pending.pop_back();
        if (current->source_slot_)
            return {TaskFailure::source_task_requires_destructor, retain(current)};
        for (auto *child = current->first_child_; child; child = child->next_)
            pending.push_back(child);
    }
    retire_subtree(task);
    if (walk_depth_ == 0) {
        reclaim_retired();
    }
    return {};
}

void TaskScheduler::reclaim_retired() {
    for (auto iterator = storage_.begin(); iterator != storage_.end();) {
        if (iterator->second->retired_) {
            iterator = storage_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

} // namespace sadx

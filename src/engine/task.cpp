#include "engine/task.hpp"
#include "player/player_task.hpp"

#include <utility>
#include <algorithm>
#include <unordered_set>

namespace sadx {

namespace {
// Only recovered built-in callbacks throw this private control transfer. It
// preserves source partial state; arbitrary host callback exceptions still pass
// through the existing native exception boundary.
struct SourceTaskFailure { TaskResult result; };
}

class TaskScheduler::WalkGuard {
public:
    explicit WalkGuard(TaskScheduler& scheduler) : scheduler_(scheduler) {
        ++scheduler_.walk_depth_;
    }
    ~WalkGuard() {
        if (--scheduler_.walk_depth_ == 0) {
            scheduler_.reclaim_retired();
        }
    }
private:
    TaskScheduler& scheduler_;
};

int TaskScheduler::canonical_index(int list_index) noexcept {
    if (list_index == 8) {
        return 7;
    }
    return list_index >= 0 && list_index < 8 ? list_index : -1;
}

TaskCallbackContextResult TaskScheduler::register_callback_context(TaskCallback callback,
    std::shared_ptr<TaskCallbackContext> owner) {
    if(!callback)return {TaskCallbackContextFailure::null_callback};
    if(!owner)return {TaskCallbackContextFailure::null_context};
    const auto found=callback_contexts_.find(callback);
    if(found!=callback_contexts_.end())
        return {found->second==owner?TaskCallbackContextFailure::none:TaskCallbackContextFailure::conflicting_owner};
    callback_contexts_.emplace(callback,std::move(owner));return {};
}

std::shared_ptr<TaskCallbackContext> TaskScheduler::callback_context(TaskCallback callback) const {
    const auto found=callback_contexts_.find(callback);
    return found==callback_contexts_.end()?std::shared_ptr<TaskCallbackContext>{}:found->second;
}

[[noreturn]] void TaskScheduler::raise_callback_failure(TaskResult result) {
    throw SourceTaskFailure{std::move(result)};
}

TaskResult TaskScheduler::validate(const TaskHandle& task) const {
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

TaskHandle TaskScheduler::retain(Task* task) const {
    if (!task) {
        return {};
    }
    const auto found = storage_.find(task);
    return found == storage_.end() ? TaskHandle{} : found->second;
}

void TaskScheduler::append_link(Task*& head, const TaskHandle& task) {
    if (!head) {
        head = task.get();
        return;
    }
    auto* tail = head;
    while (tail->next_) {
        tail = tail->next_;
    }
    tail->next_ = task.get();
    task->previous_ = tail;
}

TaskResult TaskScheduler::append_task(int list_index, TaskCallback callback,
                                       std::shared_ptr<void> field30) {
    const int index = canonical_index(list_index);
    if (index < 0) {
        return {TaskFailure::invalid_list_index, {}};
    }
    TaskHandle task(new Task);
    task->owner_ = this;
    task->list_ = static_cast<std::size_t>(index);
    task->exec = callback;
    task->field30 = std::move(field30);
    task->linked_ = true;
    storage_.emplace(task.get(), task);
    append_link(heads_[task->list_], task);
    return {TaskFailure::none, task};
}

TaskResult TaskScheduler::append_child(const TaskHandle& parent, TaskCallback callback,
                                        std::shared_ptr<void> field30) {
    const auto accepted = validate(parent);
    if (!accepted) {
        return accepted;
    }
    TaskHandle task(new Task);
    task->owner_ = this;
    task->parent_ = parent.get();
    task->exec = callback;
    task->field30 = std::move(field30);
    task->linked_ = true;
    storage_.emplace(task.get(), task);
    append_link(parent->first_child_, task);
    return {TaskFailure::none, task};
}

TaskResult TaskScheduler::run_index(int list_index) {
    const int index = canonical_index(list_index);
    if (index < 0) {
        return {TaskFailure::invalid_list_index, {}};
    }
    WalkGuard guard(*this);
    auto task = retain(heads_[static_cast<std::size_t>(index)]);
    while (task) {
        if (task->retired_) {
            return {TaskFailure::deleted_snapshot_next, task};
        }
        if (task->source_slot_ && !task->linked_)
            return {TaskFailure::detached_snapshot_next, task, {}, 0x40b0e0};
        const auto next = retain(task->next_); // snapshot BEFORE callback
        const auto callback = task->exec;
        context_.current_function = callback;
        if (callback) {
            context_.field30 = task->field30;
            try {
                callback(*this, task);
            } catch (const SourceTaskFailure& failed) {
                return failed.result;
            } catch (...) {
                // Native callback exceptions have no x86 counterpart. Release
                // native context ownership, retain the exception and stop.
                context_.field30.reset();
                context_.current_function = nullptr;
                throw;
            }
            context_.field30.reset();
        }
        context_.current_function = nullptr;
        task = resolve_snapshot(next);
    }
    return {};
}

TaskResult TaskScheduler::run_game_tasks() {
    auto result=run_index(8);
    if(!result)return result;
    for(int index=0;index<7;++index){
        result=run_index(index);
        if(!result)return result;
    }
    return {};
}

TaskResult TaskScheduler::run_children(const TaskHandle& parent) {
    const auto accepted = validate(parent);
    if (!accepted) {
        return accepted;
    }
    WalkGuard guard(*this);
    auto task = retain(parent->first_child_);
    while (task) {
        if (task->retired_) {
            return {TaskFailure::deleted_snapshot_next, task};
        }
        if (task->source_slot_ && !task->linked_)
            return {TaskFailure::detached_snapshot_next, task, {}, 0x40b433};
        const auto next = retain(task->next_); // snapshot BEFORE callback
        const auto callback = task->exec;
        context_.field30 = task->field30;
        if (!callback) {
            // Original ff 50 10 attempts a call through zero. Surface the
            // failure and release the native handle instead of crashing.
            context_.field30.reset();
            return {TaskFailure::null_child_callback, task};
        }
        try {
            callback(*this, task);
        } catch (const SourceTaskFailure& failed) {
            return failed.result;
        } catch (...) {
            context_.field30.reset();
            throw;
        }
        context_.field30.reset();
        // Deliberately neither set nor restore current_function. Nested
        // run_index calls can clear it; the original does not restore globals.
        task = resolve_snapshot(next);
    }
    return {};
}

TaskResult TaskScheduler::free_task(const TaskHandle& task) {
    const auto accepted = validate(task);
    if (!accepted) {
        return accepted;
    }
    if (!active_member(task)) return {TaskFailure::inactive_task, task, {}, 0x401000};
    task->exec = &destroy_callback;
    return {};
}

void TaskScheduler::retire_subtree(const TaskHandle& task) {
    while (task->first_child_) {
        retire_subtree(retain(task->first_child_));
    }
    if (task->linked_) {
        Task*& head = task->parent_ ? task->parent_->first_child_ : heads_[task->list_];
        if (task->previous_) task->previous_->next_ = task->next_;
        else head = task->next_;
        if (task->next_) task->next_->previous_ = task->previous_;
    }
    task->linked_ = false;
    task->exec = nullptr;
    task->retired_ = true;
    task->next_ = nullptr;
    task->previous_ = nullptr;
    task->parent_ = nullptr;
}

TaskHandle TaskScheduler::resolve_snapshot(const TaskHandle& task) const {
    if (task && task->source_slot_ && task->retired_) {
        const auto& current = source_slots_[*task->source_slot_];
        // The source cached a pool address, so a same-slot new generation is
        // observed on its next iteration. Unreused freed slots stay a tagged
        // unsupported freed-list traversal instead of host dangling pointers.
        if (current && !current->retired_) return current;
    }
    return task;
}

TaskHandle TaskScheduler::list_head(int index) const {
    const auto actual = canonical_index(index);
    return actual < 0 ? TaskHandle{} : retain(heads_[static_cast<std::size_t>(actual)]);
}
TaskHandle TaskScheduler::first_child(const TaskHandle& task) const {
    return task && task->owner_ == this ? retain(task->first_child_) : TaskHandle{};
}
TaskHandle TaskScheduler::next_task(const TaskHandle& task) const {
    return task && task->owner_ == this ? retain(task->next_) : TaskHandle{};
}

bool TaskScheduler::active_member(const TaskHandle& sought) const {
    if (!validate(sought)) return false;
    std::vector<Task*> pending;
    for (auto* head : heads_) if (head) pending.push_back(head);
    std::unordered_set<Task*> seen;
    while (!pending.empty()) {
        auto* current = pending.back(); pending.pop_back();
        if (!seen.insert(current).second) continue;
        if (current == sought.get()) return true;
        if (current->next_) pending.push_back(current->next_);
        if (current->first_child_) pending.push_back(current->first_child_);
    }
    return false;
}

TaskResult TaskScheduler::source_work_failure(const TaskHandle& task,
                                             const TaskWorkResult& failure,
                                             std::uint32_t boundary) const {
    return {TaskFailure::work_failure, task, failure.failure, boundary};
}

TaskResult TaskScheduler::initialize_source_pool(std::shared_ptr<TaskWorkArena> arena,
    std::optional<std::shared_ptr<void>> default_field30) {
    if (arena_) return {TaskFailure::source_pool_already_initialized, {}, {}, 0x40b460};
    if (!storage_.empty()) return {TaskFailure::source_pool_requires_empty_scheduler, {}, {}, 0x40b466};
    if (!arena) return {TaskFailure::source_pool_uninitialized, {}, {}, 0x40b46b};
    const auto reserved = arena->allocate(0x34000);
    if (!reserved) return source_work_failure({}, reserved, 0x40b46b);
    arena_ = std::move(arena); source_pool_work_ = reserved.work;
    default_field30_ = std::move(default_field30);
    context_.field30.reset();
    // The source pool is zeroed after allocation, with only free-list linkage
    // nonzero. Those opaque links are represented by native slot indices.
    const auto zeroed = arena_->fill(source_pool_work_, 0, 0, 0x34000);
    if (!zeroed) return source_work_failure({}, zeroed, 0x40b4bd);
    for (std::size_t i = 4096; i != 0; --i)
        free_slots_.push_back(static_cast<std::uint16_t>(i - 1));
    return {};
}

void TaskScheduler::source_no_op(TaskScheduler&, const TaskHandle&) {}

TaskResult TaskScheduler::allocate_task(int level, TaskCallback callback) {
    if (level < 0 || level >= 9) return {TaskFailure::invalid_list_index, {}, {}, 0x40b03e};
    if (!arena_) return {TaskFailure::source_pool_uninitialized, {}, {}, 0x40b030};
    if (free_slots_.empty()) return {TaskFailure::source_pool_exhausted, {}, {}, 0x40b037};
    const auto slot = free_slots_.back(); free_slots_.pop_back();
    TaskHandle task(new Task);
    task->owner_ = this; task->source_slot_ = slot;
    task->exec = callback ? callback : &source_no_op;
    source_slots_[slot] = task;
    storage_.emplace(task.get(), task);
    if (level == 7 && !free_slots_.empty())
        task->detached_free_next_slot_ = free_slots_.back();
    if (level != 7) {
        task->list_ = static_cast<std::size_t>(canonical_index(level));
        task->next_ = heads_[task->list_];
        if (task->next_) task->next_->previous_ = task.get();
        heads_[task->list_] = task.get(); task->linked_ = true;
    }
    return {TaskFailure::none, task};
}

TaskResult TaskScheduler::create_element(std::uint32_t flags, int level,
                                          TaskCallback callback) {
    auto created = allocate_task(level, callback);
    if (!created) return created;
    const auto task = created.task;
    struct Field { std::uint8_t flag; std::uint32_t bytes; TaskWorkHandle Task::* member;
                   std::uint32_t boundary; };
    const Field fields[] = {{2,64,&Task::primary_work,0x40b884},
                           {4,56,&Task::force_work,0x40b8ab},
                           {1,64,&Task::motion_work,0x40b8ce},
                           {8,16,&Task::auxiliary_work,0x40b8f1}};
    for (const auto& field : fields) {
        if (!(static_cast<std::uint8_t>(flags) & field.flag)) continue;
        const auto allocated = arena_->allocate(field.bytes);
        if (!allocated) {
            const auto rollback = destroy_task(task);
            if (!rollback) return {TaskFailure::detached_creation_rollback_rejected,
                                  task, allocated.failure, 0x40b927};
            return source_work_failure(task, allocated, field.boundary);
        }
        task.get()->*(field.member) = allocated.work;
        const auto zeroed = arena_->fill(allocated.work, 0, 0, field.bytes);
        if (!zeroed) return source_work_failure(task, zeroed, field.boundary);
    }
    if (context_.field30) task->field30 = context_.field30;
    else if (default_field30_) task->field30 = *default_field30_;
    else return {TaskFailure::default_field30_unobserved, task, {}, 0x40b918};
    return created;
}

TaskResult TaskScheduler::create_child(std::uint32_t flags, TaskCallback callback,
                                        const TaskHandle& parent) {
    const auto accepted = validate(parent);
    if (!accepted) return accepted;
    auto created = create_element(flags, 7, callback);
    if (!created) return created;
    const auto task = created.task;
    task->next_ = parent->first_child_;
    if (task->next_) task->next_->previous_ = task.get();
    parent->first_child_ = task.get(); task->parent_ = parent.get(); task->linked_ = true;
    if (task->primary_work && parent->primary_work) {
        if (parent->primary_state) {
            task->primary_state = std::make_shared<PlayerTaskState>();
            auto& child = *task->primary_state;
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
            if (!parent->primary_state->position)
                return {TaskFailure::work_failure,task,TaskWorkFailure::unobserved_live_field,0x40b98d};
            child.position->x = parent->primary_state->position->x;
            child.position->y = parent->primary_state->position->y;
            child.position->z = parent->primary_state->position->z;
            child.orientation.angles[0] = parent->primary_state->orientation.angles[0];
            child.orientation.angles[1] = parent->primary_state->orientation.angles[1];
            child.orientation.angles[2] = parent->primary_state->orientation.angles[2];
            // These live fields are authoritative. Arena pose bytes are not
            // read or rewritten into a second mutable task-state snapshot.
        } else {
        // Source loads/stores each raw word, preserving NaN payloads and angle
        // bits. Position comes before angles; failure keeps already copied words.
        for (const auto at : {32u,36u,40u,20u,24u,28u}) {
            std::uint32_t word = 0;
            auto result = arena_->read(parent->primary_work, at, &word, 4);
            if (!result) return source_work_failure(task, result, 0x40b98d);
            result = arena_->write(task->primary_work, at, &word, 4);
            if (!result) return source_work_failure(task, result, 0x40b992);
        }
        }
    }
    task->field30 = parent->field30;
    return created;
}

void TaskScheduler::destroy_callback(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto result = scheduler.destroy_task(task);
    if (!result) throw SourceTaskFailure{result};
}
void TaskScheduler::free_children_callback(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto result = scheduler.free_children(task);
    if (!result) throw SourceTaskFailure{result};
}

TaskResult TaskScheduler::free_children(const TaskHandle& parent) {
    const auto accepted = validate(parent);
    if (!accepted) return accepted;
    if (parent->exec == &free_children_callback) {
        const auto dispatched = run_children(parent);
        if (!dispatched) return dispatched;
        if (!parent->first_child_) parent->exec = &destroy_callback;
        return {};
    }
    while (parent->first_child_) {
        if (!active_member(parent)) {
            // Actual inactive multi-child path writes next.prev/next.next to
            // next itself, then loops forever. Do not corrupt the native graph.
            if (parent->first_child_->next_)
                return {TaskFailure::inactive_children_corruption, parent, {}, 0x40b834};
            auto child = retain(parent->first_child_);
            if (!child || child->parent_ != parent.get())
                return {TaskFailure::inactive_children_corruption, parent, {}, 0x40b83e};
            parent->first_child_ = nullptr; // actual sole-child branch; child not freed
            child->linked_ = false;
            return {};
        }
        const auto destroyed = destroy_task(retain(parent->first_child_));
        if (!destroyed) return destroyed;
    }
    return {};
}

TaskResult TaskScheduler::destroy_task(const TaskHandle& task) {
    const auto accepted = validate(task);
    if (!accepted) return accepted;
    if (!active_member(task)) return {TaskFailure::inactive_task, task, {}, 0x401000};
    if (task->destructor_active_)
        return {TaskFailure::callback_mutation_invalidated_destructor, task, {}, 0x40b591};
    if (task->destruction_stage_ != Task::DestructionStage::live)
        return {TaskFailure::callback_mutation_invalidated_destructor, task, {}, 0x40b586};
    const auto destructor = task->destructor;
    if (destructor) {
        task->destructor_active_ = true;
        try { destructor(*this, task); }
        catch (...) { task->destructor_active_ = false; throw; }
        task->destructor_active_ = false;
    }
    if (task->retired_) return {TaskFailure::callback_mutation_invalidated_destructor, task, {}, 0x40b596};
    task->destruction_previous_ = retain(task->previous_);
    task->destruction_next_ = retain(task->next_);
    task->exec = task->display = task->destructor = nullptr;
    task->destruction_stage_ = Task::DestructionStage::children;
    return finish_destruction(task);
}

TaskResult TaskScheduler::resume_destruction(const TaskHandle& task) {
    const auto accepted = validate(task);
    if (!accepted) return accepted;
    if (task->destruction_stage_ == Task::DestructionStage::live)
        return {TaskFailure::inactive_task, task, {}, 0x40b608};
    return finish_destruction(task);
}

TaskResult TaskScheduler::finish_destruction(const TaskHandle& task) {
    using Stage = Task::DestructionStage;
    if (task->destruction_stage_ == Stage::children) {
        const auto children = free_children(task);
        if (!children) return children;
        const auto previous = task->destruction_previous_;
        const auto next = task->destruction_next_;
        if ((previous && previous->retired_) || (next && next->retired_))
            return {TaskFailure::callback_mutation_invalidated_destructor, task, {}, 0x40b5b4};
        if (previous) previous->next_ = next.get();
        else if (task->parent_) task->parent_->first_child_ = next.get();
        else {
            for (auto& head : heads_) if (head == task.get()) { head = next.get(); break; }
        }
        if (next) next->previous_ = previous.get();
        task->linked_ = false;
        task->destruction_stage_ = Stage::collision;
    }
    if (task->destruction_stage_ == Stage::collision) {
        if (task->primary_work) {
            const auto collision = arena_ ? arena_->reference(task->primary_work, 0x38)
                                         : TaskWorkResult{TaskWorkFailure::foreign_work,{}};
            if (!collision) return source_work_failure(task, collision, 0x40b617);
            if (collision.work) {
                std::uint8_t flags = 0;
                auto result = arena_->read(collision.work, 5, &flags, 1);
                if (!result) return source_work_failure(task, result, 0x41c4f1);
                if (flags & 0x80) {
                    const auto nested = arena_->reference(collision.work, 12);
                    if (!nested) return source_work_failure(task, nested, 0x41c4f7);
                    result = arena_->release(nested.work);
                    if (!result) return source_work_failure(task, result, 0x41c4ff);
                    result = arena_->set_reference(collision.work, 12, {});
                    if (!result) return source_work_failure(task, result, 0x41c507);
                }
                result = arena_->release(collision.work);
                if (!result) return source_work_failure(task, result, 0x41c50f);
                result = arena_->set_reference(task->primary_work, 0x38, {});
                if (!result) return source_work_failure(task, result, 0x41c517);
            }
        }
        task->destruction_stage_ = Stage::object;
    }
    if (task->destruction_stage_ == Stage::object) {
        if (task->primary_work) {
            const auto object = arena_->reference(task->primary_work, 0x3c);
            if (!object) return source_work_failure(task, object, 0x40b628);
            if (object.work) {
                task->cleanup_object_ = object.work;
                const auto cursor = arena_->reference(object.work, 0x40);
                if (!cursor) return source_work_failure(task, cursor, 0x417ea9);
                task->cleanup_cursor_ = cursor.work;
                task->destruction_stage_ = Stage::object_chain40;
            }
        }
        if (task->destruction_stage_ == Stage::object)
            task->destruction_stage_ = Stage::primary;
    }
    if (task->destruction_stage_ == Stage::object_chain40) {
        while (task->cleanup_cursor_) {
            const auto next = arena_->reference(task->cleanup_cursor_, 0x10);
            if (!next) return source_work_failure(task, next, 0x417eb3);
            const auto released = arena_->release(task->cleanup_cursor_);
            if (!released) return source_work_failure(task, released, 0x417eb7);
            task->cleanup_cursor_ = next.work; // source snapshot before free
        }
        const auto cleared = arena_->set_reference(task->cleanup_object_, 0x40, {});
        if (!cleared) return source_work_failure(task, cleared, 0x417ecb);
        task->destruction_stage_ = Stage::object_block44;
    }
    if (task->destruction_stage_ == Stage::object_block44) {
        const auto extra = arena_->reference(task->cleanup_object_, 0x44);
        if (!extra) return source_work_failure(task, extra, 0x417ec6);
        const auto released = arena_->release(extra.work);
        if (!released) return source_work_failure(task, released, 0x417ed1);
        if (extra.work) {
            const auto cleared = arena_->set_reference(task->cleanup_object_, 0x44, {});
            if (!cleared) return source_work_failure(task, cleared, 0x417ed9);
        }
        task->destruction_stage_ = Stage::object_clear3c;
    }
    if (task->destruction_stage_ == Stage::object_clear3c) {
        const auto cleared = arena_->fill(task->cleanup_object_, 0x3c, 0, 4);
        if (!cleared) return source_work_failure(task, cleared, 0x417edc);
        const auto cursor = arena_->reference(task->cleanup_object_, 0x4c);
        if (!cursor) return source_work_failure(task, cursor, 0x418338);
        task->cleanup_cursor_ = cursor.work;
        task->destruction_stage_ = Stage::object_chain4c;
    }
    if (task->destruction_stage_ == Stage::object_chain4c) {
        while (task->cleanup_cursor_) {
            const auto next = arena_->reference(task->cleanup_cursor_, 0xc);
            if (!next) return source_work_failure(task, next, 0x418340);
            const auto released = arena_->release(task->cleanup_cursor_);
            if (!released) return source_work_failure(task, released, 0x418344);
            task->cleanup_cursor_ = next.work;
        }
        const auto cleared = arena_->set_reference(task->cleanup_object_, 0x4c, {});
        if (!cleared) return source_work_failure(task, cleared, 0x418356);
        task->destruction_stage_ = Stage::object_clear48;
    }
    if (task->destruction_stage_ == Stage::object_clear48) {
        const auto cleared = arena_->fill(task->cleanup_object_, 0x48, 0, 4);
        if (!cleared) return source_work_failure(task, cleared, 0x418360);
        task->destruction_stage_ = Stage::object_release;
    }
    if (task->destruction_stage_ == Stage::object_release) {
        const auto released = arena_->release(task->cleanup_object_);
        if (!released) return source_work_failure(task, released, 0x4187bc);
        const auto cleared = arena_->set_reference(task->primary_work, 0x3c, {});
        if (!cleared) return source_work_failure(task, cleared, 0x4187c4);
        task->cleanup_object_.reset();
        task->destruction_stage_ = Stage::primary;
    }
    struct Field { Stage stage; Stage next; TaskWorkHandle Task::* member; std::uint32_t boundary; };
    const Field fields[] = {{Stage::primary,Stage::force,&Task::primary_work,0x40b634},
                           {Stage::force,Stage::motion,&Task::force_work,0x40b647},
                           {Stage::motion,Stage::auxiliary,&Task::motion_work,0x40b65a},
                           {Stage::auxiliary,Stage::control,&Task::auxiliary_work,0x40b66d}};
    for (const auto& field : fields) {
        if (task->destruction_stage_ != field.stage) continue;
        auto& work = task.get()->*(field.member);
        if (work) {
            const auto released = arena_ ? arena_->release(work)
                                        : TaskWorkResult{TaskWorkFailure::foreign_work,work};
            if (!released) return source_work_failure(task, released, field.boundary);
            work.reset();
        }
        task->destruction_stage_ = field.next;
    }
    if (task->object_control) {
        task->object_control->task_at_4.reset();
        task->object_control->flags_at_2 &= 0xfe;
        task->object_control.reset();
    }
    task->destruction_stage_ = Stage::recycle;
    task->retired_ = true; task->next_ = task->previous_ = task->parent_ = task->first_child_ = nullptr;
    task->destruction_previous_.reset(); task->destruction_next_.reset();
    if (task->source_slot_) {
        source_slots_[*task->source_slot_].reset();
        free_slots_.push_back(*task->source_slot_);
    }
    if (!walk_depth_) reclaim_retired();
    return {};
}

TaskResult TaskScheduler::retire(const TaskHandle& task) {
    const auto accepted = validate(task);
    if (!accepted) {
        return accepted;
    }
    std::vector<Task*> pending{task.get()};
    while (!pending.empty()) {
        auto* current = pending.back(); pending.pop_back();
        if (current->source_slot_) return {TaskFailure::source_task_requires_destructor, retain(current)};
        for (auto* child = current->first_child_; child; child = child->next_)
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

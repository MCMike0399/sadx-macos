#include "engine/task.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sadx {
namespace {
struct Marker { std::uint32_t value; };
struct Fixture : TaskCallbackContext {
    std::vector<std::array<std::uint32_t, 4>> trace;
    std::array<std::uint32_t, 64> actions{}, targets{};
    std::array<bool, 64> consumed{};
    unsigned callbacks = 0, fail_at = 0;
};
}
class TaskSourceProbe {
    static void display_one(TaskScheduler &s, const TaskHandle &t) { callback(s, t, 1); }
    static void display_two(TaskScheduler &s, const TaskHandle &t) { callback(s, t, 2); }
    static void exec_marker(TaskScheduler &s, const TaskHandle &t) { callback(s, t, 3); }
    static std::uint32_t callback_id(TaskCallback callback) {
        if (callback == display_one) return 1;
        if (callback == display_two) return 2;
        if (callback == exec_marker) return 3;
        if (callback == TaskScheduler::source_no_op) return 4;
        return 0;
    }
    static std::uint32_t slot(const TaskHandle &task) {
        return task && task->source_slot_ ? *task->source_slot_ + 1U : 0;
    }
    static std::uint32_t marker(const std::shared_ptr<void> &owner) {
        return owner ? std::static_pointer_cast<Marker>(owner)->value : 0;
    }
    static std::shared_ptr<Fixture> fixture(TaskScheduler &s) {
        return std::static_pointer_cast<Fixture>(s.callback_context(display_one));
    }
    static void accept(const TaskResult &result) {
        if (!result) TaskScheduler::raise_callback_failure(result);
    }
    static void callback(TaskScheduler &s, const TaskHandle &task, std::uint32_t code) {
        const auto f = fixture(s);
        f->trace.push_back({slot(task), code, callback_id(s.context_.current_function), marker(s.context_.field30)});
        if (++f->callbacks == f->fail_at)
            TaskScheduler::raise_callback_failure({TaskFailure::callback_failed, task});
        const auto index = *task->source_slot_;
        if (f->consumed[index]) return;
        f->consumed[index] = true;
        const auto target_index = f->targets[index];
        auto target = target_index ? s.source_task(static_cast<std::uint16_t>(target_index - 1)) : TaskHandle{};
        switch (f->actions[index]) {
        case 1: if (target) target->exec = nullptr; break;
        case 2: if (target) target->exec = exec_marker; break;
        case 3: {
            const auto made = s.create_child(0, exec_marker, task);
            accept(made);
            made.task->destructor = display_two;
            break;
        }
        case 4: {
            const auto level = target_index % 8;
            const auto made = s.create_element(0, level == 7 ? 8 : static_cast<int>(level), exec_marker);
            accept(made);
            made.task->destructor = display_two;
            break;
        }
        case 5: accept(s.free_children(task)); break;
        case 6: accept(s.destroy_task(task)); break;
        case 7: {
            accept(s.destroy_task(task));
            target = target_index ? s.source_task(static_cast<std::uint16_t>(target_index - 1)) : TaskHandle{};
            if (target) {
                const auto replacement = s.create_child(0, exec_marker, target);
                accept(replacement);
                replacement.task->display = display_two;
                const auto added = s.create_child(0, exec_marker, replacement.task);
                accept(added);
                added.task->display = display_two;
            }
            break;
        }
        case 8: task->destructor = display_two; break;
        case 9: TaskScheduler::raise_callback_failure({TaskFailure::callback_failed, task});
        case 10: if (target) accept(s.destroy_task(target)); break;
        case 11: {
            if (target) accept(s.destroy_task(target));
            const auto made = s.create_element(0, static_cast<int>(target_index % 7), exec_marker);
            accept(made);
            made.task->destructor = display_two;
            break;
        }
        }
    }
public:
    static int run(const std::uint32_t *in, std::size_t size, std::uint32_t *out, std::size_t capacity) {
        if (size < 8 || size != 8 + in[6] * 6 || in[6] > 24 || capacity < 2048) return 0;
        TaskScheduler scheduler;
        auto arena = std::make_shared<TaskWorkArena>();
        auto default_owner = std::make_shared<Marker>(Marker{8000});
        if (!scheduler.initialize_source_pool(arena, default_owner)) return 0;
        auto f = std::make_shared<Fixture>();
        f->fail_at = in[3];
        for (auto fn : {display_one, display_two, exec_marker})
            if (!scheduler.register_callback_context(fn, f)) return 0;
        std::vector<TaskHandle> tasks;
        for (unsigned i = 0; i < in[6]; ++i) {
            const auto *row = in + 8 + i * 6;
            auto made = row[1] ? scheduler.create_child(0, exec_marker, tasks.at(row[1] - 1)) :
                                scheduler.create_element(0, static_cast<int>(row[0]), exec_marker);
            if (!made) return 0;
            made.task->exec = row[2] ? exec_marker : nullptr;
            made.task->destructor = display_one;
            made.task->field30 = row[5] ? std::make_shared<Marker>(Marker{row[5]}) : nullptr;
            tasks.push_back(made.task);
            f->actions[i] = row[3];
            f->targets[i] = row[4];
        }
        scheduler.context_.current_function = in[4] ? exec_marker : nullptr;
        scheduler.context_.field30 = in[5] ? std::make_shared<Marker>(Marker{9000}) : nullptr;
        TaskResult result;
        result = scheduler.delete_all_tasks();
        std::fill_n(out, capacity, 0);
        out[0] = result.failure == TaskFailure::none ? 0 :
                 (result.failure == TaskFailure::callback_failed ? 1 :
                  ((result.failure == TaskFailure::deleted_snapshot_next || result.failure == TaskFailure::detached_snapshot_next) ? 2 : 100 + static_cast<unsigned>(result.failure)));
        out[1] = callback_id(scheduler.context_.current_function);
        out[2] = marker(scheduler.context_.field30);
        out[3] = static_cast<std::uint32_t>(f->trace.size());
        out[4] = static_cast<std::uint32_t>(scheduler.free_slots_.size());
        out[5] = scheduler.free_slots_.empty() ? 0 : scheduler.free_slots_.back() + 1U;
        for (unsigned list = 0; list < 8; ++list) out[8 + list] = slot(scheduler.retain(scheduler.heads_[list]));
        for (unsigned i = 0; i < 64; ++i) {
            auto *node = out + 16 + i * 9;
            const auto task = scheduler.source_task(static_cast<std::uint16_t>(i));
            if (!task) continue;
            node[0] = 1;
            node[1] = slot(scheduler.retain(task->next_));
            node[2] = slot(scheduler.retain(task->previous_));
            node[3] = slot(scheduler.retain(task->parent_));
            node[4] = slot(scheduler.retain(task->first_child_));
            node[5] = callback_id(task->exec);
            node[6] = callback_id(task->display);
            node[7] = callback_id(task->destructor);
            node[8] = marker(task->field30);
        }
        auto cursor = out + 640;
        for (const auto &entry : f->trace) cursor = std::copy(entry.begin(), entry.end(), cursor);
        return 1;
    }
};
} // namespace sadx
extern "C" int sadx_native_task_delete(const std::uint32_t *in, std::size_t size,
                                         std::uint32_t *out, std::size_t capacity) {
    return sadx::TaskSourceProbe::run(in, size, out, capacity);
}

#include "engine/task.hpp"
#include <cassert>
#include <functional>
#include <stdexcept>
#include <vector>

namespace {
using namespace sadx;
struct DisplayContext : TaskCallbackContext {
    std::vector<int> visits;
    std::function<void(TaskScheduler &, const TaskHandle &)> action;
    TaskCallback expected_exec = nullptr;
    std::shared_ptr<void> expected_work;
};
void display(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto context = std::static_pointer_cast<DisplayContext>(scheduler.callback_context(display));
    assert(scheduler.context().current_function == context->expected_exec);
    assert(scheduler.context().field30 == context->expected_work);
    context->visits.push_back(*std::static_pointer_cast<int>(task->field30));
    if (context->action)
        context->action(scheduler, task);
}
void execute_display(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto context = std::static_pointer_cast<DisplayContext>(scheduler.callback_context(display));
    context->expected_exec = execute_display;
    context->expected_work = task->field30;
    assert(scheduler.display_index(0));
    assert(scheduler.context().current_function == execute_display);
    assert(scheduler.context().field30 == task->field30);
}
void forbidden_exec(TaskScheduler &, const TaskHandle &) { assert(false); }
TaskHandle append(TaskScheduler &scheduler, int list, int number) {
    auto result = scheduler.append_task(list, forbidden_exec, std::make_shared<int>(number));
    assert(result);
    result.task->display = display;
    return result.task;
}
TaskHandle child(TaskScheduler &scheduler, const TaskHandle &parent, int number) {
    auto result = scheduler.append_child(parent, forbidden_exec, std::make_shared<int>(number));
    assert(result);
    result.task->display = display;
    return result.task;
}
void verify_auxiliary_lifetime() {
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}));
    auto task = scheduler.create_element(15, 0, nullptr).task;
    assert(task);
    const auto primary = task->primary_work, force = task->force_work;
    const auto motion = task->motion_work, auxiliary = task->auxiliary_work;
    auto control = std::make_shared<Task::ObjectControl>();
    control->flags_at_2 = 1;
    task->object_control = control;
    unsigned released = 0;
    task->auxiliary_native_work = std::shared_ptr<int>(new int(1), [&](int *work) {
        assert(primary->released() && force->released() && motion->released() && auxiliary->released());
        assert(!task->primary_work && !task->force_work && !task->motion_work && !task->auxiliary_work);
        assert(task->object_control == control && control->flags_at_2 == 1 && !task->retired());
        ++released;
        delete work;
    });
    assert(scheduler.destroy_task(task));
    assert(released == 1 && task->retired() && !task->auxiliary_native_work);

    auto second = scheduler.create_element(8, 0, nullptr).task;
    auto native_work = std::make_shared<int>(7);
    std::weak_ptr<int> weak = native_work;
    second->auxiliary_native_work = native_work;
    auto graph = scheduler.source_graph();
    second->auxiliary_native_work.reset();
    native_work.reset();
    assert(!weak.expired());
    assert(scheduler.restore_source_graph(graph));
    second = scheduler.source_task(*second->source_slot());
    assert(second->auxiliary_native_work == graph.nodes.front().auxiliary_native_work);
    graph = {};
    auto retained_graph = scheduler.source_graph();
    assert(scheduler.destroy_task(second));
    assert(!second->auxiliary_native_work && !weak.expired());
    retained_graph = {};
    assert(weak.expired());

    auto interrupted = scheduler.create_element(8, 0, nullptr).task;
    auto retained_native = std::make_shared<int>(9);
    interrupted->auxiliary_native_work = retained_native;
    retained_native.reset();
    assert(arena->release(interrupted->auxiliary_work));
    const auto stopped = scheduler.destroy_task(interrupted);
    assert(stopped.failure == TaskFailure::work_failure);
    assert(interrupted->auxiliary_native_work && !interrupted->retired());
}
} // namespace

int main() {
    using namespace sadx;
    TaskScheduler scheduler;
    auto context = std::make_shared<DisplayContext>();
    assert(scheduler.register_callback_context(display, context));
    auto first = append(scheduler, 0, 1);
    auto sibling = append(scheduler, 0, 2);
    auto quiet = child(scheduler, first, 3);
    quiet->display = nullptr;
    child(scheduler, quiet, 4);
    append(scheduler, 8, 7);
    assert(scheduler.display_game_tasks([&] { context->visits.push_back(0); }));
    assert((context->visits == std::vector<int>{0, 7, 1, 4, 2}));
    context->visits.clear();
    context->action = [&](TaskScheduler &active, const TaskHandle &task) {
        if (task == first) {
            child(active, first, 5);
            sibling->display = nullptr;
            append(active, 0, 6);
        }
    };
    assert(scheduler.display_index(0));
    // New child is loaded after the callback; the original saved sibling is
    // visited even after its display changes, and its later next includes 6.
    assert((context->visits == std::vector<int>{1, 4, 5, 6}));
    context->action = {};
    context->visits.clear();
    auto nested = scheduler.append_task(1, execute_display, std::make_shared<int>(11));
    assert(nested && scheduler.run_index(1));
    assert(!scheduler.context().field30 && !scheduler.context().current_function);
    context->expected_exec = nullptr;
    context->expected_work.reset();
    context->action = [](TaskScheduler &active, const TaskHandle &task) {
        active.raise_callback_failure({TaskFailure::callback_failed, task});
    };
    const auto failed = scheduler.display_index(0);
    assert(failed.failure == TaskFailure::callback_failed);
    assert(!scheduler.context().field30 && !scheduler.context().current_function);
    assert(scheduler.display_index(-1).failure == TaskFailure::invalid_list_index);
    assert(scheduler.display_children({}).failure == TaskFailure::foreign_task);
    verify_auxiliary_lifetime();
}

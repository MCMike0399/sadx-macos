#include "game/task.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

using sadx::TaskCallback;
using sadx::TaskFailure;
using sadx::TaskHandle;
using sadx::TaskScheduler;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_ok(const sadx::TaskResult& result) {
    require(static_cast<bool>(result), "unexpected task failure");
}

struct Payload {
    int number = 0;
    std::shared_ptr<std::vector<int>> visits;
    TaskCallback expected_function = nullptr;
    TaskHandle target;
};

std::shared_ptr<Payload> payload(int number,
                                 const std::shared_ptr<std::vector<int>>& visits,
                                 TaskCallback expected_function) {
    auto data = std::make_shared<Payload>();
    data->number = number;
    data->visits = visits;
    data->expected_function = expected_function;
    return data;
}

std::shared_ptr<Payload> check_context(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = std::static_pointer_cast<Payload>(task->field30);
    require(scheduler.context().field30 == task->field30, "wrong field30 callback context");
    require(scheduler.context().current_function == data->expected_function,
            "wrong current_function callback context");
    return data;
}

void record(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
}

void retire_self(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.retire(task));
    require(scheduler.context().field30 == data, "self retirement lost active native work");
}

void schedule_self_free(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.free_task(task));
    require(task->exec != &schedule_self_free && task->exec != nullptr,
            "FreeTask did not schedule a future callback");
}

void replace_next(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    data->target->exec = &record;
}

void retire_next(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.retire(data->target));
}

void append_during_callback(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.append_task(0, &record, payload(9, data->visits, &record)));
    task->exec = &record;
    data->expected_function = &record;
}

void walk_children(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.run_children(task));
    require(!scheduler.context().field30, "child walker wrongly restored parent field30");
    require(scheduler.context().current_function == &walk_children,
            "child walker modified current_function");
}

void child_runs_top_level(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.run_index(1));
    require(!scheduler.context().field30 && !scheduler.context().current_function,
            "nested top-level dispatch wrongly restored caller globals");
}

void parent_with_nested_index(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    data->visits->push_back(data->number);
    require_ok(scheduler.run_children(task));
    require(!scheduler.context().current_function,
            "child walker wrongly restored function cleared by nested index");
    require(!scheduler.context().field30, "nested walker wrongly restored parent resource");
}

void empty_nested_walks(TaskScheduler& scheduler, const TaskHandle& task) {
    const auto data = check_context(scheduler, task);
    require_ok(scheduler.run_index(6));
    require(scheduler.context().current_function == &empty_nested_walks &&
            scheduler.context().field30 == data,
            "empty top-level walk must leave caller globals unchanged");
    require_ok(scheduler.run_children(task));
    require(scheduler.context().current_function == &empty_nested_walks &&
            scheduler.context().field30 == data,
            "empty child walk must leave caller globals unchanged");
}

void callback_throws(TaskScheduler& scheduler, const TaskHandle& task) {
    (void)check_context(scheduler, task);
    require_ok(scheduler.retire(task));
    throw std::runtime_error("native callback failure");
}

TaskHandle append(TaskScheduler& scheduler, int index, TaskCallback callback,
                  const std::shared_ptr<Payload>& data) {
    auto result = scheduler.append_task(index, callback, data);
    require_ok(result);
    return result.task;
}

TaskHandle child(TaskScheduler& scheduler, const TaskHandle& parent, TaskCallback callback,
                 const std::shared_ptr<Payload>& data) {
    auto result = scheduler.append_child(parent, callback, data);
    require_ok(result);
    return result.task;
}

void test_lists_and_nulls() {
    TaskScheduler scheduler;
    auto visits = std::make_shared<std::vector<int>>();
    for (int i = 0; i < 8; ++i) {
        append(scheduler, i, &record, payload(i, visits, &record));
    }
    for (int i = 0; i < 8; ++i) {
        require_ok(scheduler.run_index(i));
    }
    require(*visits == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}), "eight independent lists");
    require_ok(scheduler.run_index(8));
    require(visits->back() == 7 && visits->size() == 9, "index eight aliases seven");
    for (int index : {-1, 9, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        require(scheduler.run_index(index).failure == TaskFailure::invalid_list_index,
                "invalid list run must fail");
        require(scheduler.append_task(index, nullptr).failure == TaskFailure::invalid_list_index,
                "invalid list append must fail");
    }
    append(scheduler, 0, nullptr, payload(90, visits, nullptr));
    append(scheduler, 0, &record, payload(91, visits, &record));
    visits->clear();
    require_ok(scheduler.run_index(0));
    require(*visits == std::vector<int>({0, 91}), "top-level null must skip and continue");
    require(!scheduler.context().field30 && !scheduler.context().current_function,
            "top-level context must clear");
}

void test_self_deletion_and_deferred_free() {
    TaskScheduler scheduler;
    auto visits = std::make_shared<std::vector<int>>();
    auto self = append(scheduler, 0, &retire_self, payload(1, visits, &retire_self));
    auto next = append(scheduler, 0, &record, payload(2, visits, &record));
    require_ok(scheduler.run_index(0));
    require(*visits == std::vector<int>({1, 2}), "self deletion must keep captured next");
    require(scheduler.retire(self).failure == TaskFailure::retired_task, "double retirement rejected");
    visits->clear();
    require_ok(scheduler.run_index(0));
    require(*visits == std::vector<int>({2}), "retired task must unlink from next pass");
    require_ok(scheduler.retire(next));
    auto scheduled = append(scheduler, 0, &schedule_self_free,
                             payload(3, visits, &schedule_self_free));
    visits->clear();
    require_ok(scheduler.run_index(0));
    require(*visits == std::vector<int>({3}), "marking free must finish existing callback");
    require_ok(scheduler.free_task(scheduled)); // repeated scheduling remains accepted
    require_ok(scheduler.run_index(0));
    require(visits->size() == 1, "next pass must retire rather than rerun game callback");
    require(scheduler.free_task(scheduled).failure == TaskFailure::retired_task,
            "inactive FreeTask membership rejected");
}

void test_callback_mutations() {
    TaskScheduler scheduler;
    auto visits = std::make_shared<std::vector<int>>();
    auto first_data = payload(1, visits, &replace_next);
    auto first = append(scheduler, 0, &replace_next, first_data);
    auto second = append(scheduler, 0, nullptr, payload(2, visits, &record));
    first_data->target = second;
    require_ok(scheduler.run_index(0));
    require(*visits == std::vector<int>({1, 2}), "saved next must use its updated callback");
    first->exec = &retire_next;
    first_data->expected_function = &retire_next;
    visits->clear();
    const auto deleted_next = scheduler.run_index(0);
    require(deleted_next.failure == TaskFailure::deleted_snapshot_next && deleted_next.task == second,
            "deleting captured next must fail explicitly without dangling access");
    require(*visits == std::vector<int>({1}), "retired next callback must not run");

    TaskScheduler appender;
    visits->clear();
    append(appender, 0, &append_during_callback, payload(3, visits, &append_during_callback));
    require_ok(appender.run_index(0));
    require(*visits == std::vector<int>({3}), "tail append must not alter already captured null next");
    require_ok(appender.run_index(0));
    require(*visits == std::vector<int>({3, 3, 9}), "appended task must run next pass");

    TaskScheduler before_tail;
    visits->clear();
    append(before_tail, 0, &append_during_callback, payload(4, visits, &append_during_callback));
    append(before_tail, 0, &record, payload(5, visits, &record));
    require_ok(before_tail.run_index(0));
    require(*visits == std::vector<int>({4, 5, 9}),
            "later sibling must see append when it snapshots next");
}

void test_children_and_nested_context() {
    TaskScheduler scheduler;
    auto visits = std::make_shared<std::vector<int>>();
    auto parent = append(scheduler, 0, &walk_children, payload(1, visits, &walk_children));
    auto first = child(scheduler, parent, &retire_self, payload(2, visits, &walk_children));
    auto second = child(scheduler, parent, &record, payload(3, visits, &walk_children));
    child(scheduler, second, &record, payload(4, visits, &record));
    require_ok(scheduler.run_index(0));
    require(*visits == std::vector<int>({1, 2, 3}), "children must snapshot next and not auto recurse");
    require(scheduler.free_task(first).failure == TaskFailure::retired_task, "retired child membership");
    visits->clear();
    std::static_pointer_cast<Payload>(second->field30)->expected_function = nullptr;
    require_ok(scheduler.run_children(parent));
    require(*visits == std::vector<int>({3}), "child walker outside caller must leave function null");

    TaskScheduler nested;
    visits->clear();
    auto nested_parent = append(nested, 0, &parent_with_nested_index,
                                payload(5, visits, &parent_with_nested_index));
    child(nested, nested_parent, &child_runs_top_level,
          payload(6, visits, &parent_with_nested_index));
    child(nested, nested_parent, &record, payload(8, visits, nullptr));
    append(nested, 1, &record, payload(7, visits, &record));
    require_ok(nested.run_index(0));
    require(*visits == std::vector<int>({5, 6, 7, 8}), "nested traversal visit order");
    require(!nested.context().field30 && !nested.context().current_function,
            "nested execution must not restore parent context");

    TaskScheduler empty_nested;
    append(empty_nested, 0, &empty_nested_walks, payload(9, visits, &empty_nested_walks));
    require_ok(empty_nested.run_index(0));

    TaskScheduler null_child;
    auto null_parent = append(null_child, 0, nullptr, {});
    auto bad = child(null_child, null_parent, nullptr, {});
    const auto failed = null_child.run_children(null_parent);
    require(failed.failure == TaskFailure::null_child_callback && failed.task == bad,
            "null child must model the original invalid indirect call");
}

void test_native_ownership_and_exception_cleanup() {
    TaskScheduler scheduler;
    TaskScheduler other;
    auto foreign = other.append_task(0, nullptr).task;
    require(scheduler.free_task(foreign).failure == TaskFailure::foreign_task, "foreign FreeTask");
    require(scheduler.run_children(foreign).failure == TaskFailure::foreign_task, "foreign parent");
    require(scheduler.retire({}).failure == TaskFailure::foreign_task, "null handle rejected");
    auto visits = std::make_shared<std::vector<int>>();
    auto task = append(scheduler, 0, &callback_throws, payload(1, visits, &callback_throws));
    bool threw = false;
    try {
        (void)scheduler.run_index(0);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "native callback exception must propagate");
    require(!scheduler.context().field30 && !scheduler.context().current_function,
            "native callback exception must release context");
    require_ok(scheduler.run_index(0));
    require(scheduler.retire(task).failure == TaskFailure::retired_task,
            "exception unwinding must finish deferred scheduler reclamation");

    auto resource = std::make_shared<int>(9);
    std::weak_ptr<int> resource_lifetime = resource;
    auto owning_task = scheduler.append_task(0, nullptr, resource).task;
    resource.reset();
    require_ok(scheduler.retire(owning_task));
    require(!resource_lifetime.expired(), "live task handle must retain native resource");
    owning_task.reset();
    require(resource_lifetime.expired(), "last retired handle must release native resource");

    auto root = scheduler.append_task(0, nullptr).task;
    auto descendant = scheduler.append_child(root, nullptr).task;
    require_ok(scheduler.retire(root));
    require(scheduler.retire(descendant).failure == TaskFailure::retired_task,
            "native subtree retirement must unlink all children");
}

void test_game_task_order() {
    TaskScheduler scheduler;
    auto visits=std::make_shared<std::vector<int>>();
    for(int index=0;index<8;++index)
        require_ok(scheduler.append_task(index,&record,payload(index,visits,&record)));
    require_ok(scheduler.run_game_tasks());
    require(*visits==std::vector<int>({7,0,1,2,3,4,5,6}),
            "game task dispatch must run source alias8 before0..6");
    TaskScheduler mutation;
    visits->clear();
    require_ok(mutation.append_task(7,&append_during_callback,
                                    payload(7,visits,&append_during_callback)));
    require_ok(mutation.run_game_tasks());
    require(*visits==std::vector<int>({7,9}),
            "a task added by list7 must run in the later list0 of the same frame");
    TaskScheduler failure;
    visits->clear();
    auto first=failure.append_task(7,&retire_next,payload(1,visits,&retire_next)).task;
    auto removed=failure.append_task(7,&record,payload(2,visits,&record)).task;
    std::static_pointer_cast<Payload>(first->field30)->target=removed;
    require_ok(failure.append_task(0,&record,payload(0,visits,&record)));
    const auto stopped=failure.run_game_tasks();
    require(stopped.failure==TaskFailure::deleted_snapshot_next&&
            *visits==std::vector<int>({1}),
            "list7 walker failure must retain its reason and stop later lists");
}

} // namespace

int main() {
    try {
        test_lists_and_nulls();
        test_self_deletion_and_deferred_free();
        test_callback_mutations();
        test_children_and_nested_context();
        test_native_ownership_and_exception_cleanup();
        test_game_task_order();
        std::cout << "Task dispatch: eight lists, alias, mutation, deferred free, child traversal, "
                     "nested context and native ownership passed.\n"
                     "Task lifecycle has separate source tests; full game integration remains unfinished.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "engine/task.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

using namespace sadx;

int main() {
    for (const auto flags : {0U, 15U, 0xffffff00U, 0xffffffffU}) {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        auto thread = std::make_shared<unsigned>(37);
        assert(scheduler.initialize_source_pool(arena, thread));
        auto created = scheduler.create_element(flags, 0, nullptr);
        assert(created && created.task->source_slot() == 0);
        assert(created.task->cooperative_thread == thread);
        const bool all_work = (flags & 15U) != 0;
        assert(bool(created.task->primary_work) == all_work);
        assert(bool(created.task->force_work) == all_work);
        assert(bool(created.task->motion_work) == all_work);
        assert(bool(created.task->auxiliary_work) == all_work);
        assert(scheduler.destroy_task(created.task));
        assert(created.task->retired() && scheduler.source_free_slots() == 4096);
        assert(scheduler.allocate_task(std::numeric_limits<int>::min(), nullptr).failure ==
               TaskFailure::InvalidListIndex);
        assert(scheduler.allocate_task(std::numeric_limits<int>::max(), nullptr).failure ==
               TaskFailure::InvalidListIndex);
        assert(scheduler.source_free_slots() == 4096);
        auto detached = scheduler.allocate_task(7, nullptr).task;
        auto last_list = scheduler.allocate_task(8, nullptr).task;
        assert(detached && last_list && !scheduler.active_member(detached));
        assert(scheduler.list_head(7) == last_list && scheduler.list_head(8) == last_list);
        assert(scheduler.destroy_task(detached).failure == TaskFailure::InactiveTask);
        assert(scheduler.destroy_task(last_list));
        std::cout << flags << ' ' << all_work << ' ' << *detached->source_slot() << ' '
                  << *last_list->source_slot() << ' ' << scheduler.source_free_slots() << '\n';
    }
    TaskScheduler first, second;
    auto first_arena = std::make_shared<TaskWorkArena>();
    auto second_arena = std::make_shared<TaskWorkArena>();
    assert(first.initialize_source_pool(first_arena, std::shared_ptr<void>{}));
    assert(second.initialize_source_pool(second_arena, std::shared_ptr<void>{}));
    const auto left = first.create_element(2, 0, nullptr).task;
    const auto right = second.create_element(2, 0, nullptr).task;
    assert(!left->cooperative_thread && !right->cooperative_thread);
    assert(first.destroy_task(right).failure == TaskFailure::ForeignTask);
    assert(first_arena->release(right->primary_work).failure == TaskWorkFailure::ForeignWork);
    assert(first.destroy_task(left) && second.destroy_task(right));
    std::cout << "Native task boundary: full flag/list widths and isolated plain owners passed.\n";
}

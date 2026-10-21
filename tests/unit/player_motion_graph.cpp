#include "player/player_initialization.hpp"
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace init = sadx::player_initialization;
int main() {
    try {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
            throw std::runtime_error("source pool unavailable");
        auto created = scheduler.create_element(7, 3, nullptr);
        if (!created)
            throw std::runtime_error("actual actor allocation failed");
        const auto slot = *created.task->source_slot();
        init::establish_zeroed_primary(scheduler, created.task);
        init::establish_zeroed_motion(scheduler, created.task);
        const auto motion = init::bind_existing_motion(created.task);
        motion->target_yaw = 16384;
        motion->motion.speed = {1, 2, 3};
        const auto graph = scheduler.source_graph();
        if (!scheduler.restore_source_graph(graph))
            throw std::runtime_error("native source graph rejected its own snapshot");
        const auto restored = scheduler.source_task(slot);
        if (restored->motion_state != motion)
            throw std::runtime_error("graph roundtrip silently lost canonical motion owner");
        if (init::bind_existing_motion(restored)->target_yaw != 16384 ||
            restored->motion_state->motion.speed.y != 2)
            throw std::runtime_error("graph roundtrip lost actual preview/world motion values");
        auto omitted = scheduler.source_graph();
        for (auto &node : omitted.nodes)
            if (node.slot == slot) node.motion_state.reset();
        const auto omission = scheduler.restore_source_graph(omitted);
        if (omission.failure != TaskFailure::invalid_source_graph ||
            scheduler.source_task(slot) != restored || restored->motion_state != motion)
            throw std::runtime_error("raw-only import silently discarded canonical motion or mutated graph");
        auto replaced = scheduler.source_graph();
        for (auto &node : replaced.nodes)
            if (node.slot == slot) node.motion_state = std::make_shared<PlayerMotionState>();
        if (scheduler.restore_source_graph(replaced).failure != TaskFailure::invalid_source_graph ||
            scheduler.source_task(slot) != restored)
            throw std::runtime_error("graph accepted a second motion owner for the same allocation");
        std::cout << "Native graph roundtrip retains canonical motion identity and values\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

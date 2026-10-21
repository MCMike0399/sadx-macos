#include "player/player_initialization.hpp"
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace init = sadx::player_initialization;
namespace {
template <class T> void require(const T &value, const char *message) {
    if (!static_cast<bool>(value))
        throw std::runtime_error(message);
}
void primary_owners() {
    auto arena = std::make_shared<TaskWorkArena>();
    TaskScheduler scheduler;
    require(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}), "actual task pool");
    for (auto flags : {6U, 7U}) {
        auto result = scheduler.create_element(flags, 3, nullptr);
        require(result, "actual camera/player CreateElement flags");
        auto task = result.task;
        init::establish_zeroed_primary(scheduler, task);
        auto owner = task->primary_state;
        auto view = init::bind_primary_view(task);
        require(view.mode == 0 && view.id == 0 && view.status == 0 && view.position.x == 0 &&
                    view.scale.z == 0,
                "source calloc primary values");
        view.mode = 4;
        view.id = 17;
        view.character = 3;
        view.player_index = 5;
        view.position = {1, 2, 3};
        view.rotation = {7, 11, 13};
        view.status = 32769;
        init::establish_zeroed_primary(scheduler, task);
        auto later = init::bind_primary_view(task);
        require(task->primary_state == owner && &view.position == &later.position &&
                    &view.rotation == &later.rotation,
                "one canonical owner, no copied camera pose");
        require(owner->position->y == 2 && owner->orientation.angles[1] == 11 &&
                    owner->task_id_at_2 == 17 && owner->orientation.flags == 32769,
                "live writes reach existing canonical task state");
        task->primary_state->position.reset();
        bool failed = false;
        try {
            init::bind_primary_view(task);
        } catch (const init::Error &e) {
            failed = e.boundary == init::Boundary::PrimaryPosition;
        }
        require(failed, "unknown canonical pose is not fabricated from stale arena zero bytes");
    }
    auto dirty = scheduler.create_element(6, 3, nullptr).task;
    const std::uint8_t byte = 1;
    require(arena->write(dirty->primary_work, 0, &byte, 1), "nonzero source fixture");
    bool failed = false;
    try {
        init::establish_zeroed_primary(scheduler, dirty);
    } catch (const init::Error &e) {
        failed = e.boundary == init::Boundary::PrimaryNotZero;
    }
    require(failed && !dirty->primary_state, "nonzero source state is never reset or guessed");
}
void native_initialization_boundary() {
    auto arena = std::make_shared<TaskWorkArena>();
    TaskScheduler scheduler;
    require(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}),
            "initialization pool");
    auto task = scheduler.create_element(7, 3, nullptr).task;
    init::establish_zeroed_primary(scheduler, task);
    auto primary = init::bind_primary_view(task);
    primary.position = {10, 20, 30};
    primary.rotation = {100, 200, 300};
    init::ParameterTable parameters{};
    parameters[0].rad = 7;
    PlayerEventFlags events{};
    events[141] = 255;
    init::NativeAccess access(parameters, events, nullptr);
    init::Registry registry;
    init::establish_zeroed_motion(scheduler, task);
    auto motion = init::bind_existing_motion(task);
    motion->target_yaw = 16384;
    init::establish_zeroed_motion(scheduler, task);
    require(init::bind_existing_motion(task) == motion && motion->target_yaw == 16384,
            "preview aim remains in the same canonical motion owner");
    motion->motion.speed = {4, 5, 6};
    init::PlayerHandle record;
    X87Context fp(127);
    bool failed = false;
    try {
        init::initialize(0, task, motion, scheduler, registry, access, fp, record);
    } catch (const init::Error &e) {
        failed = e.boundary == init::Boundary::AuxiliaryCallback;
    }
    require(failed && record && record->stage == init::Stage::TaskScale,
            "actual unresolved auxiliary callback remains an explicit boundary");
    require(registry.tasks[0] == task && registry.primary[0] == task->primary_state &&
                registry.motion[0] == motion && registry.players[0] == record,
            "four plain actual registration owners");
    require(record->physics->equipment_at_4 == 2 && record->parameters.rad == 7 &&
                record->physics->shadow_scale_at_1fc == 1.4f,
            "typed content/save facts preserved");
    require(motion->motion.speed.x == 4 && record->physics->motion.speed.x == 0 &&
                motion->friction_scale == 1 && registry.motion[0] == task->motion_state &&
                motion->target_yaw == 16384,
            "world/relative motion owners remain distinct");
    auto current = init::bind_primary_view(task);
    require(&primary.position == &current.position && current.position.y == 20 &&
                current.rotation[1] == 200 && current.id == 1 && current.scale.x == 1,
            "initialization borrows actual pose and performs source scale/id stores");
}
} // namespace
int main() {
    try {
        primary_owners();
        native_initialization_boundary();
        std::cout << "Actual flags6/7 primary binding, canonical pose aliases, typed startup and "
                     "explicit auxiliary boundary pass\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

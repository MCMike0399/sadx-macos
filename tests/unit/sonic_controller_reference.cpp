#include "guest/sonic_player_reference.hpp"
#include <cassert>
#include <iostream>

namespace {
using namespace sadx;
void primary_observer_can_remove_the_player_binding() {
    TaskScheduler scheduler;
    auto object = scheduler.append_task(0, &TaskScheduler::source_no_op).task;
    auto task = std::make_shared<PlayerTaskState>();
    task->action_at_0 = 86;
    auto world = std::make_shared<PlayerMotionState>();
    auto record = std::make_shared<PlayerStartupPlayer>();
    record->player = std::make_shared<PlayerPhysicsState>();
    SonicPlayerFrame frame{object, task, world, record, nullptr};
    PlayerPositionCollisionScene scene;
    scene.task_collision = PositionResourceRef{};
    frame.collision = &scene;
    SonicPlayerGlobals globals;
    globals.forced_at_3b29d0c = 0;
    std::optional<std::uint16_t> forced = 0;
    globals.forced_at_3b29d08 = &forced;
    actor_current::Observation observer;
    observer.primary = [&](const TaskWorkHandle &) { frame.player.reset(); };
    globals.current_observation = &observer;
    PlayerStartupRegistry registry;
    SonicPlayerConstants constants{};
    SonicPlayerRuntime runtime{globals, scheduler, registry, nullptr, nullptr, constants, {}};
    X87Context fp(127);
    const auto result = run_sonic_player(frame, runtime, fp);
    assert(result.failure == SonicPlayerFailure::unknown_player_binding &&
           result.source_boundary == 0x49bdb4);
    assert(!globals.current.player);
}
void ground_callbacks_reload_parameters_and_keep_captured_physics() {
    TaskScheduler scheduler;
    auto task = std::make_shared<PlayerTaskState>();
    task->orientation.flags = 0x2000;
    auto world = std::make_shared<PlayerMotionState>();
    auto record = std::make_shared<PlayerStartupPlayer>();
    record->player = std::make_shared<PlayerPhysicsState>();
    record->parameters = PlayerParameters{};
    auto replacement = std::make_shared<PlayerStartupPlayer>();
    replacement->player = std::make_shared<PlayerPhysicsState>();
    replacement->parameters = PlayerParameters{};
    record->parameters->run_speed = 3;
    replacement->parameters->run_speed = 9;
    SonicPlayerFrame frame{{}, task, world, record, nullptr};
    NinjaMatrix unit{};
    NinjaMatrixStack stack({unit, unit}, 1, 0);
    PlayerCoordinates coordinates({}, unit, stack);
    PlayerAccelerationRuntime acceleration;
    GroundAccelerationConstants ground{};
    GroundAccelerationCallees ground_calls;
    bool reset = false, rotated = false;
    SonicPlayerRuntime *active_runtime = nullptr;
    const SonicPlayerNativeMotion *next_motion = nullptr;
    ground_calls.direction_reset = [&](PlayerTaskState &, PlayerMotionState &,
                                       PlayerPhysicsState &p, const PlayerParameters &params,
                                       X87Context &) {
        assert(&p == record->player.get() && &params == &*record->parameters);
        frame.player = replacement;
        if (active_runtime)
            active_runtime->motion = next_motion;
        reset = true;
        return PlayerAccelerationResult{};
    };
    ground_calls.yaw_regular = [&](PlayerTaskState &, PlayerMotionState &, PlayerPhysicsState &p,
                                   const PlayerParameters &params, std::int32_t, X87Context &) {
        assert(reset && &p == record->player.get() && &params == &*replacement->parameters);
        rotated = true;
        return PlayerAccelerationResult{};
    };
    AirAccelerationConstants air{};
    AirAccelerationCallees air_calls;
    FloorAngleConstants floor{};
    CrtAsinRuntime inverse;
    PlayerPositionConstants position{};
    PlayerRotationConstants rotation{};
    PlayerPositionRuntime position_runtime;
    PlayerPositionCallees position_calls;
    SonicPlayerNativeMotion motion{coordinates,      acceleration,  ground,  ground_calls, air,
                                   air_calls,        floor,         inverse, position,     rotation,
                                   position_runtime, position_calls};
    SonicPlayerConstants constants{};
    X87Context fp(127);
    assert(sonic_ground_input(frame, motion, constants, fp));
    assert(reset && rotated);
    // A callback may replace the caller's binding; this helper keeps its captured
    // motion owner, while the next movement helper consumes the replacement.
    GroundAccelerationCallees next_calls;
    next_calls.yaw_regular = [](PlayerTaskState &, PlayerMotionState &, PlayerPhysicsState &,
                                const PlayerParameters &, std::int32_t,
                                X87Context &) -> PlayerAccelerationResult {
        throw std::runtime_error("replacement owner used inside captured heading helper");
    };
    SonicPlayerNativeMotion next{coordinates,      acceleration,  ground,  next_calls, air,
                                 air_calls,        floor,         inverse, position,   rotation,
                                 position_runtime, position_calls};
    SonicPlayerGlobals globals;
    PlayerStartupRegistry registry;
    SonicPlayerRuntime runtime{globals, scheduler, registry, nullptr, &motion, constants, {}};
    active_runtime = &runtime;
    next_motion = &next;
    reset = false;
    rotated = false;
    frame.player = record;
    task->action_at_0 = 1;
    const auto result = dispatch_sonic_player_action(frame, runtime, fp);
    assert(result.failure == SonicPlayerFailure::acceleration_failed);
    assert(result.source_boundary == 0x49ad3c && reset && rotated && runtime.motion == &next);
}
} // namespace
int main() {
    primary_observer_can_remove_the_player_binding();
    ground_callbacks_reload_parameters_and_keep_captured_physics();
    std::cout << "Reference Sonic callback reload contracts passed\n";
}

#include "guest/sonic_player_reference.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace sadx;
struct Fixture {
    TaskScheduler scheduler;
    std::shared_ptr<PlayerTaskState> task = std::make_shared<PlayerTaskState>();
    std::shared_ptr<PlayerMotionState> world = std::make_shared<PlayerMotionState>();
    std::shared_ptr<PlayerStartupPlayer> record = std::make_shared<PlayerStartupPlayer>();
    // Retain both objects explicitly: no witness relies on a dangling source ref.
    const std::shared_ptr<PlayerPhysicsState> original = std::make_shared<PlayerPhysicsState>();
    const std::shared_ptr<PlayerPhysicsState> replacement = std::make_shared<PlayerPhysicsState>();
    SonicPlayerFrame frame{{}, task, world, record, nullptr};
    NinjaMatrix unit{};
    NinjaMatrixStack stack;
    PlayerCoordinates coordinates;
    PlayerAccelerationRuntime acceleration;
    GroundAccelerationConstants ground{};
    GroundAccelerationCallees calls;
    AirAccelerationConstants air{};
    AirAccelerationCallees air_calls;
    FloorAngleConstants floor{};
    CrtAsinRuntime inverse;
    PlayerPositionConstants position{};
    PlayerRotationConstants rotation{};
    PlayerPositionRuntime position_runtime;
    PlayerPositionCallees position_calls;
    SonicPlayerNativeMotion motion;
    SonicPlayerConstants constants{};
    X87Context fp{127};
    static NinjaMatrix identity() {
        NinjaMatrix m{};
        m[0] = m[5] = m[10] = m[15] = 1;
        return m;
    }
    Fixture()
        : unit(identity()), stack({unit, unit}, 1, 0), coordinates({}, unit, stack),
          motion{coordinates,      acceleration,  ground,  calls,    air,
                 air_calls,        floor,         inverse, position, rotation,
                 position_runtime, position_calls} {
        task->orientation.flags = 0x2000;
        record->player = original;
        record->parameters = PlayerParameters{};
        original->motion.speed.x = 3;
        replacement->motion.speed.x = 103;
        record->parameters->run_speed = 3;
        world->angle_x_at_1c = 0;
        world->angle_z_at_24 = 0;
    }
};
bool ground_same_record(bool clear, bool nested) {
    Fixture f;
    bool received_original = false, received_new = false;
    float live_parameter = 0;
    f.calls.direction_reset = [&](PlayerTaskState &, PlayerMotionState &,
                                  PlayerPhysicsState &physics, const PlayerParameters &,
                                  X87Context &) {
        if (&physics != f.original.get())
            throw std::runtime_error("wrong initial physics");
        f.record->player = clear ? std::shared_ptr<PlayerPhysicsState>{} : f.replacement;
        f.record->parameters->run_speed = 19;
        if (nested)
            f.task->orientation.flags = 0x4000;
        return PlayerAccelerationResult{};
    };
    f.calls.yaw_regular = [&](PlayerTaskState &, PlayerMotionState &, PlayerPhysicsState &physics,
                              const PlayerParameters &parameters, std::int32_t, X87Context &) {
        received_original = &physics == f.original.get();
        live_parameter = parameters.run_speed;
        physics.motion.speed.x += 7;
        return PlayerAccelerationResult{};
    };
    f.calls.event_acceleration = [&](PlayerTaskState &, PlayerMotionState &,
                                     PlayerPhysicsState &physics,
                                     const PlayerParameters &parameters, X87Context &) {
        received_new = &physics == f.replacement.get() && parameters.run_speed == 19;
        physics.motion.speed.x += 11;
        return PlayerAccelerationResult{PlayerAccelerationFailure::missing_callee, 0x111111, 0};
    };
    SonicPlayerResult result;
    if (nested) {
        f.task->action_at_0 = 1;
        SonicPlayerGlobals globals;
        PlayerStartupRegistry registry;
        SonicPlayerRuntime runtime{globals,   f.scheduler, registry, nullptr,
                                   &f.motion, f.constants, {}};
        result = dispatch_sonic_player_action(f.frame, runtime, f.fp);
    } else
        result = sonic_ground_input(f.frame, f.motion, f.constants, f.fp);
    const bool stop_ok = !nested ? bool(result)
                         : clear ? (result.failure == SonicPlayerFailure::null_required_resource &&
                                    result.source_boundary == 0x49ad44)
                                 : (result.failure == SonicPlayerFailure::acceleration_failed &&
                                    result.source_boundary == 0x49ad3c);
    const float expected_new = nested && !clear ? 114 : 103;
    const bool good = stop_ok && received_original && f.original->motion.speed.x == 10 &&
                      f.replacement->motion.speed.x == expected_new && live_parameter == 19 &&
                      (!nested || clear || received_new) && f.original.use_count() == 1;
    std::cout << "{\"case\":\"ground\",\"null\":" << clear << ",\"nested\":" << nested
              << ",\"original_identity\":" << received_original
              << ",\"nested_identity\":" << received_new
              << ",\"original_speed\":" << f.original->motion.speed.x
              << ",\"replacement_speed\":" << f.replacement->motion.speed.x
              << ",\"live_parameter\":" << live_parameter << ",\"good\":" << good << "}\n";
    return good;
}
bool movement_keeps_its_own_capture(bool clear) {
    Fixture f;
    f.task->orientation.flags = 0x4000;
    f.task->action_at_0 = 2;
    f.original->previous_animation_at_128 = 0;
    f.original->motion.acceleration.x = 5;
    f.replacement->motion.acceleration.x = 17;
    bool called = false;
    f.calls.event_acceleration = [&](PlayerTaskState &, PlayerMotionState &,
                                     PlayerPhysicsState &physics, const PlayerParameters &,
                                     X87Context &) {
        if (&physics != f.original.get())
            throw std::runtime_error("movement entered with wrong physics");
        called = true;
        f.record->player = clear ? std::shared_ptr<PlayerPhysicsState>{} : f.replacement;
        f.record->parameters->jog_speed = 19;
        return PlayerAccelerationResult{};
    };
    SonicPlayerGlobals globals;
    PlayerStartupRegistry registry;
    SonicPlayerRuntime runtime{globals, f.scheduler, registry, nullptr, &f.motion, f.constants, {}};
    const auto result = dispatch_sonic_player_action(f.frame, runtime, f.fp);
    const bool good = called && result.failure == SonicPlayerFailure::speed_failed &&
                      result.source_boundary == 0x49ad97 && f.original->motion.speed.x == 8 &&
                      f.replacement->motion.speed.x == 103 && f.original.use_count() == 1;
    std::cout << "{\"case\":\"movement_capture\",\"null\":" << clear
              << ",\"original_speed\":" << f.original->motion.speed.x
              << ",\"replacement_speed\":" << f.replacement->motion.speed.x << ",\"good\":" << good
              << "}\n";
    return good;
}
bool copy_recaptures_finalize(bool clear) {
    Fixture f;
    f.task->orientation.flags = 1;
    f.original->attributes = 0x4000;
    f.original->floor_normal = NinjaVector{0, 1, 2};
    f.original->motion.speed = {7, 8, 9};
    f.original->dotp = 2;
    f.replacement->attributes = 0x4000;
    f.replacement->prior_attributes = 12;
    f.replacement->motion.speed = {103, 203, 303};
    f.replacement->dotp = 3;
    f.world->motion.speed = {3, 4, 5};
    f.floor = {3, -3, 0, 1, 1, -1};
    f.inverse.matherr_control = 0;
    unsigned callbacks = 0;
    f.inverse.matherr_handler = [&](CrtMathFunction, unsigned, double, double, std::uint16_t,
                                    X87Context &) {
        ++callbacks;
        f.record->player = clear ? std::shared_ptr<PlayerPhysicsState>{} : f.replacement;
        f.record->parameters->run_accel = 9;
        return CrtMathErrorResult{CrtAsinFailure::none, 0.0};
    };
    const auto result = sonic_copy_local_speed(f.frame, f.motion, f.constants, f.fp);
    const bool result_ok = clear ? result.failure == SonicPlayerFailure::null_required_resource &&
                                       result.source_boundary == 0x43eee5
                                 : bool(result);
    const bool destination_ok =
        clear ? f.replacement->motion.speed.x == 103
              : f.replacement->motion.speed.x == 3 && f.replacement->motion.speed.y == 0 &&
                    f.replacement->motion.speed.z == 5 && f.replacement->dotp == 1;
    const bool good = result_ok && callbacks == 1 && destination_ok &&
                      f.original->motion.speed.x == 7 && f.original->motion.speed.y == 8 &&
                      f.original->motion.speed.z == 9 && f.original->dotp == 2 &&
                      f.stack.depth() == 1 && f.original.use_count() == 1;
    std::cout << "{\"case\":\"copy_finalize\",\"null\":" << clear << ",\"callbacks\":" << callbacks
              << ",\"original_speed\":" << f.original->motion.speed.x
              << ",\"replacement_speed\":" << f.replacement->motion.speed.x << ",\"good\":" << good
              << "}\n";
    return good;
}
} // namespace
int main(int argc, char **argv) {
    const bool repro = argc == 2 && std::string(argv[1]) == "repro";
    bool good = ground_same_record(false, false);
    if (!repro) {
        good = ground_same_record(true, false) && good;
        good = ground_same_record(false, true) && good;
        good = ground_same_record(true, true) && good;
        good = movement_keeps_its_own_capture(false) && good;
        good = movement_keeps_its_own_capture(true) && good;
        good = copy_recaptures_finalize(false) && good;
        good = copy_recaptures_finalize(true) && good;
    }
    return !good;
}

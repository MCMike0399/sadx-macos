#include "player/ground_acceleration_internal.hpp"
#include "runtime/native_sonic_context.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
struct GroundObservation {
    sadx::PlayerMotion *player;
    sadx::NinjaVector acceleration;
    float speed, run_speed;
};
} // namespace
namespace sadx::physics {
// Test observation of the still-uncomposed callee boundary. It never returns
// success and never supplies yaw/game behavior. All earlier arithmetic is real.
void turn_ground_yaw(GroundContext &context, std::int32_t, GroundYaw) {
    throw GroundObservation{&context.motion.player, context.acceleration,
                            context.motion.player.speed.x, context.parameters.run_speed};
}
} // namespace sadx::physics
namespace {
using namespace sadx;
void auxiliary(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("uninvoked test token");
}
bool check(bool null_replacement, bool live_contact = false) {
    native::PlayerSystem players;
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
        throw std::runtime_error("pool");
    const auto task = scheduler.create_element(7, 3, nullptr).task;
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    player_initialization::ParameterTable table{};
    PlayerEventFlags events{};
    table[0].rad = 4;
    table[0].height = 10;
    X87Context fp(127);
    std::int16_t stage = 0;
    std::uint8_t alternate = 0;
    std::shared_ptr<const animation::TableStorage> animations =
        std::make_shared<animation::Table>();
    sonic::Context context{players, scheduler, task, fp};
    context.stage = &stage;
    context.alternate_model = &alternate;
    context.parameters = &table;
    context.events = &events;
    context.auxiliary = auxiliary;
    context.animation_table = &animations;
    context.read_editor = [] { return std::int16_t{0}; };
    context.read_forced = context.read_editor;
    try {
        sonic::run(context);
    } catch (const sonic::Unavailable &error) {
        if (std::string(error.what()) != "sonic_model_graph")
            throw;
    }
    const auto record = players.registry.players[0];
    const auto original = record->physics;
    const auto replacement = std::make_shared<PlayerPhysicsState>();
    original->motion.speed = {2, 0, 0};
    original->dotp = 0.05f;
    original->attributes = 0;
    original->contact_work = PlayerContactWork{};
    original->contact_work->direction_at_1c = NinjaVector{1, 0, 0};
    replacement->motion.speed = {103, 0, 0};
    replacement->contact_work = PlayerContactWork{};
    replacement->contact_work->direction_at_1c = NinjaVector{0, 0, 1};
    auto &params = record->parameters;
    params.weight = 1;
    params.run_accel = .25f;
    params.jog_speed = 4;
    params.run_speed = 6;
    params.max_x_spd = 16;
    params.rush_speed = 12;
    params.dash_speed = 16;
    task->primary_state->action_at_0 = 2;
    task->primary_state->orientation.flags = 0x2001;
    auto world = task->motion_state;
    world->motion.speed = {0, 0, 3};
    world->motion.acceleration = {1, 0, 0};
    world->target_yaw = 1234;
    NinjaMatrix unit{};
    unit[0] = unit[5] = unit[10] = unit[15] = 1;
    NinjaMatrixStack stack({unit, unit}, 1, 0);
    PlayerCoordinates coordinates({}, unit, stack);
    PlayerAccelerationRuntime acceleration;
    acceleration.world_gravity = NinjaVector{0, -1, 0};
    acceleration.input.stage = 2;
    acceleration.input.act = 2;
    acceleration.input.current_character = 0;
    acceleration.input.camera_yaw = 0;
    unsigned queries = 0;
    acceleration.forced_angle = [&](X87Context &arithmetic) {
        ++queries;
        record->physics = null_replacement ? std::shared_ptr<PlayerPhysicsState>{} : replacement;
        original->motion.speed.x = 3;
        if (live_contact) {
            original->contact_work->direction_at_1c = NinjaVector{0, 0, 1};
            replacement->contact_work->direction_at_1c = NinjaVector{1, 0, 0};
        }
        record->parameters.run_speed = 8;
        task->primary_state->orientation.flags = 1;
        return convert_forced_input_angle({0.0, 1.0, 1.0, 1.0}, arithmetic);
    };
    AirAccelerationConstants air{};
    AirAccelerationCallees air_calls;
    FloorAngleConstants floor{};
    CrtAsinRuntime inverse;
    PlayerPositionConstants position{};
    sonic::Motion motion{coordinates, acceleration, air, air_calls, floor, inverse, position};
    context.motion = &motion;
    try {
        sonic::dispatch(context);
    } catch (const GroundObservation &observed) {
        const bool good = observed.player == &original->motion &&
                          observed.acceleration.x == (live_contact ? 4.225f : 0.0f) &&
                          observed.acceleration.y == -1 && observed.acceleration.z == 0 &&
                          observed.speed == 3 && observed.run_speed == 8 &&
                          original.use_count() == 1 && replacement->motion.speed.x == 103 &&
                          world->target_yaw == 0 && queries == 1;
        std::cout << "{\"null\":" << null_replacement
                  << ",\"same_player\":" << (observed.player == &original->motion)
                  << ",\"acceleration_x\":" << observed.acceleration.x
                  << ",\"speed\":" << observed.speed
                  << ",\"new_speed\":" << replacement->motion.speed.x
                  << ",\"target_yaw\":" << world->target_yaw << "}\n";
        return good;
    } catch (const sonic::Unavailable &error) {
        std::cout << "{\"null\":" << null_replacement << ",\"wrong_stop\":\"" << error.what()
                  << "\"}\n";
        return false;
    }
    return false;
}
bool movement_capture(bool clear) {
    native::PlayerSystem players;
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
        throw std::runtime_error("pool");
    const auto task = scheduler.create_element(7, 3, nullptr).task;
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    player_initialization::ParameterTable table{};
    table[0].rad = 4;
    table[0].height = 10;
    PlayerEventFlags events{};
    X87Context fp(127);
    player_initialization::NativeAccess access(table, events, auxiliary);
    player_initialization::PlayerHandle record;
    player_initialization::initialize(0, task, task->motion_state, scheduler, players.registry,
                                      access, fp, record);
    const auto original = record->physics;
    const auto replacement = std::make_shared<PlayerPhysicsState>();
    original->motion.speed = {3, 0, 0};
    original->motion.acceleration = {5, 0, 0};
    replacement->motion.speed = {103, 0, 0};
    replacement->motion.acceleration = {17, 0, 0};
    task->primary_state->action_at_0 = 9;
    task->primary_state->orientation.flags = 0x4000;
    NinjaMatrix unit{};
    unit[0] = unit[5] = unit[10] = unit[15] = 1;
    NinjaMatrixStack stack({unit, unit}, 1, 0);
    PlayerCoordinates coordinates({}, unit, stack);
    PlayerAccelerationRuntime acceleration;
    AirAccelerationConstants air{};
    AirAccelerationCallees air_calls;
    bool called = false;
    air_calls.event_acceleration = [&](PlayerTaskState &, PlayerMotionState &,
                                       PlayerPhysicsState &physics, const PlayerParameters &,
                                       X87Context &) {
        if (&physics != original.get())
            throw std::runtime_error("movement entered with wrong physics");
        called = true;
        record->physics = clear ? std::shared_ptr<PlayerPhysicsState>{} : replacement;
        record->parameters.jog_speed = 19;
        return PlayerAccelerationResult{};
    };
    FloorAngleConstants floor{};
    CrtAsinRuntime inverse;
    PlayerPositionConstants position{};
    sonic::Motion motion{coordinates, acceleration, air, air_calls, floor, inverse, position};
    sonic::Context context{players, scheduler, task, fp};
    context.motion = &motion;
    bool stopped = false;
    try {
        sonic::dispatch(context);
    } catch (const sonic::Unavailable &error) {
        stopped = std::string(error.what()) == "sonic_calculate_speed";
    }
    const bool good = called && stopped && original->motion.speed.x == 8 &&
                      replacement->motion.speed.x == 103 && original.use_count() == 1;
    std::cout << "{\"case\":\"native_movement_capture\",\"null\":" << clear
              << ",\"original_speed\":" << original->motion.speed.x
              << ",\"replacement_speed\":" << replacement->motion.speed.x << ",\"good\":" << good
              << "}\n";
    return good;
}
bool copy_finalize(bool clear) {
    native::PlayerSystem players;
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    if (!scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}))
        throw std::runtime_error("pool");
    const auto task = scheduler.create_element(7, 3, nullptr).task;
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    player_initialization::ParameterTable table{};
    table[0].rad = 4;
    table[0].height = 10;
    PlayerEventFlags events{};
    X87Context fp(127);
    player_initialization::NativeAccess access(table, events, auxiliary);
    player_initialization::PlayerHandle record;
    player_initialization::initialize(0, task, task->motion_state, scheduler, players.registry,
                                      access, fp, record);
    const auto original = record->physics;
    const auto replacement = std::make_shared<PlayerPhysicsState>();
    original->attributes = 0x4000;
    original->floor_normal = NinjaVector{0, 1, 2};
    original->motion.speed = {7, 8, 9};
    original->dotp = 2;
    replacement->attributes = 0x4000;
    replacement->prior_attributes = 12;
    replacement->motion.speed = {103, 203, 303};
    replacement->dotp = 3;
    task->primary_state->orientation.flags = 1;
    task->motion_state->motion.speed = {3, 4, 5};
    NinjaMatrix unit{};
    unit[0] = unit[5] = unit[10] = unit[15] = 1;
    NinjaMatrixStack stack({unit, unit}, 1, 0);
    PlayerCoordinates coordinates({}, unit, stack);
    PlayerAccelerationRuntime acceleration;
    AirAccelerationConstants air{};
    AirAccelerationCallees air_calls;
    FloorAngleConstants floor{3, -3, 0, 1, 1, -1};
    CrtAsinRuntime inverse;
    inverse.matherr_control = 0;
    unsigned callbacks = 0;
    inverse.matherr_handler = [&](CrtMathFunction, unsigned, double, double, std::uint16_t,
                                  X87Context &) {
        ++callbacks;
        record->physics = clear ? std::shared_ptr<PlayerPhysicsState>{} : replacement;
        record->parameters.run_accel = 9;
        return CrtMathErrorResult{CrtAsinFailure::none, 0.0};
    };
    PlayerPositionConstants position{};
    sonic::Motion motion{coordinates, acceleration, air, air_calls, floor, inverse, position};
    sonic::Context context{players, scheduler, task, fp};
    context.motion = &motion;
    bool completed = false, null_stop = false;
    try {
        sonic::copy_local_speed(context);
        completed = true;
    } catch (const sonic::Unavailable &error) {
        null_stop = std::string(error.what()) == "sonic_player_physics";
    }
    const bool destination = clear ? replacement->motion.speed.x == 103
                                   : replacement->motion.speed.x == 3 &&
                                         replacement->motion.speed.y == 0 &&
                                         replacement->motion.speed.z == 5 && replacement->dotp == 1;
    const bool good = (clear ? null_stop : completed) && callbacks == 1 && destination &&
                      original->motion.speed.x == 7 && original->motion.speed.y == 8 &&
                      original->motion.speed.z == 9 && original->dotp == 2 &&
                      original.use_count() == 1 && stack.depth() == 1;
    std::cout << "{\"case\":\"native_copy_finalize\",\"null\":" << clear
              << ",\"callbacks\":" << callbacks
              << ",\"original_speed\":" << original->motion.speed.x
              << ",\"replacement_speed\":" << replacement->motion.speed.x << ",\"good\":" << good
              << "}\n";
    return good;
}

} // namespace
int main() {
    const bool replaced = check(false), cleared = check(true), live = check(false, true);
    const bool copied = copy_finalize(false), null_final = copy_finalize(true);
    const bool moved = movement_capture(false), null_move = movement_capture(true);
    return !(replaced && cleared && live && copied && null_final && moved && null_move);
}

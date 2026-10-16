#include "player/player_acceleration_ground.hpp"
#include "guest/ground_acceleration_reference.hpp"
#include "player/player_acceleration_friction.hpp"
#include "player/player_heading.hpp"
#include <limits>

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <class T> T read_exact(const char *path) {
    T result;
    std::ifstream input(path, std::ios::binary);
    input.read(reinterpret_cast<char *>(&result), sizeof(result));
    require(input.gcount() == sizeof(result) && input.peek() == std::char_traits<char>::eof(),
            "Exact source asset required");
    return result;
}
bool same(const sadx::NinjaVector &a, const sadx::NinjaVector &b) {
    return std::memcmp(&a, &b, sizeof(a)) == 0;
}

void boundaries(const sadx::PlayerParameters &p, const sadx::GroundAccelerationConstants &constants,
                sadx::PlayerCoordinates &coordinates) {
    sadx::PlayerTaskState task{{0, {0, 0, 0}}, 0, 0, {}};
    sadx::PlayerMotionState world{{{1, 0, 1}, {0, 0, 0}}, 111, 123, 456, 1};
    sadx::PlayerPhysicsState player{{{11, 22, 33}, {0, 0, 1}},
                                    1,
                                    {0, 0, 0},
                                    {},
                                    0,
                                    0,
                                    sadx::PlayerContactWork{sadx::NinjaVector{0, 1, 0}}};
    const auto prior = player.motion.acceleration;
    sadx::PlayerAccelerationRuntime runtime;
    sadx::GroundAccelerationCallees callees;
    sadx::X87Context fp(0x027f);
    const auto run = [&]() {
        return sadx::get_player_acceleration_ground(task, world, player, p, constants, runtime,
                                                    callees, fp);
    };
    task.orientation.flags = 0x4000;
    auto result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::missing_callee &&
                result.source_boundary == 0x44c293 && result.callee_address == 0x447510 &&
                !result.temporary_acceleration && same(player.motion.acceleration, prior),
            "Event acceleration must select its boundary before unknown gravity");
    task.orientation.flags = 0;
    player.attributes = 0x24000;
    result = run();
    require(result.source_boundary == 0x44c2b8 && result.callee_address == 0x44b0e0,
            "Attribute20000 must precede attribute4000");
    callees.attribute_20000_acceleration = [](auto &t, auto &w, auto &, const auto &, auto &) {
        t.orientation.angles[1] = 987;
        w.source_field_34 = 654;
        sadx::PlayerAccelerationResult failed{sadx::PlayerAccelerationFailure::input_unavailable,
                                              0x44b166, 0x43bdc0, sadx::NinjaVector{7, 8, 9}};
        failed.input_failure = sadx::PlayerInputFailure::unavailable_normalized_input;
        return failed;
    };
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::callee_unavailable &&
                result.callee_failure == sadx::PlayerAccelerationFailure::input_unavailable &&
                task.orientation.angles[1] == 987 && world.source_field_34 == 654 &&
                result.nested_source_boundary == 0x44b166 &&
                result.input_failure == sadx::PlayerInputFailure::unavailable_normalized_input &&
                result.temporary_acceleration->x == 7,
            "Failed callee must retain its mutations, producing boundary and partial facts");
    task.orientation.angles[1] = 0;
    player.attributes = 0;
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::unavailable_gravity &&
                !result.temporary_acceleration && same(player.motion.acceleration, prior),
            "Unknown gravity must remain unknown");
    runtime.world_gravity = sadx::NinjaVector{0, -1, 0};
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::missing_world_to_player &&
                result.source_boundary == 0x44c357 && result.temporary_acceleration,
            "Known temporary gravity must survive missing transform");
    callees.world_to_player = [&](const auto &orientation, auto &value, auto &context) {
        return coordinates.world_to_player(orientation, value, context);
    };
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::input_unavailable &&
                result.input_failure == sadx::PlayerInputFailure::unavailable_stage &&
                result.nested_source_boundary == 0x43be6d &&
                same(player.motion.acceleration, prior),
            "Input failure reason/source boundary must survive the caller");
    runtime.input.stage = 1;
    runtime.input.normalized[0] = sadx::NormalizedPlayerInput{0, 0};
    player.contact_work.reset();
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::unavailable_collision_vector &&
                result.callee_address == 0x7889f0,
            "Unknown collision vector must not become a default floor normal");
    player.contact_work = sadx::PlayerContactWork{sadx::NinjaVector{0, 1, 0}};
    player.dotp = 0;
    const auto field34 = world.source_field_34;
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::missing_callee &&
                result.source_boundary == 0x44c620 && world.source_field_34 == field34 &&
                same(player.motion.acceleration, prior),
            "Passive rotation failure must stop before scalar/acceleration stores");
    player.dotp = 1;
    runtime.input.normalized[0] = sadx::NormalizedPlayerInput{0x4000, 1};
    runtime.input.no_control_timer[0] = 0;
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::missing_callee &&
                result.source_boundary == 0x44c81e && result.callee_address == 0x443df0 &&
                world.target_yaw == 0x4000 && world.source_field_34 == field34 &&
                same(player.motion.acceleration, prior),
            "Fast-heading boundary must preserve prior yaw-target store");
    callees.yaw_fast = [&](auto &t, auto &, auto &selected, const auto &, std::int32_t angle,
                           auto &context) {
        const auto turned =
            sadx::turn_player_heading(t.orientation, selected.motion, angle, coordinates, context);
        return sadx::PlayerAccelerationResult{
            turned ? sadx::PlayerAccelerationFailure::none
                   : sadx::PlayerAccelerationFailure::coordinate_unavailable};
    };
    result = run();
    require(static_cast<bool>(result) && task.orientation.angles[1] == 0x2000 &&
                world.source_field_34 == 0 && !same(player.motion.acceleration, prior),
            "Actual heading must share task state and complete the selected ground branch");

    // Entire real-input/coordinate branch completes while unused callbacks stay
    // unavailable. No no-op callback is supplied to make this case pass.
    task.orientation.angles = {0, 0, 0};
    player.dotp = 1;
    player.motion.speed = {3, 0, 1};
    runtime.input.normalized[0] = sadx::NormalizedPlayerInput{{}, 0};
    world.source_field_34 = 123;
    result = run();
    require(static_cast<bool>(result) && world.source_field_34 == 0 &&
                player.motion.acceleration.y == -p.weight,
            "Inactive ground branch must lazily consume only its actual dependencies");
}
void callback_facts(const sadx::PlayerParameters &source,
                    const sadx::GroundAccelerationConstants &constants,
                    sadx::PlayerCoordinates &coordinates) {
    auto parameters = source;
    parameters.jog_speed = 2;
    parameters.run_speed = 4;
    parameters.max_x_spd = 8;
    parameters.rush_speed = 6;
    sadx::PlayerTaskState task{{0, {0, 0, 0}}, 0, 0, {}};
    sadx::PlayerMotionState world{{{1, 0, 1}, {0, 0, 0}}, 111, 123, 456, 1};
    sadx::PlayerPhysicsState player{{{11, 22, 33}, {3, 0, 0}},
                                    1,
                                    {0, 0, 0},
                                    {},
                                    0,
                                    0,
                                    sadx::PlayerContactWork{sadx::NinjaVector{0, 1, 0}}};
    sadx::PlayerAccelerationRuntime runtime;
    runtime.world_gravity = sadx::NinjaVector{0, -1, 0};
    sadx::GroundAccelerationCallees callees;
    sadx::X87Context fp(0x037f);
    callees.world_to_player = [&](const auto &orientation, auto &value, auto &context) {
        const auto result = coordinates.world_to_player(orientation, value, context);
        task.orientation.flags = 0x4000;
        task.event_move = sadx::PlayerEventMoveData{1, {}, {}, {}};
        return result;
    };
    const auto run = [&]() {
        task.orientation.flags = 0;
        return sadx::get_player_acceleration_ground(task, world, player, parameters, constants,
                                                    runtime, callees, fp);
    };
    player.contact_work.reset();
    auto result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::unavailable_collision_vector,
            "missing contact precedes any consumption of unwritten active input scratch");
    player.contact_work = sadx::PlayerContactWork{sadx::NinjaVector{0, 1, 0}};
    player.dotp = 0.9f;
    player.attributes = 0x800000;
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::missing_callee &&
                result.source_boundary == 0x44c6c5,
            "attribute rotation precedes missing angle scratch");
    callees.attribute_rotation = [&](auto &t, auto &, auto &, const auto &, auto &) {
        t.orientation.angles[1] = 987;
        return sadx::PlayerAccelerationResult{};
    };
    result = run();
    require(result.failure == sadx::PlayerAccelerationFailure::input_outputs_unavailable &&
                result.source_boundary == 0x44c6d3 && task.orientation.angles[1] == 987 &&
                world.source_field_34 == 123,
            "unwritten angle stops at its source read after retained rotation mutation");
    task.orientation.angles = {0, 0, 0};
    player.attributes = 0;
    callees.world_to_player = [&](const auto &orientation, auto &value, auto &context) {
        const auto result = coordinates.world_to_player(orientation, value, context);
        task.orientation.flags = 0x6000;
        task.event_move = sadx::PlayerEventMoveData{1, {}, {}, {}};
        return result;
    };
    callees.direction_reset = [](auto &, auto &w, auto &, const auto &, auto &) {
        w.target_yaw = 222;
        return sadx::PlayerAccelerationResult{};
    };
    struct MissingMagnitude {
        float speed, alignment;
        std::uint32_t boundary;
    };
    for (const auto row : {MissingMagnitude{1, 1, 0x44c6fb},
                           {3, 1, 0x44c72a},
                           {5, 1, 0x44c781},
                           {9, -0.2f, 0x44c7c8},
                           {9, 0.2f, 0x44c7d4}}) {
        player.motion.speed.x = row.speed;
        player.dotp = row.alignment;
        result = run();
        require(
            result.failure == sadx::PlayerAccelerationFailure::input_outputs_unavailable &&
                result.source_boundary == row.boundary && world.target_yaw == 222 &&
                world.source_field_34 == 123,
            "reset replaces missing angle while magnitude remains unknown at its selected read");
    }
    // Mutable owners remain live references, including parameters changed by a callback.
    parameters = source;
    player.motion.speed = {0, 0, 0};
    player.dotp = 0.5f;
    world.motion.acceleration = {1, 0, 0};
    runtime.input.stage = 1;
    runtime.input.normalized[0] = sadx::NormalizedPlayerInput{0, 0};
    task.event_move.reset();
    task.orientation.flags = 0;
    callees.world_to_player = [&](const auto &orientation, auto &value, auto &context) {
        return coordinates.world_to_player(orientation, value, context);
    };
    const auto original_weight = parameters.weight;
    callees.passive_rotation = [&](auto &, auto &w, auto &selected, const auto &, auto &) {
        selected.dotp = 1;
        parameters.weight = 10;
        parameters.lim_frict = 0;
        w.friction_scale = 0;
        return sadx::PlayerAccelerationResult{};
    };
    result = run();
    require(
        bool(result) && player.motion.acceleration.x == 1 &&
            player.motion.acceleration.y == -original_weight,
        "friction sees callback mutations while already formed gravity keeps the earlier weight");
}
void friction_failure_order(const sadx::PlayerParameters &source,
                            const sadx::GroundAccelerationConstants &constants) {
    auto parameters = source;
    parameters.lim_frict = 0;
    sadx::PlayerMotionState world{{{0, 0, 0}, {0, 0, 0}}, 0, 0, 0, 1};
    sadx::PlayerPhysicsState player{{{11, 22, 33}, {0, 0, 0}}, 1, {0, 0, 0}, {}, 4, 0, {}};
    sadx::NinjaVector gravity{7, std::numeric_limits<float>::max(), 0};
    sadx::X87Context overflow(0x0377);
    bool stopped = false;
    try {
        sadx::finish_player_ground_friction(gravity, 0, std::numeric_limits<float>::max(), false,
                                            world, player, parameters, constants,
                                            sadx::LongitudinalFriction::skip, overflow);
    } catch (const sadx::X87Error &error) {
        stopped = error.failure() == sadx::X87Failure::unmasked_exception &&
                  (error.exceptions() & 8U) != 0;
    }
    require(stopped && player.motion.acceleration.x == 7 && player.motion.acceleration.y == 22 &&
                player.motion.acceleration.z == 33,
            "failed Y spill retains X store and precedes Y/Z destination stores");
    player.motion.acceleration = {11, 22, 33};
    player.motion.speed.z = 1;
    gravity = {7, std::numeric_limits<float>::infinity(), 0};
    sadx::X87Context invalid(0x037e);
    stopped = false;
    try {
        sadx::finish_player_ground_friction(gravity, 0, -std::numeric_limits<float>::infinity(),
                                            false, world, player, parameters, constants,
                                            sadx::LongitudinalFriction::skip, invalid);
    } catch (const sadx::X87Error &error) {
        stopped = error.failure() == sadx::X87Failure::unmasked_exception &&
                  (error.exceptions() & 1U) != 0;
    }
    require(stopped && player.motion.acceleration.x == 11 && player.motion.acceleration.y == 22 &&
                player.motion.acceleration.z == 33,
            "failed final Y arithmetic precedes X store");
}
void primitives() {
    require(sadx::angle_distance(0xffff, 0) == 1 && sadx::angle_distance(0, 0x8000) == 0x8000 &&
                sadx::angle_distance(-2147483647 - 1, 2147483647) == 1,
            "Source low16 distance and NEG wrap");
    sadx::X87Context fp(0x037f);
    sadx::NinjaVector first{1, 2, 3}, second{4, 5, 6};
    const auto length = sadx::cross_product_length_extended(first, second, first, fp);
    require(first.x == -3 && first.y == 6 && first.z == -3 && fp.to_float(length) > 7 &&
                fp.to_float(length) < 8,
            "Source ECX/EDX cross orientation, aliased output and retained length");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 5, "usage: player_acceleration_ground_test PARAMETER_BYTES "
                           "GROUND_CONSTANTS QUARTER_TABLE UNIT_MATRIX");
        std::array<std::uint8_t, 132> parameter_bytes;
        std::ifstream parameters(argv[1], std::ios::binary);
        parameters.read(reinterpret_cast<char *>(parameter_bytes.data()), 132);
        require(parameters.gcount() == 132, "Sonic source parameter row required");
        const auto decoded = sadx::decode_player_parameters(parameter_bytes.data(), 132);
        require(static_cast<bool>(decoded), "Parameter decoding failed");
        sadx::GroundAccelerationConstants constants{
            read_exact<std::array<std::uint32_t, 24>>(argv[2])};
        const auto table = read_exact<sadx::NinjaMath::SineTable>(argv[3]);
        const auto unit = read_exact<sadx::NinjaMatrix>(argv[4]);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3, unit), 1, 0);
        sadx::PlayerCoordinates coordinates(table, unit, stack);
        require(constants.bits == sadx::native_ground_constant_words(),
                "All 24 native literal defaults match exact PE constant bits");
        boundaries(*decoded.parameters, constants, coordinates);
        callback_facts(*decoded.parameters, constants, coordinates);
        friction_failure_order(*decoded.parameters, constants);
        primitives();
        std::cout << "Ground acceleration: explicit/lazy boundaries, partial stores, shared task "
                     "yaw, actual heading and cross primitives passed.\n"
                     "Alternate acceleration and remaining rotation callees are unresolved; full "
                     "gameplay/hardware is unverified.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

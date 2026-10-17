#include "math/ninja_sine_table.hpp"
#include "player/player_scene_position_helpers.hpp"
#include <iostream>
#include <stdexcept>

using namespace sadx;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void check(std::uint16_t control) {
    X87Context fp(control);
    scene::Actor actor{{1, 4, 9}, {0, {0, 0, 0}}, 0};
    MotionWork world{{2, 5, 8}, {0, 0, 0}};
    scene::nudge_scene_normal(actor, world, {1, 2, -3}, fp);
    require(actor.position.x == 4 && actor.position.y == 10 && actor.position.z == 0 &&
                world.speed.x == 4 && world.speed.y == 5 && world.speed.z == 2,
            "normal push and horizontal impulse");

    PlayerMotion motion{{0, 0, 0}, {1, 0, 0}};
    NinjaVector center{20, 30, 40};
    const NinjaVector offset{2, 3, 4};
    TaskScheduler scheduler;
    const auto player = scheduler.append_task(0, TaskScheduler::source_no_op);
    require(static_cast<bool>(player), "native registered actor");
    std::array<TaskHandle, 8> players{};
    players[0] = player.task;
    scene::PlatformWork work{};
    actor.position = {1, 2, 3};
    bool called = false;
    work.rows[0].callback = [&](const TaskHandle &registered, scene::Actor &live,
                                scene::PlatformRow &row, X87Context &arithmetic) {
        require(registered == player.task && &arithmetic == &fp, "registered callback owners");
        require(live.position.x == 20 && live.position.z == 40, "temporary callback position");
        players[0].reset();
        require(registered == player.task, "callback receives a copied registry handle");
        called = true;
        live.position = {99, 98, 97};
        live.orientation.angles = {1000, 2000, 3000};
        live.player_index = 1;
        row.displacement = {1, 2, 3};
        row.angle_delta = {65536, -1, 131073};
    };
    work.rows[1].displacement = {100, 200, 300};
    scene::move_scene_platform(actor, motion, center, offset, &work, players, fp);
    require(called && actor.position.x == 1 && actor.position.y == 2 && actor.position.z == 3,
            "restore saved position only after callback completion");
    require(center.x == 21 && center.y == 32 && center.z == 43 &&
                actor.orientation.angles == std::array<std::int32_t, 3>{1000, 1999, 3001},
            "same live row after callback index change; wrapped angle additions");

    actor.player_index = 0;
    center = {20, 30, 40};
    work.rows[0].callback = [](const auto &, auto &, auto &, auto &) {
        throw std::runtime_error("controlled callback failure");
    };
    bool failed = false;
    try {
        scene::move_scene_platform(actor, motion, center, offset, &work, players, fp);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    require(failed && actor.position.x == 20 && actor.position.z == 40 && center.x == 20,
            "failed callback retains temporary position without displacement");

    work.rows[0].callback = {};
    work.rows[0].displacement = {1, 2, 3};
    work.rows[0].angle_delta = {0, 0, 0};
    motion.speed = {0, 0, 0};
    actor.position = {1, 2, 3};
    scene::move_scene_platform(actor, motion, center, offset, &work, players, fp);
    require(center.x == 4 && center.y == 7 && center.z == 10, "idle center plus live displacement");
    scene::move_scene_platform(actor, motion, center, offset, nullptr, players, fp);
    require(center.x == 4 && center.y == 7 && center.z == 10, "observed absent platform work");

    const NinjaMatrix unit{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    NinjaMatrixStack stack(std::vector<NinjaMatrix>(4, unit), 1, 0);
    PlayerCoordinates coordinates(ninja_sine_table(), unit, stack);
    PlayerParameters parameters{};
    parameters.jog_speed = 1;
    parameters.run_speed = 2;
    parameters.rush_speed = 3;
    parameters.dash_speed = 4;
    scene::FloorContact floor{{0, 1, 0}, {0, 1, 0}, 16384, -16384};
    actor.orientation = {0, {0, 0, 0}};
    const std::uint16_t grounded = 1;
    scene::rotate_scene_floor(actor, motion, grounded, parameters, floor, {0, -1, 0}, coordinates,
                              fp);
    require(actor.orientation.angles[0] == 1024 && actor.orientation.angles[2] == 64512 &&
                stack.depth() == 1,
            "native floor approach and slow-speed clamp");
    const std::uint16_t airborne = 0;
    scene::rotate_scene_floor(actor, motion, airborne, parameters, floor, {0, -1, 0}, coordinates,
                              fp);
    require(actor.orientation.angles[0] == 16384 && actor.orientation.angles[2] == -16384,
            "airborne floor copies full source angles");
}
} // namespace
int main() {
    try {
        for (unsigned precision : {0U, 512U, 768U})
            for (unsigned rounding : {0U, 1024U, 2048U, 3072U})
                check(static_cast<std::uint16_t>(precision | rounding | 127U));
        std::cout
            << "Native scene motion: push, live platform callbacks and floor turns; 12 controls.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

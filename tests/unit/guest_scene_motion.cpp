#include "math/ninja_sine_table.hpp"
#include "guest/player_scene_position_reference.hpp"
#include "player/player_scene_position_helpers.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace sadx;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class T> bool same_bits(T left, T right) {
    return std::memcmp(&left, &right, sizeof(T)) == 0;
}
void check(const PlayerScenePositionConstants &constants) {
    X87Context fp(0x7f);
    PlayerTaskState task;
    task.position = NinjaVector{1, 2, 3};
    PlayerContactWork contact;
    auto result = nudge_scene_normal(task, nullptr, contact, constants, fp);
    require(result.source_boundary == 0x43cb40 && task.position->x == 1,
            "normal unavailable first");
    contact.vector_at_3c = NinjaVector{1, 2, 3};
    result = nudge_scene_normal(task, nullptr, contact, constants, fp);
    require(result.failure == ScenePositionFailure::null_motion &&
                result.source_boundary == 0x43cb72 && task.position->x == 4 &&
                task.position->y == 8 && task.position->z == 12,
            "null motion retains preceding position stores");
    task.position = NinjaVector{1, 2, 3};
    contact.vector_at_3c = NinjaVector{1, std::numeric_limits<float>::max(), 3};
    X87Context overflow(0x77);
    bool trapped = false;
    try {
        (void)nudge_scene_normal(task, nullptr, contact, constants, overflow);
    } catch (const X87Error &) {
        trapped = true;
    }
    require(trapped && task.position->x == 4 && task.position->y == 2 && task.position->z == 3,
            "unmasked overflow retains the first position store");

    const NinjaMatrix unit{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    NinjaMatrixStack stack(std::vector<NinjaMatrix>(4, unit), 1, 0);
    PlayerCoordinates coordinates(ninja_sine_table(), unit, stack);
    PlayerPhysicsState player;
    PlayerParameters parameters{};
    contact.direction_at_1c = NinjaVector{0, 1, 0};
    contact.vector_at_48 = NinjaVector{0, 0, 0};
    contact.angle_x_at_34 = 123;
    task.orientation.angles = {9, 8, 7};
    result = rotate_scene_floor(task, player, parameters, contact, constants, NinjaVector{0, 1, 0},
                                coordinates, fp);
    require(result.source_boundary == 0x445094 && task.orientation.angles[0] == 123 &&
                task.orientation.angles[2] == 7,
            "ordered-less bypasses unknown flags; missing roll preserves pitch store");
    contact.vector_at_48 = NinjaVector{std::numeric_limits<float>::quiet_NaN(), 1, 0};
    result = rotate_scene_floor(task, player, parameters, contact, constants, NinjaVector{0, 1, 0},
                                coordinates, fp);
    require(result.source_boundary == 0x444f4a, "unordered alignment still reads player flags");

    TaskScheduler scheduler;
    const auto moving = scheduler.append_task(0, TaskScheduler::source_no_op);
    require(static_cast<bool>(moving), "moving task");
    player.moving_task_at_6c = moving.task;
    player.motion.speed = {std::numeric_limits<float>::quiet_NaN(), 0, 0};
    task.position.reset();
    contact.center_at_4.reset();
    X87Context untouched(0x7f);
    result = move_scene_platform(task, player, contact, {0, 0, 0}, {nullptr, nullptr}, {0, 1},
                                 untouched);
    require(static_cast<bool>(result) && untouched.exception_flags() == 0,
            "null force work does not consume speed, position, center or registry");

    TaskWorkArena arena;
    const auto allocation = arena.allocate(56);
    require(static_cast<bool>(allocation), "platform force work");
    moving.task->force_work = allocation.work;
    ScenePlatformWork work;
    std::unordered_map<const Task *, ScenePlatformWork *> views{{moving.task.get(), &work}};
    result =
        move_scene_platform(task, player, contact, {0, 0, 0}, {nullptr, &views}, {0, 1}, untouched);
    require(result.source_boundary == 0x43ca93 && untouched.exception_flags() == 0,
            "missing selected row stops before arithmetic");
    work.rows[0] = ScenePlatformRow{ScenePlatformCallback{nullptr}, {}, {}};
    player.motion.speed = {1, 0, 0};
    result = move_scene_platform(task, player, contact, {0, 0, 0}, {nullptr, &views}, {0, 1}, fp);
    require(result.source_boundary == 0x43caf0, "center checked before displacement");
    task.position = NinjaVector{1, 2, 3};
    contact.center_at_4 = NinjaVector{20, 30, 40};
    task.player_index = 0;
    work.rows[0]->callback =
        +[](const TaskHandle &, PlayerTaskState &, ScenePlatformRow &, X87Context &) {
            PlayerScenePositionResult failed;
            failed.failure = ScenePositionFailure::unavailable_field;
            failed.source_boundary = 0x43cb40;
            return failed;
        };
    result = move_scene_platform(task, player, contact, {0, 0, 0}, {nullptr, &views}, {0, 1}, fp);
    require(result.source_boundary == 0x43caca && task.position->x == 20 &&
                result.temporary_vector->x == 1,
            "unknown registry retains temporary callback position and saved diagnostic");
    PlayerStartupRegistry registry;
    registry.tasks[0] = TaskHandle{};
    task.position = NinjaVector{1, 2, 3};
    result = move_scene_platform(task, player, contact, {0, 0, 0}, {&registry, &views}, {0, 1}, fp);
    require(result.failure == ScenePositionFailure::platform_callback_failed &&
                result.source_boundary == 0x43cad4 && result.nested_source_boundary == 0x43cb40 &&
                result.platform_callback && task.position->x == 20 && contact.center_at_4->x == 20,
            "callback failure retains temporary pose, child result and original center");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 2, "usage: guest-scene-motion-test SCENE_CONSTANTS");
        std::ifstream file(argv[1], std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        const auto constants = decode_scene_position_constants(bytes.data(), bytes.size());
        require(constants.has_value(), "source scene constant pack");
        const scene::MotionConstants native;
        require(same_bits(native.push_distance, constants->single(0x7dccd8)) &&
                    same_bits(native.zero, constants->single(0x7dcb2c)) &&
                    same_bits(native.angle_units, constants->wide(0x7dccd0)) &&
                    same_bits(native.radians_to_turns, constants->wide(0x7dccc8)),
                "native literals exactly equal source encodings");
        check(*constants);
        std::cout << "Scene motion reference: lazy facts, partial stores, callback diagnostics and "
                     "literals.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

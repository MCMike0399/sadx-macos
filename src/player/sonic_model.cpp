#include "player/sonic_model.hpp"
#include "player/player_initialization.hpp"
#include <stdexcept>

namespace sadx::sonic_model {
void initialize_prefix(State &state, character_pack::ObjectState &objects, ObjectId attach,
                       ObjectId root, Player player, bool alternate, Calls &calls) {
    if (!alternate) {
        auto &object = objects.object(root);
        if (object.sibling != attach) {
            object.sibling = attach;
            player.joins = &state.joins;
            // Reread the live object after publication, just as the source does.
            state.joins.rows.at(11).destination_object =
                &objects.object(objects.object(root).sibling);
            player.join_mode = 0;
            calls.join_vertices();
            player.joins = &state.joins;
        }
    }
    // Metal skips only the splice above. Both paths read the same live slot six.
    state.model_sibling = objects.object(root).sibling;
    calls.initialize_collision();
    // CCL_Init's return is ignored. Its failed allocation must still fault here.
    calls.store_collision_height();
    player.mode = 1;
    player.idle_timer = 0;
}
void initialize_resources(State &state, InitializationCalls &calls) {
    calls.load_textures(0);
    if (calls.metal())
        calls.load_textures(8);
    calls.clear_free_word(0);
    calls.clear_free_word(1);
    calls.load_collision_task(calls.player_index()); // The source ignores failure.
    calls.set_motion();
    const auto metal = calls.metal();
    auto &table = metal ? state.metal_joins : state.joins;
    const auto stage = static_cast<std::uint32_t>(static_cast<std::int32_t>(calls.stage()));
    const auto act = calls.act();
    constexpr std::uint32_t kStageMask = 0xff00;
    constexpr std::uint32_t kFinalEgg = 0x1300;
    const bool final_egg = (((stage << 8) | act) & kStageMask) == kFinalEgg;
    for (std::size_t row = 0; table.rows.at(row).object; ++row) {
        auto &join = table.rows.at(row);
        if (join.object == calls.join_target())
            join.input_mode = final_egg ? 2 : 0;
    }
    calls.publish_join_table(table);
    calls.join_vertices();
    if (!calls.metal()) {
        calls.create_effect(2);
        calls.create_effect(3);
    }
    calls.install_callbacks();
    calls.set_motion(); // Common tail: the source calls PSetMotion a second time.
}
void update_common_model_links(character_pack::ObjectState &objects, CommonTailCalls &calls) {
    constexpr std::uint16_t kSkipModelJoin = 0x100;
    constexpr std::uint16_t kLightShoes = 2, kCrystalRing = 1; // Upgrades
    if (calls.status() & kSkipModelJoin)
        return;
    if (!calls.metal()) {
        const auto equipment = calls.equipment();
        if (equipment & kLightShoes) {
            const auto root = calls.model_slot(19);
            const auto sibling = calls.model_slot(58);
            objects.object(root).sibling = sibling;
        } else {
            const auto sibling = calls.model_slot(20);
            const auto root = calls.model_slot(19);
            objects.object(root).sibling = sibling;
        }
        const auto sibling = calls.model_slot((equipment & kLightShoes) ? 60 : 15);
        const auto root = calls.model_slot(14);
        objects.object(root).sibling = sibling;
        const auto upgraded = (calls.equipment() & kCrystalRing) != 0;
        const auto hand = calls.model_slot(62);
        const auto held = calls.model_slot(upgraded ? 63 : 3);
        objects.object(hand).sibling = held;
    }
    calls.join_vertices();
}
TaskHandle create_initialization_task(TaskScheduler &scheduler, unsigned flags, int level,
                                      TaskCallback callback) {
    const auto created = scheduler.create_element(flags, level, callback);
    if (created)
        return created.task;
    const bool work_exhausted = created.failure == TaskFailure::WorkFailure &&
                                created.work_failure == TaskWorkFailure::Exhausted &&
                                (created.step == TaskStep::PrimaryInitialization ||
                                 created.step == TaskStep::MotionInitialization) &&
                                created.task && created.task->retired();
    if (created.failure == TaskFailure::SourcePoolExhausted || work_exhausted)
        return {};
    TaskScheduler::raise_callback_failure(created);
}
bool load_collision_task(const std::array<ObjectId, 3> &objects, std::uint8_t player,
                         TaskScheduler &scheduler, TaskWorkArena &arena) {
    const auto task = create_initialization_task(scheduler, 2, 2, collision_task);
    if (!task)
        return false;
    const auto allocated = arena.allocate(136);
    if (!allocated) {
        if (allocated.failure != TaskWorkFailure::Exhausted)
            throw std::runtime_error("sonic_collision_motion_allocation");
        const auto freed = scheduler.free_task(task);
        if (!freed)
            TaskScheduler::raise_callback_failure(freed);
        return false;
    }
    // Publication precedes the primary index and individual object reads.
    task->motion_work = allocated.work;
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::bind_primary_view(task).player_index = player;
    auto work = std::make_shared<CollisionTaskWork>();
    work->allocation = allocated.work;
    task->auxiliary_native_work = work;
    for (std::size_t index = 0; index < work->objects.size(); ++index)
        work->objects[index] = objects[index];
    return true;
}
} // namespace sadx::sonic_model

#include "engine/collision_component.hpp"
#include "engine/character_pack.hpp"
#include "engine/game_frame_control.hpp"
#include "engine/sprite_registry.hpp"
#include "player/player_initialization.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_player_system.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
template <class T> void require(const T &value, const char *message) {
    if (!static_cast<bool>(value))
        throw std::runtime_error(message);
}

int child_runs = 0;
void record_child(sadx::TaskScheduler &, const sadx::TaskHandle &) { ++child_runs; }

using Bytes = std::vector<std::uint8_t>;
void put(Bytes &bytes, std::size_t offset, std::uint32_t value) {
    for (std::size_t byte = 0; byte < 4; ++byte)
        bytes.at(offset + byte) = static_cast<std::uint8_t>(value >> (byte * 8));
}
void append(Bytes &bytes, std::uint32_t value) {
    const auto end = bytes.size();
    bytes.resize(end + 4);
    put(bytes, end, value);
}
void add_record(Bytes &bytes, sadx::character_pack::ResourceKind kind, const Bytes &payload) {
    append(bytes, static_cast<std::uint32_t>(kind));
    append(bytes, static_cast<std::uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}
// Same synthetic SDXACT01 graph the character-pack unit already accepts.
Bytes minimal_pack() {
    using sadx::character_pack::ResourceKind;
    Bytes bytes{'S', 'D', 'X', 'A', 'C', 'T', '0', '1'};
    for (auto value : {5, 2, 100, 1, 101, 4})
        append(bytes, value);
    Bytes action(8);
    put(action, 0, 2);
    put(action, 4, 3);
    add_record(bytes, ResourceKind::Action, action);
    add_record(bytes, ResourceKind::Object, Bytes(52));
    Bytes motion(16);
    put(motion, 0, 5);
    put(motion, 4, 1);
    put(motion, 8, 3 + (2 << 16));
    put(motion, 12, 1);
    add_record(bytes, ResourceKind::Motion, motion);
    add_record(bytes, ResourceKind::Action, action);
    Bytes tracks(20);
    put(tracks, 0, 1);
    add_record(bytes, ResourceKind::Tracks, tracks);
    return bytes;
}

struct Fixture {
    std::shared_ptr<sadx::TaskWorkArena> arena = std::make_shared<sadx::TaskWorkArena>();
    sadx::TaskScheduler scheduler;
    sadx::native::PlayerSystem players;
    std::int32_t card_result = 0;
    std::uint32_t global_timer = 0;
    std::int16_t rings = 0;
    std::uint32_t movie_number = 0;
    std::uint16_t game_mode = 0;
    sadx::bootstrap::State bootstrap{{card_result, global_timer, rings, movie_number, game_mode}};
    sadx::frame::ModeState timing{};
    sadx::native::Display display{{}, timing};
    sadx::TaskHandle task;
    std::int16_t stage = 0;
    std::int16_t act = 0;
    sadx::sprite_registry::State sprites{};
    sadx::character_pack::CharacterPack pack = sadx::character_pack::CharacterPack::decode(minimal_pack());
    sadx::character_pack::SceneState scene{pack};
    sadx::regular_textures::CharacterCatalog catalog{nullptr, nullptr, 0};
    sadx::gamma_selection::Context context{players, scheduler, bootstrap, display, task, stage, act,
                                           nullptr, nullptr, nullptr, catalog, nullptr, nullptr,
                                           sprites, scene, nullptr, nullptr, nullptr, {}, nullptr};
    Fixture() {
        require(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}), "source pool");
        const auto created = scheduler.create_element(7, 3, nullptr);
        require(created, "gamma task");
        task = created.task;
        sadx::player_initialization::establish_zeroed_primary(scheduler, task);
    }
};

void collision_range_uses_the_existing_owner() {
    Fixture world;
    sadx::collision_component::Info info;
    const std::vector<sadx::collision_component::Info> infos{info};
    auto &status = sadx::player_initialization::bind_primary_view(world.task).status;
    require(sadx::collision_component::initialize(*world.arena, world.task, status, infos, 1, 0,
                                                 world.display.arithmetic()) ==
                sadx::collision_component::InitResult::Initialized,
            "collision init");
    auto component = sadx::collision_component::component(*world.arena, world.task->primary_work);
    require(component && component->range == 25.0f, "empty sphere keeps the source floor");
    component->info[0].center = {3, 4, 0};
    component->info[0].a = 2;
    sadx::collision_component::calculate_range(component.get(), world.display.arithmetic());
    const float expected = component->range;
    component->range = 0;
    sadx::gamma_selection::calculate_collision_range(
        world.context, sadx::player_initialization::bind_primary_view(world.task));
    require(expected > 7.06f && expected < 7.08f && component->range == expected,
            "gamma binding stores CCL_CalcRange");
}

void child_dispatch_uses_the_scheduler() {
    Fixture world;
    const auto child = world.scheduler.append_child(world.task, &record_child, {});
    require(child, "child task");
    sadx::gamma_selection::run_children(world.context, world.task);
    sadx::gamma_selection::run_children(world.context, world.task);
    require(child_runs == 2, "both updates dispatch the same child");

    const auto parent = world.scheduler.append_task(0, nullptr);
    require(static_cast<bool>(parent), "parent");
    const auto missing = world.scheduler.append_child(parent.task, nullptr, {});
    require(static_cast<bool>(missing), "null child");
    bool failed = false;
    try {
        sadx::gamma_selection::run_children(world.context, parent.task);
    } catch (const std::runtime_error &error) {
        failed = std::string(error.what()) == "native gamma child dispatch failed";
    }
    require(failed && child_runs == 2, "null child stays a dispatch failure");
}
} // namespace

int main() {
    collision_range_uses_the_existing_owner();
    child_dispatch_uses_the_scheduler();
    std::cout << "gamma common-tail binding: collision range and child dispatch\n";
}

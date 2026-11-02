#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "game/game_mode.hpp"
#include <cassert>
#include <iostream>

namespace {
void unrecovered(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("unexpected actor callback");
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 2);
    const std::filesystem::path directory(argv[1]);
    const auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    native::PlayerSystem players;
    players.gamma_assets = std::make_unique<native::GammaActorAssets>(
        native::app::load_gamma_actor_assets(directory, pack));
    character_pack::SceneState scene(pack);
    native::bind_gamma_resources(players, scene, *players.gamma_assets);
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    const auto task = scheduler.create_element(7, 3, nullptr).task;
    assert(task);
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    auto pose = player_initialization::bind_primary_view(task);
    pose.id = 2; pose.player_index = 5; pose.status = 0x14;
    pose.position = {13, 27, -9}; pose.rotation = {111, 222, 333};
    auto primary_owner = task->primary_state;
    auto motion_owner = task->motion_state;
    std::int32_t card = 0;
    std::uint32_t timer = 0, movie = 0;
    std::int16_t rings = 0;
    std::uint16_t game_mode = 0;
    bootstrap::State bootstrap({card, timer, rings, movie, game_mode});
    bootstrap.event_work[393] = bootstrap.event_work[465] = 1;
    bootstrap.frames = 17;
    frame::ModeState timing;
    native::Display display(directory / "system", timing);
    auto texture_assets = native::app::load_assets(directory).character_textures;
    std::array<std::vector<regular_textures::List>, 9> lists;
    for (unsigned character = 0; character < lists.size(); ++character) {
        lists[character].resize(texture_assets[character].size());
        for (unsigned i = 0; i < lists[character].size(); ++i)
            lists[character][i].entries.resize(texture_assets[character][i].texture_count);
    }
    std::array<std::int8_t, 9> loaded{};
    native::CharacterTextureViews views(texture_assets, lists);
    std::uint8_t resync = 0;
    const std::int32_t language = 1;
    regular_textures::List *common = nullptr;
    std::array<regular_textures::List *, 32> regular{};
    const regular_textures::Localizations localizations{};
    regular_textures::State texture_state{resync, language, common, regular, localizations, {}, {}};
    regular_textures::Services services{
        [&](std::string_view name) {
            return regular_textures::ArchiveResult{1, display.output().load_pvm_archive(name)};
        },
        [&](regular_textures::List &list) { return display.output().load_individual_textures(list); },
        [](std::string_view message) { throw std::runtime_error(std::string(message)); }};
    bool missing_rejected = false;
    try { (void)native::app::load_gamma_effects_definition(directory / "missing-assets"); }
    catch (const std::runtime_error &) { missing_rejected = true; }
    assert(missing_rejected);
    const auto setup = native::app::load_gamma_setup_definition(directory);
    const auto effects = native::app::load_gamma_effects_definition(directory);
    assert(setup && !effects.empty());
    native::bind_gamma_setup_resources(players, texture_assets, lists, *setup);
    auto invalid = texture_assets;
    invalid[6][4].resource_name = "wrong_hit_identity";
    bool rejected = false;
    try { native::bind_gamma_effects_resources(players, invalid, lists, effects); }
    catch (const std::runtime_error &) { rejected = true; }
    assert(rejected && !players.gamma_effects_asset);
    native::bind_gamma_effects_resources(players, texture_assets, lists, effects);
    auto &asset = *players.gamma_effects_asset;
    assert(asset.textures == &lists[6][4] && asset.textures != &lists[6][2]);
    const auto *stable = players.gamma_effects_asset.get();
    rejected = false;
    try { native::bind_gamma_effects_resources(players, texture_assets, lists, effects); }
    catch (const std::runtime_error &) { rejected = true; }
    assert(rejected && players.gamma_effects_asset.get() == stable);
    const std::int16_t stage = 0, act = 0;
    sprite_registry::State sprites;
    gamma_selection::Context context{players, scheduler, bootstrap, display, task, stage, act,
        unrecovered, unrecovered, unrecovered, views.catalog(loaded), &texture_state, &services, sprites, scene};
    bool stopped = false;
    try { gamma_selection::run(context, task); }
    catch (const flow::UnrecoveredGameFlowCall &error) {
        assert(std::string(error.what()) == "native_gamma_set_motion");
        stopped = true;
    }
    assert(stopped && players.gamma_effects.burst_task && pose.status == 1);
    const auto pool_task = players.gamma_effects.burst_task;
    assert(scheduler.list_head(4) == pool_task);
    const auto work = std::static_pointer_cast<particles::Work>(pool_task->auxiliary_native_work);
    assert(work->allocation == pool_task->auxiliary_work && work->frames == asset.burst.data() &&
           work->capacity == 40 && work->active_count == 0);
    assert(asset.hit[0].global_index == lists[6][4].entries[1]->global_index &&
           asset.burst[0].global_index == lists[6][4].entries[0]->global_index);
    assert(lists[6][4].entries[1] != lists[6][2].entries[1] &&
           lists[6][4].entries[1]->global_index != lists[6][2].entries[1]->global_index);
    assert(sprites.head == players.gamma_setup.sprite && sprites.count == 1);
    assert(players.registry.tasks[5] == task && task->primary_state == primary_owner &&
           task->motion_state == motion_owner && players.current.primary == task->primary_work);
    auto &memory = *lists[6][4].entries[1];
    const auto old_index = memory.global_index;
    memory.global_index = 193751;
    const auto old_burst = asset.burst[0].global_index;
    gamma_selection::initialize_effects(context);
    assert(asset.hit[0].global_index == 193751 && players.gamma_effects.burst_task == pool_task &&
           asset.burst[0].global_index == old_burst && sprites.count == 1);
    memory.global_index = old_index;
    // The frame declaration itself remains live, including a changed source index.
    asset.hit[0].texture_index = 2;
    gamma_selection::initialize_effects(context);
    assert(asset.hit[0].global_index == lists[6][4].entries[2]->global_index);
    std::cout << "Real Gamma/PInitialize/setup/material caller reaches PSetMotion with shared hit list and pool\n";
}

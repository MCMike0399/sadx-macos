#include "apps/native_gamma_actor_assets.hpp"
#include "game/game_mode.hpp"
#include "player/player_vertex_weld.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_input.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_gamma_display_context.hpp"
#include "runtime/native_event_actions_context.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
void unrecovered(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("unexpected actor callback");
}
} // namespace
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 2);
    const std::filesystem::path directory(argv[1]);
    const auto pack =
        character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    native::PlayerSystem players;
    players.gamma_assets = std::make_unique<native::GammaActorAssets>(
        native::app::load_gamma_actor_assets(directory, pack));
    players.gamma_motion_assets =
        std::make_unique<native::GammaMotionAssets>(pack, *players.gamma_assets);
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
    pose.id = 2;
    pose.player_index = 5;
    pose.status = 0x14;
    pose.position = {13, 27, -9};
    pose.rotation = {111, 222, 333};
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
        [&](regular_textures::List &list) {
            return display.output().load_individual_textures(list);
        },
        [](std::string_view message) { throw std::runtime_error(std::string(message)); }};
    const auto setup = native::app::load_gamma_setup_definition(directory);
    const auto effects = native::app::load_gamma_effects_definition(directory);
    assert(setup && !effects.empty());
    native::bind_gamma_setup_resources(players, texture_assets, lists, *setup);
    native::bind_gamma_effects_resources(players, texture_assets, lists, effects);
    auto &asset = *players.gamma_effects_asset;
    assert(asset.textures == &lists[6][4] && asset.textures != &lists[6][2]);
    const std::int16_t stage = 0, act = 0;
    sprite_registry::State sprites;
    gamma_selection::Context context{players,
                                     scheduler,
                                     bootstrap,
                                     display,
                                     task,
                                     stage,
                                     act,
                                     unrecovered,
                                     unrecovered,
                                     unrecovered,
                                     views.catalog(loaded),
                                     &texture_state,
                                     &services,
                                     sprites,
                                     scene};
    std::string input_error;
    auto input = native::Input::create(native::app::load_assets(directory).input, display.sse(),
                                       input_error);
    assert(input && input_error.empty());
    auto &analogs = input->normalized_analogs();
    context.normalized_input = &analogs;
    for (unsigned i = 0; i < analogs.size(); ++i)
        analogs[i] = {static_cast<std::int32_t>(900 + i), 0.125f * (i + 1)};
    model_matrices::State matrices;
    context.model_matrices = &matrices;
    const auto arithmetic_control = display.arithmetic().control_word();
    std::int32_t selected_palette = -1;
    context.display_palette = [&](std::int32_t value) { selected_palette = value; };
    // Compose a real event allocation before the constructor. The source display
    // dereferences eventwk even when its action list is empty.
    event_actions::State event_state;
    const auto fields = native::event_primary_fields();
    native::EventActionsContext event(*arena, event_state, players.registry, pack, fields);
    event_actions::initialize(event.core, task->primary_work);
    bool stopped = false;
    try {
        gamma_selection::run(context, task);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        assert(std::string(error.what()) == "native_gamma_display_action_clip_no_scale");
        stopped = true;
    }
    assert(stopped && selected_palette == 4);
    assert(players.current.primary == task->primary_work && !players.current.player);
    const auto player = players.registry.players[5];
    assert(player && task->primary_state == primary_owner && task->motion_state == motion_owner);
    auto &assets = *players.gamma_assets;
    auto &objects = scene.objects;
    const auto row = [&](unsigned index) { return assets.display_objects.at(index); };
    assert(objects.object(row(22)).sibling == row(14));
    assert(objects.object(row(23)).sibling == row(18));
    assert(players.gamma_weld.rows[0].source_object ==
           &objects.object(objects.object(row(14)).child));
    const auto collision = collision_component::component(*arena, task->primary_work);
    assert(collision && collision->info.size() == 3);
    assert(collision->info[0].center.x == pose.position.x);
    assert(collision->info[0].center.y == pose.position.y);
    assert(collision->info[0].center.z == pose.position.z);
    assert(collision->info[1].center.x == pose.position.x);
    assert(collision->info[1].center.y == pose.position.y + 5.0f);
    assert(collision->info[1].center.z == pose.position.z);
    gamma_display::Context inspection(context);
    const auto render = inspection.render_state();
    assert(render.control == 16 &&
           render.saved.object_callback == render_snapshot::ObjectCallback::Gamma);
    const auto allocation = gamma_selection::extra_work(context, player);
    assert(players.gamma.current_extra == allocation);
    std::uint32_t phase = 999;
    assert(arena->read(allocation, 44, &phase, 4) && phase == 0);
    // Reached display remains independent of current-player publications.
    const auto foreign = arena->allocate(8).work;
    players.current.player = foreign;
    player->physics->equipment_at_4 = 512;
    event_actions::append_action(event.core, task->primary_work, assets.stand,
                                &lists[6][0], 1.0f, 1, 0);
    bool event_stopped = false;
    try {
        gamma_selection::display(context, task);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        assert(std::string(error.what()) == "native_gamma_display_event_animation");
        event_stopped = true;
    }
    assert(event_stopped && players.current.player == foreign);
    assert(objects.object(row(22)).sibling == row(16));
    assert(objects.object(row(23)).sibling == row(20));
    assert(players.gamma_weld.rows[0].source_object ==
           &objects.object(objects.object(row(16)).child));
    // Missing later imported content is explicit and retains the first sibling store.
    const auto saved = assets.display_objects.extract(18);
    player->physics->equipment_at_4 = 0;
    bool asset_stopped = false;
    try { gamma_selection::display(context, task); }
    catch (const flow::UnrecoveredGameFlowCall &error) {
        asset_stopped = std::string(error.what()) == "native_gamma_display_object_row_18";
    }
    assert(asset_stopped && objects.object(row(22)).sibling == row(14));
    assert(objects.object(row(23)).sibling == row(20));
    assert(players.current.player == foreign && display.arithmetic().control_word() == arithmetic_control);
    (void)saved;
    std::cout << "Actual Gamma constructor/display composition, shared siblings/weld/collision, "
                 "passed player, event and missing-asset prefixes checked\n";
}

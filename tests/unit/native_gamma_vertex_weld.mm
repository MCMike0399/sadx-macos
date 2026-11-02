#include "apps/native_gamma_actor_assets.hpp"
#include "game/game_mode.hpp"
#include "player/player_vertex_weld.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_player_system.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

namespace {
void unrecovered(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("unexpected actor callback");
}
} // namespace
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 3);
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
    const auto arithmetic_control = display.arithmetic().control_word();
    bool stopped = false;
    try {
        gamma_selection::run(context, task);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        assert(std::string(error.what()) == "native_gamma_normalized_input_owner");
        stopped = true;
    }
    assert(stopped && players.gamma_effects.burst_task && pose.status == 1);
    assert(display.arithmetic().control_word() == arithmetic_control);
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
    const auto player = players.registry.players[5];
    assert(player && !players.current.player); // Entry captured NULL before PInitialize.
    auto &state = *player->physics;
    auto &tail = state.animation_tail;
    assert(state.join_vertex_mode_at_122 == 1);
    assert(tail.join_table && *tail.join_table == &players.gamma_weld);
    const auto parent = std::filesystem::path(argv[2]).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);
    std::ofstream observations(argv[2]);
    assert(observations);
    for (character_pack::ResourceId id = 1; id <= pack.record_count(); ++id) {
        if (pack.record(id).kind != character_pack::ResourceKind::Object)
            continue;
        const auto &object = scene.objects.object(id);
        observations << "object " << id << ' ' << object.evaluation_flags << ' ' << object.child
                     << ' ' << object.sibling << '\n';
    }
    observations << "mode " << *state.join_vertex_mode_at_122 << '\n';
    // Original PJoinVertexes over this actual imported graph: SOURCE-EVIDENCE
    // and the independent imported-topology comparison retain the source result.
    const std::array<std::uint8_t, 5> expected_depths{37, 46, 20, 8, 0};
    for (unsigned index = 0; index < players.gamma_weld.rows.size(); ++index) {
        const auto &row = players.gamma_weld.rows[index];
        const auto &declaration = players.gamma_assets->joins[index];
        assert(row.object ==
               (declaration.object ? &scene.objects.object(declaration.object) : nullptr));
        assert(row.source_object == (declaration.source_object
                                         ? &scene.objects.object(declaration.source_object)
                                         : nullptr));
        assert(!row.destination_object && !row.original_vertices && !row.vertex_pairs);
        assert(row.vertex_count == declaration.vertex_count &&
               row.input_mode == declaration.input_mode);
        assert(row.destination_depth == 0);
        assert(row.source_depth == expected_depths[index]);
        observations << "row " << declaration.object << ' ' << declaration.source_object << ' '
                     << declaration.destination_object << ' ' << unsigned(row.vertex_count) << ' '
                     << int(row.input_mode) << ' ' << unsigned(row.source_depth) << ' '
                     << unsigned(row.destination_depth) << '\n';
    }
    observations.close();
    // Source mode 4 overwrites an existing original pointer without freeing it.
    // This controlled reentry follows the real table, arena and passed player.
    auto previous = arena->allocate(96).work;
    assert(previous);
    players.gamma_weld.rows[0].original_vertices = previous;
    state.join_vertex_mode_at_122 = 0;
    gamma_selection::join_vertices(context, pose, task->motion_work, player);
    assert(state.join_vertex_mode_at_122 == 1 && !previous->released());
    assert(!players.gamma_weld.rows[0].original_vertices && !players.current.player);
    // Active processing must fail before a sentinel could hide missing matrices.
    const auto saved = tail.join_table;
    tail.join_table.reset();
    bool matrix_missing = false;
    try {
        gamma_selection::join_vertices(context, pose, task->motion_work, player);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        matrix_missing = std::string(error.what()) == "native_gamma_join_vertices_CalcMMMatrix";
    }
    assert(matrix_missing && !tail.join_table);
    tail.join_table = saved;
    assert(players.current.primary == task->primary_work && !players.current.player);
    std::cout
        << "Actual Gamma common tail initializes canonical weld nodes and reaches laser targets; "
           "captured null alias and active matrix boundary preserved\n";
}

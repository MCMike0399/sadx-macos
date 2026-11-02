#include "apps/native_gamma_actor_assets.hpp"
#include "game/game_mode.hpp"
#include "player/player_vertex_weld.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_player_system.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
#include "runtime/native_input.hpp"

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
    auto input = native::Input::create(native::app::load_assets(directory).input,
                                       display.sse(), input_error);
    assert(input && input_error.empty());
    auto &analogs = input->normalized_analogs();
    context.normalized_input = &analogs;
    for (unsigned i = 0; i < analogs.size(); ++i)
        analogs[i] = {static_cast<std::int32_t>(900 + i), 0.125f * (i + 1)};
    const auto before_input = analogs;
    const auto arithmetic_control = display.arithmetic().control_word();
    bool stopped = false;
    try {
        gamma_selection::run(context, task);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        assert(std::string(error.what()) == "native_gamma_get_model_matrix");
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
    // The actual first callback reaches restoration after CAlloc. It must
    // overwrite only the selected player's angle and stroke with source zeros.
    assert(input->analog_snapshot(5).angle == 0 && input->analog_snapshot(5).magnitude == 0);
    for (unsigned i = 0; i < analogs.size(); ++i) {
        if (i != 5)
            assert(analogs[i].angle == before_input[i].angle &&
                   analogs[i].magnitude == before_input[i].magnitude);
    }
    const auto allocation = gamma_selection::extra_work(context, player);
    assert(allocation && allocation == players.gamma_work.front().allocation);
    std::array<std::uint32_t, 145> extra_before{}, extra_after{};
    assert(arena->read(allocation, 0, extra_before.data(), 580));
    for (unsigned i = 125; i < 141; ++i)
        assert(extra_before[i] == 0); // all eight source input_buf entries
    const std::uint32_t angle = 0x87654321, stroke = 0x7f812345;
    assert(arena->write(allocation, 556, &angle, 4));
    assert(arena->write(allocation, 560, &stroke, 4));
    assert(arena->read(allocation, 0, extra_before.data(), 580));
    const auto previous_current = players.gamma.current_extra;
    players.gamma.current_extra = arena->allocate(580).work;
    assert(players.gamma.current_extra != allocation);
    gamma_selection::update_laser_targets(context, allocation, 2);
    assert(static_cast<std::uint32_t>(input->analog_snapshot(2).angle) == angle);
    std::uint32_t actual_stroke;
    std::memcpy(&actual_stroke, &analogs[2].magnitude, 4);
    assert(actual_stroke == stroke);
    assert(arena->read(allocation, 0, extra_after.data(), 580));
    assert(extra_before == extra_after && !players.current.player);
    players.gamma.current_extra = previous_current;
    // A source stroke read failure keeps the earlier angle publication.
    const auto short_work = arena->allocate(560).work;
    const std::uint32_t partial_angle = 0x12345678;
    assert(arena->write(short_work, 556, &partial_angle, 4));
    arena->set_payload_transport([&](std::uint32_t at, void *out, std::uint32_t bytes) {
        if (at == short_work->data_offset() + 560)
            return false;
        std::memcpy(out, arena->source_bytes().data() + at, bytes);
        return true;
    }, {});
    analogs[7] = {-77, -0.75f};
    bool stroke_failed = false;
    try {
        gamma_selection::update_laser_targets(context, short_work, 7);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        stroke_failed = std::string(error.what()) == "native_gamma_buffered_input_stroke";
    }
    assert(stroke_failed && analogs[7].angle == 0x12345678 && analogs[7].magnitude == -0.75f);
    arena->set_payload_transport({}, {});
    bool angle_failed = false;
    try {
        gamma_selection::update_laser_targets(context, {}, 7);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        angle_failed = std::string(error.what()) == "native_gamma_buffered_input_angle";
    }
    assert(angle_failed && analogs[7].angle == 0x12345678 && analogs[7].magnitude == -0.75f);
    context.normalized_input = nullptr;
    bool owner_failed = false;
    try {
        gamma_selection::update_laser_targets(context, allocation, 7);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        owner_failed = std::string(error.what()) == "native_gamma_normalized_input_owner";
    }
    assert(owner_failed && analogs[7].angle == 0x12345678 && analogs[7].magnitude == -0.75f);
    context.normalized_input = &analogs;
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
        << "Actual Gamma common tail restores the canonical input owner and reaches model matrices; "
           "captured null alias and active matrix boundary preserved\n";
}

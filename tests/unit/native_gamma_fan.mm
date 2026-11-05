#include "apps/native_gamma_actor_assets.hpp"
#include "game/game_mode.hpp"
#include "player/player_vertex_weld.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_input.hpp"
#include "runtime/native_player_system.hpp"
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
    const auto before_input = analogs;
    model_matrices::State matrices;
    context.model_matrices = &matrices;
    const auto arithmetic_control = display.arithmetic().control_word();
    // The source common tail now reaches the admitted display prefix; palette
    // selection is the caller's borrowed lighting operation.
    std::int32_t selected_palette = -1;
    context.display_palette = [&](std::int32_t value) { selected_palette = value; };
    bool stopped = false;
    try {
        gamma_selection::run(context, task);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        // The canonical imported pack has no Gamma display object rows, so the
        // display stops at the first tagged missing row. The append-only
        // development pack continues to the first animated draw seam.
        const auto message = std::string(error.what());
        const bool complete_pack = !players.gamma_assets->display_objects.empty();
        // A complete pack continues into the draw chain; the reached draw boundary
        // is named and may move as more of the body is recovered.
        const bool draw_boundary = message.rfind("native_gamma_display_", 0) == 0 &&
                                   message != "native_gamma_display_object_row_14";
        assert(complete_pack ? draw_boundary : message == "native_gamma_display_object_row_14");
        stopped = true;
    }
    const bool complete_pack = !players.gamma_assets->display_objects.empty();
    assert(stopped && selected_palette == (complete_pack ? 4 : -1) &&
           players.gamma_effects.burst_task && pose.status == 1);
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
    std::array<std::uint32_t, 145> first{}, before{}, after{};
    assert(arena->read(allocation, 0, first.data(), 580));
    // Original njSin uses its pinned table: 0.19508999586105347, not host sin().
    const auto bits = [](float value) {
        std::uint32_t result;
        std::memcpy(&result, &value, 4);
        return result;
    };
    assert(first[7] == 2048 && first[6] == 1044891052U && first[10] == 0);
    assert(matrices.count == 0 && matrices.limit == 0);
    for (const auto &matrix : matrices.matrices)
        for (const auto value : matrix)
            assert(bits(value) == 0);
    const std::array<const NinjaMatrix *, 5> destinations = {
        &players.gamma.node54_matrix, &players.gamma.node30_matrix, &players.gamma.node39_matrix,
        &players.gamma.node2_matrix, &players.gamma.node37_matrix};
    for (unsigned node = 0; node < destinations.size(); ++node)
        for (unsigned component = 0; component < 16; ++component)
            assert(bits((*destinations[node])[component]) ==
                   ((node == 2 && component >= 4 && component < 8) ? 0x80000000U : 0U));
    assert(!players.current.player && players.gamma.current_extra == allocation);
    // Nonzero speeds use the passed initialized player even with a null current alias.
    state.motion.speed = {1.5f, 123.0f, 0.25f};
    const auto original_primary = pose.position;
    std::uint32_t phase = 0xfffffff0;
    assert(arena->write(allocation, 28, &phase, 4));
    assert(arena->read(allocation, 0, before.data(), 580));
    const auto foreign = arena->allocate(580).work;
    assert(arena->fill(foreign, 0, 165, 580));
    players.gamma.current_extra = foreign;
    gamma_selection::update_fan(context, pose, player, allocation);
    assert(arena->read(allocation, 0, after.data(), 580));
    assert(after[7] == 3738 && after[10] == 2730);
    for (unsigned i = 0; i < 145; ++i)
        if (i != 6 && i != 7 && i != 10)
            assert(before[i] == after[i]);
    assert(after[6] != before[6] && pose.position.x == original_primary.x);
    assert(state.motion.speed.x == 1.5f && state.motion.speed.y == 123.0f &&
           state.motion.speed.z == 0.25f);
    assert(!players.current.player && players.gamma.current_extra == foreign);
    std::array<std::uint32_t, 145> foreign_words{};
    assert(arena->read(foreign, 0, foreign_words.data(), 580));
    for (auto word : foreign_words)
        assert(word == 0xa5a5a5a5U);
    // Reentry consumes the prior frame's actual stored phase, not an initial mirror.
    before = after;
    gamma_selection::update_fan(context, pose, player, allocation);
    assert(arena->read(allocation, 0, after.data(), 580));
    assert(after[7] == before[7] + 3754 && after[10] == 2730);
    // Failure after the phase publication must retain that prefix.
    const auto prior_phase = after[7];
    const auto prior_height = after[6];
    const auto prior_tilt = after[10];
    arena->set_payload_transport({}, [&](std::uint32_t at, const void *, std::uint32_t) {
        return at != allocation->data_offset() + 24;
    });
    bool stopped_height = false;
    try {
        gamma_selection::update_fan(context, pose, player, allocation);
    } catch (const flow::UnrecoveredGameFlowCall &e) {
        stopped_height = std::string(e.what()) == "native_gamma_fan_height_store";
    }
    assert(stopped_height);
    arena->set_payload_transport({}, {});
    assert(arena->read(allocation, 0, after.data(), 580));
    assert(after[7] == prior_phase + 3754 && after[6] == prior_height && after[10] == prior_tilt);
    // A store observer changes live speed. The source reloads X after njSin;
    // it must still use the originally passed allocation for all three stores.
    state.motion.speed = {1.5f, 123.0f, 0.25f};
    phase = 0;
    assert(arena->write(allocation, 28, &phase, 4));
    bool observed_phase = false;
    arena->set_payload_transport({}, [&](std::uint32_t at, const void *, std::uint32_t) {
        if (at == allocation->data_offset() + 28) {
            observed_phase = true;
            state.motion.speed.x = -3.0f;
            players.gamma.current_extra = foreign;
        }
        return true;
    });
    gamma_selection::update_fan(context, pose, player, allocation);
    arena->set_payload_transport({}, {});
    assert(arena->read(allocation, 0, after.data(), 580));
    assert(observed_phase && after[7] == 3754 && static_cast<std::int32_t>(after[10]) == -4551);
    assert(!players.current.player && players.gamma.current_extra == foreign);
    std::cout << "Actual Gamma PInitialize/common tail reaches display; live E102WK fan words, "
                 "null current alias, reentry and failure prefix checked\n";
}

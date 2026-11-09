#include "apps/native_gamma_actor_assets.hpp"
#include "apps/native_lighting_assets.hpp"
#include "runtime/model_draw_environment.hpp"
#include "player/animation/animation_native_access.hpp"
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
    // The source places the body at position + rotate(0, center_height, 0).
    // Zero angles make that offset exact, so the first pass can compare stores.
    pose.rotation = {0, 0, 0};
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
    lighting::State lighting(native::app::load_lighting_assets(directory));
    native::ModelDrawEnvironment environment{pack, scene, bootstrap.material_callback, lighting};
    context.model_environment = &environment;
    // Compose a real event allocation before the constructor. The source display
    // dereferences eventwk even when its action list is empty.
    event_actions::State event_state;
    const auto fields = native::event_primary_fields();
    native::EventActionsContext event(*arena, event_state, players.registry, pack, fields);
    event_actions::initialize(event.core, task->primary_work);
    context.controller = [](std::uint8_t) {
        return gamma_selection::Context::ControllerSample{};
    };
    bool stopped = false;
    try {
        gamma_selection::run(context, task);
    } catch (const basic_model::Unavailable &error) {
        std::cerr << "Model boundary " << unsigned(error.boundary) << std::endl;
        throw;
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        std::cerr << "Actual display stop: " << error.what() << std::endl;
        assert(std::string(error.what()) == "native_gamma_underwater");
        stopped = true;
    }
    assert(stopped && selected_palette == 0);
    assert(display.has_queued_model());
    assert(context.collision_entries.lists[0].count == 1);
    assert(context.collision_entries.lists[0].slots[0].task == task);
    assert(context.collision_entries.special_stage == 0);
    const auto &draws = display.output().frame().commands;
    std::cout << "Body frame commands: " << draws.size() << std::endl;
    std::size_t body_draws = 0, body_vertices = 0;
    for (const auto &command : draws) {
        if (const auto *draw = std::get_if<graphics::Draw>(&command)) {
            ++body_draws;
            body_vertices += draw->vertices.size();
        }
    }
    assert(body_draws > 98 && body_vertices > 2862);
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
    // playerwk+0x11c is the decoded parameter center_height, not a zero scratch.
    const auto center_height = player->parameters.center_height;
    assert(center_height == 14.0f);
    assert(collision->info[0].center.x == pose.position.x);
    assert(collision->info[0].center.y == pose.position.y + center_height);
    assert(collision->info[0].center.z == pose.position.z);
    assert(collision->info[1].center.x == pose.position.x);
    assert(collision->info[1].center.y == pose.position.y + 5.0f);
    assert(collision->info[1].center.z == pose.position.z);
    gamma_display::Context inspection(context);
    const auto render = inspection.render_state();
    assert(render.control == render.saved.control &&
           render.saved.object_callback == render_snapshot::ObjectCallback::None);
    std::cout << "Final arm nodes: " << inspection.animation_state().node
              << ", draws: " << body_draws << ", vertices: " << body_vertices << std::endl;
    assert(inspection.animation_state().node == 3 && inspection.animation_state().motion_type == 7);
    assert(inspection.animation_state().model_callback == animated_model::ModelCallback::DrawModel);
    assert(inspection.animation_state().clip_scale == 0);
    const auto allocation = gamma_selection::extra_work(context, player);
    assert(players.gamma.current_extra == allocation);
    std::uint32_t phase = 999;
    assert(arena->read(allocation, 44, &phase, 4) && phase == 0);
    // A generated MDATA2 node with known-null key references uses its live pose.
    gamma_display::capture(inspection, task);
    gamma_display::capture_extra(inspection);
    animation::View animation_view{*player->physics, player->allocation};
    auto animation_fields = animation::detail::NativeAccess{}.state(animation_view);
    const auto saved_buffer = animation_fields.buffer;
    const auto stand_root = pack.resolve(assets.stand).object;
    auto &live_root = objects.object(stand_root);
    const auto saved_root = live_root;
    live_root.model = live_root.child = live_root.sibling = 0;
    live_root.evaluation_flags &= ~64U;
    const auto data = arena->allocate(16).work;
    assert(data && arena->fill(data, 0, 0, 16));
    assert(arena->set_reference(data, 0, {}) && arena->set_reference(data, 4, {}));
    auto motion = std::make_shared<PlayerAnimationAllocatedMotion>();
    motion->data = data;
    motion->frame_count = 2;
    motion->type = 3;
    motion->input_function = 2;
    auto generated = std::make_shared<PlayerAnimationAllocatedAction>();
    generated->object = SceneResourceReference{stand_root};
    generated->motion = motion;
    animation_fields.buffer = generated;
    gamma_display::draw_generated_action(inspection, 0.5f);
    assert(inspection.animation_state().node == 1 && inspection.animation_state().channel == 2);
    const auto scaled_data = arena->allocate(24).work;
    assert(scaled_data && arena->fill(scaled_data, 0, 0, 24));
    for (unsigned offset : {0U, 4U, 8U})
        assert(arena->set_reference(scaled_data, offset, {}));
    const auto scale_keys = arena->allocate(16).work;
    const std::array<float, 4> scale_key = {0, 1.25f, 2.5f, 3.75f};
    const std::uint32_t one_key = 1;
    assert(scale_keys && arena->write(scale_keys, 0, scale_key.data(), sizeof(scale_key)));
    assert(arena->set_reference(scaled_data, 8, scale_keys));
    assert(arena->write(scaled_data, 20, &one_key, sizeof(one_key)));
    motion->data = scaled_data;
    motion->type = 7;
    motion->input_function = 3;
    gamma_display::draw_generated_action(inspection, 0.5f);
    assert(inspection.animation_state().node == 1 && inspection.animation_state().channel == 3);
    motion->data = data;
    motion->type = 3;
    motion->input_function = 2;
    const std::uint32_t unbound_pointer = 123456;
    assert(arena->write(data, 0, &unbound_pointer, 4));
    bool unbound_stopped = false;
    try {
        gamma_display::draw_generated_action(inspection, 0.5f);
    } catch (const flow::UnrecoveredGameFlowCall &error) {
        unbound_stopped = std::string(error.what()) == "native_gamma_draw_generated_key_reference";
    }
    assert(unbound_stopped && inspection.animation_state().channel == 0);
    // The source loop guard precedes dereferencing an incomplete generated header.
    generated->object.reset();
    generated->motion.reset();
    timing.pending_logic_ticks = 1;
    gamma_display::draw_generated_action(inspection, 0.5f);
    const auto saved_held_task = player->physics->held_task_at_68;
    player->physics->held_task_at_68.reset();
    auto saved_resources = std::move(players.gamma_motion_assets);
    gamma_display::draw_table_action(inspection, 0, 0.5f);
    players.gamma_motion_assets = std::move(saved_resources);
    player->physics->held_task_at_68 = saved_held_task;
    timing.pending_logic_ticks = 0;
    animation_fields.buffer = saved_buffer;
    live_root = saved_root;
    // Reached display remains independent of current-player publications.
    const auto foreign = arena->allocate(8).work;
    players.current.player = foreign;
    player->physics->equipment = 512;
    // Exercise the real rotation path; placement is covered by the exact first pass.
    pose.rotation = {111, 222, 333};
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
    player->physics->equipment = 0;
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

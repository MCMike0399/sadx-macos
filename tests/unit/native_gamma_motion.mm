#include "apps/native_gamma_actor_assets.hpp"
#include "game/game_mode.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_player_system.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

namespace {
void unrecovered(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("unexpected actor callback");
}
using namespace sadx;
unsigned word(float value) {
    unsigned bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}
unsigned read_word(TaskWorkArena &arena, const TaskWorkHandle &work, unsigned offset) {
    unsigned value;
    assert(arena.read(work, offset, &value, sizeof(value)));
    return value;
}
void observe(const char *label, std::ostream &out,
             const player_initialization::PlayerHandle &player, TaskWorkArena &arena,
             X87Context &fp) {
    const auto &state = *player->physics;
    const auto &tail = state.animation_tail;
    out << label << " state " << *state.current_animation_at_126 << ' '
        << *state.requested_animation_at_124 << ' ' << *state.previous_animation_at_128 << ' '
        << *state.animation_mode_at_120 << ' ' << *tail.next_at_12a << ' ' << *tail.timer_at_12c
        << ' ' << *tail.flags_at_12e << ' ' << word(*tail.frame_at_130) << ' ' << fp.control_word()
        << ' ' << fp.exception_flags() << '\n';
    const auto generated = *(*tail.buffer_at_148)->motion;
    const auto data = *generated->data;
    for (unsigned node = 0; node < 77; ++node) {
        for (unsigned channel = 0; channel < 2; ++channel) {
            const auto reference = arena.reference(data, node * 16 + channel * 4);
            assert(reference);
            out << label << ' ' << node << ' ' << channel << ' ' << bool(reference.work) << ' '
                << read_word(arena, data, node * 16 + 8 + channel * 4);
            if (reference.work)
                for (unsigned field = 0; field < 8; ++field)
                    out << ' ' << read_word(arena, reference.work, field * 4);
            out << '\n';
        }
    }
}
void verify_transition(const character_pack::CharacterPack &pack, character_pack::SceneState &scene,
                       const player_initialization::PlayerHandle &player,
                       const native::GammaMotionAssets &assets, TaskWorkArena &arena,
                       X87Context &fp, std::ostream &observations) {
    auto &state = *player->physics;
    auto &tail = state.animation_tail;
    animation::View view{state, player->allocation};
    animation::Runtime runtime{&arena, assets.resources(), &scene.objects};
    const auto root =
        pack.resolve(
                ActionHandle{std::dynamic_pointer_cast<const animation::Table>(*tail.table_at_140)
                                 ->records.at(0)
                                 .action.identity})
            .object;
    const auto node = scene.objects.object(root).child;
    assert(node);
    auto &pose = scene.objects.object(node);
    const auto saved = pose.rotation;
    for (unsigned sample = 0; sample < 2; ++sample) {
        assert(animation::release_keys(view, runtime));
        pose.rotation = sample == 0 ? std::array<std::int32_t, 3>{1234, -2345, 30000}
                                    : std::array<std::int32_t, 3>{-321, 4321, -12345};
        state.current_animation_at_126 = 0;
        state.requested_animation_at_124 = 43;
        state.animation_mode_at_120 = 1;
        tail.timer_at_12c = 0;
        tail.frame_at_130 = 4.5f;
        fp.clear_exceptions();
        assert(native::update_gamma_motion(view, arena, assets, scene.objects, fp));
        assert(tail.frame_at_130 == 1.0f / 6.0f && state.animation_mode_at_120 == 2);
        const auto generated = *(*tail.buffer_at_148)->motion;
        const auto output = arena.reference(*generated->data, 16 + 4);
        assert(output && output.work);
        for (unsigned axis = 0; axis < 3; ++axis)
            assert(static_cast<std::int32_t>(read_word(arena, output.work, 4 + axis * 4)) ==
                   pose.rotation[axis]);
        observe(sample == 0 ? "idle_pose_a" : "idle_pose_b", observations, player, arena, fp);
    }
    pose.rotation = saved;
}
void verify_playback(const player_initialization::PlayerHandle &player,
                     const native::GammaMotionAssets &assets, character_pack::SceneState &scene,
                     TaskWorkArena &arena, X87Context &fp) {
    auto &state = *player->physics;
    auto &tail = state.animation_tail;
    animation::View view{state, player->allocation};
    animation::Runtime runtime{&arena, assets.resources(), &scene.objects};
    assert(animation::release_keys(view, runtime));
    state.current_animation_at_126 = state.requested_animation_at_124 = 0;
    state.animation_mode_at_120 = 3;
    tail.frame_at_130 = 2.25f;
    assert(native::update_gamma_motion(view, arena, assets, scene.objects, fp));
    assert(word(*tail.frame_at_130) == word(2.55f));
    state.current_animation_at_126 = state.requested_animation_at_124 = 1;
    tail.frame_at_130 = 0;
    tail.flags_at_12e = 65535;
    const auto timer_before = *tail.timer_at_12c;
    const auto &unavailable_row =
        std::dynamic_pointer_cast<const animation::Table>(*tail.table_at_140)->records.at(1);
    const auto unavailable = native::update_gamma_motion(view, arena, assets, scene.objects, fp);
    assert(unavailable.failure == animation::Failure::UnresolvedResource &&
           unavailable.phase == animation::Phase::LoopLength &&
           unavailable.unavailable_resource_reason == unavailable_row.action.unavailable_reason);
    assert(tail.frame_at_130 == unavailable_row.frame_step && tail.flags_at_12e == 0 &&
           state.previous_animation_at_128 == 1 &&
           tail.timer_at_12c == static_cast<std::uint16_t>(timer_before + 1));
    // The real allocation's aliases, rather than cached multiplier values, are consumed live.
    auto table = std::make_shared<animation::Table>();
    const auto canonical = *tail.table_at_140;
    *table = *std::dynamic_pointer_cast<const animation::Table>(canonical);
    table->records.at(0).mode = animation::Mode::SpeedLoop;
    tail.table_at_140 = table;
    state.current_animation_at_126 = state.requested_animation_at_124 = 0;
    state.animation_mode_at_120 = 9;
    state.motion.speed.x = 2;
    tail.frame_at_130 = 0;
    assert(native::update_gamma_motion(view, arena, assets, scene.objects, fp));
    assert(word(*tail.frame_at_130) == word(0.6f));
    state.motion.speed.x = 3;
    assert(native::update_gamma_motion(view, arena, assets, scene.objects, fp));
    assert(word(*tail.frame_at_130) == word(1.5f));
    state.animation_mode_at_120 = 10;
    state.source_charge_at_0 = 2;
    tail.frame_at_130 = 0;
    assert(native::update_gamma_motion(view, arena, assets, scene.objects, fp));
    assert(word(*tail.frame_at_130) == word(0.6f));
    state.source_charge_at_0 = 3;
    assert(native::update_gamma_motion(view, arena, assets, scene.objects, fp));
    assert(word(*tail.frame_at_130) == word(1.5f));
    tail.table_at_140 = canonical;
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
    bool missing_rejected = false;
    try {
        (void)native::app::load_gamma_effects_definition(directory / "missing-assets");
    } catch (const std::runtime_error &) {
        missing_rejected = true;
    }
    assert(missing_rejected);
    const auto setup = native::app::load_gamma_setup_definition(directory);
    const auto effects = native::app::load_gamma_effects_definition(directory);
    assert(setup && !effects.empty());
    native::bind_gamma_setup_resources(players, texture_assets, lists, *setup);
    auto invalid = texture_assets;
    invalid[6][4].resource_name = "wrong_hit_identity";
    bool rejected = false;
    try {
        native::bind_gamma_effects_resources(players, invalid, lists, effects);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    assert(rejected && !players.gamma_effects_asset);
    native::bind_gamma_effects_resources(players, texture_assets, lists, effects);
    auto &asset = *players.gamma_effects_asset;
    assert(asset.textures == &lists[6][4] && asset.textures != &lists[6][2]);
    const auto *stable = players.gamma_effects_asset.get();
    rejected = false;
    try {
        native::bind_gamma_effects_resources(players, texture_assets, lists, effects);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    assert(rejected && players.gamma_effects_asset.get() == stable);
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
        assert(std::string(error.what()) == "native_gamma_join_vertices");
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
    const auto &resources = players.gamma_motion_assets->resources();
    auto table = players.gamma_assets->animation_table;
    assert(tail.table_at_140 == table && table->records.size() == 91);
    assert(state.current_animation_at_126 == 0 && state.requested_animation_at_124 == 0 &&
           state.previous_animation_at_128 == 0 && state.animation_mode_at_120 == 1);
    assert(tail.frame_at_130 == 0 && tail.start_at_134 == 0 && tail.timer_at_12c == 1 &&
           tail.flags_at_12e == 0);
    const auto buffer = *tail.buffer_at_148;
    const auto stand = pack.resolve(players.gamma_assets->stand);
    assert(buffer && buffer->object->id == stand.object && *buffer->motion);
    assert(resources.actions.at(players.gamma_assets->stand.identity).motion.id == stand.motion);
    assert(!resources.objects);
    const auto generated = *buffer->motion;
    assert(generated->frame_count == 2 && generated->type == 3 && generated->input_function == 2);
    const auto data = *generated->data;
    std::vector<std::uint8_t> initial(111 * 24);
    assert(arena->read(data, 0, initial.data(), initial.size()));
    assert(std::all_of(initial.begin(), initial.end(), [](auto value) { return value == 0; }));
    const auto speed = arena->reference_view(player->allocation, 312);
    const auto charge = arena->reference_view(player->allocation, 316);
    assert(speed && charge && speed.work == player->allocation && speed.byte_offset == 56 &&
           charge.work == player->allocation && charge.byte_offset == 0);
    // Allocation is the first call only. The next PSetMotion performs real Stand blending
    // against the same mutable scene, not a private pose map.
    display.arithmetic().clear_exceptions();
    gamma_selection::set_motion(context, player);
    assert(state.animation_mode_at_120 == 2 && tail.frame_at_130 == 0.0625f);
    assert(*tail.buffer_at_148 == buffer && *buffer->motion == generated &&
           *generated->data == data);
    assert(tail.table_at_140 == table);
    // Locate a channel with incoming keys and absent outgoing keys by using the real
    // decoded Stand closure below; pose fallback must reread the authoritative object.
    std::ofstream observations(argv[2]);
    observe("stand", observations, player, *arena, display.arithmetic());
    verify_transition(pack, scene, player, *players.gamma_motion_assets, *arena,
                      display.arithmetic(), observations);
    verify_playback(player, *players.gamma_motion_assets, scene, *arena, display.arithmetic());
    assert(players.current.primary == task->primary_work && !players.current.player);
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
    const auto retained_key = resources.key_arrays.begin()->second;
    std::weak_ptr<const animation::TableStorage> table_lifetime = table;
    players.gamma_motion_assets.reset();
    players.gamma_assets.reset();
    table.reset();
    assert(retained_key.bytes && retained_key.byte_extent > 0 && !table_lifetime.expired());
    assert(tail.table_at_140->get() == table_lifetime.lock().get() &&
           *tail.buffer_at_148 == buffer);
    std::cout << "Real Gamma initialization reaches PJoinVertexes; canonical motion, live scene, "
                 "keys and aliases pass\n";
}

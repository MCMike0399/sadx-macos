// Actual Sonic caller -> recovered updater -> existing common-tail boundary.
// A typed build additionally checks native-runtime precedence over legacy binding.
#include "player/sonic_player.hpp"
#ifdef ANIMATION_TYPED
#include "player/animation/animation.hpp"
#endif
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

namespace {
using namespace sadx;
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
unsigned bits(float value) { unsigned result; std::memcpy(&result, &value, 4); return result; }
void run(const SonicPlayerConstants &constants, unsigned binding, bool missing_timer) {
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    auto object = scheduler.append_task(0, &TaskScheduler::source_no_op).task;
    object->primary_work = arena->allocate(64).work;
    auto task = std::make_shared<PlayerTaskState>();
    object->primary_state = task;
    task->action_at_0 = 255;
    auto world = std::make_shared<PlayerMotionState>();
    auto player = std::make_shared<PlayerStartupPlayer>();
    player->player = std::make_shared<PlayerPhysicsState>();
    player->player_work = arena->allocate(512).work;
    PlayerPositionCollisionScene scene;
    SonicPlayerFrame frame{object, task, world, player, &scene};
    SonicPlayerGlobals globals;
    PlayerStartupRegistry registry;
    SonicPlayerRuntime runtime{globals, scheduler, registry, nullptr, nullptr, constants, {}};
    runtime.external = [](const SonicPlayerCall &, SonicPlayerFrame &, X87Context &) -> SonicPlayerResult {
        require(false, "recovered updater must never fall through to an external callback");
        return {};
    };
    PlayerAnimationResources resources;
    PlayerAnimationConstants arithmetic{0, 1, 4294967296.0f};
    PlayerAnimationRuntime legacy{arena.get(), resources, arithmetic};
    auto legacy_table = std::make_shared<PlayerAnimationTable>();
    auto &row = legacy_table->records[0];
    row.action = AnimationResourceReference{0}; row.frame_step = 0.5f;
    resources.actions[0].motion = AnimationResourceReference{0};
    resources.motions[0].frame_count = 10;
    auto &state = *player->player;
    state.current_animation_at_126 = 0; state.requested_animation_at_124 = 0;
    state.previous_animation_at_128 = 0; state.animation_mode_at_120 = 3;
    state.animation_tail.frame_at_130 = 2.25f;
    state.animation_tail.flags_at_12e = 0;
    if (!missing_timer) state.animation_tail.timer_at_12c = 17;
    state.animation_tail.table_at_140 = legacy_table;
    if (binding != 0) runtime.animation_runtime = &legacy;
#ifdef ANIMATION_TYPED
    animation::Resources native_resources;
    animation::Runtime native{arena.get(), native_resources};
    auto native_table = std::make_shared<animation::Table>();
    native_table->records[0].frame_step = 0.5f;
    native_table->records[0].action = {animation::ResourceStatus::Loaded, 0, {}};
    native_resources.actions[0].motion = {0};
    native_resources.motions[0].frame_count = 10;
    if (binding == 2) {
        runtime.native_animation_runtime = &native;
        state.animation_tail.table_at_140 = native_table;
        legacy.arena = nullptr; // Deliberately unusable fallback must not win precedence.
    }
#endif
    const auto identity = *state.animation_tail.table_at_140;
    X87Context fp(0x7f);
    const auto result = dispatch_sonic_player_action(frame, runtime, fp);
    require(!result.completed, "unrecovered common tail must remain stopped");
    require(*state.animation_tail.table_at_140 == identity, "updater must retain canonical table identity");
    if (binding == 0) {
        require(result.failure == SonicPlayerFailure::missing_native_owner &&
                result.source_boundary == 0x49be0b, "missing runtime boundary");
    } else if (missing_timer) {
        require(result.failure == SonicPlayerFailure::animation_failed && result.animation &&
                result.nested_source_boundary == 0x44a842, "reached missing timer prefix");
        require(state.animation_tail.frame_at_130 == 2.25f, "missing timer must not advance frame");
    } else {
        require(result.failure == SonicPlayerFailure::unimplemented_action &&
                result.source_boundary == 0x49be10 && result.animation && result.animation->completed,
                "actual updater receipt before common-tail stop");
        require(state.animation_tail.timer_at_12c == 18 && state.animation_tail.frame_at_130 == 2.75f,
                "nonzero updater step and timer must reach the canonical owner");
    }
    std::cout << binding << ',' << missing_timer << ',' << static_cast<unsigned>(result.failure)
              << ',' << result.source_boundary << ',' << result.nested_source_boundary << ','
              << bits(*state.animation_tail.frame_at_130) << ',' << fp.exception_flags() << '\n';
}
void auxiliary_token(TaskScheduler &, const TaskHandle &) {
    require(false, "unrecovered auxiliary callback must stay stored, never invoked");
}
void attachment(const SonicPlayerConstants &constants, const PlayerStartupParameterTable &parameters,
                bool missing_callback) {
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    require(bool(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{})), "real startup pool");
    const auto object = scheduler.create_element(3, 1, nullptr).task;
    require(bool(object), "real startup task");
    object->primary_state = std::make_shared<PlayerTaskState>();
    auto task = object->primary_state;
    task->action_at_0 = 0; task->character_id = 0; task->task_id_at_2 = 0;
    task->player_index = 0; task->position = NinjaVector{3, 4, 5};
    task->scale_at_2c = NinjaVector{7, 8, 9};
    auto world = std::make_shared<PlayerMotionState>();
    PlayerPositionCollisionScene scene;
    SonicPlayerFrame frame{object, task, world, std::shared_ptr<PlayerStartupPlayer>{}, &scene};
    SonicPlayerGlobals globals; globals.alternate_model_at_3b18db5 = false;
    PlayerStartupRegistry registry;
    PlayerEventFlags events; events.fill(2);
    PlayerStartupRuntime startup{&parameters, &events, missing_callback ? nullptr : auxiliary_token};
    SonicPlayerRuntime runtime{globals, scheduler, registry, &startup, nullptr, constants, {}};
    auto table = std::make_shared<PlayerAnimationTable>();
    runtime.initialized_animation_table = table;
    X87Context fp(0x37f);
    const auto result = dispatch_sonic_player_action(frame, runtime, fp);
    require(frame.player && *frame.player, "actual startup partial owner");
    const auto player = *frame.player;
    if (missing_callback) {
        require(result.failure == SonicPlayerFailure::startup_failed && result.startup &&
                result.startup->source_boundary == 0x44297b, "earlier startup failure retained");
        require(player->player->animation_tail.table_at_140 &&
                !*player->player->animation_tail.table_at_140, "failed startup never attaches table");
    } else {
        require(player->stage == PlayerStartupStage::complete && result.failure ==
                SonicPlayerFailure::unresolved_resource && result.source_boundary == 0x49ab64,
                "real startup reaches unrecovered model boundary");
        require(player->player->animation_tail.table_at_140 == table &&
                player->player->requested_animation_at_124 == 0,
                "action zero publishes same canonical table and request zero");
        require(player->player_work && player->auxiliary_task &&
                player->auxiliary_task->exec == auxiliary_token &&
                registry.lookup_player_work(player->player_work) == player,
                "real allocation, registration and stored callback identity");
    }
    std::cout << "attachment," << missing_callback << ',' << static_cast<unsigned>(result.failure)
              << ',' << result.source_boundary << ',' << static_cast<unsigned>(player->stage) << '\n';
}
} // namespace
int main(int argc, char **argv) {
    require(argc == 3, "requires pinned Sonic constants and startup parameters");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), {}};
    const auto constants = decode_sonic_player_constants(bytes.data(), bytes.size());
    require(bool(constants), "pinned Sonic constants decode");
    std::ifstream parameter_input(argv[2], std::ios::binary);
    PlayerStartupParameterTable parameters{};
    parameter_input.read(reinterpret_cast<char *>(parameters.data()), sizeof(parameters));
    require(parameter_input.gcount() == sizeof(parameters), "source parameter bytes");
    run(*constants, 0, false); run(*constants, 1, false); run(*constants, 1, true);
#ifdef ANIMATION_TYPED
    run(*constants, 2, false); run(*constants, 2, true);
#endif
    attachment(*constants, parameters, false);
    attachment(*constants, parameters, true);
}

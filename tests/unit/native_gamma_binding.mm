#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "game/game_mode.hpp"
#include <iostream>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
void lower_callback(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("unrecovered test callback invoked");
}
void check_binding(const std::filesystem::path &directory) {
    using namespace sadx;
    namespace gamma = gamma_selection;
    const auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    native::PlayerSystem players;
    players.gamma_assets = std::make_unique<native::GammaActorAssets>(
        native::app::load_gamma_actor_assets(directory, pack));
    auto &assets = *players.gamma_assets;
    require(assets.animation_table->records.size() == 91 &&
        assets.declarations[0].action.identity == assets.stand.identity &&
        assets.declarations[61].status == native::ActorResourceStatus::KnownNull &&
        assets.declarations[1].status == native::ActorResourceStatus::Unavailable &&
        !assets.declarations[1].boundary.empty(), "complete imported table preserves resource states");
    require(!assets.animation_table->records.at(1).action &&
        assets.animation_table->records.at(0).action->id == assets.stand.identity,
        "unavailable animation must not become a known-null or stand substitute");
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    require(bool(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{})), "actual task pool");
    const auto task = scheduler.create_element(7, 3, nullptr).task;
    require(bool(task), "actual preview task");
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    const auto primary_owner = task->primary_state;
    const auto motion_owner = task->motion_state;
    auto pose = player_initialization::bind_primary_view(task);
    pose.id = 2;
    pose.player_index = 5;
    pose.position = {13, 27, -9};
    pose.rotation = {111, 222, 333};
    motion_owner->target_yaw = 16384;
    std::int32_t card_result = 0;
    std::uint32_t global_timer = 0, movie_number = 0;
    std::int16_t rings = 0;
    std::uint16_t game_mode = 0;
    bootstrap::State bootstrap({card_result, global_timer, rings, movie_number, game_mode});
    bootstrap.event_work[393] = 1;
    bootstrap.event_work[465] = 1;
    frame::ModeState timing;
    native::Display display(directory / "system", timing);
    const std::int16_t stage = 0, act = 0;
    gamma::Context context{players, scheduler, bootstrap, display, task, stage, act,
        lower_callback, lower_callback, lower_callback};
    // Consumer bindings must read bootstrap timer bytes and the actual task handle,
    // independent of similarly named cutscene control/work or game timer fields.
    bootstrap.cutscene_active = 1;
    bootstrap.cutscene_control = 255;
    bootstrap.cutscene_work = 91;
    bootstrap.game_timer = 12345;
    require(gamma::event_cancel(context) == 0 && gamma::is_time_zero(context) == 1,
        "unrelated cutscene/timer state must not substitute for the source owners");
    bootstrap.cutscene_task = task;
    require(gamma::event_cancel(context) == 1, "actual nonnull task drives event predicate");
    bootstrap.cutscene_task.reset();
    bootstrap.frames = 1;
    require(gamma::is_time_zero(context) == 0, "actual frame byte consumed");
    bootstrap.frames = 0;
    bootstrap.seconds = 1;
    require(gamma::is_time_zero(context) == 0, "actual second byte consumed");
    bootstrap.seconds = 0;
    bootstrap.minutes = 1;
    require(gamma::is_time_zero(context) == 0, "actual minute byte consumed");
    bootstrap.minutes = 0;
    bootstrap.frames = 17;
    const auto stale = arena->allocate(512).work;
    players.current.player = stale;
    bool stopped = false;
    try { gamma::run(context, task); }
    catch (const flow::UnrecoveredGameFlowCall &error) {
        require(std::string(error.what()) == "native_gamma_CCL_Init", "first real lower boundary");
        stopped = true;
    }
    require(stopped, "unrecovered collision must throw");
    require(players.gamma.time_zero == 0 && players.gamma.event_enabled == 1,
        "core consumes the actual bootstrap-bound predicates before initialization");
    const auto player = players.registry.players[5];
    require(player && player->stage == player_initialization::Stage::Complete,
        "real PInitialize completes before collision boundary");
    require(players.registry.tasks[5] == task && players.registry.primary[5] == primary_owner &&
        players.registry.motion[5] == motion_owner && task->primary_state == primary_owner &&
        task->motion_state == motion_owner, "registry borrows actual task owners");
    require(gamma::player_for_motion(context, task->motion_work) == player &&
        players.registry.allocations.at(player->allocation.get()) == player,
        "arena reference joins the actual allocation identity");
    require(players.current.primary == task->primary_work && !players.current.player,
        "captured null player publication remains null after registration");
    require(pose.id == 2 && pose.character == 6 && pose.mode == 0 && pose.player_index == 5 &&
        pose.position.x == 13 && pose.position.y == 27 && pose.rotation[2] == 333 &&
        motion_owner->target_yaw == 16384, "constructor preserves real preview pose and ID");
    require(player->parameters.height == assets.parameters[6].height &&
        *player->physics->equipment_at_4 == (1024 | 2048),
        "typed parameters and actual event flags feed PInitialize");
    require(*player->physics->animation_tail.table_at_140 == assets.animation_table &&
        *player->physics->requested_animation_at_124 == 0,
        "actual animation table and request zero precede CCL_Init");
    require(player->auxiliary && player->auxiliary_task &&
        player->auxiliary_task->exec == lower_callback && !task->display && !task->destructor,
        "source auxiliary task exists and later Gamma callback stores have not run");
    auto extra = gamma::allocate_zero(context, 1, 580);
    gamma::store_extra_work(context, player, extra);
    require(extra && extra != player->auxiliary && gamma::extra_work(context, player) == extra &&
        extra->requested_size() == 580, "Gamma and island-list allocations stay distinct");
    gamma::extra(context, extra).general_timer = 37;
    require(gamma::extra(context, gamma::extra_work(context, player)).general_timer == 37,
        "typed E102WK borrows its actual arena allocation");
    require(gamma::display_callback(context) == lower_callback &&
        gamma::destructor_callback(context) == lower_callback,
        "callback getters preserve stored native wrappers");
    bool callback_stopped = false;
    try { gamma::display_callback(context)(scheduler, task); }
    catch (const std::runtime_error &) { callback_stopped = true; }
    require(callback_stopped, "lower callback cannot silently succeed");
    // Bad metadata must fail before a resource identity is published.
    const auto document = native::app::read_metadata(directory / "characters/gamma-actor.json");
    NSMutableDictionary *row = [native::app::dictionary(document[@"declarations"][0], "declaration") mutableCopy];
    row[@"action"] = @0;
    bool rejected = false;
    try { (void)native::app::gamma_declaration(row, pack); }
    catch (const std::runtime_error &) { rejected = true; }
    require(rejected, "invalid loaded resource identity rejected");
    std::cout << "PASS: imported Gamma -> live task/PInitialize/current owners -> CCL_Init boundary\n";
}
}
int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            if (argc != 2) throw std::runtime_error("expected imported asset directory");
            check_binding(argv[1]);
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}

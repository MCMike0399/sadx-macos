// Real imported table + real PInitialize + real Gamma bind function, without playback.
#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "game/game_mode.hpp"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
void unavailable_callback(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    require(false, "stored auxiliary callback must not be replaced by a successful fixture");
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    namespace gamma = gamma_selection;
    require(argc == 2, "requires imported assets");
    const std::filesystem::path directory(argv[1]);
    auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    character_pack::SceneState scene(pack);
    native::PlayerSystem players;
    players.gamma_assets = std::make_unique<native::GammaActorAssets>(
        native::app::load_gamma_actor_assets(directory, pack));
    auto &assets = *players.gamma_assets;
    const auto table = assets.animation_table;
    require(table && table->records.size() == 91, "actual complete imported table");
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    require(bool(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{})), "real task pool");
    std::array<TaskHandle, 2> tasks;
    for (unsigned index = 0; index < tasks.size(); ++index) {
        tasks[index] = scheduler.create_element(7, 3, nullptr).task;
        require(bool(tasks[index]), "real player task allocation");
        player_initialization::establish_zeroed_primary(scheduler, tasks[index]);
        player_initialization::establish_zeroed_motion(scheduler, tasks[index]);
        auto pose = player_initialization::bind_primary_view(tasks[index]);
        pose.id = 2;
        pose.character = 6;
        pose.player_index = static_cast<std::uint8_t>(index);
        pose.position = {float(index + 3), 27, -9};
        pose.status = 0;
    }
    std::int32_t card = 0;
    std::uint32_t timer = 0, movie = 0;
    std::int16_t rings = 0, stage = 0, act = 0;
    std::uint16_t mode = 0;
    bootstrap::State bootstrap({card, timer, rings, movie, mode});
    frame::ModeState timing;
    native::Display display(directory / "system", timing);
    sprite_registry::State sprites;
    gamma::Context context{players, scheduler, bootstrap, display, tasks[0], stage, act,
        unavailable_callback, unavailable_callback, unavailable_callback,
        {nullptr, nullptr, 0}, nullptr, nullptr, sprites, scene};
    for (unsigned index = 0; index < tasks.size(); ++index)
        gamma::initialize_player(context, static_cast<std::int32_t>(index), tasks[index]);
    const auto first = players.registry.players[0], second = players.registry.players[1];
    require(first && second && first != second &&
        first->stage == player_initialization::Stage::Complete &&
        second->stage == player_initialization::Stage::Complete, "two real completed PInitialize owners");
    auto a = player_initialization::bind_initialized_player(first);
    auto b = player_initialization::bind_initialized_player(second);
    a.requested_animation = 11; a.current_animation = 33;
    b.requested_animation = 22; b.current_animation = 44;
    gamma::bind_animation_table(context, first);
    gamma::bind_animation_table(context, second);
    require(a.animation_table == table && b.animation_table == table &&
        a.animation_table == b.animation_table, "both players borrow SAME imported table");
    require(&a.requested_animation != &b.requested_animation &&
        a.requested_animation == 11 && b.requested_animation == 22 &&
        a.current_animation == 33 && b.current_animation == 44, "per-player animation state remains isolated");
    for (bool absent : {true, false}) {
        auto incomplete = std::make_shared<animation::Table>();
        if (!absent) {
            for (unsigned index = 0; index < 90; ++index)
                incomplete->records.emplace(index, animation::Record{});
        }
        assets.animation_table = absent ? std::shared_ptr<const animation::Table>{} : incomplete;
        bool stopped = false;
        try { gamma::bind_animation_table(context, second); }
        catch (const flow::UnrecoveredGameFlowCall &error) {
            stopped = std::string(error.what()) == "native_gamma_complete_animation_table";
        }
        require(stopped && a.animation_table == table && b.animation_table == table,
                "missing/wrong-size binding stops before changing prior attachments");
        require(a.requested_animation == 11 && b.requested_animation == 22,
                "failed binding preserves each player's requests");
    }
    assets.animation_table = table;
    require(assets.declarations[61].status == native::ActorResourceStatus::KnownNull &&
            table->records.at(72).action.status == animation::ResourceStatus::Unavailable,
            "actual null/unavailable content classifications survive shared binding");
    std::cout << "Two real Gamma players share one imported table; isolated state and failed-binding prefixes pass\n";
}

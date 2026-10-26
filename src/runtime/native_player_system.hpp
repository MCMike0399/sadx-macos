#pragma once
#include "runtime/gamma_actor_assets.hpp"
#include "player/actor_current.hpp"
#include "player/gamma_selection_core.hpp"
#include "game/game_bootstrap.hpp"

namespace sadx::native {
class Display;
// NOTSA: typed fields share the identity of the actual zero-filled E102WK block.
struct GammaExtraWork {
    TaskWorkHandle allocation;
    std::uint8_t laser_mode = 0;
    std::uint16_t general_timer = 0;
    float ground_y = 0, ground_distance = 0;
    std::uint32_t ground_attributes = 0;
    float walk_distance = 0;
};
// NOTSA: one PLAYER system owner for Sonic and Gamma, not one session field per actor.
struct PlayerSystem {
    actor_current::State current;
    player_initialization::Registry registry;
    gamma_selection::State gamma;
    std::deque<GammaExtraWork> gamma_work;
    std::unique_ptr<GammaActorAssets> gamma_assets;
};
} // namespace sadx::native

namespace sadx::gamma_selection {
// NOTSA: borrowed real owners for this source callback invocation. No pose copy.
struct Context {
    native::PlayerSystem &players;
    TaskScheduler &scheduler;
    bootstrap::State &bootstrap;
    native::Display &rendering;
    const TaskHandle &task;
    const std::int16_t &stage, &act;
    TaskCallback display, destructor, auxiliary;
};
} // namespace sadx::gamma_selection

#pragma once
#include "runtime/gamma_actor_assets.hpp"
#include "engine/character_geometry.hpp"
#include "player/actor_current.hpp"
#include "player/gamma_selection_core.hpp"
#include "game/game_bootstrap.hpp"
#include "game/character_textures.hpp"

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
    gamma_cursors::WireModel cursor1, cursor2;
};
// NOTSA: one PLAYER system owner for Sonic and Gamma, not one session field per actor.
struct PlayerSystem {
    actor_current::State current;
    player_initialization::Registry registry;
    gamma_selection::State gamma;
    // GLOBAL: SONIC 0x3c54558 GammaWeldInfo size=120
    PlayerJoinTable gamma_weld;
    gamma_cursors::Templates gamma_cursor_templates;
    std::deque<GammaExtraWork> gamma_work;
    std::unique_ptr<GammaActorAssets> gamma_assets;
};
// NOTSA: existing session asset load binds imported declarations to its shared scene owners.
void bind_gamma_resources(PlayerSystem &, character_pack::SceneState &, const GammaActorAssets &);
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
    regular_textures::CharacterCatalog character_textures;
    regular_textures::State *texture_state;
    regular_textures::Services *texture_services;
};
} // namespace sadx::gamma_selection

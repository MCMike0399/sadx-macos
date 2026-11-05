#pragma once
#include "runtime/gamma_actor_assets.hpp"
#include "engine/model_matrices.hpp"
#include "runtime/gamma_motion.hpp"
#include "runtime/sonic_animation_assets.hpp"
#include "runtime/character_texture_assets.hpp"
#include "engine/character_geometry.hpp"
#include "player/actor_current.hpp"
#include "player/gamma_selection_core.hpp"
#include "player/gamma_setup_resource.hpp"
#include "player/gamma_effects.hpp"
#include "player/scene_input_producer.hpp"
#include "game/game_bootstrap.hpp"
#include "game/editor_state.hpp"
#include "game/character_textures.hpp"

namespace sadx::native {
class Display;
struct ModelDrawEnvironment;
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
    std::unique_ptr<GammaMotionAssets> gamma_motion_assets;
    std::unique_ptr<SonicAnimationAssets> sonic_animation_assets;
    std::unique_ptr<const gamma_setup::Asset> gamma_setup_asset;
    gamma_setup::State gamma_setup;
    std::unique_ptr<gamma_effects::Asset> gamma_effects_asset;
    gamma_effects::State gamma_effects;
};
// NOTSA: existing session asset load binds imported declarations to its shared scene owners.
void bind_gamma_resources(PlayerSystem &, character_pack::SceneState &, const GammaActorAssets &);
// NOTSA: bind imported setup content to the same character texlist used by rendering.
void bind_gamma_setup_resources(PlayerSystem &, const CharacterTextureAssets &,
    std::array<std::vector<regular_textures::List>, 9> &, const std::vector<std::uint8_t> &);
// NOTSA: imported effects borrow the existing E102HIT list and stable mutable declarations.
void bind_gamma_effects_resources(PlayerSystem &, const CharacterTextureAssets &,
    std::array<std::vector<regular_textures::List>, 9> &, const std::vector<std::uint8_t> &);
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
    sprite_registry::State &sprites;
    character_pack::SceneState &scene;
    const editor::View *editor_state = nullptr;
    std::array<NativeAnalogInput, 8> *normalized_input = nullptr;
    model_matrices::State *model_matrices = nullptr;
    // NOTSA: borrowed palette operation uses the Session lighting/time owners.
    std::function<void(std::int32_t)> display_palette = {};
    const native::ModelDrawEnvironment *model_environment = nullptr;
};
} // namespace sadx::gamma_selection

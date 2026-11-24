#pragma once
#include "runtime/gamma_actor_assets.hpp"
#include "engine/model_matrices.hpp"
#include "runtime/gamma_motion.hpp"
#include "runtime/sonic_animation_assets.hpp"
#include "runtime/character_texture_assets.hpp"
#include "engine/character_geometry.hpp"
#include "engine/collision_component.hpp"
#include "player/actor_current.hpp"
#include "player/gamma_selection_core.hpp"
#include "player/gamma_setup_resource.hpp"
#include "player/gamma_effects.hpp"
#include "player/player_underwater.hpp"
#include "player/big_selection_core.hpp"
#include "runtime/big_actor_assets.hpp"
#include "runtime/big_motion.hpp"
#include "player/amy_selection_core.hpp"
#include "runtime/amy_actor_assets.hpp"
#include "runtime/amy_motion.hpp"
#include "player/miles_selection_core.hpp"
#include "runtime/miles_actor_assets.hpp"
#include "runtime/miles_motion.hpp"
#include "player/knuckles_selection_core.hpp"
#include "runtime/knuckles_actor_assets.hpp"
#include "runtime/knuckles_motion.hpp"
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
    TaskHandle search_task = {}, aim_task = {};
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
    // The unnamed character-region word Amy mode zero clears; role unknown.
    std::uint32_t amy_event_word = 0;
    // The unnamed character-region word the Amy tail clears last; role unknown.
    std::uint32_t amy_finish_word = 0;
    std::unique_ptr<BigActorAssets> big_assets;
    std::unique_ptr<BigMotionAssets> big_motion_assets;
    PlayerJoinTable big_weld;
    big_selection::State big;
    // NOTSA: typed fields share the identity of the actual player allocation.
    struct BigExtraWork {
        TaskWorkHandle allocation;
        std::uint32_t free_flags = 0;
        TaskHandle lure;
        // The free-work float the stand slide check compares with the
        // analog magnitude; its producer is unrecovered.
        float slide_threshold = 0;
        float morph_amount = 0;
    };
    std::deque<BigExtraWork> big_work;
    std::unique_ptr<AmyActorAssets> amy_assets;
    std::unique_ptr<AmyMotionAssets> amy_motion_assets;
    PlayerJoinTable amy_weld;
    // NOTSA: typed fields share the identity of the actual player allocation.
    struct AmyExtraWork {
        TaskWorkHandle allocation;
        // The display fade pair and free bytes mode zero clears at playerwk
        // +0x94/+0x98/+0x80/+0x81/+0x82; their consumer is AmyDisplay.
        float display_target = 0, display_current = 0;
        std::uint8_t byte_80 = 0, byte_81 = 0, byte_82 = 0;
    };
    std::deque<AmyExtraWork> amy_work;
    std::unique_ptr<MilesActorAssets> miles_assets;
    std::unique_ptr<MilesMotionAssets> miles_motion_assets;
    PlayerJoinTable miles_weld;
    miles_selection::State miles;
    std::unique_ptr<KnucklesActorAssets> knuckles_assets;
    std::unique_ptr<KnucklesMotionAssets> knuckles_motion_assets;
    PlayerJoinTable knuckles_weld;
    knuckles_selection::State knuckles;
    player_underwater::State underwater;
};
// NOTSA: existing session asset load binds imported declarations to its shared scene owners.
void bind_gamma_resources(PlayerSystem &, character_pack::SceneState &, const GammaActorAssets &);
// NOTSA: bind imported setup content to the same character texlist used by rendering.
void bind_gamma_setup_resources(PlayerSystem &, const CharacterTextureAssets &,
    std::array<std::vector<regular_textures::List>, 9> &, const std::vector<std::uint8_t> &);
// NOTSA: imported effects borrow the existing E102HIT list and stable mutable declarations.
void bind_gamma_effects_resources(PlayerSystem &, const CharacterTextureAssets &,
    std::array<std::vector<regular_textures::List>, 9> &, const std::vector<std::uint8_t> &);
// NOTSA: Big weld declarations bind to the same shared scene objects.
void bind_big_resources(PlayerSystem &, character_pack::SceneState &, const BigActorAssets &);
// NOTSA: Amy weld and tail-object declarations bind to the same shared scene objects.
void bind_amy_resources(PlayerSystem &, character_pack::SceneState &, const AmyActorAssets &);
// NOTSA: Miles weld and tail-object declarations bind to the same shared scene objects.
void bind_miles_resources(PlayerSystem &, character_pack::SceneState &, const MilesActorAssets &);
// NOTSA: Knuckles weld declarations bind to the same shared scene objects.
void bind_knuckles_resources(PlayerSystem &, character_pack::SceneState &,
                             const KnucklesActorAssets &);
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
    struct ControllerSample {
        std::uint32_t mask = 0;
        std::uint32_t held = 0;
        std::uint32_t pressed = 0;
    };
    std::function<ControllerSample(std::uint8_t slot)> controller = {};
    std::function<void(std::int32_t tone)> play_effect = {};
    std::array<NativeAnalogInput, 8> *normalized_input = nullptr;
    model_matrices::State *model_matrices = nullptr;
    // NOTSA: borrowed palette operation uses the Session lighting/time owners.
    std::function<void(std::int32_t)> display_palette = {};
    const native::ModelDrawEnvironment *model_environment = nullptr;
    // Source land-candidate count. Zero is the empty table; a positive count
    // still takes the unrecovered attribute walk.
    std::int16_t land_candidate_count = 0;
    // Borrowed session collision-entry owner; never a per-callback copy.
    collision_component::EntryState *collision_entries = nullptr;
};
} // namespace sadx::gamma_selection

namespace sadx::big_selection {
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
    character_pack::SceneState &scene;
    const native::ModelDrawEnvironment *model_environment = nullptr;
    // Borrowed session collision-entry owner; never a per-callback copy.
    collision_component::EntryState *collision_entries = nullptr;
    // Source land-candidate count. Zero is the empty table; a positive count
    // still takes the unrecovered shadow land walk.
    std::int16_t land_candidate_count = 0;
};
} // namespace sadx::big_selection

namespace sadx::amy_selection {
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
    character_pack::SceneState &scene;
    const native::ModelDrawEnvironment *model_environment = nullptr;
    // NOTSA: borrowed palette operation uses the Session lighting/time owners.
    std::function<void(std::int32_t)> display_palette = {};
    // NOTSA: borrowed mission/editor words for CheckEditMode.
    const editor::View *editor_state = nullptr;
    // Borrowed session collision-entry owner; never a per-callback copy.
    collision_component::EntryState *collision_entries = nullptr;
    // Source land-candidate count. Zero is the empty table; a positive count
    // still takes the unrecovered shadow land walk.
    std::int16_t land_candidate_count = 0;
};
} // namespace sadx::amy_selection

namespace sadx::miles_selection {
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
    character_pack::SceneState &scene;
    const native::ModelDrawEnvironment *model_environment = nullptr;
    // Borrowed session collision-entry owner; never a per-callback copy.
    collision_component::EntryState *collision_entries = nullptr;
    // Source land-candidate count. Zero is the empty table; a positive count
    // still takes the unrecovered shadow land walk.
    std::int16_t land_candidate_count = 0;
};
} // namespace sadx::miles_selection

namespace sadx::knuckles_selection {
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
    character_pack::SceneState &scene;
    const native::ModelDrawEnvironment *model_environment = nullptr;
    // NOTSA: borrowed mission/editor words for CheckEditMode.
    const editor::View *editor_state = nullptr;
    // Borrowed session collision-entry owner; never a per-callback copy.
    collision_component::EntryState *collision_entries = nullptr;
    // Source land-candidate count. Zero is the empty table; a positive count
    // still takes the unrecovered shadow land walk.
    std::int16_t land_candidate_count = 0;
};
} // namespace sadx::knuckles_selection

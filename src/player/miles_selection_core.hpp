#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/character_pack.hpp"
#include "player/player_initialization.hpp"
#include "player/player_join_vertices.hpp"

#include <cstdint>

namespace sadx::miles_selection {
struct Context;
using Player = player_initialization::PlayerHandle;
using Primary = player_initialization::PrimaryView;

struct State {
    // GLOBAL: SONIC 0x3c49c58 MILES_OBJECTS_31_Sibling size=4
    character_pack::ResourceId object_31_sibling = 0;
    // GLOBAL: SONIC 0x3c49ce8 MILES_OBJECTS_28_Sibling size=4
    character_pack::ResourceId object_28_sibling = 0;
    // GLOBAL: SONIC 0x3c49cf0 MILES_OBJECTS_51_Sibling size=4
    character_pack::ResourceId object_51_sibling = 0;
    // GLOBAL: SONIC 0x3c49cec MILES_OBJECTS_48_Sibling size=4
    character_pack::ResourceId object_48_sibling = 0;
};

// NOTSA: per-consumption bindings into existing native owners. Missing native
// facts fail here. Captured work handles preserve aliases if callees republish.
State &state(Context &);
Primary primary(Context &, const TaskWorkHandle &);
Player player_for_motion(Context &, const TaskWorkHandle &);
TaskWorkHandle player_allocation(Context &, const Player &);
std::uint16_t &requested_animation(Context &, const Player &);
std::uint16_t &current_animation(Context &, const Player &);
std::uint16_t &equipment(Context &, const Player &);
std::uint16_t &idle_timer(Context &, const Player &);
bool has_held_task(Context &, const Player &);
bool has_collision(Context &, const TaskWorkHandle &);
std::uint32_t &collision_attributes(Context &, const TaskWorkHandle &, unsigned record);
float &collision_first_extent(Context &, const TaskWorkHandle &);
std::int32_t floor_angle(Context &, const TaskWorkHandle &, unsigned axis);
float vertical_speed(Context &, const TaskWorkHandle &);
float player_radius(Context &, const Player &);
TaskCallback display_callback(Context &);
TaskCallback destructor_callback(Context &);
fidelity::ExactFloat exact(Context &, float);
std::int32_t game_mode(Context &);
// NOTSA: the high byte of the task counter owns the tail's collision gate.
bool collision_range_skipped(Context &, Primary);
// NOTSA: delegates to the existing canonical PInitialize; no second initializer.
void initialize_player(Context &, std::int32_t index, const TaskHandle &);
// NOTSA: bind imported complete table and native weld resources at source stores.
void bind_animation_table(Context &, const Player &);
void bind_weld_table(Context &, const Player &);
// NOTSA: native binding to canonical collision_component::initialize.
void initialize_collision(Context &, const TaskHandle &, unsigned records, std::uint8_t id);
// NOTSA: native binding to canonical regular_textures::load_character.
void load_character_textures(Context &, std::uint8_t character);
// NOTSA: native binding to canonical collision_component::enter_list.
void enter_collision_list(Context &, Primary);
// NOTSA: native binding calls the shared native::run_underwater product owner.
void underwater(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native/reference binding to the canonical PSetMotion owner.
void set_motion(Context &, const Player &);
// NOTSA: native/reference binding to the canonical PJoinVertexes owner.
void join_vertices(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native binding calls character_shadow::update.
void character_shadow(Context &, Primary, const Player &);
// NOTSA: borrowed mutable scene object rows for the Miles tail swaps.
character_pack::ObjectResource &tail_object(Context &, unsigned table_index);
character_pack::ResourceId sibling_object(Context &, unsigned table_index);
character_pack::ResourceId tail_model(Context &, unsigned table_index);
character_pack::ObjectResource &scene_object(Context &, character_pack::ResourceId);
PlayerJoinTable &weld_table(Context &);
// NOTSA: native/reference binding to the canonical PGetRotation owner.
void get_rotation(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native/reference binding to the canonical PGetAcceleration owner.
void get_acceleration(Context &, Primary, const TaskWorkHandle &, const Player &);
// Source seam owned by the Big core's identical PGetSpeed declaration.
void get_speed(Context &, Primary, const TaskWorkHandle &, const Player &);
// Source seam owned by the Big core's identical PSetPosition declaration.
std::int32_t set_position(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native/reference binding to the canonical PResetPosition owner.
void reset_position(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: the reached display prefix checks the view through the shared owner.
bool check_view(Context &, Primary, float radius);
// NOTSA: marks an unrecovered source mode body; must fail loudly.
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t mode);

// The reached entry prefix, mode zero, mode one and common tail are recovered
// in the core; these callees stay named seams until their bodies land.
// STUB: SONIC 0x45e5d0 Tails_Act1
void update_action_state(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x461450 FUN_00461450
void initialize_auxiliary_a(Context &, std::uint8_t index);
// STUB: SONIC 0x4614b0 FUN_004614b0
void initialize_auxiliary_b(Context &, std::uint8_t index);
// STUB: SONIC 0x459bf0 FUN_00459bf0
void stand_tail(Context &, const TaskWorkHandle &);
// NOTSA: the source center update is recovered in the Sonic player owner; the
// Miles binding stays a named boundary until that path is wired.
void update_collision_center(Context &);
// STUB: SONIC 0x45d460 FUN_0045d460
void update_held_task(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x45b6a0 FUN_0045b6a0
void update_collision_range(Context &, Primary);
// STUB: SONIC 0x41c530 SET_COLLI_RANGE
void set_collision_range(Context &, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x4345d0 SetDefaultNormalCameraMode
void set_default_normal_camera_mode(std::int32_t mode, std::int32_t adjust);
// STUB: SONIC 0x460c40 MilesDisplay animated submit after the view check
[[noreturn]] void missing_display_submit(Context &);
// STUB: SONIC 0x45bea0 MilesDestruct
void destroy(Context &, const TaskHandle &);
// NOTSA: child tasks Miles creates in mode 0 keep their source callbacks as
// seams until those callbacks are recovered. The binding stores the player
// index byte into each created child's primary work, as the source does.
TaskHandle create_child_task(Context &, unsigned kind, std::int32_t level, std::uint8_t index);

// STUB: SONIC 0x461700 MilesTalesPrower mode 0, mode 1 and common tail recovered; other modes unresolved
void run(Context &, const TaskHandle &);
} // namespace sadx::miles_selection

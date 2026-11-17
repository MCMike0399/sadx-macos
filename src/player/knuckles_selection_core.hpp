#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/character_pack.hpp"
#include "player/player_initialization.hpp"
#include "player/player_join_vertices.hpp"

#include <cstdint>

namespace sadx::knuckles_selection {
struct Context;
using Player = player_initialization::PlayerHandle;
using Primary = player_initialization::PrimaryView;

struct State {
    // The tail clears this source word every frame; its readers are unrecovered,
    // so no annotated owner is claimed yet.
    std::int32_t tail_word = 0;
    // The post-dispatch store marks the current mode active; its readers are
    // unrecovered, so no annotated owner is claimed yet.
    std::int32_t action_active = 0;
    // Mode zero clears the reference point the unrecovered tail helper compares
    // against. The roles are not established, so no annotated owner is claimed.
    float reference_x = 0, reference_y = 0, reference_z = 0;
};

// NOTSA: per-consumption bindings into existing native owners. Missing native
// facts fail here. Captured work handles preserve aliases if callees republish.
State &state(Context &);
Primary primary(Context &, const TaskWorkHandle &);
Player player_for_motion(Context &, const TaskWorkHandle &);
TaskWorkHandle player_allocation(Context &, const Player &);
std::uint16_t &requested_animation(Context &, const Player &);
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
std::int32_t check_edit_mode(Context &);
// NOTSA: the shared EV_CheckCansel leaf declared by the Gamma core.
std::int32_t event_cancel(Context &);
// NOTSA: delegates to the existing canonical PInitialize; no second initializer.
void initialize_player(Context &, std::int32_t index, const TaskHandle &);
// NOTSA: bind imported complete table and native weld resources at source stores.
void bind_animation_table(Context &, const Player &);
void bind_weld_table(Context &, const Player &);
// NOTSA: native binding to canonical collision_component::initialize.
void initialize_collision(Context &, const TaskHandle &, unsigned records, std::uint8_t id);
// NOTSA: native binding to canonical regular_textures::load_character.
void load_character_textures(Context &, std::uint8_t character);
// NOTSA: native binding to canonical collision_component::calculate_range.
void calculate_collision_range(Context &, Primary);
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
// STUB: SONIC 0x472010 Knuckles_IdleState
void update_idle_state(Context &, Primary, const Player &);
// STUB: SONIC 0x478020 Knuckles_Act1
void update_action_state(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x477d30 Knuckles_AuxiliaryTask
void initialize_auxiliary(Context &, std::uint8_t index);
// STUB: SONIC 0x4726a0 Knuckles_Upgrades
void update_upgrades(Context &, Primary, const TaskWorkHandle &);
// STUB: SONIC 0x472d60 Knuckles_Something_A
void stand_tail(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x477700 Knuckles_HeldObject
void update_held_task(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x474ba0 Knuckles_CollisionShape
void update_collision_shape(Context &, Primary, const Player &);
// STUB: SONIC 0x474f80 Knuckles_PositionReference
void update_reference_position(Context &, Primary);
// STUB: SONIC 0x4751b0 IsInDeathZone_
bool is_in_death_zone(Context &, Primary);
// STUB: SONIC 0x4419c0 GetPlayerPosition
void other_player_position(Context &, std::uint8_t player, std::int8_t frame, NinjaVector &);
// STUB: SONIC 0x4721b0 KnucklesDisplay animated submit after the view check
[[noreturn]] void missing_display_submit(Context &);
// No Ghidra procedure exists for this stop; the binding fails explicitly.
// STUB: SONIC 0x472180 KnucklesDestruct
void destroy(Context &, const TaskHandle &);
// NOTSA: child tasks Knuckles creates in mode 0 keep their source callbacks as
// seams until those callbacks are recovered. The binding stores the player
// index byte into each created child's primary work, as the source does.
TaskHandle create_child_task(Context &, unsigned kind, std::int32_t level, std::uint8_t index);
// STUB: SONIC 0x476440 EmeraldRadarHud_Load_Load
TaskHandle create_radar_task(Context &);

// STUB: SONIC 0x47a770 KnucklesTheEchidna mode 0, mode 1 and common tail recovered; other modes unresolved
void run(Context &, const TaskHandle &);
} // namespace sadx::knuckles_selection

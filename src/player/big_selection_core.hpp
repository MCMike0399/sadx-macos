#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/character_pack.hpp"
#include "player/player_initialization.hpp"

#include <cstdint>

namespace sadx::big_selection {
struct Context;
using Player = player_initialization::PlayerHandle;
using Primary = player_initialization::PrimaryView;

struct State {
    // GLOBAL: SONIC 0x3c524e8 Sakana_Num size=4
    std::int32_t fish_count = 0;
    // GLOBAL: SONIC 0x3c524ec Big_Fish_Flag size=2
    std::int16_t fish_flag = 0;
    // GLOBAL: SONIC 0x3c52500 Big_Lure_Ptr size=4
    TaskHandle lure_task;
    // GLOBAL: SONIC 0x3c52518 Big_Stg12_Flag size=4
    std::int32_t stage12_flag = 0;
    // The source float mode 0 writes (1.5 in event mode, else 5.5). Its
    // consumers are unrecovered, so no annotated owner is claimed yet.
    float display_scale = 0;
};

// NOTSA: borrowed Sonic/Big publications, never a second current-player owner.
struct CurrentView {
    TaskWorkHandle &primary, &player;
};
// NOTSA: per-consumption bindings into existing native owners. Missing native
// facts fail here. Captured work handles preserve aliases if callees republish.
State &state(Context &);
CurrentView current(Context &);
Primary primary(Context &, const TaskWorkHandle &);
Player player_for_motion(Context &, const TaskWorkHandle &);
TaskWorkHandle player_allocation(Context &, const Player &);
std::uint16_t &requested_animation(Context &, const Player &);
std::uint16_t &current_animation(Context &, const Player &);
std::uint16_t &previous_animation(Context &, const Player &);
std::uint16_t &player_flags(Context &, const Player &);
std::uint16_t &equipment(Context &, const Player &);
std::uint16_t &item_word(Context &, const Player &);
std::uint32_t &free_flags(Context &, const Player &);
TaskHandle &lure_handle(Context &, const Player &);
std::uint16_t &idle_timer(Context &, const Player &);
bool has_collision(Context &, const TaskWorkHandle &);
std::uint8_t &collision_damage(Context &, const TaskWorkHandle &, unsigned record);
std::uint32_t &collision_attributes(Context &, const TaskWorkHandle &, unsigned record);
float &collision_first_extent(Context &, const TaskWorkHandle &);
std::int32_t floor_angle(Context &, const TaskWorkHandle &, unsigned axis);
bool has_held_task(Context &, const Player &);
float vertical_speed(Context &, const TaskWorkHandle &);
float player_radius(Context &, const Player &);
TaskCallback display_callback(Context &);
TaskCallback destructor_callback(Context &);
fidelity::ExactFloat exact(Context &, float);
std::int32_t game_mode(Context &);
std::int16_t stage(Context &);
std::uint16_t act(Context &);
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
// NOTSA: marks an unrecovered source mode body; must fail loudly.
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t mode);

// The reached entry and stand paths of BigUpdateActionState are recovered in
// the core; its checker callees stay named seams until their bodies land.
// STUB: SONIC 0x48d1f0 BigCheckDamage
bool check_damage(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x48c550 BigHurtReaction
void hurt_reaction(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x4506f0 DamegeRingScatter
void scatter_rings(Context &, std::uint8_t index);
// STUB: SONIC 0x48d400 BigStandCheckA
bool stand_check_a(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x48c190 BigStandCheckB
bool stand_check_b(Context &);
// STUB: SONIC 0x48db90 BigStandCheckC
bool stand_check_c(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x48d2a0 BigStandCheckD
bool stand_check_d(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x48c1c0 BigStandCheckE
bool stand_check_e(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x48d380 BigStandCheckF
bool stand_check_f(Context &, const TaskWorkHandle &);
// NOTSA: native/reference bindings to the canonical stage pickup/action owners.
bool stage_pickup(Context &, Primary, const TaskWorkHandle &, const Player &);
bool stage_action(Context &, Primary, const Player &);
// STUB: SONIC 0x48b690 BigChaoSetup
void chao_setup(Context &, const TaskWorkHandle &, std::uint8_t index);
// NOTSA: native/reference binding to the canonical analog input gate.
bool analog_input(Context &, Primary);
float analog_magnitude(Context &);
float contact_depth(Context &, const Player &);
float &slide_threshold(Context &, const Player &);
std::uint16_t &wait_timer(Context &);
// STUB: SONIC 0x470330 BigCreateFlagTask
TaskHandle create_flag_task(Context &, const TaskHandle &);
// STUB: SONIC 0x4909a0 BigSetupForPlayer
void setup_for_player(Context &, const TaskWorkHandle &motion, std::uint8_t index);
// NOTSA: native/reference binding to the canonical PGetRotation owner.
void get_rotation(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native/reference binding to the canonical PGetAcceleration owner.
void get_acceleration(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x443f50 PGetSpeed
void get_speed(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x44cdf0 PSetPosition
std::int32_t set_position(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native/reference binding to the canonical PResetPosition owner.
void reset_position(Context &, Primary, const TaskWorkHandle &, const Player &);
// NOTSA: native/reference binding to the canonical PSetMotion owner.
void set_motion(Context &, const Player &);
// NOTSA: native/reference binding to the canonical PJoinVertexes owner.
void join_vertices(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x48b790 BigUpdateRod
void update_rod(Context &, const TaskWorkHandle &);
// The reached BigDisplay prefix is recovered in the core; its view check and
// animated submit stay seams until their bodies land.
// NOTSA: native/reference binding to the canonical dsCheckViewV owner.
bool check_view(Context &, Primary);
// STUB: SONIC 0x48b8b0 BigDisplay animated submit after the view check
[[noreturn]] void missing_display_submit(Context &);
// STUB: SONIC 0x48b730 BigDestruct
void destroy(Context &, const TaskHandle &);
// STUB: SONIC 0x48e000 BigUpdateHeldTask
void update_held_task(Context &, const TaskWorkHandle &);
// NOTSA: native binding calls character_shadow::update.
void character_shadow(Context &, Primary, const Player &);
// STUB: SONIC 0x48ba80 BigStandTail
void stand_tail(Context &, const TaskWorkHandle &);
// NOTSA: borrowed mutable scene object rows for the equipment tail swaps.
character_pack::ObjectResource &equipment_object(Context &, unsigned table_index);
character_pack::ResourceId equipment_identity(Context &, unsigned table_index);
character_pack::ResourceId equipment_model(Context &, unsigned table_index);
float &morph_amount(Context &, const Player &);
// STUB: SONIC 0x48c0c0 BigCheckMorph
bool check_morph(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x4385b0 BigMorphVertices
void morph_vertices(Context &, unsigned target, float amount);
// NOTSA: child tasks Big creates in mode 0 keep their source callbacks as
// seams until those callbacks are recovered. The binding stores the player
// index byte into each created child's primary work, as the source does.
TaskHandle create_child_task(Context &, unsigned kind, std::int32_t level, std::uint8_t index);
// STUB: SONIC 0x471ed0 FishingHud_Load
TaskHandle create_lure_task(Context &, const TaskHandle &);
// NOTSA: the stage-19 texture branch loads texlist_big directly instead of
// the regular character textures.
void load_big_pvm_textures(Context &);

// STUB: SONIC 0x490a00 BigTheCat mode 0, mode 1 and common tail recovered; other modes unresolved
void run(Context &, const TaskHandle &);
} // namespace sadx::big_selection

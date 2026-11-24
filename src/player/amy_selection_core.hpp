#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/character_pack.hpp"
#include "player/player_initialization.hpp"

#include <cstdint>

namespace sadx::amy_selection {
struct Context;
using Player = player_initialization::PlayerHandle;
using Primary = player_initialization::PrimaryView;
enum class Axis { X, Y, Z };

// NOTSA: per-player free fields mode zero clears and the display prefix reads.
struct ExtraView {
    float &display_target, &display_current;
    std::uint8_t &byte_80, &byte_81, &byte_82;
};

// NOTSA: per-consumption bindings into existing native owners. Missing native
// facts fail here. Captured work handles preserve aliases if callees republish.
ExtraView extra(Context &, const Player &);
Primary primary(Context &, const TaskWorkHandle &);
Player player_for_motion(Context &, const TaskWorkHandle &);
TaskWorkHandle player_allocation(Context &, const Player &);
std::uint16_t &requested_animation(Context &, const Player &);
std::uint16_t &current_animation(Context &, const Player &);
std::uint16_t &equipment(Context &, const Player &);
// NOTSA: the source item word at playerwk+6, distinct from the +0x10 status flag.
std::uint16_t &item_word(Context &, const Player &);
// NOTSA: the source task wait timer at taskwk+6.
std::uint16_t &wait_timer(Context &);
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
// NOTSA: CheckEditMode binding borrows the native owners or calls the reference.
std::int32_t check_edit_mode(Context &);
// NOTSA: native stage owner consumed by the Chao branch and the stage seams.
std::int16_t stage(Context &);
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
// NOTSA: common-tail sibling selection from AMY_OBJECTS rows 36..38.
void update_tail_objects(Context &, const Player &);
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
// NOTSA: marks an unrecovered source mode body; must fail loudly.
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t mode);

// The reached entry prefix, mode zero, mode one and common tail are recovered
// in the core; these callees stay named seams until their bodies land.
// FUNCTION: SONIC 0x488880 AmyUpdateActionState
void update_action_state(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x4842c0 AmyChaoSetup
void chao_setup(Context &, const TaskWorkHandle &motion, const Player &, std::uint8_t index);
// STUB: SONIC 0x484f40 AmyCheckDamage
bool check_damage(Context &, Primary, const TaskWorkHandle &motion, const Player &);
// STUB: SONIC 0x4856a0 AmyHurtReaction
void hurt_reaction(Context &, Primary, const Player &);
// Source seam owned by the Big core's identical DamegeRingScatter declaration.
void scatter_rings(Context &, std::uint8_t index);
// The stand chain stops at the first nonzero transition. Its preview-zero
// branches are recovered where an existing native owner proves them; the
// transition bodies stay seams.
// STUB: SONIC 0x487810 AmyCheckNextAction
bool check_next_action(Context &, Primary, const Player &, const TaskWorkHandle &motion);
// STUB: SONIC 0x485730 AmyCheckFall
bool check_fall(Context &, Primary, const Player &, const TaskWorkHandle &motion);
// STUB: SONIC 0x4858e0 AmyCheckDropOff
bool check_drop_off(Context &, Primary, const Player &);
// STUB: SONIC 0x485870 AmyCheckDrtk
bool check_drtk(Context &, Primary, const Player &);
// STUB: SONIC 0x487640 AmyCheckJump
bool check_jump(Context &, Primary, const Player &);
// STUB: SONIC 0x485380 AmyCheckGrab
bool check_grab(Context &, Primary, const Player &, const TaskWorkHandle &motion);
// STUB: SONIC 0x485040 AmyCheckHandle
bool check_handle(Context &, Primary, const Player &, const TaskWorkHandle &motion);
// STUB: SONIC 0x4850c0 AmyCheckBalance
bool check_balance(Context &, Primary, const Player &);
// STUB: SONIC 0x485800 AmyCheckHat
bool check_hat(Context &, Primary, const Player &);
// NOTSA: native/reference bindings to the canonical stage pickup/action owners.
bool stage_pickup(Context &, Primary, const TaskWorkHandle &motion, const Player &);
bool stage_action(Context &, Primary, const TaskWorkHandle &motion, const Player &);
// NOTSA: native/reference binding to the canonical GetAnalog input gate.
bool analog_input(Context &, Primary);
// NOTSA: horizontal source motion speed consumed by the stand-to-walk gate.
float horizontal_speed(Context &, const TaskWorkHandle &motion);
// STUB: SONIC 0x488820 AmySetupForPlayer
void setup_for_player(Context &, const TaskWorkHandle &motion, std::uint8_t index);
// STUB: SONIC 0x4c6820 FUN_004c6820
void initialize_auxiliary_a(Context &);
// STUB: SONIC 0x4b3d90 FUN_004b3d90
void initialize_auxiliary_b(Context &);
// STUB: SONIC 0x486a40 FUN_00486a40
void initialize_auxiliary_c(Context &);
// STUB: SONIC 0x4c5610 FUN_004c5610
void initialize_auxiliary_d(Context &);
// STUB: SONIC 0x486ca0 FUN_00486ca0
void initialize_auxiliary_e(Context &);
// STUB: SONIC 0x484520 AmyStandTail
void stand_tail(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x4843e0 FUN_004843e0
void post_display(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x488390 AmyUpdateHeldTask
void update_held_task(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x488640 FUN_00488640
void post_shadow(Context &);
// STUB: SONIC 0x4c56c0 FUN_004c56c0
void finish_frame(Context &);
// NOTSA: display-prefix and body bindings to the canonical renderer owners.
void set_z_compare(Context &, std::uint32_t);
void reset_z_compare(Context &);
std::int32_t event_cancel(Context &);
std::uint16_t animation_mode(Context &, const Player &);
float animation_frame(Context &, const Player &);
float player_height(Context &, const Player &);
bool has_event_action(Context &, const TaskWorkHandle &);
character_pack::ObjectResource &tail_sibling(Context &);
character_pack::ObjectResource &tail_sibling_child(Context &);
void select_palette(Context &, std::int32_t);
void select_body_textures(Context &);
void save_control(Context &);
void set_control(Context &, std::uint32_t);
void restore_control(Context &);
void set_white_material(Context &);
void restore_material(Context &);
void push_current_frame(Context &);
void push_unit_frame(Context &);
void pop_frame(Context &);
void rotate(Context &, Axis axis, std::int32_t angle);
NinjaVector calculate_vector(Context &, const NinjaVector &);
void translate(Context &, const NinjaVector &);
void scale(Context &, const NinjaVector &);
void store_collision_center(Context &, const TaskWorkHandle &, const NinjaVector &);
void draw_action(Context &, const Player &, std::uint16_t animation, float frame);
// NOTSA: generated-action display path awaiting a native owner.
[[noreturn]] void draw_generated_action(Context &, const Player &, float frame);
// STUB: SONIC 0x4187d0 DrawEventModel awaiting recovery
[[noreturn]] void draw_event_model(Context &);
// STUB: SONIC 0x486cd0 AmyDrawSpecialBody special modes awaiting recovery
bool special_body(Context &, std::uint8_t mode);
bool paused(Context &);
void draw_character_shadow(Context &, const Player &, const TaskHandle &);

// FUNCTION: SONIC 0x487260 AmyDisplay
void display(Context &, const TaskHandle &);
// STUB: SONIC 0x484360 AmyDestruct
void destroy(Context &, const TaskHandle &);
// NOTSA: child tasks Amy creates in mode 0 keep their source callbacks as
// seams until those callbacks are recovered. The binding stores the player
// index byte into each created child's primary work, as the source does.
TaskHandle create_child_task(Context &, unsigned kind, std::int32_t level, std::uint8_t index);

// STUB: SONIC 0x48abf0 AmyRose mode 0, mode 1 and common tail recovered; other modes unresolved
void run(Context &, const TaskHandle &);
} // namespace sadx::amy_selection

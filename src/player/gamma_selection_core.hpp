#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/ninja_matrix.hpp"
#include "player/player_initialization.hpp"
#include "player/player_scene_resources.hpp"

namespace sadx::gamma_selection {
struct Context;
using Player = player_initialization::PlayerHandle;
using Primary = player_initialization::PrimaryView;
using MaterialPointer = SceneMaterial *;
using PointPointer = NinjaVector *;
using ColorPointer = std::uint32_t *;
using TextureCoordinatesPointer = const void *;

// NOTSA: borrowed Sonic/Gamma publications, never a second current-player owner.
struct CurrentView {
    TaskWorkHandle &primary, &player;
};
// NOTSA: recovered fields of the actual E102WK allocation. The binding owns
// this view with that allocation; it must not create another player or pose.
struct ExtraView {
    std::uint8_t &laser_mode;
    std::uint16_t &general_timer;
    float &ground_y, &ground_distance;
    std::uint32_t &ground_attributes;
    float &walk_distance;
};
struct State {
    // GLOBAL: SONIC 0x3c53c40 e102_hover_flag size=1
    std::uint8_t hover = 0;
    // GLOBAL: SONIC 0x3c53c41 e102_hover_flag_p size=1
    std::uint8_t previous_hover = 0;
    // GLOBAL: SONIC 0x3c53c42 Gamma_IsTimeZero size=1
    std::uint8_t time_zero = 0;
    // GLOBAL: SONIC 0x3c53b18 GammaEventEnabled size=1
    std::uint8_t event_enabled = 0;
    // GLOBAL: SONIC 0x3c53bf8 GammaTimeHudEnabled size=1
    std::uint8_t time_hud_enabled = 0;
    // GLOBAL: SONIC 0x3c53b70 e102_work_ptr size=4
    TaskWorkHandle current_extra;
    // GLOBAL: SONIC 0x3c54608 GammaHeadlightMaterial size=4
    MaterialPointer headlight_material = nullptr;
    // The source groups these four fields as NJS_POINT3COL for late_DrawLine3D.
    // GLOBAL: SONIC 0x3c53b60 GammaLinePoints size=4
    PointPointer line_points = nullptr;
    // GLOBAL: SONIC 0x3c53b64 GammaLineColors size=4
    ColorPointer line_colors = nullptr;
    // GLOBAL: SONIC 0x3c53b68 GammaLineTexture size=4
    TextureCoordinatesPointer line_texture = nullptr;
    // GLOBAL: SONIC 0x3c53b6c GammaLinePointCount size=4
    std::uint32_t line_point_count = 0;
    // GLOBAL: SONIC 0x3c53f68 GammaLineVertices size=24
    std::array<NinjaVector, 2> line_vertices = {};
    // GLOBAL: SONIC 0x3c53ad8 GammaModelNode54Matrix size=48
    NinjaMatrix node54_matrix = {};
    // GLOBAL: SONIC 0x3c53c48 GammaModelNode30Matrix size=48
    NinjaMatrix node30_matrix = {};
    // GLOBAL: SONIC 0x3c53b78 GammaModelNode39Matrix size=48
    NinjaMatrix node39_matrix = {};
    // GLOBAL: SONIC 0x3c53b20 GammaModelNode2Matrix size=48
    NinjaMatrix node2_matrix = {};
    // GLOBAL: SONIC 0x3c53c00 GammaModelNode37Matrix size=48
    NinjaMatrix node37_matrix = {};
};

// NOTSA: per-consumption bindings into existing native owners. Missing native
// facts fail here. Captured work handles preserve aliases if callees republish.
State &state(Context &);
CurrentView current(Context &);
Primary primary(Context &, const TaskWorkHandle &);
Player player_for_motion(Context &, const TaskWorkHandle &);
TaskWorkHandle player_allocation(Context &, const Player &);
TaskWorkHandle extra_work(Context &, const Player &);
void store_extra_work(Context &, const Player &, const TaskWorkHandle &);
ExtraView extra(Context &, const TaskWorkHandle &);
std::uint16_t &player_timer(Context &, const Player &);
std::uint16_t &player_flags(Context &, const Player &);
std::uint16_t &requested_animation(Context &, const Player &);
std::uint16_t &current_animation(Context &, const Player &);
float &eyes_height(Context &, const Player &);
float player_height(Context &, const Player &);
float forward_speed(Context &, const Player &);
float vertical_speed(Context &, const TaskWorkHandle &);
bool has_held_task(Context &, const Player &);
bool has_collision(Context &, const TaskWorkHandle &);
std::uint8_t &collision_damage(Context &, const TaskWorkHandle &, unsigned record);
std::uint32_t &collision_attributes(Context &, const TaskWorkHandle &, unsigned record);
float &collision_first_extent(Context &, const TaskWorkHandle &);
std::int32_t floor_angle(Context &, const TaskWorkHandle &, unsigned axis);
SceneMaterial &material(Context &, unsigned table_row, unsigned material_row);
ColorPointer line_colors(Context &);
TaskCallback display_callback(Context &);
TaskCallback destructor_callback(Context &);
fidelity::ExactFloat exact(Context &, float);
// NOTSA: delegates to the existing canonical PInitialize; no second initializer.
void initialize_player(Context &, std::int32_t index, const TaskHandle &);
// NOTSA: bind imported complete table and native weld resources at source stores.
void bind_animation_table(Context &, const Player &);
void bind_weld_table(Context &, const Player &);
// NOTSA: marks an unrecovered source mode body; must fail loudly.
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t mode);

// STUB: SONIC 0x42fb00 EV_CheckCansel
std::int32_t event_cancel(Context &);
// STUB: SONIC 0x426690 IsTimeZero
std::uint8_t is_time_zero(Context &);
// STUB: SONIC 0x4c3780 GammaUpdateLaserMode
void update_laser_mode(Context &, const TaskWorkHandle &, std::uint8_t index, bool mode_three);
// STUB: SONIC 0x4258f0 CheckEditMode
std::int32_t check_edit_mode(Context &);
// STUB: SONIC 0x481460 GammaProcessAction
void process_action(Context &, const TaskHandle &, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x47fb90 GammaUpdateActionState
void update_action_state(Context &, Primary, const Player &);
// STUB: SONIC 0x41caf0 CCL_Init
void initialize_collision(Context &, const TaskHandle &, unsigned records, std::uint8_t id);
// STUB: SONIC 0x421280 LoadCharTextures
void load_character_textures(Context &, std::uint8_t character);
// STUB: SONIC 0x40b2d0 CAlloc
TaskWorkHandle allocate_zero(Context &, unsigned count, unsigned bytes);
// STUB: SONIC 0x4c2e10 GammaInitializeCursors
void initialize_cursors(Context &, const TaskWorkHandle &);
// STUB: SONIC 0x4c5420 GammaLoadSetupResource
void load_setup_resource(Context &);
// STUB: SONIC 0x40b940 CreateChildTask
void create_time_hud(Context &, unsigned level, const TaskHandle &);
// STUB: SONIC 0x49fb10 GammaInitializeEffects
void initialize_effects(Context &);
// STUB: SONIC 0x44a800 PSetMotion
void set_motion(Context &, const Player &);
// STUB: SONIC 0x43fa90 PJoinVertexes
void join_vertices(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x4c3870 GammaUpdateLaserTargets
void update_laser_targets(Context &, const TaskWorkHandle &, std::uint8_t index);
// STUB: SONIC 0x4b82d0 GetMMMatrix
void get_model_matrix(Context &, unsigned node, NinjaMatrix &);
// STUB: SONIC 0x784fb0 njRotateZ
void rotate_z(Context &, NinjaMatrix &, std::int32_t angle);
// STUB: SONIC 0x4c2e90 GammaUpdateFan
void update_fan(Context &, Primary, const Player &, const TaskWorkHandle &);
// STUB: SONIC 0x47fd50 E102Display
void display(Context &, const TaskHandle &);
// STUB: SONIC 0x47eed0 E102Destruct
void destroy(Context &, const TaskHandle &);
// STUB: SONIC 0x4c4da0 GammaUpdateTargetEffects
void update_target_effects(Context &, Primary, const Player &, const TaskWorkHandle &);
// STUB: SONIC 0x4c44a0 GammaUpdateGunEffects
void update_gun_effects(Context &, const TaskHandle &, const Player &, const TaskWorkHandle &);
// STUB: SONIC 0x481180 GammaUpdateHeldTask
void update_held_task(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x49ee30 Shadow
fidelity::ExactFloat shadow(Context &, Primary, float scale);
// STUB: SONIC 0x4c2f50 GammaGroundAttributes
std::uint32_t ground_attributes(Context &, Primary);
// STUB: SONIC 0x4c33c0 GammaRunningEffects
void running_effects(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x41bac0 CCL_CalcRange
void calculate_collision_range(Context &, Primary);
// STUB: SONIC 0x41c280 EntryColliList
void enter_collision_list(Context &, Primary);
// STUB: SONIC 0x446f10 RunUnderwaterStuff
void underwater(Context &, Primary, const TaskWorkHandle &, const Player &);
// STUB: SONIC 0x40b420 LoopTaskC
void run_children(Context &, const TaskHandle &);
// NOTSA: source stage and act are borrowed signed/unsigned 16-bit owners.
std::int16_t stage(Context &);
std::uint16_t act(Context &);

// STUB: SONIC 0x483430 E102 mode 0 and common tail recovered; other modes unresolved
void run(Context &, const TaskHandle &);
} // namespace sadx::gamma_selection

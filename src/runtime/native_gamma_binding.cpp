#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "player/selection_player_queries.hpp"
#include "game/game_mode.hpp"

namespace sadx::gamma_selection {
namespace {
[[noreturn]] void missing(const char *boundary) {
    throw flow::UnrecoveredGameFlowCall(boundary);
}
TaskWorkArena &arena(Context &context) {
    const auto owner = context.scheduler.work_arena();
    if (!owner) missing("native_gamma_task_arena");
    return *owner;
}
native::GammaActorAssets &assets(Context &context) {
    if (!context.players.gamma_assets) missing("native_gamma_actor_assets");
    return *context.players.gamma_assets;
}
PlayerPhysicsState &physics(const Player &player) {
    if (!player || !player->physics) missing("native_gamma_player_physics");
    return *player->physics;
}
// NOTSA: per-consumption access follows the actual primary allocation reference.
std::shared_ptr<collision_component::Work> collision(Context &context,
                                                     const TaskWorkHandle &primary_work) {
    auto component = collision_component::component(arena(context), primary_work);
    if (!component) missing("native_gamma_null_collision_component");
    return component;
}
collision_component::Info &collision_info(Context &context, const TaskWorkHandle &primary_work,
                                          unsigned record) {
    auto component = collision(context, primary_work);
    if (record >= component->info_count || record >= component->info.size())
        missing("native_gamma_collision_info_record");
    return component->info[record];
}
constexpr unsigned kMotionPlayerField = 0, kCollisionField = 56, kFreeWorkField = 124;
} // namespace
State &state(Context &context) { return context.players.gamma; }
CurrentView current(Context &context) {
    return {context.players.current.primary, context.players.current.player};
}
Primary primary(Context &context, const TaskWorkHandle &work) {
    if (work != context.task->primary_work) missing("native_gamma_primary_identity");
    return player_initialization::bind_primary_view(context.task);
}
Player player_for_motion(Context &context, const TaskWorkHandle &motion) {
    const auto reference = arena(context).reference(motion, kMotionPlayerField);
    if (!reference) missing("native_gamma_motion_player_reference");
    if (!reference.work) return {};
    const auto found = context.players.registry.allocations.find(reference.work.get());
    if (found == context.players.registry.allocations.end())
        missing("native_gamma_player_allocation_identity");
    return found->second;
}
TaskWorkHandle player_allocation(Context &, const Player &player) {
    return player ? player->allocation : TaskWorkHandle{};
}
TaskWorkHandle extra_work(Context &context, const Player &player) {
    if (!player) missing("native_gamma_null_player_free_work");
    const auto reference = arena(context).reference(player->allocation, kFreeWorkField);
    if (!reference) missing("native_gamma_player_free_work_reference");
    return reference.work;
}
void store_extra_work(Context &context, const Player &player, const TaskWorkHandle &work) {
    if (!player || !arena(context).set_reference(player->allocation, kFreeWorkField, work))
        missing("native_gamma_player_free_work_store");
}
ExtraView extra(Context &context, const TaskWorkHandle &allocation) {
    if (!allocation || allocation->released()) missing("native_gamma_null_E102WK");
    for (auto &work : context.players.gamma_work)
        if (work.allocation == allocation)
            return {work.laser_mode, work.general_timer, work.ground_y, work.ground_distance,
                    work.ground_attributes, work.walk_distance};
    missing("native_gamma_E102WK_allocation_identity");
}
std::uint16_t &player_timer(Context &, const Player &player) {
    // The source field is the unsigned view of playerwk.wtimer.
    return player_initialization::bind_initialized_player(player).timer;
}
std::uint16_t &player_flags(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).flags;
}
std::uint16_t &requested_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).requested_animation;
}
std::uint16_t &current_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).current_animation;
}
float &eyes_height(Context &, const Player &player) {
    if (!player) missing("native_gamma_player_parameters");
    return player->parameters.eyes_height;
}
float player_height(Context &, const Player &player) {
    if (!player) missing("native_gamma_player_parameters");
    return player->parameters.height;
}
float forward_speed(Context &, const Player &player) { return physics(player).motion.speed.x; }
float vertical_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work) missing("native_gamma_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.y;
}
bool has_held_task(Context &, const Player &player) {
    return bool(player_initialization::bind_initialized_player(player).held_task);
}
bool has_collision(Context &context, const TaskWorkHandle &primary_work) {
    const auto reference = arena(context).reference(primary_work, kCollisionField);
    if (!reference) missing("native_gamma_collision_reference");
    return bool(reference.work);
}
std::uint8_t &collision_damage(Context &context, const TaskWorkHandle &primary_work,
                               unsigned record) {
    return collision_info(context, primary_work, record).damage;
}
std::uint32_t &collision_attributes(Context &context, const TaskWorkHandle &primary_work,
                                    unsigned record) {
    return collision_info(context, primary_work, record).attributes;
}
float &collision_first_extent(Context &context, const TaskWorkHandle &primary_work) {
    return collision_info(context, primary_work, 0).a;
}
std::int32_t floor_angle(Context &context, const TaskWorkHandle &primary_work, unsigned axis) {
    const auto task = collision(context, primary_work)->task.lock();
    if (!task) missing("native_gamma_collision_task_backlink");
    return player_initialization::bind_primary_view(task).rotation.at(axis);
}
SceneMaterial &material(Context &, unsigned, unsigned) {
    missing("native_gamma_shared_material_binding");
}
ColorPointer line_colors(Context &context) { return assets(context).line_colors.data(); }
TaskCallback display_callback(Context &context) {
    if (!context.display) missing("native_gamma_display_callback");
    return context.display;
}
TaskCallback destructor_callback(Context &context) {
    if (!context.destructor) missing("native_gamma_destructor_callback");
    return context.destructor;
}
fidelity::ExactFloat exact(Context &context, float value) { return {context.rendering.arithmetic(), value}; }
void initialize_player(Context &context, std::int32_t index, const TaskHandle &task) {
    player_initialization::NativeAccess access(assets(context).parameters,
                                              context.bootstrap.event_work, context.auxiliary);
    Player record;
    player_initialization::initialize(index, task,
        player_initialization::bind_existing_motion(task), context.scheduler,
        context.players.registry, access, context.rendering.arithmetic(), record);
}
void bind_animation_table(Context &context, const Player &player) {
    const auto &table = assets(context).animation_table;
    if (!table || table->records.size() != 91) missing("native_gamma_complete_animation_table");
    player_initialization::bind_initialized_player(player).animation_table = table;
}
void bind_weld_table(Context &, const Player &) { missing("native_gamma_weld_binding"); }
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) {
    missing("native_gamma_unrecovered_mode");
}
std::int32_t event_cancel(Context &context) {
    return selection_player_queries::event_cancel(context.bootstrap.cutscene_task);
}
std::uint8_t is_time_zero(Context &context) {
    return selection_player_queries::is_time_zero(context.bootstrap.frames,
        context.bootstrap.seconds, context.bootstrap.minutes);
}
std::int16_t stage(Context &context) { return context.stage; }
std::uint16_t act(Context &context) { return context.act; }
// NOTSA: native CCL_Init boundary; source seam declared in the Gamma core.
void initialize_collision(Context &context, const TaskHandle &task, unsigned records,
                           std::uint8_t id) {
    auto primary = player_initialization::bind_primary_view(task);
    // Gamma ignores the source return. A failed allocation remains null and
    // faults at the subsequent first-extent access; do not turn failure into success.
    (void)collision_component::initialize(arena(context), task, primary.status,
        assets(context).collision_info, static_cast<std::int32_t>(records), id,
        context.rendering.arithmetic());
}
TaskWorkHandle allocate_zero(Context &context, unsigned count, unsigned bytes) {
    constexpr unsigned kWordAlignmentMask = 3;
    const auto extent = ((bytes + kWordAlignmentMask) & ~kWordAlignmentMask) * count;
    auto allocation = arena(context).allocate(extent);
    if (!allocation) return {};
    if (!arena(context).fill(allocation.work, 0, 0, extent))
        missing("native_gamma_CAlloc_zero_fill");
    constexpr unsigned kExtraWorkBytes = 580;
    if (count == 1 && bytes == kExtraWorkBytes)
        context.players.gamma_work.push_back({allocation.work, 0, 0, 0, 0, 0, 0});
    return allocation.work;
}
// NOTSA: native boundary for GammaUpdateLaserMode; source seam declared in the Gamma core.
void update_laser_mode(Context &, const TaskWorkHandle &, std::uint8_t, bool) {
    missing("native_gamma_update_laser_mode");
}
// NOTSA: native boundary for CheckEditMode; source seam declared in the Gamma core.
std::int32_t check_edit_mode(Context &) {
    missing("native_gamma_check_edit_mode");
}
// NOTSA: native boundary for GammaProcessAction; source seam declared in the Gamma core.
void process_action(Context &, const TaskHandle &, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_process_action");
}
// NOTSA: native boundary for GammaUpdateActionState; source seam declared in the Gamma core.
void update_action_state(Context &, Primary, const Player &) {
    missing("native_gamma_update_action_state");
}
// NOTSA: native boundary for LoadCharTextures; source seam declared in the Gamma core.
void load_character_textures(Context &, std::uint8_t) {
    missing("native_gamma_load_character_textures");
}
// NOTSA: native boundary for GammaInitializeCursors; source seam declared in the Gamma core.
void initialize_cursors(Context &, const TaskWorkHandle &) {
    missing("native_gamma_initialize_cursors");
}
// NOTSA: native boundary for GammaLoadSetupResource; source seam declared in the Gamma core.
void load_setup_resource(Context &) {
    missing("native_gamma_load_setup_resource");
}
// NOTSA: native boundary for CreateChildTask; source seam declared in the Gamma core.
void create_time_hud(Context &, unsigned, const TaskHandle &) {
    missing("native_gamma_create_time_hud");
}
// NOTSA: native boundary for GammaInitializeEffects; source seam declared in the Gamma core.
void initialize_effects(Context &) {
    missing("native_gamma_initialize_effects");
}
// NOTSA: native boundary for PSetMotion; source seam declared in the Gamma core.
void set_motion(Context &, const Player &) {
    missing("native_gamma_set_motion");
}
// NOTSA: native boundary for PJoinVertexes; source seam declared in the Gamma core.
void join_vertices(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_join_vertices");
}
// NOTSA: native boundary for GammaUpdateLaserTargets; source seam declared in the Gamma core.
void update_laser_targets(Context &, const TaskWorkHandle &, std::uint8_t) {
    missing("native_gamma_update_laser_targets");
}
// NOTSA: native boundary for GetMMMatrix; source seam declared in the Gamma core.
void get_model_matrix(Context &, unsigned, NinjaMatrix &) {
    missing("native_gamma_get_model_matrix");
}
// NOTSA: native boundary for njRotateZ; source seam declared in the Gamma core.
void rotate_z(Context &, NinjaMatrix &, std::int32_t) {
    missing("native_gamma_rotate_z");
}
// NOTSA: native boundary for GammaUpdateFan; source seam declared in the Gamma core.
void update_fan(Context &, Primary, const Player &, const TaskWorkHandle &) {
    missing("native_gamma_update_fan");
}
// NOTSA: native boundary for E102Display; source seam declared in the Gamma core.
void display(Context &, const TaskHandle &) {
    missing("native_gamma_display");
}
// NOTSA: native boundary for E102Destruct; source seam declared in the Gamma core.
void destroy(Context &, const TaskHandle &) {
    missing("native_gamma_destroy");
}
// NOTSA: native boundary for GammaUpdateTargetEffects; source seam declared in the Gamma core.
void update_target_effects(Context &, Primary, const Player &, const TaskWorkHandle &) {
    missing("native_gamma_update_target_effects");
}
// NOTSA: native boundary for GammaUpdateGunEffects; source seam declared in the Gamma core.
void update_gun_effects(Context &, const TaskHandle &, const Player &, const TaskWorkHandle &) {
    missing("native_gamma_update_gun_effects");
}
// NOTSA: native boundary for GammaUpdateHeldTask; source seam declared in the Gamma core.
void update_held_task(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_update_held_task");
}
// NOTSA: native boundary for Shadow; source seam declared in the Gamma core.
fidelity::ExactFloat shadow(Context &, Primary, float) {
    missing("native_gamma_shadow");
}
// NOTSA: native boundary for GammaGroundAttributes; source seam declared in the Gamma core.
std::uint32_t ground_attributes(Context &, Primary) {
    missing("native_gamma_ground_attributes");
}
// NOTSA: native boundary for GammaRunningEffects; source seam declared in the Gamma core.
void running_effects(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_running_effects");
}
// NOTSA: native boundary for CCL_CalcRange; source seam declared in the Gamma core.
void calculate_collision_range(Context &, Primary) {
    missing("native_gamma_calculate_collision_range");
}
// NOTSA: native boundary for EntryColliList; source seam declared in the Gamma core.
void enter_collision_list(Context &, Primary) {
    missing("native_gamma_enter_collision_list");
}
// NOTSA: native boundary for RunUnderwaterStuff; source seam declared in the Gamma core.
void underwater(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_underwater");
}
// NOTSA: native boundary for LoopTaskC; source seam declared in the Gamma core.
void run_children(Context &, const TaskHandle &) {
    missing("native_gamma_run_children");
}
} // namespace sadx::gamma_selection

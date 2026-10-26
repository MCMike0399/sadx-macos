#include "player/gamma_selection_core.hpp"

namespace sadx::gamma_selection {
namespace {
constexpr std::uint8_t kGamma = 6;
constexpr std::uint16_t kFollowFloorReference = 32; // Status_Unknown3; role from this source guard
constexpr std::uint16_t kOnCollision = 2; // Status_OnColli
constexpr std::uint16_t kInvincibleItem = 32768; // Powerups_Invincibility
enum class Mode : std::uint8_t { // MD_E102, New.h names
    Initialize = 0, // MD_E102_INIT
    Stand = 1, // MD_E102_STND
    GroundRun = 31, // MD_E102_GRUN
    WallJump = 35, // MD_E102_WJMP
    WallJumpHolding = 36, // MD_E102_WJPH
    GroundStopHolding = 40, // MD_E102_GHST
    Kill = 51, // MD_E102_KILL
    Cart = 53, // MD_E102_CART
    PetLoop = 56, // MD_E102_CNADE_LOOP
    Edit = 60 // MD_E102_EDIT
};
constexpr std::uint32_t kMaterialRetainedFlags = 0x03efffff;
constexpr std::uint32_t kMaterialGammaFlags = 0x80100000;
constexpr std::uint32_t kOpaqueAlpha = 0xff000000;
constexpr unsigned kExtraWorkBytes = 580;

// NOTSA: named entry step; captured primary/motion/player are source locals.
void enter(Context &context, Primary task, const TaskWorkHandle &primary_work,
           const TaskWorkHandle &motion, const Player &player) {
    auto &globals = state(context);
    const auto previous_hover = globals.hover;
    auto publications = current(context);
    publications.primary = primary_work;
    publications.player = player_allocation(context, player);
    globals.previous_hover = previous_hover;
    globals.hover = 0;
    globals.event_enabled = event_cancel(context) == 0;
    const auto zero = is_time_zero(context);
    const auto flags = task.status;
    globals.time_zero = zero;
    bool aligned = false;
    if ((flags & kFollowFloorReference) != 0) {
        const auto comparison = exact(context, vertical_speed(context, motion)).compare(0.0f);
        aligned = comparison == X87Comparison::less || comparison == X87Comparison::equal;
    }
    task.status = aligned ? flags | kOnCollision : flags & ~kOnCollision;
    if (aligned) {
        task.rotation[0] = floor_angle(context, primary_work, 0);
        task.rotation[2] = floor_angle(context, primary_work, 2);
    }
}

// NOTSA: the pre-dispatch collision changes are shared by all source modes.
void prepare_collision(Context &context, Primary task, const TaskWorkHandle &primary_work,
                       const Player &player) {
    if (!has_collision(context, primary_work))
        return;
    const auto invincible = (player_flags(context, player) & kInvincibleItem) != 0;
    const std::uint8_t outgoing = invincible ? 3 : 0;
    const auto mode = static_cast<Mode>(task.mode);
    const std::uint8_t incoming = invincible ? 3 : mode == Mode::Cart ? 1 : 0;
    constexpr std::uint8_t kDamageClassMask = 15;
    auto &first = collision_damage(context, primary_work, 0);
    first = (first & ~kDamageClassMask) | outgoing | (incoming << 2);
    auto &second = collision_damage(context, primary_work, 1);
    second = (second & ~kDamageClassMask) | outgoing | (incoming << 2);
    const auto signed_mode = static_cast<std::int8_t>(task.mode);
    const bool low_eyes = signed_mode >= static_cast<std::int8_t>(Mode::GroundRun) &&
                          signed_mode <= static_cast<std::int8_t>(Mode::GroundStopHolding) &&
                          mode != Mode::WallJump && mode != Mode::WallJumpHolding;
    eyes_height(context, player) = low_eyes ? 10.0f : 21.0f;
    constexpr std::uint32_t kCollisionEnabled = 16;
    auto &attributes = collision_attributes(context, primary_work, 2);
    attributes = mode == Mode::PetLoop ? attributes & ~kCollisionEnabled
                                      : attributes | kCollisionEnabled;
}

// NOTSA: named constructor step; each material lookup re-reads the shared table.
void initialize_materials(Context &context) {
    auto &first = material(context, 1, 6);
    first.words[4] = (first.words[4] & kMaterialRetainedFlags) | kMaterialGammaFlags;
    for (unsigned row : {5U, 4U, 3U, 2U}) {
        auto &entry = material(context, row, 2);
        if (row == 2)
            state(context).headlight_material = &entry;
        entry.words[4] = (entry.words[4] & kMaterialRetainedFlags) | kMaterialGammaFlags;
        entry.words[0] |= kOpaqueAlpha;
    }
}

// NOTSA: mode 0 is one named source step, not a replacement actor factory.
Player initialize(Context &context, const TaskHandle &object, Primary task,
                  const TaskWorkHandle &primary_work, const TaskWorkHandle &motion,
                  TaskWorkHandle &local_extra) {
    const auto index = task.player_index; // captured before the character store
    task.character = kGamma;
    initialize_player(context, index, object);
    auto player = player_for_motion(context, motion);
    bind_animation_table(context, player);
    requested_animation(context, player) = 0;
    initialize_collision(context, object, 3, 0);
    collision_first_extent(context, primary_work) = player_height(context, player);
    load_character_textures(context, kGamma);
    bind_weld_table(context, player);
    object->destructor = destructor_callback(context);
    object->display = display_callback(context);
    local_extra = allocate_zero(context, 1, kExtraWorkBytes);
    store_extra_work(context, player, local_extra);
    player_timer(context, player) = 0;
    // Original calls the cursor initializer even when allocation returned null.
    initialize_cursors(context, local_extra);
    extra(context, local_extra).walk_distance = 0.0f;
    auto &globals = state(context);
    globals.line_points = globals.line_vertices.data();
    globals.line_texture = nullptr;
    globals.line_point_count = 2;
    globals.line_colors = line_colors(context);
    task.mode = static_cast<std::uint8_t>(Mode::Stand);
    load_setup_resource(context);
    initialize_materials(context);
    globals.time_hud_enabled = 0;
    constexpr std::uint8_t kGameplayTask = 1;
    constexpr std::int32_t kLastActionStage = 25 << 8;
    if (task.id == kGameplayTask && event_cancel(context) != 1) {
        const auto packed = (static_cast<std::int32_t>(stage(context)) * 256) | act(context);
        constexpr std::uint32_t kStageMask = 0xff00;
        const auto stage_bits = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(packed) & kStageMask);
        if (stage_bits <= kLastActionStage) {
            globals.time_hud_enabled = 1;
            create_time_hud(context, 2, object);
        }
    }
    initialize_effects(context);
    task.status |= 1;
    return player;
}

// NOTSA: shared source tail, including the actual display and child-task calls.
void common_tail(Context &context, const TaskHandle &object, Primary task,
                 const TaskWorkHandle &motion, const Player &player,
                 const TaskWorkHandle &local_extra) {
    auto &globals = state(context);
    globals.current_extra = local_extra;
    set_motion(context, player);
    join_vertices(context, task, motion, player);
    update_laser_targets(context, extra_work(context, player), task.player_index);
    constexpr unsigned kModelNode54 = 54, kModelNode30 = 30, kModelNode39 = 39;
    constexpr unsigned kModelNode2 = 2, kModelNode37 = 37;
    get_model_matrix(context, kModelNode54, globals.node54_matrix);
    get_model_matrix(context, kModelNode30, globals.node30_matrix);
    get_model_matrix(context, kModelNode39, globals.node39_matrix);
    constexpr std::int32_t kQuarterTurn = 16384;
    rotate_z(context, globals.node39_matrix, kQuarterTurn);
    get_model_matrix(context, kModelNode2, globals.node2_matrix);
    get_model_matrix(context, kModelNode37, globals.node37_matrix);
    update_fan(context, task, player, local_extra);
    display(context, object);
    update_target_effects(context, task, player, extra_work(context, player));
    update_gun_effects(context, object, player, extra_work(context, player));
    if (has_held_task(context, player))
        update_held_task(context, task, motion, player);
    const auto ground_extra = extra_work(context, player);
    const auto ground = shadow(context, task, 1.5f);
    extra(context, ground_extra).ground_y = ground.rounded();
    extra(context, ground_extra).ground_distance =
        (exact(context, task.position.y) - ground).rounded();
    const auto attributes = ground_attributes(context, task);
    auto work = extra(context, ground_extra);
    ++work.general_timer;
    work.ground_attributes = attributes;
    const auto animation = requested_animation(context, player);
    constexpr std::uint16_t kRun = 5, kRunAlternate = 53;
    if ((animation == kRun || animation == kRunAlternate) &&
        exact(context, forward_speed(context, player)).compare(3.5f) == X87Comparison::greater)
        running_effects(context, task, motion, player);
    constexpr std::uint16_t kNoCollisionAnimationA = 67, kNoCollisionAnimationB = 68;
    const auto playing = current_animation(context, player);
    if (playing != kNoCollisionAnimationA && playing != kNoCollisionAnimationB) {
        calculate_collision_range(context, task);
        enter_collision_list(context, task);
    }
    underwater(context, task, motion, player);
    run_children(context, object);
}
} // namespace

void run(Context &context, const TaskHandle &object) {
    const auto motion = object->motion_work;
    auto player = player_for_motion(context, motion);
    const auto primary_work = object->primary_work;
    auto task = primary(context, primary_work);
    enter(context, task, primary_work, motion, player);
    TaskWorkHandle local_extra;
    if (player) {
        local_extra = extra_work(context, player);
        const bool mode_three = extra(context, local_extra).laser_mode == 3;
        update_laser_mode(context, local_extra, task.player_index, mode_three);
        if (check_edit_mode(context) != 0)
            task.mode = static_cast<std::uint8_t>(Mode::Edit);
        else if (static_cast<Mode>(task.mode) != Mode::Kill) {
            process_action(context, object, motion, player);
            update_action_state(context, task, player);
        }
    }
    prepare_collision(context, task, primary_work, player);
    if (static_cast<Mode>(task.mode) == Mode::Initialize)
        player = initialize(context, object, task, primary_work, motion, local_extra);
    else
        unrecovered_mode(context, task.mode);
    common_tail(context, object, task, motion, player, local_extra);
}
} // namespace sadx::gamma_selection

#include "player/knuckles_selection_core.hpp"

namespace sadx::knuckles_selection {
namespace {
constexpr std::uint8_t kKnuckles = 3;
constexpr std::uint16_t kFollowFloorReference = 32;
constexpr std::uint16_t kOnCollision = 2;
constexpr std::uint16_t kSkipVertexJoin = 256;
constexpr std::uint32_t kCollisionEnable = 16;
constexpr std::uint16_t kIdleTimerReset = 65535;
constexpr std::uint16_t kThruAnimation = 39;
constexpr std::uint16_t kShakAnimation = 40;
constexpr float kViewRadius = 20.0f;
// The death-zone rescue probes the other player positions within 0.01 units.
constexpr float kDeathZoneSeparation = 0.010000001f;
enum class Mode : std::uint8_t { // MD_KNUCK, SADXEnumsNew.h names
    Initialize = 0,              // MD_KNUCK_INIT
    Stand = 1,                   // MD_KNUCK_STND
    Glide = 11,                  // MD_KNUCK_GLID
    Turn = 12,                   // MD_KNUCK_GTRN
    Kill = 51,                   // MD_KNUCK_KILL
    CnadeLoop = 54,              // MD_KNUCK_CNADE_LOOP
    Edit = 57                    // MD_KNUCK_EDIT forced by CheckEditMode
};
enum class GameMode : std::int32_t { // MD_GameMode, SADXEnumsNew.h names
    Action = 4,                      // MD_ACTION
    Adventure = 5,                   // MD_ADVENTURE
    Trial = 9,                       // MD_TRIAL
    Mission = 10                     // MD_MISSION
};

// NOTSA: named entry step; captured primary/motion/player are source locals.
void enter(Context &context, Primary task, const TaskWorkHandle &primary_work,
           const TaskWorkHandle &motion, const Player &player) {
    const auto flags = task.status;
    bool aligned = false;
    if ((flags & kFollowFloorReference) != 0) {
        const auto comparison = exact(context, vertical_speed(context, motion)).compare(0.0f);
        aligned = comparison == X87Comparison::less || comparison == X87Comparison::equal;
    }
    if (!aligned) {
        task.status = flags & ~kOnCollision;
    } else {
        task.status = flags | kOnCollision;
        task.rotation[0] = floor_angle(context, primary_work, 0);
        task.rotation[2] = floor_angle(context, primary_work, 2);
    }
    // The idle timer runs before the edit-mode gate and the action update.
    update_idle_state(context, task, player);
    if (check_edit_mode(context) != 0)
        task.mode = static_cast<std::uint8_t>(Mode::Edit);
    else if (static_cast<Mode>(task.mode) != Mode::Kill)
        update_action_state(context, task, motion, player);
    if (!has_collision(context, primary_work))
        return;
    auto &attributes = collision_attributes(context, primary_work, 2);
    attributes = static_cast<Mode>(task.mode) == Mode::CnadeLoop ? attributes & ~kCollisionEnable
                                                                 : attributes | kCollisionEnable;
}

// NOTSA: mode 0 is one named source step, not a replacement actor factory.
Player initialize(Context &context, const TaskHandle &object, Primary task,
                  const TaskWorkHandle &primary_work, const TaskWorkHandle &motion) {
    const auto index = task.player_index; // captured before the character store
    task.character = kKnuckles;
    initialize_player(context, index, object);
    auto player = player_for_motion(context, motion);
    bind_animation_table(context, player);
    requested_animation(context, player) = 0;
    initialize_collision(context, object, 5, 0);
    collision_first_extent(context, primary_work) = player_radius(context, player);
    task.mode = static_cast<std::uint8_t>(Mode::Stand);
    load_character_textures(context, kKnuckles);
    bind_weld_table(context, player);
    idle_timer(context, player) = kIdleTimerReset;
    initialize_auxiliary(context, index);
    (void)create_child_task(context, 0, 3, index);
    (void)create_child_task(context, 1, 3, index);
    object->destructor = destructor_callback(context);
    object->display = display_callback(context);
    if (event_cancel(context) != 1 &&
        (game_mode(context) == static_cast<std::int32_t>(GameMode::Action) ||
         game_mode(context) == static_cast<std::int32_t>(GameMode::Mission) ||
         game_mode(context) == static_cast<std::int32_t>(GameMode::Trial))) {
        (void)create_radar_task(context);
    }
    // The heat-attack child also receives a source pointer to the idle timer.
    // Its consumer is unrecovered and the timer is not a task allocation, so
    // the binding publishes the player index only.
    (void)create_child_task(context, 2, 3, index);
    // Source order: z, y, then x.
    auto &globals = state(context);
    globals.reference_z = 0;
    globals.reference_y = 0;
    globals.reference_x = 0;
    return player;
}

// NOTSA: named display step of the common tail. The view check gates the
// whole draw; the animated submit lands with it.
void display(Context &context, const TaskHandle &object) {
    auto task = primary(context, object->primary_work);
    if (!check_view(context, task, kViewRadius))
        return;
    missing_display_submit(context);
}

// NOTSA: the death-zone rescue walks the other player positions until one is
// far enough away, then nudges out over the frame history. IsInDeathZone_ and
// GetPlayerPosition stay seams until their bodies land.
void rescue_from_death_zone(Context &context, Primary task) {
    if (!is_in_death_zone(context, task))
        return;
    std::uint8_t frame = 1;
    float dx = 0, dz = 0;
    do {
        NinjaVector position{};
        other_player_position(context, task.player_index, static_cast<std::int8_t>(frame), position);
        dx = position.x - task.position.x;
        dz = position.z - task.position.z;
        if (kDeathZoneSeparation < dx * dx + dz * dz)
            break;
        ++frame;
    } while (frame < 128);
    frame = 128;
    task.position.x = dx + task.position.x;
    task.position.z = dz + task.position.z;
    do {
        if (!is_in_death_zone(context, task))
            break;
        --frame;
        task.position.x = dx + task.position.x;
        task.position.z = dz + task.position.z;
    } while (frame != 0);
}

// NOTSA: named common tail step; runs after every mode in source order.
void common_tail(Context &context, const TaskHandle &object, Primary task,
                 const TaskWorkHandle &motion, const Player &player) {
    set_motion(context, player);
    update_upgrades(context, task, motion);
    if ((task.status & kSkipVertexJoin) == 0)
        join_vertices(context, task, motion, player);
    display(context, object);
    if (has_held_task(context, player))
        update_held_task(context, motion);
    character_shadow(context, task, player);
    update_collision_shape(context, task, player);
    update_reference_position(context, task);
    state(context).tail_word = 0;
    if (requested_animation(context, player) != kThruAnimation &&
        requested_animation(context, player) != kShakAnimation) {
        calculate_collision_range(context, task);
        enter_collision_list(context, task);
    }
    underwater(context, task, motion, player);
}
} // namespace

void run(Context &context, const TaskHandle &object) {
    const auto motion = object->motion_work;
    auto player = player_for_motion(context, motion);
    const auto primary_work = object->primary_work;
    auto task = primary(context, primary_work);
    enter(context, task, primary_work, motion, player);
    switch (static_cast<Mode>(task.mode)) {
    case Mode::Initialize:
        player = initialize(context, object, task, primary_work, motion);
        break;
    case Mode::Stand:
        get_rotation(context, task, motion, player);
        get_acceleration(context, task, motion, player);
        get_speed(context, task, motion, player);
        (void)set_position(context, task, motion, player);
        reset_position(context, task, motion, player);
        stand_tail(context, motion);
        break;
    default:
        unrecovered_mode(context, task.mode);
    }
    // The source marks the active action before the death-zone rescue.
    const auto mode = static_cast<Mode>(task.mode);
    state(context).action_active = mode == Mode::Kill || mode == Mode::Initialize ? 0 : 1;
    if (mode != Mode::Glide && mode != Mode::Turn &&
        (game_mode(context) == static_cast<std::int32_t>(GameMode::Action) ||
         game_mode(context) == static_cast<std::int32_t>(GameMode::Adventure) ||
         game_mode(context) == static_cast<std::int32_t>(GameMode::Trial)))
        rescue_from_death_zone(context, task);
    common_tail(context, object, task, motion, player);
}
} // namespace sadx::knuckles_selection

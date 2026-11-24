#include "player/knuckles_selection_core.hpp"

#include <array>
#include <cstddef>

namespace sadx::knuckles_selection {
namespace {
constexpr std::uint8_t kKnuckles = 3;
constexpr std::uint16_t kFollowFloorReference = 32;
constexpr std::uint16_t kOnCollision = 2;
constexpr std::uint16_t kSkipVertexJoin = 256;
constexpr std::uint32_t kCollisionEnable = 16;
constexpr std::uint16_t kIdleTimerReset = 65535;
constexpr std::uint16_t kIdleTimerLimit = 0x78;
constexpr std::uint16_t kIdleEquipmentFlag = 0x20;
constexpr std::uint16_t kActionBitsMask = 3;
constexpr std::uint16_t kIdleLoopMask = 15;
constexpr std::uint16_t kIdleTaskTimer = 0x00b4;
constexpr std::uint16_t kIdleRequestedAnimation = 0x23;
constexpr std::int32_t kIdleCameraStageLimit = 0xf00;
constexpr std::int16_t kIdleCameraMode = 0x44;
constexpr std::int32_t kIdleTransitionSound = 0x319;
constexpr std::int32_t kIdleLoopSound = 0x318;
constexpr std::int32_t kIdleClampSound = 0x15;
constexpr std::uint16_t kThruAnimation = 39;
constexpr std::uint16_t kShakAnimation = 40;
constexpr float kViewRadius = 20.0f;
// Knuckles_Act1's damage gate and stand-chain literals.
constexpr std::uint16_t kHurt = 4;
constexpr std::uint16_t kHurtReactionStatus = 0x800;
constexpr std::uint16_t kStatusHighBit = 0x8000;
constexpr std::uint16_t kStatusHighClear = 0x7fff;
constexpr std::uint16_t kItemHighBit = 0x8000;
constexpr std::uint16_t kItemLowBitsClear = 0xfffc;
constexpr std::uint16_t kDamageHighByteKeep = 0xc6ff;
constexpr std::uint16_t kWaitTimerKeep = 0x7ff0;
constexpr std::uint8_t kStagePickupMode = 0x35;   // MD_KNUCK_CNADE_START
constexpr std::uint16_t kStagePickupAnimation = 0x6f;
constexpr std::uint32_t kStageActionArgument = 0x723801;
constexpr std::uint16_t kWalkAnimation = 9;
// Knuckles_Upgrades: playerwk+4 equipment bits and the two model lists they
// select. KNUCKLES_OBJECTS rows 58..73 receive one model each in table order.
constexpr std::uint16_t kUpgradeMask = 0x60;
constexpr std::uint16_t kFightingGloves = 0x20;
constexpr std::uint16_t kShovelClaw = 0x40;
constexpr unsigned kFirstEquipmentObjectRow = 58;
using EquipmentModels = std::array<std::uint8_t, 16>;
constexpr EquipmentModels kNoUpgradeModels = {
    7, 8, 9, 10, 11, 12, 13, 14, 7, 8, 9, 10, 11, 12, 13, 14};
constexpr EquipmentModels kFightingGlovesModels = {
    15, 16, 17, 18, 19, 20, 21, 22, 15, 16, 17, 18, 19, 20, 21, 22};
constexpr EquipmentModels kShovelClawModels = {
    7, 8, 0, 10, 11, 12, 2, 14, 7, 8, 0, 10, 11, 12, 2, 14};
constexpr EquipmentModels kFightingGlovesShovelClawModels = {
    15, 16, 1, 18, 19, 20, 3, 22, 15, 16, 1, 18, 19, 20, 3, 22};
// The death-zone rescue probes the other player positions within 0.01 units.
constexpr float kDeathZoneSeparation = 0.010000001f;
enum class Mode : std::uint8_t { // MD_KNUCK, SADXEnumsNew.h names
    Initialize = 0,              // MD_KNUCK_INIT
    Stand = 1,                   // MD_KNUCK_STND
    Walk = 2,                    // MD_KNUCK_WALK
    Roll = 4,                    // MD_KNUCK_ROLL
    Stop = 10,                   // MD_KNUCK_STOP
    Glide = 11,                  // MD_KNUCK_GLID
    Turn = 12,                   // MD_KNUCK_GTRN
    Hmg0 = 18,                   // MD_KNUCK_HMG0
    Dig = 19,                    // MD_KNUCK_DIGD
    Punch = 26,                  // MD_KNUCK_PUNC
    Doza = 50,                   // MD_KNUCK_DOZA
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
    update_upgrades(context, player);
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

void update_idle_state(Context &context, Primary task, const Player &player) {
    if (!player)
        return;
    if ((equipment(context, player) & kIdleEquipmentFlag) == 0) {
        idle_timer(context, player) = kIdleTimerReset;
        return;
    }
    const auto mode = static_cast<Mode>(task.mode);
    if (mode == Mode::Doza || mode == Mode::Kill || mode == Mode::Punch ||
        mode == Mode::Hmg0)
        return;
    const auto timer = static_cast<std::int16_t>(idle_timer(context, player));
    if (timer < 0)
        return;
    const auto attack_held =
        (held_buttons(context, task.player_index) & attack_buttons(context)) != 0;
    const auto action_bits = (task.status & kActionBitsMask) != 0;
    const auto action_mode = mode == Mode::Stand || mode == Mode::Walk || mode == Mode::Stop;
    if (!attack_held || !action_bits || !action_mode) {
        if (timer == static_cast<std::int16_t>(kIdleTimerLimit)) {
            const auto stage_word = (static_cast<std::int32_t>(stage(context)) << 8) |
                                    static_cast<std::int32_t>(act(context));
            if (stage_word < kIdleCameraStageLimit)
                set_event_camera(context, kIdleCameraMode, 0);
            start_idle_camera_effect(context);
            reset_idle_camera_effect(context);
            task_timer(context, task) = kIdleTaskTimer;
            task.mode = static_cast<std::uint8_t>(Mode::Hmg0);
            requested_animation(context, player) = kIdleRequestedAnimation;
        }
        idle_timer(context, player) = kIdleTimerReset;
        stop_id(context);
        if (static_cast<Mode>(task.mode) != Mode::Dig)
            play_one_shot(context, kIdleTransitionSound, 0, 0, 0);
        return;
    }
    // The source increments through the 16-bit timer word, then tests the signed result.
    const auto incremented = static_cast<std::uint16_t>(static_cast<std::uint16_t>(timer) + 1U);
    idle_timer(context, player) = incremented;
    if (static_cast<std::int16_t>(incremented) < static_cast<std::int16_t>(kIdleTimerLimit)) {
        if ((incremented & kIdleLoopMask) == 0)
            play_loop(context, kIdleLoopSound, 0, 0, 0);
    } else {
        idle_timer(context, player) = kIdleTimerLimit;
        if (timer != static_cast<std::int16_t>(kIdleTimerLimit)) {
            start_held_camera_effect(context);
            stop_id(context);
            play_one_shot(context, kIdleClampSound, 0, 0, 0);
        }
    }
    auto &speed = speed_x(context, player);
    if (0.5f < speed) {
        const auto half = exact(context, 0.5f);
        auto adjusted = half - speed;
        adjusted = adjusted * 0.5f;
        adjusted = adjusted + speed;
        speed = adjusted.rounded();
    }
}

// The reached prefix of Knuckles_Act1: the Chao branch, the damage gate and
// MD_KNUCK_STND's transition chain. The chain's first nonzero result stops the
// frame; every later mode body stays unrecovered and fails loudly.
void update_action_state(Context &context, Primary task, const TaskWorkHandle &motion,
                         const Player &player) {
    const auto current_stage = stage(context);
    if (current_stage >= 39 && current_stage <= 42)
        chao_setup(context, motion, player, task.player_index);
    // The source takes the no-damage branch when playerwk is null (the
    // pre-initialization frame) or when the item high bit is clear.
    const bool damage_item = player && (item_word(context, player) & kItemHighBit) != 0;
    if (!damage_item) {
        auto &wait = wait_timer(context);
        if (wait == 0) {
            if (!check_damage(context, task, player)) {
                task.status = static_cast<std::uint16_t>(task.status & kStatusHighClear);
            } else {
                task.status = static_cast<std::uint16_t>(task.status | kStatusHighBit);
                if ((task.status & kHurtReactionStatus) != 0)
                    hurt_reaction(context, motion);
                task.status = static_cast<std::uint16_t>(task.status & kDamageHighByteKeep);
                auto &item = item_word(context, player);
                if ((item & 3) == 0)
                    scatter_rings(context, task.player_index);
                else
                    item = static_cast<std::uint16_t>(item & kItemLowBitsClear);
            }
        } else if (wait != kWaitTimerKeep) {
            task.status = static_cast<std::uint16_t>(task.status & ~kHurt);
            wait = static_cast<std::uint16_t>(wait - 1U);
        }
    } else {
        task.status = static_cast<std::uint16_t>(task.status | kStatusHighBit);
        wait_timer(context) = 0;
    }
    switch (static_cast<Mode>(task.mode)) {
    case Mode::Initialize:
        // The source switch has no case 0; mode zero's work is the run switch.
        break;
    case Mode::Stand:
        // Source order: the six transition checkers, then the Chao-stage
        // pickup/action pair, then the analog/speed walk gate.
        if (check_next_action(context, task, motion, player) ||
            check_fall(context, motion) || check_dig(context, motion, player) ||
            check_jump(context, task, player) || check_grab(context, task, motion) ||
            check_attack(context, task))
            return;
        if (stage_pickup(context, task, motion, player)) {
            task.mode = kStagePickupMode;
            requested_animation(context, player) = kStagePickupAnimation;
        } else if (stage_action(context, task, motion, player, kStageActionArgument)) {
            return;
        }
        if (analog_input(context, task, motion, player) ||
            exact(context, speed_x(context, player)).compare(0.0f) != X87Comparison::equal) {
            task.mode = static_cast<std::uint8_t>(Mode::Walk);
            requested_animation(context, player) = kWalkAnimation;
            return;
        }
        // Source case 1 ends here; the idle path has no timer or animation store.
        break;
    default:
        unrecovered_mode(context, task.mode);
    }
}

// The source switch masks the equipment byte with the two Knuckles upgrade
// bits. It writes the model rows first, then the shared hand-index block.
void update_upgrades(Context &context, const Player &player) {
    const EquipmentModels *models = nullptr;
    auto hand_index_set = HandIndexSet::Hand;
    switch (equipment(context, player) & kUpgradeMask) {
    case 0:
        models = &kNoUpgradeModels;
        break;
    case kFightingGloves:
        models = &kFightingGlovesModels;
        break;
    case kShovelClaw:
        models = &kShovelClawModels;
        hand_index_set = HandIndexSet::ShovelClaw;
        break;
    case kFightingGloves | kShovelClaw:
        models = &kFightingGlovesShovelClawModels;
        hand_index_set = HandIndexSet::ShovelClaw;
        break;
    }
    if (!models)
        return;
    for (std::size_t index = 0; index < models->size(); ++index) {
        auto &object = equipment_object(
            context, kFirstEquipmentObjectRow + static_cast<unsigned>(index));
        object.model = equipment_model(context, (*models)[index]);
    }
    state(context).hand_index_set = hand_index_set;
}

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

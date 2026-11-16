#include "player/big_selection_core.hpp"

namespace sadx::big_selection {
namespace {
constexpr std::uint8_t kBig = 7;
constexpr std::uint16_t kFollowFloorReference = 32;
constexpr std::uint16_t kOnCollision = 2;
enum class Mode : std::uint8_t { // MD_BIG, New.h names
    Initialize = 0, // MD_BIG_INIT
    Stand = 1, // MD_BIG_STND
    PetLoop = 52, // MD_BIG_CNADE_LOOP
    Cart = 55, // MD_BIG_CART
    SwimWait = 47, // MD_BIG_W_WAIT
    Swim = 48, // MD_BIG_W_SWIM
    Kill = 58 // MD_BIG_KILL
};
constexpr std::uint16_t kCollisionEnabled = 16;
constexpr std::uint16_t kFishFlagHighBit = 0x8000;
constexpr std::uint16_t kItemHighBit = 0x8000;
constexpr std::uint8_t kCartDamageKeep = 0xf4;
constexpr std::uint8_t kDamageClassClear = 0xf0;
constexpr std::uint16_t kStatusGroundReference = 0x400;
constexpr std::uint16_t kActHighByteMask = 0xff00;
constexpr std::uint16_t kStage19Word = 0x1300;
constexpr std::uint16_t kSwimAnimation = 0x30;

// NOTSA: named entry step; captured primary/motion/player are source locals.
void enter(Context &context, Primary task, const TaskWorkHandle &primary_work,
           const TaskWorkHandle &motion, const Player &player) {
    auto publications = current(context);
    publications.primary = primary_work;
    publications.player = player_allocation(context, player);
    const auto flags = task.status;
    bool aligned = false;
    if ((flags & kFollowFloorReference) != 0) {
        const auto comparison = exact(context, vertical_speed(context, motion)).compare(0.0f);
        aligned = comparison == X87Comparison::less || comparison == X87Comparison::equal;
    }
    const auto mode = static_cast<Mode>(task.mode);
    if (!aligned || mode == Mode::SwimWait || mode == Mode::Swim) {
        task.status = flags & ~kOnCollision;
        return;
    }
    task.status = flags | kOnCollision;
    task.rotation[0] = floor_angle(context, primary_work, 0);
    task.rotation[2] = floor_angle(context, primary_work, 2);
    (void)player;
}

// NOTSA: the free-work flag words are consumed before the mode dispatch.
void consume_flags(Context &context, const TaskHandle &object, const Player &player) {
    if (!player)
        return;
    auto &globals = state(context);
    if ((free_flags(context, player) & 1) != 0) {
        lure_handle(context, player) = create_flag_task(context, object);
        free_flags(context, player) &= ~1u;
    }
    if ((free_flags(context, player) & 2) != 0) {
        globals.fish_flag = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(globals.fish_flag) | kFishFlagHighBit);
        free_flags(context, player) &= ~2u;
    }
}

// NOTSA: the pre-dispatch collision changes are shared by all source modes.
void prepare_collision(Context &context, Primary task, const TaskWorkHandle &primary_work,
                       const Player &player) {
    if (!has_collision(context, primary_work))
        return;
    const auto mode = static_cast<Mode>(task.mode);
    auto &damage = collision_damage(context, primary_work, 0);
    if ((item_word(context, player) & kItemHighBit) != 0) {
        damage = damage | 15;
    } else if (mode == Mode::Cart) {
        damage = (damage & kCartDamageKeep) | 4;
    } else {
        auto &attributes = collision_attributes(context, primary_work, 1);
        attributes = (task.status & kStatusGroundReference) == 0 ? attributes | kCollisionEnabled
                                                : attributes & ~kCollisionEnabled;
        damage = damage & kDamageClassClear;
    }
    auto &first = collision_attributes(context, primary_work, 2);
    first = mode == Mode::PetLoop ? first & ~kCollisionEnabled : first | kCollisionEnabled;
}

// NOTSA: mode 0 is one named source step, not a replacement actor factory.
Player initialize(Context &context, const TaskHandle &object, Primary task,
                  const TaskWorkHandle &primary_work, const TaskWorkHandle &motion) {
    const auto index = task.player_index; // captured before the character store
    task.character = kBig;
    initialize_player(context, index, object);
    auto player = player_for_motion(context, motion);
    bind_animation_table(context, player);
    requested_animation(context, player) = 0;
    free_flags(context, player) = 0;
    auto &globals = state(context);
    globals.display_scale = game_mode(context) != 10 ? 5.5f : 1.5f;
    initialize_collision(context, object, 3, 0);
    collision_first_extent(context, primary_work) = player_radius(context, player);
    task.mode = static_cast<std::uint8_t>(Mode::Stand);
    idle_timer(context, player) = 0;
    const auto stage_word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(stage(context)) << 8) |
        (act(context) & kActHighByteMask));
    if (stage_word == kStage19Word)
        load_big_pvm_textures(context);
    else
        load_character_textures(context, kBig);
    setup_for_player(context, index);
    bind_weld_table(context, player);
    if (const auto child = create_child_task(context, 0, 3)) {
        auto child_primary = primary(context, child->primary_work);
        child_primary.player_index = index;
    }
    if (const auto child = create_child_task(context, 1, 3)) {
        auto child_primary = primary(context, child->primary_work);
        child_primary.player_index = index;
    }
    if (index == 0) {
        globals.lure_task = create_lure_task(context, object);
        lure_handle(context, player) = globals.lure_task;
        if (index == 0)
            (void)create_child_task(context, 2, 6);
    }
    globals.fish_count = 0;
    globals.stage12_flag = 0;
    globals.fish_flag = 0;
    object->destructor = destructor_callback(context);
    object->display = display_callback(context);
    return player;
}

// NOTSA: named common tail step; runs after every mode in source order.
void common_tail(Context &context, const TaskHandle &object, Primary task,
                 const TaskWorkHandle &primary_work, const TaskWorkHandle &motion,
                 const Player &player) {
    set_motion(context, player);
    update_equipment(context, player);
    join_vertices(context, task, motion, player);
    update_rod(context, motion);
    display(context, object);
    if (has_held_task(context, player))
        update_held_task(context, motion);
    character_shadow(context, task, player);
    if (current_animation(context, player) != kSwimAnimation) {
        calculate_collision_range(context, task);
        enter_collision_list(context, task);
    }
    underwater(context, task, motion, player);
    (void)primary_work;
}
} // namespace

void run(Context &context, const TaskHandle &object) {
    const auto motion = object->motion_work;
    auto player = player_for_motion(context, motion);
    const auto primary_work = object->primary_work;
    auto task = primary(context, primary_work);
    enter(context, task, primary_work, motion, player);
    if (static_cast<Mode>(task.mode) != Mode::Kill)
        update_action_state(context, task, motion);
    consume_flags(context, object, player);
    prepare_collision(context, task, primary_work, player);
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
    common_tail(context, object, task, primary_work, motion, player);
}
} // namespace sadx::big_selection

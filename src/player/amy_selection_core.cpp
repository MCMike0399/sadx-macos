#include "player/amy_selection_core.hpp"

namespace sadx::amy_selection {
namespace {
constexpr std::uint8_t kAmy = 5;
constexpr std::uint16_t kFollowFloorReference = 32;
constexpr std::uint16_t kOnCollision = 2;
constexpr std::uint16_t kNoCollisionAnimation = 0x2a;
constexpr std::uint32_t kCollisionEnable = 16;
enum class Mode : std::uint8_t { // MD_AMY, New.h names not established
    Initialize = 0,              // MD_AMY_INIT
    Stand = 1,                   // MD_AMY_STND
    Walk = 2,                    // MD_AMY_WALK
    Slash = 47,                  // '/' source mode
    Cart = 50,                   // '2' source mode
    Edit = 53                    // '5' source mode forced by CheckEditMode
};

// The damage gate's status and item words. The 0x800 hurt-reaction test is the
// Amy source literal; Big's equivalent gate tests 0x400.
constexpr std::uint16_t kItemHighBit = 0x8000;
constexpr std::uint16_t kStatusHighBit = 0x8000;
constexpr std::uint16_t kHurtReactionStatus = 0x800;
constexpr std::uint16_t kStatusHighClear = 0x7fff;
constexpr std::uint16_t kHurtClear = 0xfffb;
constexpr std::uint16_t kDamageHighByteKeep = 0xc6ff;
constexpr std::uint16_t kItemLowBitsClear = 0xfffc;
constexpr std::uint8_t kCandyStartMode = 0x31; // MD_AMY_CNADE_START
constexpr std::uint16_t kCandyStartAnimation = 0x51;
constexpr std::uint16_t kWalkAnimation = 9;

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
        return;
    }
    task.status = flags | kOnCollision;
    task.rotation[0] = floor_angle(context, primary_work, 0);
    task.rotation[2] = floor_angle(context, primary_work, 2);
    (void)player;
}

// NOTSA: the pre-dispatch collision change is shared by all source modes; only
// record 2's attributes word is written.
void prepare_collision(Context &context, Primary task, const TaskWorkHandle &primary_work,
                       const Player &player) {
    if (!has_collision(context, primary_work))
        return;
    (void)player;
    auto &attributes = collision_attributes(context, primary_work, 2);
    attributes = static_cast<Mode>(task.mode) == Mode::Cart ? attributes & ~kCollisionEnable
                                                            : attributes | kCollisionEnable;
}

// NOTSA: mode 0 is one named source step, not a replacement actor factory.
Player initialize(Context &context, const TaskHandle &object, Primary task,
                  const TaskWorkHandle &primary_work, const TaskWorkHandle &motion) {
    const auto index = task.player_index; // captured before the character store
    task.character = kAmy;
    initialize_player(context, index, object);
    auto player = player_for_motion(context, motion);
    bind_animation_table(context, player);
    requested_animation(context, player) = 0;
    initialize_collision(context, object, 3, 0);
    collision_first_extent(context, primary_work) = player_radius(context, player);
    task.mode = static_cast<std::uint8_t>(Mode::Stand);
    load_character_textures(context, kAmy);
    bind_weld_table(context, player);
    setup_for_player(context, motion, index);
    (void)create_child_task(context, 0, 3, index);
    (void)create_child_task(context, 1, 3, index);
    (void)create_child_task(context, 2, 3, index);
    auto fields = extra(context, player);
    fields.display_target = 0;
    fields.display_current = 0;
    fields.byte_81 = 0;
    fields.byte_82 = 0;
    object->destructor = destructor_callback(context);
    object->display = display_callback(context);
    initialize_auxiliary_a(context);
    initialize_auxiliary_b(context);
    initialize_auxiliary_c(context);
    fields.byte_80 = 0;
    initialize_auxiliary_d(context);
    initialize_auxiliary_e(context);
    return player;
}

// NOTSA: named common tail step; runs after every mode in source order.
void common_tail(Context &context, const TaskHandle &object, Primary task,
                 const TaskWorkHandle &motion, const Player &player) {
    set_motion(context, player);
    update_tail_objects(context, player);
    join_vertices(context, task, motion, player);
    display(context, object);
    post_display(context, motion);
    if (has_held_task(context, player))
        update_held_task(context, motion);
    character_shadow(context, task, player);
    post_shadow(context);
    if (requested_animation(context, player) != kNoCollisionAnimation) {
        calculate_collision_range(context, task);
        enter_collision_list(context, task);
    }
    underwater(context, task, motion, player);
    finish_frame(context);
    extra(context, player).byte_82 = 0;
}
} // namespace

// The reached prefix of AmyUpdateActionState: the Chao branch, the damage gate
// and MD_AMY_STND's transition chain. The chain's first nonzero result stops
// the frame; every later mode body stays unrecovered and fails loudly.
void update_action_state(Context &context, Primary task, const TaskWorkHandle &motion,
                         const Player &player) {
    const auto current_stage = stage(context);
    if (current_stage >= 39 && current_stage <= 42)
        chao_setup(context, motion, player, task.player_index);
    const bool damage_item = player && (item_word(context, player) & kItemHighBit) != 0;
    if (!damage_item) {
        auto &wait = wait_timer(context);
        if (wait == 0) {
            if (!check_damage(context, task, motion, player)) {
                task.status = static_cast<std::uint16_t>(task.status & kStatusHighClear);
            } else {
                task.status = static_cast<std::uint16_t>(task.status | kStatusHighBit);
                if ((task.status & kHurtReactionStatus) != 0)
                    hurt_reaction(context, task, player);
                task.status = static_cast<std::uint16_t>(task.status & kDamageHighByteKeep);
                auto &item = item_word(context, player);
                if ((item & 3) == 0)
                    scatter_rings(context, task.player_index);
                else
                    item = static_cast<std::uint16_t>(item & kItemLowBitsClear);
            }
        } else {
            task.status = static_cast<std::uint16_t>(task.status & kHurtClear);
            wait = static_cast<std::uint16_t>(wait - 1);
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
        if (check_next_action(context, task, player, motion) ||
            check_fall(context, task, player, motion) ||
            check_drop_off(context, task, player) || check_drtk(context, task, player) ||
            check_jump(context, task, player) ||
            check_grab(context, task, player, motion) ||
            check_handle(context, task, player, motion) ||
            check_balance(context, task, player) || check_hat(context, task, player))
            return;
        if (stage_pickup(context, task, motion, player)) {
            task.mode = kCandyStartMode;
            requested_animation(context, player) = kCandyStartAnimation;
        } else if (stage_action(context, task, motion, player)) {
            return;
        }
        // GetAnalog is consumed before speed.x. Both gates share the walk store;
        // the pickup store above is overwritten when either gate is nonzero.
        if (analog_input(context, task) ||
            exact(context, horizontal_speed(context, motion)).compare(0.0f) !=
                X87Comparison::equal) {
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

void run(Context &context, const TaskHandle &object) {
    const auto motion = object->motion_work;
    auto player = player_for_motion(context, motion);
    const auto primary_work = object->primary_work;
    auto task = primary(context, primary_work);
    enter(context, task, primary_work, motion, player);
    if (check_edit_mode(context) != 0)
        task.mode = static_cast<std::uint8_t>(Mode::Edit);
    else if (static_cast<Mode>(task.mode) != Mode::Slash)
        update_action_state(context, task, motion, player);
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
    common_tail(context, object, task, motion, player);
}
} // namespace sadx::amy_selection

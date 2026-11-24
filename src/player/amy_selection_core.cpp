#include "player/amy_selection_core.hpp"
#include <cstring>

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
// AmyDisplay's fade, z-compare and body literals.
constexpr std::uint16_t kFadeOneAction = 0xd;
constexpr std::uint16_t kFadeKeepAction = 0x54;
constexpr float kFadeStep = 0.1f;
constexpr std::uint32_t kDisplayZCompare = 3;
constexpr std::uint32_t kDisplayFlagZero = 8;
constexpr std::uint32_t kDisplayFlagOne = 4;
constexpr std::uint32_t kOneFloatBits = 0x3f800000U;
constexpr std::uint16_t kDisplayGate = 2;
constexpr std::uint32_t kEnableConstantMaterial = 16;
constexpr std::uint16_t kScaleAnimation = 14;
constexpr std::uint16_t kScaleStatus = 3;
constexpr std::int32_t kHalfTurn = 0x8000;
constexpr std::int32_t kScaleRotation = 0x2000;
constexpr float kModelCenterHeight = 0.5f;
constexpr float kScaleX = 0.7f, kScaleY = 1.1f, kScaleZ = 0.8f;

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
// NOTSA: AmyDisplay's fade target gate. The reached branch is the source's
// unsigned request comparison; byte 0x81 keeps the target and the cancel leaf
// falls through to zero.
void update_fade(Context &context, const Player &player, const ExtraView &fields) {
    if (fields.byte_81 == 1)
        return;
    if (event_cancel(context) != 1) {
        const auto request = requested_animation(context, player);
        if (request == kFadeOneAction) {
            fields.display_target = 1.0f;
            return;
        }
        if (request > kFadeKeepAction)
            return;
    }
    fields.display_target = 0.0f;
}

// NOTSA: the source compares the extended sum against the target memory word,
// then either stores the sum or clamps the reversed difference.
void step_display(Context &context, const ExtraView &fields) {
    auto forward = exact(context, fields.display_current) + kFadeStep;
    const auto forward_comparison = forward.compare(fields.display_target);
    if (forward_comparison != X87Comparison::greater &&
        forward_comparison != X87Comparison::unordered) {
        fields.display_current = forward.rounded();
        return;
    }
    auto backward = exact(context, fields.display_current) - kFadeStep;
    const auto backward_comparison = backward.compare(fields.display_target);
    if (backward_comparison == X87Comparison::less ||
        backward_comparison == X87Comparison::equal ||
        backward_comparison == X87Comparison::unordered) {
        fields.display_current = fields.display_target;
    } else {
        fields.display_current = backward.rounded();
    }
}

// NOTSA: AMY_OBJECTS row 36 selects its sibling (row 37 or 38); the scale pair
// is that sibling and its child, not row 36 itself.
void write_display_scale(Context &context, const ExtraView &fields) {
    const auto value = fields.display_current;
    auto &root = tail_sibling(context);
    auto &child = tail_sibling_child(context);
    root.scale = {value, value, value};
    child.scale = {value, value, value};
    const auto zero = exact(context, value).compare(0.0f) == X87Comparison::equal;
    if (zero) {
        root.evaluation_flags |= kDisplayFlagZero;
        child.evaluation_flags |= kDisplayFlagZero;
        return;
    }
    root.evaluation_flags &= ~kDisplayFlagZero;
    child.evaluation_flags &= ~kDisplayFlagZero;
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    if (bits == kOneFloatBits) {
        root.evaluation_flags |= kDisplayFlagOne;
        child.evaluation_flags |= kDisplayFlagOne;
    } else {
        root.evaluation_flags &= ~kDisplayFlagOne;
        child.evaluation_flags &= ~kDisplayFlagOne;
    }
}

// NOTSA: the source guards each full angle word before narrowing the store.
void rotate_task(Context &context, Primary task, Axis axis) {
    const auto value = task.rotation.at(static_cast<unsigned>(axis));
    if (value != 0)
        rotate(context, axis, static_cast<std::uint16_t>(value));
}

// NOTSA: the body's unit-frame carry places the player center at half height.
void place_body(Context &context, Primary task, const TaskWorkHandle &primary_work,
                const Player &player) {
    push_unit_frame(context);
    const auto center = (exact(context, player_height(context, player)) * kModelCenterHeight).rounded();
    const NinjaVector local{0, center, 0};
    rotate_task(context, task, Axis::Z);
    rotate_task(context, task, Axis::X);
    const auto yaw = 0U - static_cast<std::uint32_t>(task.rotation[1]);
    if (yaw != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(yaw));
    auto offset = calculate_vector(context, local);
    pop_frame(context);
    offset.x = (exact(context, offset.x) + task.position.x).rounded();
    offset.y = (exact(context, offset.y) + task.position.y).rounded();
    offset.z = (exact(context, offset.z) + task.position.z).rounded();
    translate(context, offset);
    store_collision_center(context, primary_work, offset);
    rotate_task(context, task, Axis::Z);
    rotate_task(context, task, Axis::X);
    const auto facing =
        static_cast<std::uint32_t>(kHalfTurn) - static_cast<std::uint32_t>(task.rotation[1]);
    if (facing != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(facing));
}

// NOTSA: the ordinary body frame; the special body modes still fail there.
void draw_body(Context &context, Primary task, const TaskWorkHandle &primary_work,
               const Player &player, std::uint16_t animation, bool special) {
    select_palette(context, 2);
    save_control(context);
    set_control(context, kEnableConstantMaterial);
    set_white_material(context);
    push_current_frame(context);
    place_body(context, task, primary_work, player);
    if (!special) {
        if (animation == kScaleAnimation && (task.status & kScaleStatus) != 0) {
            translate(context, {0, -1.0f, 0});
            rotate(context, Axis::Z, kScaleRotation);
            scale(context, {kScaleX, kScaleY, kScaleZ});
        }
        if (has_event_action(context, primary_work))
            draw_event_model(context);
        else if (animation_mode(context, player) == 2)
            draw_generated_action(context, player, animation_frame(context, player));
        else
            draw_action(context, player, animation, animation_frame(context, player));
    }
    pop_frame(context);
    restore_material(context);
    restore_control(context);
    select_palette(context, 0);
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
// NOTSA: the recovered AmyDisplay body. The special-mode preamble and the
// event/generated action branches remain named seams; the reached selection
// path draws the Stand table action and runs the source shadow tail.
void display(Context &context, const TaskHandle &object) {
    const auto player = player_for_motion(context, object->motion_work);
    const auto primary_work = object->primary_work;
    auto task = primary(context, primary_work);
    set_z_compare(context, kDisplayZCompare);
    auto fields = extra(context, player);
    update_fade(context, player, fields);
    step_display(context, fields);
    write_display_scale(context, fields);
    const auto animation = animation_mode(context, player) == 2
                               ? current_animation(context, player)
                               : requested_animation(context, player);
    if ((wait_timer(context) & kDisplayGate) == 0) {
        select_body_textures(context);
        const bool special = special_body(context, task.mode);
        draw_body(context, task, primary_work, player, animation, special);
    }
    reset_z_compare(context);
    if (paused(context))
        draw_character_shadow(context, player, object);
}
} // namespace sadx::amy_selection

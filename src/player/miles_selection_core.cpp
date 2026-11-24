#include "player/miles_selection_core.hpp"

#include <array>
#include <cstddef>

namespace sadx::miles_selection {
namespace {
constexpr std::uint8_t kMiles = 2;
constexpr std::uint16_t kFollowFloorReference = 32;
constexpr std::uint16_t kOnCollision = 2;
constexpr std::uint16_t kEquipmentSkip = 256;
constexpr std::uint32_t kCollisionEnable = 16;
constexpr std::uint16_t kEquipmentSiblingBit = 8;
constexpr std::uint16_t kEquipmentModelBit = 16;
constexpr std::uint16_t kHurt = 4;
constexpr std::uint16_t kHurtReactionStatus = 0x800;
constexpr std::uint16_t kStatusHighBit = 0x8000;
constexpr std::uint16_t kStatusHighClear = 0x7fff;
constexpr std::uint16_t kDamageHighByteKeep = 0xc6ff;
constexpr std::uint16_t kItemHighBit = 0x8000;
constexpr std::uint16_t kItemLowBitsClear = 0xfffc;
constexpr std::int16_t kChaoGardenFirstStage = 39;
constexpr std::int16_t kChaoGardenLastStage = 42;
constexpr std::uint32_t kStageActionArgument = 0x00874201;
constexpr std::uint16_t kStagePickupAnimation = 0x84;
constexpr std::uint16_t kWalkAnimation = 9;
constexpr std::uint16_t kSlideAnimation = 0x17;
constexpr std::int32_t kCharacterSelectGameMode = 12;
constexpr float kViewRadius = 10.0f;
constexpr float kSlideContactDepth = 0.77f;
enum class Mode : std::uint8_t { // MD_MILES, SADXEnumsNew.h names
    Initialize = 0,  // MD_MILES_INIT
    Stand = 1,       // MD_MILES_STND
    Walk = 2,        // MD_MILES_WALK
    Slide = 17,      // MD_MILES_SLID
    CnadeStart = 63, // MD_MILES_CNADE_START
    Kill = 60,       // MD_MILES_KILL
    CnadeLoop = 64   // MD_MILES_CNADE_LOOP
};
struct WeldPatch {
    std::size_t row;
    unsigned source_row, destination_row;
};
// The tail rewrites four weld rows when the join mode word is set.
constexpr std::array<WeldPatch, 4> kWeldPatches = {{
    {8, 65, 61}, {9, 64, 60}, {20, 65, 61}, {21, 64, 60},
}};

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
    if (static_cast<Mode>(task.mode) != Mode::Kill)
        update_action_state(context, task, motion, player);
}

// NOTSA: the pre-dispatch collision change is shared by all source modes; only
// record 2's attributes word is written.
void prepare_collision(Context &context, Primary task, const TaskWorkHandle &primary_work,
                       const Player &player) {
    if (!has_collision(context, primary_work))
        return;
    (void)player;
    auto &attributes = collision_attributes(context, primary_work, 2);
    attributes = static_cast<Mode>(task.mode) == Mode::CnadeLoop ? attributes & ~kCollisionEnable
                                                                 : attributes | kCollisionEnable;
}

// NOTSA: mode 0's sibling-only swap, before the child tasks.
void swap_start_siblings(Context &context, const Player &player) {
    const auto equipped = equipment(context, player);
    auto &object31 = tail_object(context, 31);
    auto &object28 = tail_object(context, 28);
    auto &object51 = tail_object(context, 51);
    auto &object48 = tail_object(context, 48);
    character_pack::ResourceId side = 0;
    if ((equipped & kEquipmentSiblingBit) == 0) {
        object31.sibling = sibling_object(context, 34);
        object28.sibling = sibling_object(context, 32);
        object51.sibling = sibling_object(context, 54);
        side = sibling_object(context, 52);
    } else {
        object31.sibling = sibling_object(context, 64);
        object28.sibling = sibling_object(context, 65);
        object51.sibling = sibling_object(context, 64);
        side = sibling_object(context, 65);
    }
    object48.sibling = side;
    auto &globals = state(context);
    globals.object_31_sibling = object31.sibling;
    globals.object_28_sibling = object28.sibling;
    globals.object_51_sibling = object51.sibling;
    globals.object_48_sibling = object48.sibling;
}

// NOTSA: the common tail's equipment swap, model words, conditional weld
// rewrite and final vertex join in source order.
void update_equipment(Context &context, Primary task, const TaskWorkHandle &motion,
                      const Player &player) {
    const auto equipped = equipment(context, player);
    auto &object31 = tail_object(context, 31);
    auto &object28 = tail_object(context, 28);
    auto &object51 = tail_object(context, 51);
    auto &object48 = tail_object(context, 48);
    character_pack::ResourceId side = 0;
    if ((equipped & kEquipmentSiblingBit) == 0) {
        object31.sibling = sibling_object(context, 34);
        object28.sibling = sibling_object(context, 32);
        object51.sibling = sibling_object(context, 54);
        side = sibling_object(context, 52);
    } else {
        object31.sibling = sibling_object(context, 64);
        object28.sibling = sibling_object(context, 65);
        object51.sibling = sibling_object(context, 64);
        side = sibling_object(context, 65);
    }
    object48.sibling = side;
    auto &object62 = tail_object(context, 62);
    auto &object63 = tail_object(context, 63);
    if ((equipped & kEquipmentModelBit) == 0) {
        object62.model = tail_model(context, 0);
        object63.model = tail_model(context, 1);
    } else {
        object62.model = tail_model(context, 14);
        object63.model = tail_model(context, 14);
    }
    auto &join_mode = player_initialization::bind_join_vertex_mode(player);
    if (join_mode != 0 && object31.sibling != state(context).object_31_sibling) {
        auto &weld = weld_table(context);
        for (const auto &patch : kWeldPatches) {
            weld.rows[patch.row].source_object =
                &scene_object(context, sibling_object(context, patch.source_row));
            weld.rows[patch.row].destination_object =
                &scene_object(context, sibling_object(context, patch.destination_row));
        }
        join_mode = 2;
        join_vertices(context, task, motion, player);
        join_mode = 0;
        join_vertices(context, task, motion, player);
    }
    auto &globals = state(context);
    globals.object_31_sibling = object31.sibling;
    globals.object_51_sibling = object51.sibling;
    globals.object_28_sibling = object28.sibling;
    globals.object_48_sibling = object48.sibling;
    join_vertices(context, task, motion, player);
}

// NOTSA: mode 0 is one named source step, not a replacement actor factory.
Player initialize(Context &context, const TaskHandle &object, Primary task,
                  const TaskWorkHandle &primary_work, const TaskWorkHandle &motion) {
    const auto index = task.player_index; // captured before the character store
    task.character = kMiles;
    initialize_player(context, index, object);
    auto player = player_for_motion(context, motion);
    bind_animation_table(context, player);
    requested_animation(context, player) = 0;
    initialize_collision(context, object, 5, 0);
    collision_first_extent(context, primary_work) = player_radius(context, player);
    task.mode = static_cast<std::uint8_t>(Mode::Stand);
    initialize_auxiliary_a(context, index);
    initialize_auxiliary_b(context, index);
    load_character_textures(context, kMiles);
    idle_timer(context, player) = 0;
    bind_weld_table(context, player);
    if ((task.status & kEquipmentSkip) == 0)
        swap_start_siblings(context, player);
    (void)create_child_task(context, 0, 3, index);
    (void)create_child_task(context, 1, 3, index);
    if (index == 0)
        set_default_normal_camera_mode(4, 20);
    object->destructor = destructor_callback(context);
    if (game_mode(context) != kCharacterSelectGameMode)
        object->display = display_callback(context);
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

// NOTSA: named common tail step; runs after every mode in source order.
void common_tail(Context &context, const TaskHandle &object, Primary task,
                 const TaskWorkHandle &motion, const Player &player) {
    set_motion(context, player);
    if ((task.status & kEquipmentSkip) == 0)
        update_equipment(context, task, motion, player);
    update_collision_center(context);
    display(context, object);
    if (has_held_task(context, player))
        update_held_task(context, motion);
    character_shadow(context, task, player);
    if (!collision_range_skipped(context, task)) {
        update_collision_range(context, task);
        enter_collision_list(context, task);
        set_collision_range(context, motion, player);
    }
    underwater(context, task, motion, player);
}
} // namespace

// The reached prefix of Tails_Act1: the Chao branch, the damage gate and
// MD_MILES_STND's transition chain. The chain's first nonzero result stops the
// frame; every later mode body stays unrecovered and fails loudly.
void update_action_state(Context &context, Primary task, const TaskWorkHandle &motion,
                         const Player &player) {
    const auto current_stage = stage(context);
    if (current_stage >= kChaoGardenFirstStage && current_stage <= kChaoGardenLastStage)
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
        // Source order: the five transition checkers, then the Chao-stage
        // pickup/action pair, then GetAnalog and the contact/speed slide gate.
        if (check_next_action(context, task, motion, player) ||
            check_fall(context, task, motion, player) ||
            check_jump(context, task, motion, player) ||
            check_grab(context, task, motion, player) ||
            check_attack(context, task, motion, player))
            return;
        if (stage_pickup(context, task, motion, player)) {
            task.mode = static_cast<std::uint8_t>(Mode::CnadeStart);
            requested_animation(context, player) = kStagePickupAnimation;
        } else if (stage_action(context, task, motion, player, kStageActionArgument)) {
            return;
        }
        // GetAnalog is consumed before the contact/speed slide gate. The input
        // owner stays a seam; the slide gate itself is source arithmetic.
        if (analog_input(context, task)) {
            task.mode = static_cast<std::uint8_t>(Mode::Walk);
            requested_animation(context, player) = kWalkAnimation;
            return;
        }
        if (exact(context, contact_depth(context, player)).compare(kSlideContactDepth) ==
                X87Comparison::less &&
            speed_magnitude(context, motion).compare(slide_speed(context, player)) ==
                X87Comparison::greater) {
            task.mode = static_cast<std::uint8_t>(Mode::Slide);
            requested_animation(context, player) = kSlideAnimation;
        }
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
} // namespace sadx::miles_selection

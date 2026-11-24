#include "engine/animated_model.hpp"
#include "engine/collision_component.hpp"
#include "engine/render_state_snapshot.hpp"
#include "game/editor_state.hpp"
#include "game/game_mode.hpp"
#include "player/animation/animation_native_access.hpp"
#include "player/character_shadow.hpp"
#include "player/event_actions.hpp"
#include "player/player_vertex_weld.hpp"
#include "player/selection_player_queries.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_sonic_fields.hpp"
#include "runtime/native_underwater_binding.hpp"
#include "runtime/object_draw_context.hpp"
#include <cstring>
#include <stdexcept>
#include <string>

namespace sadx::amy_selection {
namespace {
[[noreturn]] void missing(const std::string &boundary) { throw flow::UnrecoveredGameFlowCall(boundary.c_str()); }
TaskWorkArena &arena(Context &context) {
    const auto owner = context.scheduler.work_arena();
    if (!owner)
        missing("native_amy_task_arena");
    return *owner;
}
native::AmyActorAssets &assets(Context &context) {
    if (!context.players.amy_assets)
        missing("native_amy_actor_assets");
    return *context.players.amy_assets;
}
PlayerPhysicsState &physics(const Player &player) {
    if (!player || !player->physics)
        missing("native_amy_player_physics");
    return *player->physics;
}
std::shared_ptr<collision_component::Work> collision(Context &context,
                                                     const TaskWorkHandle &primary_work) {
    auto component = collision_component::component(arena(context), primary_work);
    if (!component)
        missing("native_amy_null_collision_component");
    return component;
}
collision_component::Info &collision_info(Context &context, const TaskWorkHandle &primary_work,
                                          unsigned record) {
    auto component = collision(context, primary_work);
    if (record >= component->info_count || record >= component->info.size())
        missing("native_amy_collision_info_record");
    return component->info[record];
}
// NOTSA: the typed free-work fields share the actual player allocation identity.
native::PlayerSystem::AmyExtraWork &extra_work(Context &context, const Player &player) {
    if (!player)
        missing("native_amy_null_player_free_work");
    for (auto &work : context.players.amy_work)
        if (work.allocation == player->allocation)
            return work;
    context.players.amy_work.push_back({player->allocation, 0, 0, 0, 0, 0});
    return context.players.amy_work.back();
}
constexpr unsigned kMotionPlayerField = 0, kCollisionField = 56;
constexpr std::uint8_t kSpecialBodyModeA = 17, kSpecialBodyModeB = 18;
constexpr std::uint16_t kPlayerFacingMask = 0x80;
// DrawCharacterShadow's source literals and late queue facts.
constexpr float kShadowDepthCull = 20000.0f;
constexpr float kShadowFalloffPerUnit = 0.03f;
constexpr float kShadowGroundLift = 0.3f;
constexpr float kShadowVerticalSquash = 0.8f;
constexpr std::uint8_t kLateShadowFlags = 6;
constexpr float kShadowSortBias = -27952.0f;
// STUB: SONIC 0x4867c0 AmyChildTaskA
void unrecovered_amy_child_a(TaskScheduler &, const TaskHandle &) { missing("native_amy_child_a"); }
// STUB: SONIC 0x485c50 AmyChildTaskB
void unrecovered_amy_child_b(TaskScheduler &, const TaskHandle &) { missing("native_amy_child_b"); }
// STUB: SONIC 0x485f40 AmyChildTaskC
void unrecovered_amy_child_c(TaskScheduler &, const TaskHandle &) { missing("native_amy_child_c"); }
// NOTSA: store the source player-index byte into a created child's primary.
void store_child_index(Context &context, const TaskHandle &child, std::uint8_t index) {
    player_initialization::establish_zeroed_primary(context.scheduler, child);
    player_initialization::bind_primary_view(child).player_index = index;
}
} // namespace

// NOTSA: one display invocation borrows the captured renderer owners. The
// render-snapshot and animation state remain the Display's existing owners.
struct DisplayContext {
    Context &owner;
    explicit DisplayContext(Context &owner) : owner(owner) {}
    render_snapshot::View render_state() {
        auto &display = owner.rendering;
        return {display.render_snapshot_, display.control_3d_, display.constant_attribute_mask_,
                display.constant_attributes_};
    }
    auto &matrices() { return *owner.rendering.coordinates_; }
    auto &arithmetic() { return owner.rendering.arithmetic_; }
    void material() {
        auto &display = owner.rendering;
        render_snapshot::set_material(display.render_snapshot_, {1, 1, 1, 1}, display.queue_state_,
                                      display.queue_services(), display.arithmetic_);
    }
    object_draw::State draw_state() {
        auto &display = owner.rendering;
        return {display.fog_.special_flags,    display.fog_.active,
                display.queued_depth_,         display.frame_mode_.pending_logic_ticks,
                display.simple_model_enabled_, display.queue_state_.values.lighting_palette};
    }
    auto &animation_state() { return owner.rendering.animated_state_; }
};

ExtraView extra(Context &context, const Player &player) {
    auto &work = extra_work(context, player);
    return {work.display_target, work.display_current, work.byte_80, work.byte_81, work.byte_82};
}
Primary primary(Context &context, const TaskWorkHandle &work) {
    if (work != context.task->primary_work)
        missing("native_amy_primary_identity");
    return player_initialization::bind_primary_view(context.task);
}
Player player_for_motion(Context &context, const TaskWorkHandle &motion) {
    const auto reference = arena(context).reference(motion, kMotionPlayerField);
    if (!reference)
        missing("native_amy_motion_player_reference");
    if (!reference.work)
        return {};
    const auto found = context.players.registry.allocations.find(reference.work.get());
    if (found == context.players.registry.allocations.end())
        missing("native_amy_player_allocation_identity");
    return found->second;
}
TaskWorkHandle player_allocation(Context &, const Player &player) {
    return player ? player->allocation : TaskWorkHandle{};
}
std::uint16_t &requested_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).requested_animation;
}
std::uint16_t &current_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).current_animation;
}
std::uint16_t &equipment(Context &, const Player &player) {
    auto &value = physics(player).equipment;
    if (!value)
        missing("native_amy_equipment");
    return *value;
}
std::uint16_t &item_word(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.item)
        missing("native_amy_item_word");
    return *fields.item;
}
std::uint16_t &wait_timer(Context &context) {
    try {
        return player_initialization::bind_wait_timer(context.task);
    } catch (const player_initialization::Error &) {
        missing("native_amy_wait_timer");
    }
}
bool has_held_task(Context &, const Player &player) {
    return bool(player_initialization::bind_initialized_player(player).held_task);
}
bool has_collision(Context &context, const TaskWorkHandle &primary_work) {
    const auto reference = arena(context).reference(primary_work, kCollisionField);
    if (!reference)
        missing("native_amy_collision_reference");
    return bool(reference.work);
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
    if (!task)
        missing("native_amy_collision_task_backlink");
    return player_initialization::bind_primary_view(task).rotation.at(axis);
}
float vertical_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work)
        missing("native_amy_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.y;
}
float horizontal_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work)
        missing("native_amy_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.x;
}
float player_radius(Context &, const Player &player) {
    if (!player)
        missing("native_amy_player_parameters");
    return player->parameters.rad;
}
TaskCallback display_callback(Context &context) {
    if (!context.display)
        missing("native_amy_display_callback");
    return context.display;
}
TaskCallback destructor_callback(Context &context) {
    if (!context.destructor)
        missing("native_amy_destructor_callback");
    return context.destructor;
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.rendering.arithmetic(), value};
}
std::int32_t check_edit_mode(Context &context) {
    if (!context.editor_state)
        missing("native_amy_check_edit_mode");
    return editor::check_mode(*context.editor_state);
}
std::int16_t stage(Context &context) { return context.stage; }

void initialize_player(Context &context, std::int32_t index, const TaskHandle &task) {
    if (!context.players.gamma_assets)
        missing("native_amy_parameters");
    player_initialization::NativeAccess access(context.players.gamma_assets->parameters,
                                               context.bootstrap.event_work, context.auxiliary);
    Player record;
    player_initialization::initialize(
        index, task, player_initialization::bind_existing_motion(task), context.scheduler,
        context.players.registry, access, context.rendering.arithmetic(), record);
}
void bind_animation_table(Context &context, const Player &player) {
    const auto &table = assets(context).animation_table;
    if (!table || table->records.size() != 102)
        missing("native_amy_complete_animation_table");
    player_initialization::bind_initialized_player(player).animation_table = table;
}
void bind_weld_table(Context &context, const Player &player) {
    if (context.players.amy_weld.rows.size() != 18)
        missing("native_amy_imported_weld_table");
    player_initialization::bind_initialized_player(player).join_table = &context.players.amy_weld;
}
void initialize_collision(Context &context, const TaskHandle &task, unsigned records,
                          std::uint8_t id) {
    auto view = player_initialization::bind_primary_view(task);
    (void)collision_component::initialize(
        arena(context), task, view.status, assets(context).collision_info,
        static_cast<std::int32_t>(records), id, context.rendering.arithmetic());
}
void load_character_textures(Context &context, std::uint8_t character) {
    if (!context.texture_state || !context.texture_services)
        missing("native_amy_character_texture_owner");
    regular_textures::load_character(character, context.character_textures, *context.texture_state,
                                     *context.texture_services);
}
void calculate_collision_range(Context &context, Primary) {
    if (!context.task)
        missing("native_amy_calculate_collision_range");
    auto work = collision(context, context.task->primary_work);
    collision_component::calculate_range(work.get(), context.rendering.arithmetic());
}
void enter_collision_list(Context &context, Primary view) {
    if (!context.task)
        missing("native_amy_null_collision_task");
    if (!context.collision_entries)
        missing("native_amy_collision_entries");
    auto work = collision(context, context.task->primary_work);
    collision_component::enter_list(*context.collision_entries, work, view.status, context.stage);
}
// NOTSA: native boundary for RunUnderwaterStuff; the shared product owner is
// native::run_underwater.
void underwater(Context &context, Primary primary, const TaskWorkHandle &motion_work,
                const Player &player) {
    if (motion_work != context.task->motion_work)
        missing("native_amy_motion_identity");
    auto &body = physics(player);
    native::run_underwater(context.players, context.rendering, context.task, primary, body, player);
}
void set_motion(Context &context, const Player &player) {
    if (!context.players.amy_motion_assets)
        missing("native_amy_set_motion");
    animation::View view{physics(player), player_allocation(context, player)};
    animation::Runtime runtime{&arena(context), context.players.amy_motion_assets->resources(),
                               &context.scene.objects};
    const auto result = animation::update(view, runtime, context.rendering.arithmetic());
    if (!result)
        missing("native_amy_set_motion_failed");
}
void join_vertices(Context &context, Primary, const TaskWorkHandle &, const Player &player) {
    auto &join_mode = player_initialization::bind_join_vertex_mode(player);
    const auto mode = join_mode;
    if (mode == 1) {
        // Active processing reaches the real motion-matrix producer before it
        // consumes the join table, including when the table is empty.
        missing("native_amy_join_vertices_CalcMMMatrix");
    }
    auto &table = physics(player).animation_tail.join_table;
    if (!table)
        missing("native_amy_weld_table_field");
    if (!*table)
        missing("native_amy_weld_null_table");
    if (mode == 0) {
        vertex_weld::initialize(**table, arena(context), context.scene);
        join_mode = 1;
    } else {
        vertex_weld::restore(**table, arena(context), context.scene);
    }
}
// NOTSA: common-tail sibling selection. Row 36 receives row 37 unless the
// player facing byte's high bit selects row 38.
void update_tail_objects(Context &context, const Player &player) {
    const auto &objects = assets(context).tail_objects;
    auto &root = context.scene.objects.object(objects[0]);
    const auto side = (equipment(context, player) & kPlayerFacingMask) != 0 ? 2u : 1u;
    root.sibling = objects[side];
}
void chao_setup(Context &, const TaskWorkHandle &, const Player &, std::uint8_t) {
    missing("native_amy_chao_setup");
}
bool check_damage(Context &, Primary, const TaskWorkHandle &, const Player &player) {
    // AmyCheckDamage tests playerwk (edi) first and returns 0 when it is
    // absent, so the pre-initialization frame takes the source no-damage store.
    if (!player)
        return false;
    missing("native_amy_check_damage");
}
void hurt_reaction(Context &, Primary, const Player &) {
    missing("native_amy_hurt_reaction");
}
void scatter_rings(Context &, std::uint8_t) { missing("native_amy_scatter_rings"); }
bool check_next_action(Context &, Primary, const Player &, const TaskWorkHandle &) {
    // AmyCheckNextAction is the event/next-action state machine: its ewp
    // move/path prefix and the kNextAction/smode table can both store a new
    // mode, so it is not a preview-zero predicate.
    missing("native_amy_check_next_action");
}
bool check_fall(Context &, Primary, const Player &, const TaskWorkHandle &) {
    // AmyCheckFall stores MD_AMY_FALL and reqaction 0x12 whenever the status
    // low bits are clear, which is the preview's first stand condition.
    missing("native_amy_check_fall");
}
bool check_drop_off(Context &context, Primary, const Player &player) {
    // AmyCheckDropOff's first guard is playerwk+0x80 == 1. Mode zero clears
    // that byte; only the unrecovered DROF entry sets it.
    if (extra(context, player).byte_80 != 1)
        return false;
    missing("native_amy_check_drop_off");
}
bool check_drtk(Context &context, Primary, const Player &player) {
    // AmyCheckDrtk returns 0 immediately when playerwk+0x80 == 1. Below that,
    // the unrecovered event-global read and the attack-pad gate decide.
    if (extra(context, player).byte_80 == 1)
        return false;
    missing("native_amy_check_drtk");
}
bool check_jump(Context &, Primary, const Player &) {
    // AmyCheckJump gates on JumpAllowed, which reads the held jump mask and the
    // task disable-control bit. The pad owner is not part of this context.
    missing("native_amy_check_jump");
}
bool check_grab(Context &, Primary, const Player &, const TaskWorkHandle &) {
    // AmyCheckGrab starts with the held grab mask; without the pad owner the
    // preview-zero branch cannot be proven here.
    missing("native_amy_check_grab");
}
bool check_handle(Context &context, Primary, const Player &player, const TaskWorkHandle &) {
    // AmyCheckHandle stores MD_AMY_HNDL only when the held attack mask and
    // playerwk+0x82 bit 0 are both set. Mode zero and the common tail leave
    // that free byte zero, so the byte conjunct alone proves the zero branch.
    if ((extra(context, player).byte_82 & 1) == 0)
        return false;
    missing("native_amy_check_handle");
}
bool check_balance(Context &context, Primary, const Player &player) {
    // AmyCheckBalance's first guard is playerwk+0x82 bit 1; the common tail
    // clears that byte at the end of every frame.
    if ((extra(context, player).byte_82 & 2) == 0)
        return false;
    missing("native_amy_check_balance");
}
bool check_hat(Context &, Primary, const Player &) {
    // AmyCheckHat starts with the held attack mask.
    missing("native_amy_check_hat");
}
bool stage_pickup(Context &context, Primary, const TaskWorkHandle &, const Player &) {
    // The recovered owner returns zero outside the Chao stages.
    if (context.stage < 39 || context.stage > 42)
        return false;
    missing("native_amy_stage_pickup");
}
bool stage_action(Context &context, Primary, const TaskWorkHandle &, const Player &) {
    if (context.stage < 39 || context.stage > 42)
        return false;
    missing("native_amy_stage_action");
}
bool analog_input(Context &, Primary) { missing("native_amy_analog_input"); }
void unrecovered_amy_direct(TaskScheduler &, const TaskHandle &) {
    missing("native_amy_direct_ahead");
}
void setup_for_player(Context &context, const TaskWorkHandle &motion, std::uint8_t index) {
    const auto created = context.scheduler.create_element(2, 2, unrecovered_amy_direct);
    if (!created || !created.task)
        return;
    constexpr unsigned kDirectMotionBytes = 0x88;
    constexpr unsigned kCopiedWordsOffset = 0x58;
    auto allocation = arena(context).allocate(kDirectMotionBytes);
    if (!allocation) {
        (void)context.scheduler.destroy_task(created.task);
        return;
    }
    created.task->motion_work = allocation.work;
    store_child_index(context, created.task, index);
    // The source copies the parent's first three motion words, clearing bit 1
    // of the player work word (through the motion work's player pointer)
    // between the first and second copies.
    const auto player = player_for_motion(context, motion);
    if (!player)
        missing("native_amy_setup_player");
    std::uint32_t work_word = 0;
    if (!arena(context).read(player->allocation, 0, &work_word, sizeof(work_word)))
        missing("native_amy_setup_motion_word");
    const auto parent_motion = player_initialization::bind_existing_motion(context.task);
    const auto word_of = [](float value) {
        std::uint32_t word = 0;
        std::memcpy(&word, &value, sizeof(word));
        return word;
    };
    const std::array<std::uint32_t, 3> words{work_word, word_of(parent_motion->motion.speed.x),
                                             word_of(parent_motion->motion.speed.y)};
    for (unsigned word = 0; word < words.size(); ++word) {
        if (!arena(context).write(allocation.work, kCopiedWordsOffset + word * 4, &words[word],
                                  sizeof(words[word])))
            missing("native_amy_setup_motion_copy");
        if (word == 0) {
            const auto cleared = work_word & ~2u;
            if (!arena(context).write(player->allocation, 0, &cleared, sizeof(cleared)))
                missing("native_amy_setup_motion_word_store");
        }
    }
}
void initialize_auxiliary_a(Context &context) {
    // Amy-only event task creation: current character, global mode and the
    // stage word must all match before any task is created.
    if (context.bootstrap.player_number != 5)
        return;
    const auto mode = static_cast<std::uint32_t>(context.bootstrap.game_mode);
    if (mode != 4 && mode != 5 && mode != 9)
        return;
    missing("native_amy_event_task");
}
void initialize_auxiliary_b(Context &context) { context.players.amy_event_word = 0; }
void initialize_auxiliary_c(Context &context) {
    // Stage-event table scan: no entry matches the CharacterSelect stage word.
    constexpr std::array<std::uint16_t, 3> kStageWords{0x0a00, 0x0a00, 0x0301};
    const auto stage_word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(context.stage) << 8) |
        static_cast<std::uint16_t>(context.act));
    for (const auto entry : kStageWords)
        if (entry == stage_word)
            missing("native_amy_stage_event");
}
void initialize_auxiliary_d(Context &context) {
    // Amy-only stage list task creation; the character gate runs first.
    if (context.bootstrap.player_number != 5)
        return;
    missing("native_amy_stage_list_task");
}
void initialize_auxiliary_e(Context &context) {
    // One stage word creates its task; CharacterSelect matches none.
    constexpr std::uint16_t kEventStageWord = 0x0c00;
    const auto stage_word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(context.stage) << 8) |
        static_cast<std::uint16_t>(context.act));
    if (stage_word == kEventStageWord)
        missing("native_amy_stage_event_task");
}
void stand_tail(Context &, const TaskWorkHandle &) { missing("native_amy_stand_tail"); }
void post_display(Context &, const TaskWorkHandle &) { missing("native_amy_post_display"); }
void update_held_task(Context &, const TaskWorkHandle &) { missing("native_amy_held_task"); }
void post_shadow(Context &) { missing("native_amy_post_shadow"); }
void finish_frame(Context &) { missing("native_amy_finish_frame"); }
void set_z_compare(Context &context, std::uint32_t mode) {
    context.rendering.set_depth_comparison(mode);
}
void reset_z_compare(Context &context) { context.rendering.restore_depth_comparison(); }
std::int32_t event_cancel(Context &context) {
    return selection_player_queries::event_cancel(context.bootstrap.cutscene_task);
}
std::uint16_t animation_mode(Context &, const Player &player) {
    auto &value = physics(player).animation_mode;
    if (!value)
        missing("native_amy_display_animation_mode");
    return static_cast<std::uint16_t>(*value);
}
float animation_frame(Context &context, const Player &player) {
    animation::View view{physics(player), player_allocation(context, player)};
    auto fields = animation::detail::NativeAccess{}.state(view);
    if (!fields.frame)
        missing("native_amy_display_animation_frame");
    return *fields.frame;
}
float player_height(Context &, const Player &player) {
    if (!player)
        missing("native_amy_display_player_height");
    return player->parameters.height;
}
bool has_event_action(Context &context, const TaskWorkHandle &primary_work) {
    return event_actions::action_head(arena(context), primary_work) != nullptr;
}
character_pack::ObjectResource &tail_sibling(Context &context) {
    auto &row = context.scene.objects.object(assets(context).tail_objects[0]);
    if (!row.sibling)
        missing("native_amy_display_tail_sibling");
    return context.scene.objects.object(row.sibling);
}
character_pack::ObjectResource &tail_sibling_child(Context &context) {
    auto &sibling = tail_sibling(context);
    if (!sibling.child)
        missing("native_amy_display_tail_child");
    return context.scene.objects.object(sibling.child);
}
void select_palette(Context &context, std::int32_t value) {
    if (!context.display_palette)
        missing("native_amy_display_palette_binding");
    context.display_palette(value);
}
void select_body_textures(Context &context) {
    constexpr unsigned kAmy = 5;
    const auto &catalog = context.character_textures;
    if (!catalog.tables || catalog.count <= kAmy)
        missing("native_amy_display_texture_catalog");
    const auto &table = catalog.tables[kAmy];
    if (!table.entries || table.count == 0 || !table.entries[0].textures ||
        !table.entries[0].archive || *table.entries[0].archive != "AMY")
        missing("native_amy_display_amy_texture_list");
    context.rendering.output().select_texture_list(*table.entries[0].textures);
}
void save_control(Context &context) {
    render_snapshot::save_control(DisplayContext{context}.render_state());
}
void set_control(Context &context, std::uint32_t value) {
    render_snapshot::set_control(DisplayContext{context}.render_state(), value);
}
void restore_control(Context &context) {
    render_snapshot::load_control(DisplayContext{context}.render_state());
}
void set_white_material(Context &context) { DisplayContext{context}.material(); }
void restore_material(Context &context) { context.rendering.reset_material(); }
void push_current_frame(Context &context) {
    (void)DisplayContext{context}.matrices().push_current_frame();
}
void push_unit_frame(Context &context) {
    (void)DisplayContext{context}.matrices().push_unit_frame();
}
void pop_frame(Context &context) { DisplayContext{context}.matrices().end_frame(); }
void rotate(Context &context, Axis axis, std::int32_t angle) {
    DisplayContext display{context};
    auto &matrix = display.matrices().current_matrix();
    switch (axis) {
    case Axis::X:
        display.matrices().rotate_x(matrix, angle, display.arithmetic());
        break;
    case Axis::Y:
        display.matrices().rotate_y(matrix, angle, display.arithmetic());
        break;
    case Axis::Z:
        display.matrices().rotate_z(matrix, angle, display.arithmetic());
        break;
    default:
        missing("native_amy_display_rotation_axis");
    }
}
NinjaVector calculate_vector(Context &context, const NinjaVector &input) {
    DisplayContext display{context};
    NinjaVector result{};
    if (!display.matrices().calculate_vector(display.matrices().current_matrix(), input, result,
                                             display.arithmetic()))
        missing("native_amy_display_vector_transform");
    return result;
}
void translate(Context &context, const NinjaVector &value) {
    DisplayContext display{context};
    display.matrices().translate_matrix(display.matrices().current_matrix(), value,
                                        display.arithmetic());
}
void scale(Context &context, const NinjaVector &value) {
    DisplayContext display{context};
    display.matrices().scale_matrix(display.matrices().current_matrix(), value.x, value.y, value.z,
                                    display.arithmetic());
}
void store_collision_center(Context &context, const TaskWorkHandle &primary_work,
                            const NinjaVector &value) {
    auto work = collision(context, primary_work);
    if (work->info.empty())
        missing("native_amy_display_collision_record");
    work->info[0].center = value;
}
namespace {
// NOTSA: imported-action adapter for the animated model owner; the generated
// branch keeps its own seam.
class ImportedMotionView final : public animated_model::Motion {
  public:
    ImportedMotionView(const animation::Motion &motion, const animation::Resources &resources)
        : motion_(&motion), resources_(&resources) {}
    std::uint32_t frame_count() override { return motion_->frame_count; }
    std::uint16_t type() override { return motion_->type; }
    std::uint16_t interpolation() override { return motion_->input_function; }
    bool has_keys(unsigned node, unsigned channel) override {
        return nodes().at(node).keys.at(channel).id.has_value();
    }
    std::uint32_t key_count(unsigned node, unsigned channel) override {
        return nodes().at(node).counts.at(channel);
    }
    std::uint32_t key_word(unsigned node, unsigned channel, unsigned key, unsigned word) override {
        std::uint32_t result;
        const auto offset = std::size_t(key) * 16 + word * 4;
        const auto identity = nodes().at(node).keys.at(channel).id;
        if (!identity)
            missing("native_amy_display_motion_keys");
        const auto &keys = resources_->key_arrays.at(*identity);
        if (!keys.bytes || offset + 4 > keys.byte_extent ||
            keys.offset + offset + 4 > keys.bytes->size())
            missing("native_amy_display_motion_key_extent");
        std::memcpy(&result, keys.bytes->data() + keys.offset + offset, 4);
        return result;
    }

  private:
    const animation::Motion *motion_;
    const animation::Resources *resources_;
    const std::vector<animation::Node> &nodes() {
        if (!motion_->data.id)
            missing("native_amy_display_motion_data");
        return resources_->node_arrays.at(*motion_->data.id);
    }
};
void submit(Context &context, character_pack::ResourceId root, animated_model::Motion *motion,
            float frame) {
    if (!context.model_environment)
        missing("native_amy_display_model_environment");
    object_draw::Context draw{context.rendering, *context.model_environment};
    animated_model::action_clip_no_scale(root, motion, frame, draw.animated(),
                                         DisplayContext{context}.draw_state(), draw);
}
} // namespace
void draw_action(Context &context, const Player &player, std::uint16_t animation, float frame) {
    animation::View view{physics(player), player_allocation(context, player)};
    const animation::Table *content;
    try {
        content = &animation::detail::NativeAccess{}.table(view, animation::Phase::None);
    } catch (const animation::Stop &) {
        missing("native_amy_display_animation_table");
    }
    const auto &resource = content->records.at(animation).action;
    if (!animated_model::drawing_enabled(DisplayContext{context}.draw_state()))
        return;
    if (resource.status == animation::ResourceStatus::Unavailable)
        missing("native_amy_display_action_" + resource.unavailable_reason);
    if (resource.status == animation::ResourceStatus::KnownNull) {
        submit(context, 0, nullptr, frame);
        return;
    }
    if (!context.players.amy_motion_assets)
        missing("native_amy_display_animation_runtime");
    const auto &resources = context.players.amy_motion_assets->resources();
    const auto &action = resources.actions.at(resource.identity);
    if (!action.object.id || !action.motion.id) {
        submit(context,
               action.object.id ? static_cast<character_pack::ResourceId>(*action.object.id) : 0,
               nullptr, frame);
        return;
    }
    ImportedMotionView motion(resources.motions.at(*action.motion.id), resources);
    submit(context, static_cast<character_pack::ResourceId>(*action.object.id), &motion, frame);
}
[[noreturn]] void draw_generated_action(Context &, const Player &, float) {
    missing("native_amy_display_generated_action");
}
[[noreturn]] void draw_event_model(Context &) { missing("native_amy_display_event_model"); }
bool special_body(Context &context, std::uint8_t mode) {
    // The source draws AMY_ACTIONS[75] for modes 17/18 and AMY_ACTIONS[74] for
    // the free-byte-1 pose. Both declaration rows are unavailable in the
    // imported pack; mode 1 (the reached preview) returns without a draw.
    auto fields = extra(context, player_for_motion(context, context.task->motion_work));
    if (mode == kSpecialBodyModeA || mode == kSpecialBodyModeB || fields.byte_80 == 1)
        missing("native_amy_display_special_body");
    return false;
}
bool paused(Context &context) {
    const auto mode = context.bootstrap.game_mode;
    return mode == 16 || mode == 25;
}
// NOTSA: DrawCharacterShadow mirrors native_big_binding's Product::draw_shadow
// and draw_late, including the late-model queue; the unrecovered land walk is
// not part of this source call.
void draw_character_shadow(Context &context, const Player &player, const TaskHandle &object) {
    if (context.rendering.pending_logic_ticks() != 0)
        return;
    auto &state = physics(player);
    const auto field = [](auto &value, const char *boundary) -> auto & {
        if (!value)
            missing(boundary);
        return *value;
    };
    namespace shadow = sadx::character_shadow;
    shadow::ShadowWork work{
        field(state.shadow_angle_x, "native_amy_display_shadow_angle_x"),
        field(state.shadow_angle_z, "native_amy_display_shadow_angle_z"),
        state.shadow_attributes_top,
        field(state.shadow_attributes_bottom, "native_amy_display_shadow_attributes_bottom"),
        field(state.shadow_top_y, "native_amy_display_shadow_top_y"),
        field(state.shadow_bottom_y, "native_amy_display_shadow_bottom_y"),
        field(state.shadow_prior_attributes_top, "native_amy_display_shadow_prior_top"),
        field(state.shadow_prior_attributes_bottom, "native_amy_display_shadow_prior_bottom"),
        field(state.shadow_scale, "native_amy_display_shadow_scale")};
    const auto primary = player_initialization::bind_primary_view(object);
    const shadow::Body body{primary.position.x, primary.position.y, primary.position.z,
                            primary.rotation[1], primary.player_index};
    auto &arithmetic = context.rendering.arithmetic();
    const auto vertical = fidelity::ExactFloat(arithmetic, body.y) - kShadowDepthCull;
    if (vertical.compare(work.bottom_y) != X87Comparison::less)
        return;
    const auto difference = fidelity::ExactFloat(arithmetic, body.y) - work.bottom_y;
    auto gap = difference + 1.0f;
    // FABS clears the sign without quieting a signaling NaN.
    if (gap.is_negative())
        gap = -gap;
    const auto denominator = gap * kShadowFalloffPerUnit + 1.0f;
    const auto scale =
        (fidelity::ExactFloat(arithmetic, work.scale) / denominator).rounded();
    if (context.texture_state && context.texture_state->common)
        context.rendering.output().select_texture_list(*context.texture_state->common);
    if (!context.rendering.push_current_matrix())
        missing("native_amy_display_shadow_matrix_capacity");
    const auto lifted =
        (fidelity::ExactFloat(arithmetic, work.bottom_y) + kShadowGroundLift).rounded();
    context.rendering.translate_current_matrix(body.x, lifted, body.z);
    if (work.angle_z != 0)
        context.rendering.rotate_current_z(static_cast<std::uint16_t>(work.angle_z));
    if (work.angle_x != 0)
        context.rendering.rotate_current_x(static_cast<std::uint16_t>(work.angle_x));
    if (body.yaw != 0) {
        const auto angle =
            static_cast<std::uint16_t>(-static_cast<std::uint16_t>(body.yaw));
        context.rendering.rotate_current_y(angle);
    }
    const auto flattened =
        (fidelity::ExactFloat(arithmetic, scale) * kShadowVerticalSquash).rounded();
    context.rendering.scale_current_matrix(scale, scale, flattened);
    if (context.rendering.late_texture_list_not_ready()) {
        context.rendering.pop_current_matrix();
        return;
    }
    if (!context.players.gamma_assets)
        missing("native_amy_display_shadow_asset_owner");
    const auto shadow_object = context.players.gamma_assets->shadow_object;
    if (!shadow_object)
        missing("native_amy_display_shadow_object");
    if (!context.model_environment)
        missing("native_amy_display_shadow_environment");
    const auto model = context.scene.objects.object(*shadow_object).model;
    const auto environment = *context.model_environment;
    auto &display = context.rendering;
    const auto *list = context.texture_state ? context.texture_state->common : nullptr;
    auto draw_mesh = [&display, environment, model, list] {
        if (list)
            display.output().select_texture_list(*list);
        object_draw::Context draw{display, environment};
        object_draw::draw_model(draw, model);
    };
    const auto queued = context.rendering.queue_late_model(
        model, kLateShadowFlags, kShadowSortBias, std::move(draw_mesh));
    if (!queued)
        missing("native_amy_display_shadow_model_queue");
    if (queued.route != LateModelRoute::queued)
        missing("native_late_model_immediate");
    context.rendering.pop_current_matrix();
}

void destroy(Context &, const TaskHandle &) { missing("native_amy_destruct"); }
void get_rotation(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_amy_get_rotation");
}
void get_acceleration(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_amy_get_acceleration");
}
void get_speed(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_amy_get_speed");
}
std::int32_t set_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_amy_set_position");
}
void reset_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_amy_reset_position");
}
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) {
    missing("native_amy_unrecovered_mode");
}
void character_shadow(Context &context, Primary task, const Player &player) {
    auto &state = physics(player);
    const auto field = [](auto &value, const char *boundary) -> auto & {
        if (!value)
            missing(boundary);
        return *value;
    };
    namespace shadow = sadx::character_shadow;
    shadow::ShadowWork work{
        field(state.shadow_angle_x, "native_amy_shadow_angle_x"),
        field(state.shadow_angle_z, "native_amy_shadow_angle_z"),
        state.shadow_attributes_top,
        field(state.shadow_attributes_bottom, "native_amy_shadow_attributes_bottom"),
        field(state.shadow_top_y, "native_amy_shadow_top_y"),
        field(state.shadow_bottom_y, "native_amy_shadow_bottom_y"),
        field(state.shadow_prior_attributes_top, "native_amy_shadow_prior_attributes_top"),
        field(state.shadow_prior_attributes_bottom, "native_amy_shadow_prior_attributes_bottom"),
        field(state.shadow_scale, "native_amy_shadow_scale")};
    const shadow::Body body{task.position.x, task.position.y, task.position.z, task.rotation[1],
                            task.player_index};
    struct Product final : shadow::Services {
        Context &context;
        const shadow::Body &body;
        shadow::ShadowWork &work;
        Product(Context &context, const shadow::Body &body, shadow::ShadowWork &work)
            : context(context), body(body), work(work) {}
        // The source land table has no entries on the reached selection path.
        // A loaded table takes the unrecovered walk instead of inventing land.
        shadow::LandCandidates list_land(const NinjaVector &, float) override {
            if (context.land_candidate_count > 0)
                missing("native_amy_shadow_land_list");
            return {};
        }
        fidelity::ExactFloat horizontal_distance(const NinjaVector &) override {
            missing("native_amy_shadow_distance");
        }
        shadow::ShadowHit get_zx_shadow(const NinjaVector &,
                                        const shadow::LandCandidate &) override {
            missing("native_amy_shadow_water_polygon");
        }
        shadow::ShadowHit polygon_test(const NinjaVector &,
                                       const shadow::LandCandidate &) override {
            missing("native_amy_shadow_polygon");
        }
        void splash() override { missing("native_amy_shadow_splash"); }
        void ripple(std::uint8_t, const NinjaVector &,
                    const std::array<std::int32_t, 3> &) override {
            missing("native_amy_shadow_ripple");
        }
        // NOTSA: native boundary for DrawCharacterShadow; the source draws the
        // common shadow blob through the late queue.
        void draw_shadow() override {
            if (context.rendering.pending_logic_ticks() != 0)
                return;
            auto &arithmetic = context.rendering.arithmetic();
            const auto vertical = fidelity::ExactFloat(arithmetic, body.y) - kShadowDepthCull;
            if (vertical.compare(work.bottom_y) != X87Comparison::less)
                return;
            const auto difference = fidelity::ExactFloat(arithmetic, body.y) - work.bottom_y;
            auto gap = difference + 1.0f;
            // FABS clears the sign without quieting a signaling NaN.
            if (gap.is_negative())
                gap = -gap;
            const auto denominator = gap * kShadowFalloffPerUnit + 1.0f;
            const auto scale =
                (fidelity::ExactFloat(arithmetic, work.scale) / denominator).rounded();
            if (context.texture_state && context.texture_state->common)
                context.rendering.output().select_texture_list(*context.texture_state->common);
            if (!context.rendering.push_current_matrix())
                missing("native_amy_shadow_matrix_capacity");
            const auto lifted =
                (fidelity::ExactFloat(arithmetic, work.bottom_y) + kShadowGroundLift).rounded();
            context.rendering.translate_current_matrix(body.x, lifted, body.z);
            if (work.angle_z != 0)
                context.rendering.rotate_current_z(static_cast<std::uint16_t>(work.angle_z));
            if (work.angle_x != 0)
                context.rendering.rotate_current_x(static_cast<std::uint16_t>(work.angle_x));
            if (body.yaw != 0) {
                const auto angle =
                    static_cast<std::uint16_t>(-static_cast<std::uint16_t>(body.yaw));
                context.rendering.rotate_current_y(angle);
            }
            const auto flattened =
                (fidelity::ExactFloat(arithmetic, scale) * kShadowVerticalSquash).rounded();
            context.rendering.scale_current_matrix(scale, scale, flattened);
            draw_late();
            context.rendering.pop_current_matrix();
        }
        void draw_late() {
            if (context.rendering.late_texture_list_not_ready())
                return;
            if (!context.players.gamma_assets)
                missing("native_amy_shadow_asset_owner");
            const auto shadow_object = context.players.gamma_assets->shadow_object;
            if (!shadow_object)
                missing("native_amy_shadow_object");
            if (!context.model_environment)
                missing("native_amy_shadow_environment");
            const auto model = context.scene.objects.object(*shadow_object).model;
            const auto environment = *context.model_environment;
            auto &display = context.rendering;
            const auto *list = context.texture_state ? context.texture_state->common : nullptr;
            auto draw_mesh = [&display, environment, model, list] {
                if (list)
                    display.output().select_texture_list(*list);
                object_draw::Context draw{display, environment};
                object_draw::draw_model(draw, model);
            };
            const auto queued = context.rendering.queue_late_model(
                model, kLateShadowFlags, kShadowSortBias, std::move(draw_mesh));
            if (!queued)
                missing("native_amy_shadow_model_queue");
            if (queued.route != LateModelRoute::queued)
                missing("native_late_model_immediate");
        }
    } services(context, body, work);
    shadow::update(body, work, services, context.rendering.arithmetic());
}
TaskHandle create_child_task(Context &context, unsigned kind, std::int32_t level,
                             std::uint8_t index) {
    TaskCallback callback = nullptr;
    std::uint32_t flags = 0;
    switch (kind) {
    case 0:
        flags = 2;
        callback = unrecovered_amy_child_a;
        break;
    case 1:
        flags = 3;
        callback = unrecovered_amy_child_b;
        break;
    case 2:
        flags = 3;
        callback = unrecovered_amy_child_c;
        break;
    default:
        missing("native_amy_child_task_kind");
    }
    const auto created = context.scheduler.create_element(flags, level, callback);
    if (!created || !created.task)
        return {};
    store_child_index(context, created.task, index);
    return created.task;
}
} // namespace sadx::amy_selection

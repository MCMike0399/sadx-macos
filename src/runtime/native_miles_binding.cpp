#include "engine/collision_component.hpp"
#include "engine/view_check.hpp"
#include "game/game_mode.hpp"
#include "player/character_shadow.hpp"
#include "player/player_vertex_weld.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_sonic_fields.hpp"
#include "runtime/native_underwater_binding.hpp"
#include "runtime/object_draw_context.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace sadx::miles_selection {
namespace {
[[noreturn]] void missing(const char *boundary) { throw flow::UnrecoveredGameFlowCall(boundary); }
TaskWorkArena &arena(Context &context) {
    const auto owner = context.scheduler.work_arena();
    if (!owner)
        missing("native_miles_task_arena");
    return *owner;
}
native::MilesActorAssets &assets(Context &context) {
    if (!context.players.miles_assets)
        missing("native_miles_actor_assets");
    return *context.players.miles_assets;
}
PlayerPhysicsState &physics(const Player &player) {
    if (!player || !player->physics)
        missing("native_miles_player_physics");
    return *player->physics;
}
std::shared_ptr<collision_component::Work> collision(Context &context,
                                                     const TaskWorkHandle &primary_work) {
    auto component = collision_component::component(arena(context), primary_work);
    if (!component)
        missing("native_miles_null_collision_component");
    return component;
}
collision_component::Info &collision_info(Context &context, const TaskWorkHandle &primary_work,
                                          unsigned record) {
    auto component = collision(context, primary_work);
    if (record >= component->info_count || record >= component->info.size())
        missing("native_miles_collision_info_record");
    return component->info[record];
}
constexpr unsigned kMotionPlayerField = 0, kCollisionField = 56;
constexpr std::uint16_t kHurt = 4;
constexpr std::uint32_t kDamageAttribute = 0x10000;
constexpr std::uint16_t kAttackLocked = 0x2000;
constexpr std::int16_t kChaoGardenFirstStage = 39;
constexpr std::int16_t kChaoGardenLastStage = 42;
std::uint32_t held_buttons(Context &context, std::uint8_t index) {
    if (!context.held_buttons)
        missing("native_miles_held_buttons");
    return context.held_buttons(index);
}
std::uint32_t attack_buttons(Context &context) {
    if (!context.attack_buttons)
        missing("native_miles_attack_buttons");
    return context.attack_buttons();
}
std::uint32_t grab_buttons(Context &context) {
    if (!context.grab_buttons)
        missing("native_miles_grab_buttons");
    return context.grab_buttons();
}
// DrawCharacterShadow's source literals and late queue facts.
constexpr float kShadowDepthCull = 20000.0f;
constexpr float kShadowFalloffPerUnit = 0.03f;
constexpr float kShadowGroundLift = 0.3f;
constexpr float kShadowVerticalSquash = 0.8f;
constexpr std::uint8_t kLateShadowFlags = 6;
constexpr float kShadowSortBias = -27952.0f;
// STUB: SONIC 0x45e470 MilesChildTaskA
void unrecovered_child_a(TaskScheduler &, const TaskHandle &) { missing("native_miles_child_a"); }
// STUB: SONIC 0x45b840 Tails_Jiggle_Main
void unrecovered_child_b(TaskScheduler &, const TaskHandle &) { missing("native_miles_child_b"); }
// NOTSA: store the source player-index byte into a created child's primary.
void store_child_index(Context &context, const TaskHandle &child, std::uint8_t index) {
    player_initialization::establish_zeroed_primary(context.scheduler, child);
    player_initialization::bind_primary_view(child).player_index = index;
}
template <std::size_t N>
std::size_t row_index(const std::array<unsigned, N> &rows, unsigned table_index,
                      const char *boundary) {
    const auto found = std::find(rows.begin(), rows.end(), table_index);
    if (found == rows.end())
        missing(boundary);
    return static_cast<std::size_t>(found - rows.begin());
}
} // namespace

State &state(Context &context) { return context.players.miles; }
Primary primary(Context &context, const TaskWorkHandle &work) {
    if (work != context.task->primary_work)
        missing("native_miles_primary_identity");
    return player_initialization::bind_primary_view(context.task);
}
Player player_for_motion(Context &context, const TaskWorkHandle &motion) {
    const auto reference = arena(context).reference(motion, kMotionPlayerField);
    if (!reference)
        missing("native_miles_motion_player_reference");
    if (!reference.work)
        return {};
    const auto found = context.players.registry.allocations.find(reference.work.get());
    if (found == context.players.registry.allocations.end())
        missing("native_miles_player_allocation_identity");
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
        missing("native_miles_equipment");
    return *value;
}
std::uint16_t &idle_timer(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.idle_timer)
        missing("native_miles_idle_timer");
    return *fields.idle_timer;
}
bool has_held_task(Context &, const Player &player) {
    return bool(player_initialization::bind_initialized_player(player).held_task);
}
bool has_collision(Context &context, const TaskWorkHandle &primary_work) {
    const auto reference = arena(context).reference(primary_work, kCollisionField);
    if (!reference)
        missing("native_miles_collision_reference");
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
        missing("native_miles_collision_task_backlink");
    return player_initialization::bind_primary_view(task).rotation.at(axis);
}
float vertical_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work)
        missing("native_miles_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.y;
}
float player_radius(Context &, const Player &player) {
    if (!player)
        missing("native_miles_player_parameters");
    return player->parameters.rad;
}
TaskCallback display_callback(Context &context) {
    if (!context.display)
        missing("native_miles_display_callback");
    return context.display;
}
TaskCallback destructor_callback(Context &context) {
    if (!context.destructor)
        missing("native_miles_destructor_callback");
    return context.destructor;
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.rendering.arithmetic(), value};
}
std::int32_t game_mode(Context &context) { return context.bootstrap.game_mode; }
bool collision_range_skipped(Context &context, Primary) {
    try {
        const auto counter =
            static_cast<std::uint32_t>(player_initialization::bind_task_counter(context.task));
        return (counter >> 24 & 8) != 0;
    } catch (const player_initialization::Error &) {
        missing("native_miles_source_counter");
    }
}

void initialize_player(Context &context, std::int32_t index, const TaskHandle &task) {
    if (!context.players.gamma_assets)
        missing("native_miles_parameters");
    player_initialization::NativeAccess access(context.players.gamma_assets->parameters,
                                               context.bootstrap.event_work, context.auxiliary);
    Player record;
    player_initialization::initialize(
        index, task, player_initialization::bind_existing_motion(task), context.scheduler,
        context.players.registry, access, context.rendering.arithmetic(), record);
}
void bind_animation_table(Context &context, const Player &player) {
    const auto &table = assets(context).animation_table;
    if (!table || table->records.size() != 136)
        missing("native_miles_complete_animation_table");
    player_initialization::bind_initialized_player(player).animation_table = table;
}
void bind_weld_table(Context &context, const Player &player) {
    if (context.players.miles_weld.rows.size() != 30)
        missing("native_miles_imported_weld_table");
    player_initialization::bind_initialized_player(player).join_table = &context.players.miles_weld;
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
        missing("native_miles_character_texture_owner");
    regular_textures::load_character(character, context.character_textures, *context.texture_state,
                                     *context.texture_services);
}
void enter_collision_list(Context &context, Primary view) {
    if (!context.task)
        missing("native_miles_null_collision_task");
    if (!context.collision_entries)
        missing("native_miles_collision_entries");
    auto work = collision(context, context.task->primary_work);
    collision_component::enter_list(*context.collision_entries, work, view.status, context.stage);
}
// NOTSA: native boundary for RunUnderwaterStuff; the shared product owner is
// native::run_underwater.
void underwater(Context &context, Primary primary, const TaskWorkHandle &motion_work,
                const Player &player) {
    if (motion_work != context.task->motion_work)
        missing("native_miles_motion_identity");
    auto &body = physics(player);
    native::run_underwater(context.players, context.rendering, context.task, primary, body, player);
}
void set_motion(Context &context, const Player &player) {
    if (!context.players.miles_motion_assets)
        missing("native_miles_set_motion");
    animation::View view{physics(player), player_allocation(context, player)};
    animation::Runtime runtime{&arena(context), context.players.miles_motion_assets->resources(),
                               &context.scene.objects};
    const auto result = animation::update(view, runtime, context.rendering.arithmetic());
    if (!result)
        missing("native_miles_set_motion_failed");
}
void join_vertices(Context &context, Primary, const TaskWorkHandle &, const Player &player) {
    auto &join_mode = player_initialization::bind_join_vertex_mode(player);
    const auto mode = join_mode;
    if (mode == 1) {
        // Active processing reaches the real motion-matrix producer before it
        // consumes the join table, including when the table is empty.
        missing("native_miles_join_vertices_CalcMMMatrix");
    }
    auto &table = physics(player).animation_tail.join_table;
    if (!table)
        missing("native_miles_weld_table_field");
    if (!*table)
        missing("native_miles_weld_null_table");
    if (mode == 0) {
        vertex_weld::initialize(**table, arena(context), context.scene);
        join_mode = 1;
    } else {
        vertex_weld::restore(**table, arena(context), context.scene);
    }
}
character_pack::ObjectResource &tail_object(Context &context, unsigned table_index) {
    constexpr std::array<unsigned, 6> kTailObjectRows = {28, 31, 48, 51, 62, 63};
    const auto index = row_index(kTailObjectRows, table_index, "native_miles_tail_object_row");
    return context.scene.objects.object(assets(context).tail_objects[index]);
}
character_pack::ResourceId sibling_object(Context &context, unsigned table_index) {
    constexpr std::array<unsigned, 8> kSiblingObjectRows = {32, 34, 52, 54, 60, 61, 64, 65};
    const auto index = row_index(kSiblingObjectRows, table_index, "native_miles_sibling_object_row");
    return assets(context).sibling_objects[index];
}
character_pack::ResourceId tail_model(Context &context, unsigned table_index) {
    constexpr std::array<unsigned, 3> kTailModelRows = {0, 1, 14};
    const auto index = row_index(kTailModelRows, table_index, "native_miles_tail_model_row");
    return assets(context).tail_models[index];
}
character_pack::ObjectResource &scene_object(Context &context, character_pack::ResourceId id) {
    return context.scene.objects.object(id);
}
PlayerJoinTable &weld_table(Context &context) { return context.players.miles_weld; }
// NOTSA: native boundary for CharacterShadow; source seam declared in the Miles core.
void character_shadow(Context &context, Primary task, const Player &player) {
    auto &state = physics(player);
    const auto field = [](auto &value, const char *boundary) -> auto & {
        if (!value)
            missing(boundary);
        return *value;
    };
    namespace shadow = sadx::character_shadow;
    shadow::ShadowWork work{
        field(state.shadow_angle_x, "native_miles_shadow_angle_x"),
        field(state.shadow_angle_z, "native_miles_shadow_angle_z"),
        state.shadow_attributes_top,
        field(state.shadow_attributes_bottom, "native_miles_shadow_attributes_bottom"),
        field(state.shadow_top_y, "native_miles_shadow_top_y"),
        field(state.shadow_bottom_y, "native_miles_shadow_bottom_y"),
        field(state.shadow_prior_attributes_top, "native_miles_shadow_prior_attributes_top"),
        field(state.shadow_prior_attributes_bottom, "native_miles_shadow_prior_attributes_bottom"),
        field(state.shadow_scale, "native_miles_shadow_scale")};
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
                missing("native_miles_shadow_land_list");
            return {};
        }
        fidelity::ExactFloat horizontal_distance(const NinjaVector &) override {
            missing("native_miles_shadow_distance");
        }
        shadow::ShadowHit get_zx_shadow(const NinjaVector &,
                                        const shadow::LandCandidate &) override {
            missing("native_miles_shadow_water_polygon");
        }
        shadow::ShadowHit polygon_test(const NinjaVector &,
                                       const shadow::LandCandidate &) override {
            missing("native_miles_shadow_polygon");
        }
        void splash() override { missing("native_miles_shadow_splash"); }
        void ripple(std::uint8_t, const NinjaVector &,
                    const std::array<std::int32_t, 3> &) override {
            missing("native_miles_shadow_ripple");
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
                missing("native_miles_shadow_matrix_capacity");
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
                missing("native_miles_shadow_asset_owner");
            const auto shadow_object = context.players.gamma_assets->shadow_object;
            if (!shadow_object)
                missing("native_miles_shadow_object");
            if (!context.model_environment)
                missing("native_miles_shadow_environment");
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
                missing("native_miles_shadow_model_queue");
            if (queued.route != LateModelRoute::queued)
                missing("native_late_model_immediate");
        }
    } services(context, body, work);
    shadow::update(body, work, services, context.rendering.arithmetic());
}
std::uint16_t &item_word(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.item)
        missing("native_miles_item_word");
    return *fields.item;
}
std::uint16_t &wait_timer(Context &context) {
    try {
        return player_initialization::bind_wait_timer(context.task);
    } catch (const player_initialization::Error &) {
        missing("native_miles_wait_timer");
    }
}
std::int16_t stage(Context &context) { return context.stage; }
void chao_setup(Context &, const TaskWorkHandle &, const Player &, std::uint8_t) {
    missing("native_miles_chao_setup");
}
bool check_damage(Context &, Primary task, const TaskWorkHandle &, const Player &player) {
    // MilesCheckDamage returns zero when playerwk is absent (the pre-initialization
    // frame), and when the task hurt bit and the player's damage-contact attribute
    // are both clear. Only the remaining branch can store a damage mode.
    if (!player)
        return false;
    const auto flags = task.status;
    const auto attributes = physics(player).attributes;
    if ((flags & kHurt) == 0 && (attributes & kDamageAttribute) == 0)
        return false;
    missing("native_miles_check_damage");
}
void hurt_reaction(Context &, Primary, const Player &) {
    missing("native_miles_hurt_reaction");
}
void scatter_rings(Context &, std::uint8_t) { missing("native_miles_scatter_rings"); }
bool check_next_action(Context &, Primary, const TaskWorkHandle &, const Player &) {
    // MilesCheckNextAction is the event/next-action state machine: its ewp
    // move/path prefix and the smode jump table can both store a new mode, so
    // it is not a preview-zero predicate.
    missing("native_miles_check_next_action");
}
bool check_fall(Context &, Primary task, const TaskWorkHandle &, const Player &) {
    // MilesCheckFall's first guard returns zero when either low status bit is
    // set. The preview task starts with status 7 and the entry step clears only
    // the on-collision bit, leaving bit 0 set; only the clear branch can store
    // MD_MILES_FALL.
    if ((task.status & 3) != 0)
        return false;
    missing("native_miles_check_fall");
}
bool check_jump(Context &, Primary, const TaskWorkHandle &, const Player &) {
    // MilesCheckJump calls JumpAllowed, which reads the held jump mask and the
    // task disable-control bit, then can store MD_MILES_JUMP.
    missing("native_miles_check_jump");
}
bool check_grab(Context &context, Primary task, const TaskWorkHandle &, const Player &) {
    // MilesCheckGrab tests the held word against GrabButtons before any holder
    // query or mode store; the selection preview holds no button.
    if ((held_buttons(context, task.player_index) & grab_buttons(context)) == 0)
        return false;
    missing("native_miles_check_grab");
}
bool check_attack(Context &context, Primary task, const TaskWorkHandle &, const Player &) {
    // MilesCheckAttack tests the held word against AttackButtons and the task
    // 0x2000 lock before any mode store; the preview holds no button and never
    // sets that bit.
    if ((held_buttons(context, task.player_index) & attack_buttons(context)) == 0 ||
        (task.status & kAttackLocked) != 0)
        return false;
    missing("native_miles_check_attack");
}
bool stage_pickup(Context &context, Primary, const TaskWorkHandle &, const Player &) {
    // The stage pickup owner returns zero outside the Chao stages before any other test.
    if (context.stage < kChaoGardenFirstStage || context.stage > kChaoGardenLastStage)
        return false;
    missing("native_miles_stage_pickup");
}
bool stage_action(Context &context, Primary, const TaskWorkHandle &, const Player &,
                  std::uint32_t) {
    // The stage action owner returns zero outside the Chao stages before any other test.
    if (context.stage < kChaoGardenFirstStage || context.stage > kChaoGardenLastStage)
        return false;
    missing("native_miles_stage_action");
}
bool analog_input(Context &, Primary) {
    // The recovered GetAnalog owner exists, but this path's normalized-input
    // and event-work facts are not composed in the Miles context yet.
    missing("native_miles_analog_input");
}
float contact_depth(Context &, const Player &player) {
    const auto *direction = player_contact_direction(physics(player));
    if (!direction)
        missing("native_miles_contact_depth");
    return direction->y;
}
fidelity::ExactFloat speed_magnitude(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work)
        missing("native_miles_motion_identity");
    const auto &speed =
        player_initialization::bind_existing_motion(context.task)->motion.speed;
    auto &arithmetic = context.rendering.arithmetic();
    return fidelity::ExactFloat::retained(
        arithmetic,
        fidelity::ExactFloat::vector_length_return(arithmetic, speed.x, speed.y, speed.z));
}
float slide_speed(Context &, const Player &player) {
    if (!player)
        missing("native_miles_player_parameters");
    return player->parameters.slide_speed;
}
void unrecovered_miles_auxiliary_a_task(TaskScheduler &, const TaskHandle &) {
    missing("native_miles_auxiliary_a_task");
}
void unrecovered_miles_auxiliary_b_task(TaskScheduler &, const TaskHandle &) {
    missing("native_miles_auxiliary_b_task");
}
// NOTSA: the shared elemental-child setup both Miles auxiliaries perform.
void create_auxiliary(Context &context, TaskCallback callback, std::uint8_t index) {
    const auto created = context.scheduler.create_element(2, 2, callback);
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
    const auto parent_motion = player_initialization::bind_existing_motion(context.task);
    const auto word_of = [](float value) {
        std::uint32_t word = 0;
        std::memcpy(&word, &value, sizeof(word));
        return word;
    };
    // The player pointer word has no native identity yet; the typed speed
    // words copy exactly, as in the Amy/Big setups.
    const std::array<std::uint32_t, 3> words{0, word_of(parent_motion->motion.speed.x),
                                             word_of(parent_motion->motion.speed.y)};
    for (unsigned word = 0; word < words.size(); ++word) {
        if (!arena(context).write(allocation.work, kCopiedWordsOffset + word * 4, &words[word],
                                  sizeof(words[word])))
            missing("native_miles_auxiliary_motion_copy");
    }
}
void initialize_auxiliary_a(Context &context, std::uint8_t index) {
    create_auxiliary(context, unrecovered_miles_auxiliary_a_task, index);
}
void initialize_auxiliary_b(Context &context, std::uint8_t index) {
    create_auxiliary(context, unrecovered_miles_auxiliary_b_task, index);
}
void stand_tail(Context &, const TaskWorkHandle &) { missing("native_miles_stand_tail"); }
void update_collision_center(Context &context) {
    // The collision center is the converted center-height offset plus the
    // task position, stored back into the first collision record.
    const auto view = player_initialization::bind_primary_view(context.task);
    const auto player = player_for_motion(context, context.task->motion_work);
    if (!player)
        missing("native_miles_collision_center_player");
    if (!has_collision(context, context.task->primary_work))
        missing("native_miles_collision_center_info");
    NinjaVector local{0.0f, player->parameters.center_height, 0.0f};
    const PlayerOrientation orientation{view.status, view.rotation};
    if (!context.rendering.convert_player_to_global(orientation, local))
        missing("native_miles_collision_center_convert");
    auto &center = collision_info(context, context.task->primary_work, 0).center;
    const auto add = [&context](float left, float right) {
        return (fidelity::ExactFloat(context.rendering.arithmetic(), left) +
                fidelity::ExactFloat(context.rendering.arithmetic(), right))
            .rounded();
    };
    center.x = add(view.position.x, local.x);
    center.y = add(view.position.y, local.y);
    center.z = add(view.position.z, local.z);
}
void update_held_task(Context &, const TaskWorkHandle &) { missing("native_miles_held_task"); }
void update_collision_range(Context &, Primary) { missing("native_miles_collision_range"); }
void set_collision_range(Context &, const TaskWorkHandle &, const Player &) {
    missing("native_miles_set_collision_range");
}
void set_default_normal_camera_mode(std::int32_t, std::int32_t) {
    missing("native_miles_default_camera_mode");
}
bool check_view(Context &context, Primary view, float radius) {
    auto &display = context.rendering;
    view_check::Services services{[&display](const NinjaVector &point) {
        return display.calculate_current_point(point);
    }};
    return view_check::check_view(view.position, radius, display.view_check_state(), services,
                                  display.arithmetic());
}
[[noreturn]] void missing_display_submit(Context &) { missing("native_miles_display_submit"); }
void destroy(Context &, const TaskHandle &) { missing("native_miles_destruct"); }
void get_rotation(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_miles_get_rotation");
}
void get_acceleration(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_miles_get_acceleration");
}
void get_speed(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_miles_get_speed");
}
std::int32_t set_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_miles_set_position");
}
void reset_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_miles_reset_position");
}
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) {
    missing("native_miles_unrecovered_mode");
}
TaskHandle create_child_task(Context &context, unsigned kind, std::int32_t level,
                             std::uint8_t index) {
    TaskCallback callback = nullptr;
    std::uint32_t flags = 0;
    switch (kind) {
    case 0:
        flags = 2;
        callback = unrecovered_child_a;
        break;
    case 1:
        flags = 3;
        callback = unrecovered_child_b;
        break;
    default:
        missing("native_miles_child_task_kind");
    }
    const auto created = context.scheduler.create_element(flags, level, callback);
    if (!created || !created.task)
        return {};
    store_child_index(context, created.task, index);
    return created.task;
}
} // namespace sadx::miles_selection

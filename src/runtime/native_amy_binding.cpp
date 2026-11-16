#include "engine/collision_component.hpp"
#include "game/editor_state.hpp"
#include "game/game_mode.hpp"
#include "player/character_shadow.hpp"
#include "player/player_vertex_weld.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_sonic_fields.hpp"
#include "runtime/native_underwater_binding.hpp"
#include "runtime/object_draw_context.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::amy_selection {
namespace {
[[noreturn]] void missing(const char *boundary) { throw flow::UnrecoveredGameFlowCall(boundary); }
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
void update_action_state(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_amy_action_update");
}
void setup_for_player(Context &, std::uint8_t) { missing("native_amy_setup_for_player"); }
void initialize_auxiliary_a(Context &) { missing("native_amy_initialize_auxiliary_a"); }
void initialize_auxiliary_b(Context &) { missing("native_amy_initialize_auxiliary_b"); }
void initialize_auxiliary_c(Context &) { missing("native_amy_initialize_auxiliary_c"); }
void initialize_auxiliary_d(Context &) { missing("native_amy_initialize_auxiliary_d"); }
void initialize_auxiliary_e(Context &) { missing("native_amy_initialize_auxiliary_e"); }
void stand_tail(Context &, const TaskWorkHandle &) { missing("native_amy_stand_tail"); }
void post_display(Context &, const TaskWorkHandle &) { missing("native_amy_post_display"); }
void update_held_task(Context &, const TaskWorkHandle &) { missing("native_amy_held_task"); }
void post_shadow(Context &) { missing("native_amy_post_shadow"); }
void finish_frame(Context &) { missing("native_amy_finish_frame"); }
void display(Context &, const TaskHandle &) { missing("native_amy_display"); }
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

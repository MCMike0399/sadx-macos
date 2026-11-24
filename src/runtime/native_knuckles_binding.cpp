#include "engine/collision_component.hpp"
#include "engine/view_check.hpp"
#include "game/game_mode.hpp"
#include "player/character_shadow.hpp"
#include "player/player_vertex_weld.hpp"
#include "player/selection_player_queries.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_sonic_fields.hpp"
#include "runtime/native_underwater_binding.hpp"
#include "runtime/object_draw_context.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace sadx::knuckles_selection {
namespace {
[[noreturn]] void missing(const char *boundary) { throw flow::UnrecoveredGameFlowCall(boundary); }
TaskWorkArena &arena(Context &context) {
    const auto owner = context.scheduler.work_arena();
    if (!owner)
        missing("native_knuckles_task_arena");
    return *owner;
}
native::KnucklesActorAssets &assets(Context &context) {
    if (!context.players.knuckles_assets)
        missing("native_knuckles_actor_assets");
    return *context.players.knuckles_assets;
}
// NOTSA: the typed free-work fields share the actual player allocation identity.
native::PlayerSystem::KnucklesExtraWork &extra(Context &context, const Player &player) {
    if (!player)
        missing("native_knuckles_null_player_free_work");
    for (auto &work : context.players.knuckles_work)
        if (work.allocation == player->allocation)
            return work;
    context.players.knuckles_work.push_back({player->allocation, 0});
    return context.players.knuckles_work.back();
}
PlayerPhysicsState &physics(const Player &player) {
    if (!player || !player->physics)
        missing("native_knuckles_player_physics");
    return *player->physics;
}
std::shared_ptr<collision_component::Work> collision(Context &context,
                                                     const TaskWorkHandle &primary_work) {
    auto component = collision_component::component(arena(context), primary_work);
    if (!component)
        missing("native_knuckles_null_collision_component");
    return component;
}
collision_component::Info &collision_info(Context &context, const TaskWorkHandle &primary_work,
                                          unsigned record) {
    auto component = collision(context, primary_work);
    if (record >= component->info_count || record >= component->info.size())
        missing("native_knuckles_collision_info_record");
    return component->info[record];
}
constexpr unsigned kMotionPlayerField = 0, kCollisionField = 56;
// KnucklesCheckDamage's source status and attribute tests.
constexpr std::uint16_t kHurt = 4;
constexpr std::uint32_t kDamageAttribute = 0x10000;
constexpr std::uint8_t kRollMode = 4;
// KnucklesCheckDig's playerwk+0x10 Status_Unknown4 gate.
constexpr std::uint16_t kDigStatus = 0x40;
// Knuckles_CollisionShape's glide range, item/status gates and damage nibble.
constexpr std::uint16_t kFirstGlideAnimation = 0x36;
constexpr std::uint16_t kLastGlideAnimation = 0x45;
constexpr std::uint16_t kGlideAnimation = 0x33;
constexpr std::uint16_t kItemHighBit = 0x8000;
constexpr std::uint16_t kStatusHighBit = 0x100;
constexpr std::uint8_t kPunchMode = 0x12;
constexpr std::uint8_t kDigMode = 0x34;
constexpr std::uint8_t kDamageLowNibble = 0xf0;
// DrawCharacterShadow's source literals and late queue facts.
constexpr float kShadowDepthCull = 20000.0f;
constexpr float kShadowFalloffPerUnit = 0.03f;
constexpr float kShadowGroundLift = 0.3f;
constexpr float kShadowVerticalSquash = 0.8f;
constexpr std::uint8_t kLateShadowFlags = 6;
constexpr float kShadowSortBias = -27952.0f;
// Knuckles_PositionReference's source literals and level tables.
constexpr float kReferenceHalfPeriod = 50.0f;
constexpr float kReferencePeriod = 100.0f;
constexpr float kReferenceNearDistance = 8.0f;
constexpr float kReferenceMiddleDistance = 15.0f;
constexpr float kReferenceWrappedNearDistance = 4.0f;
constexpr float kReferenceWrappedFarDistance = 20.0f;
constexpr float kReferenceLevelRamp = 0.125f;
constexpr std::array<float, 7> kReferenceLevels = {
    0.0f, 0.2f, 0.12f, 0.08f, 0.03f, 0.05f, 0.02f};
constexpr std::array<float, 7> kReferenceWrappedMiddleLevels = {
    0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
constexpr std::array<float, 7> kReferenceWrappedNearLevels = {
    0.0f, 0.15f, 0.04f, 0.01f, 0.0f, 0.0f, 0.3f};
// Per-stage and per-act OR of the source killing-collision character masks.
// Zero is a null stage list, a null act entry, or the absent fourth act slot of
// a three-act list. A nonzero mask still needs the source object intersection,
// which no native stage object supplies yet.
constexpr std::array<std::array<std::uint8_t, 4>, 43> kDeathZoneCharacters = {{
    {0x00, 0x00, 0x00, 0x00}, // 0
    {0xff, 0xff, 0xff, 0x00}, // 1
    {0xff, 0xff, 0xff, 0x00}, // 2
    {0xff, 0xff, 0xff, 0x00}, // 3
    {0xff, 0xff, 0xff, 0x00}, // 4, three source acts
    {0x01, 0x41, 0x08, 0x00}, // 5
    {0x05, 0x05, 0x09, 0x00}, // 6, three source acts
    {0xff, 0xff, 0xff, 0x00}, // 7
    {0x00, 0xff, 0xff, 0xff}, // 8
    {0xff, 0xff, 0x00, 0x00}, // 9
    {0xff, 0xff, 0xff, 0xff}, // 10
    {0x00, 0x00, 0x00, 0x00}, // 11
    {0xff, 0xff, 0xff, 0x00}, // 12
    {0x00, 0x00, 0x00, 0x00}, // 13
    {0x00, 0x00, 0x00, 0x00}, // 14
    {0x00, 0x00, 0x00, 0x00}, // 15
    {0x00, 0x00, 0x00, 0x00}, // 16
    {0x00, 0x00, 0x00, 0x00}, // 17
    {0x00, 0x00, 0x00, 0x00}, // 18
    {0x00, 0x00, 0x00, 0x00}, // 19
    {0x00, 0x00, 0x00, 0x00}, // 20
    {0x00, 0x00, 0x00, 0x00}, // 21
    {0x00, 0x00, 0x00, 0x00}, // 22
    {0x00, 0x00, 0x00, 0x00}, // 23
    {0x00, 0x00, 0x00, 0x00}, // 24
    {0x00, 0x00, 0x00, 0x00}, // 25
    {0xff, 0xff, 0xff, 0xff}, // 26
    {0x00, 0x00, 0x00, 0x00}, // 27
    {0x00, 0x00, 0x00, 0x00}, // 28
    {0xff, 0xff, 0xff, 0xff}, // 29
    {0x00, 0x00, 0x00, 0x00}, // 30
    {0x00, 0x00, 0x00, 0x00}, // 31
    {0xff, 0xff, 0xff, 0xff}, // 32
    {0xff, 0xff, 0xff, 0xff}, // 33
    {0xff, 0xff, 0xff, 0x00}, // 34, three source acts
    {0xff, 0xff, 0xff, 0xff}, // 35
    {0x00, 0x00, 0x00, 0x00}, // 36
    {0x00, 0x00, 0x00, 0x00}, // 37
    {0x00, 0x00, 0x00, 0x00}, // 38
    {0x00, 0x00, 0x00, 0x00}, // 39
    {0x00, 0x00, 0x00, 0x00}, // 40
    {0x00, 0x00, 0x00, 0x00}, // 41
    {0x00, 0x00, 0x00, 0x00}, // 42
}};

// NOTSA: the source helper wraps one axis into the [0, 100) reference period.
float wrap_reference_axis(Context &context, float value) {
    auto &arithmetic = context.rendering.arithmetic();
    const auto offset =
        (fidelity::ExactFloat(arithmetic, value) + kReferenceHalfPeriod).rounded();
    const auto quotient =
        (fidelity::ExactFloat(arithmetic, offset) / kReferencePeriod).rounded_double();
    const auto multiple = fidelity::ExactFloat::wide(arithmetic, std::floor(quotient)) *
                          kReferencePeriod;
    const auto remainder = offset - multiple;
    return (offset - remainder).rounded();
}

// NOTSA: the far branch measures the raw-to-wrapped offset, not the reference
// displacement. The source subtracts in z, y, x order and squares x first.
fidelity::ExactFloat wrapped_offset_distance(Context &context, Primary view,
                                            float wrapped_x, float wrapped_z) {
    auto &arithmetic = context.rendering.arithmetic();
    const auto dx = fidelity::ExactFloat(arithmetic, view.position.x) - wrapped_x;
    const auto dy = fidelity::ExactFloat(arithmetic, view.position.y) - view.position.y;
    const auto dz = fidelity::ExactFloat(arithmetic, view.position.z) - wrapped_z;
    // Named in source order because the two sides of + are not sequenced.
    const auto xx = dx * dx;
    const auto yy = dy * dy;
    const auto zz = dz * dz;
    const auto squared = ((xx + yy) + zz).rounded();
    return fidelity::ExactFloat(arithmetic, squared).square_root();
}

// STUB: SONIC 0x475660 Knuckles_ChildTaskA
void unrecovered_child_a(TaskScheduler &, const TaskHandle &) { missing("native_knuckles_child_a"); }
// STUB: SONIC 0x473ce0 Knuckles_ChildTaskB
void unrecovered_child_b(TaskScheduler &, const TaskHandle &) { missing("native_knuckles_child_b"); }
// The seam declaration in the Knuckles core owns this source entry.
void unrecovered_radar(TaskScheduler &, const TaskHandle &) { missing("native_knuckles_radar"); }
// STUB: SONIC 0x474070 InitKnucklesMaximumHeatAttack
void unrecovered_heat_attack(TaskScheduler &, const TaskHandle &) {
    missing("native_knuckles_heat_attack");
}
// NOTSA: store the source player-index byte into a created child's primary.
void store_child_index(Context &context, const TaskHandle &child, std::uint8_t index) {
    player_initialization::establish_zeroed_primary(context.scheduler, child);
    player_initialization::bind_primary_view(child).player_index = index;
}
} // namespace

State &state(Context &context) { return context.players.knuckles; }
Primary primary(Context &context, const TaskWorkHandle &work) {
    if (work != context.task->primary_work)
        missing("native_knuckles_primary_identity");
    return player_initialization::bind_primary_view(context.task);
}
Player player_for_motion(Context &context, const TaskWorkHandle &motion) {
    const auto reference = arena(context).reference(motion, kMotionPlayerField);
    if (!reference)
        missing("native_knuckles_motion_player_reference");
    if (!reference.work)
        return {};
    const auto found = context.players.registry.allocations.find(reference.work.get());
    if (found == context.players.registry.allocations.end())
        missing("native_knuckles_player_allocation_identity");
    return found->second;
}
TaskWorkHandle player_allocation(Context &, const Player &player) {
    return player ? player->allocation : TaskWorkHandle{};
}
std::uint16_t &requested_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).requested_animation;
}
std::uint16_t &idle_timer(Context &context, const Player &player) {
    return extra(context, player).idle_timer;
}
std::uint16_t &equipment(Context &, const Player &player) {
    auto &value = physics(player).equipment;
    if (!value)
        missing("native_knuckles_equipment");
    return *value;
}
std::uint16_t &task_timer(Context &context, Primary) {
    return player_initialization::bind_task_timer_high(context.task);
}
std::uint16_t &item_word(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.item)
        missing("native_knuckles_item_word");
    return *fields.item;
}
std::uint16_t &wait_timer(Context &context) {
    try {
        return player_initialization::bind_wait_timer(context.task);
    } catch (const player_initialization::Error &) {
        missing("native_knuckles_wait_timer");
    }
}
std::uint16_t &player_flags(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.flags)
        missing("native_knuckles_player_status");
    return *fields.flags;
}
std::uint32_t held_buttons(Context &context, std::uint8_t player_index) {
    if (!context.held_buttons)
        missing("native_knuckles_held_buttons");
    return context.held_buttons(player_index);
}
std::uint32_t attack_buttons(Context &context) {
    if (!context.attack_buttons)
        missing("native_knuckles_attack_buttons");
    return context.attack_buttons();
}
float &speed_x(Context &, const Player &player) { return physics(player).motion.speed.x; }
bool has_held_task(Context &, const Player &player) {
    return bool(player_initialization::bind_initialized_player(player).held_task);
}
bool has_collision(Context &context, const TaskWorkHandle &primary_work) {
    const auto reference = arena(context).reference(primary_work, kCollisionField);
    if (!reference)
        missing("native_knuckles_collision_reference");
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
        missing("native_knuckles_collision_task_backlink");
    return player_initialization::bind_primary_view(task).rotation.at(axis);
}
float vertical_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work)
        missing("native_knuckles_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.y;
}
float player_radius(Context &, const Player &player) {
    if (!player)
        missing("native_knuckles_player_parameters");
    return player->parameters.rad;
}
TaskCallback display_callback(Context &context) {
    if (!context.display)
        missing("native_knuckles_display_callback");
    return context.display;
}
TaskCallback destructor_callback(Context &context) {
    if (!context.destructor)
        missing("native_knuckles_destructor_callback");
    return context.destructor;
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.rendering.arithmetic(), value};
}
std::int32_t game_mode(Context &context) { return context.bootstrap.game_mode; }
std::int16_t stage(Context &context) { return context.stage; }
std::int16_t act(Context &context) { return context.act; }
std::int32_t check_edit_mode(Context &context) {
    if (!context.editor_state)
        missing("native_knuckles_check_edit_mode");
    return editor::check_mode(*context.editor_state);
}
std::int32_t event_cancel(Context &context) {
    return selection_player_queries::event_cancel(context.bootstrap.cutscene_task);
}

void initialize_player(Context &context, std::int32_t index, const TaskHandle &task) {
    if (!context.players.gamma_assets)
        missing("native_knuckles_parameters");
    player_initialization::NativeAccess access(context.players.gamma_assets->parameters,
                                               context.bootstrap.event_work, context.auxiliary);
    Player record;
    player_initialization::initialize(
        index, task, player_initialization::bind_existing_motion(task), context.scheduler,
        context.players.registry, access, context.rendering.arithmetic(), record);
}
void bind_animation_table(Context &context, const Player &player) {
    const auto &table = assets(context).animation_table;
    if (!table || table->records.size() != 115)
        missing("native_knuckles_complete_animation_table");
    player_initialization::bind_initialized_player(player).animation_table = table;
}
void bind_weld_table(Context &context, const Player &player) {
    if (context.players.knuckles_weld.rows.size() != 30)
        missing("native_knuckles_imported_weld_table");
    player_initialization::bind_initialized_player(player).join_table =
        &context.players.knuckles_weld;
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
        missing("native_knuckles_character_texture_owner");
    regular_textures::load_character(character, context.character_textures, *context.texture_state,
                                     *context.texture_services);
}
void calculate_collision_range(Context &context, Primary) {
    if (!context.task)
        missing("native_knuckles_calculate_collision_range");
    auto work = collision(context, context.task->primary_work);
    collision_component::calculate_range(work.get(), context.rendering.arithmetic());
}
void enter_collision_list(Context &context, Primary view) {
    if (!context.task)
        missing("native_knuckles_null_collision_task");
    if (!context.collision_entries)
        missing("native_knuckles_collision_entries");
    auto work = collision(context, context.task->primary_work);
    collision_component::enter_list(*context.collision_entries, work, view.status, context.stage);
}
// NOTSA: native boundary for RunUnderwaterStuff; the shared product owner is
// native::run_underwater.
void underwater(Context &context, Primary primary, const TaskWorkHandle &motion_work,
                const Player &player) {
    if (motion_work != context.task->motion_work)
        missing("native_knuckles_motion_identity");
    auto &body = physics(player);
    native::run_underwater(context.players, context.rendering, context.task, primary, body, player);
}
void set_motion(Context &context, const Player &player) {
    if (!context.players.knuckles_motion_assets)
        missing("native_knuckles_set_motion");
    animation::View view{physics(player), player_allocation(context, player)};
    animation::Runtime runtime{&arena(context), context.players.knuckles_motion_assets->resources(),
                               &context.scene.objects};
    const auto result = animation::update(view, runtime, context.rendering.arithmetic());
    if (!result)
        missing("native_knuckles_set_motion_failed");
}
void join_vertices(Context &context, Primary, const TaskWorkHandle &, const Player &player) {
    auto &join_mode = player_initialization::bind_join_vertex_mode(player);
    const auto mode = join_mode;
    if (mode == 1) {
        // Active processing reaches the real motion-matrix producer before it
        // consumes the join table, including when the table is empty.
        missing("native_knuckles_join_vertices_CalcMMMatrix");
    }
    auto &table = physics(player).animation_tail.join_table;
    if (!table)
        missing("native_knuckles_weld_table_field");
    if (!*table)
        missing("native_knuckles_weld_null_table");
    if (mode == 0) {
        vertex_weld::initialize(**table, arena(context), context.scene);
        join_mode = 1;
    } else {
        vertex_weld::restore(**table, arena(context), context.scene);
    }
}
// NOTSA: native boundary for CharacterShadow; source seam declared in the
// Knuckles core.
void character_shadow(Context &context, Primary task, const Player &player) {
    auto &state = physics(player);
    const auto field = [](auto &value, const char *boundary) -> auto & {
        if (!value)
            missing(boundary);
        return *value;
    };
    namespace shadow = sadx::character_shadow;
    shadow::ShadowWork work{
        field(state.shadow_angle_x, "native_knuckles_shadow_angle_x"),
        field(state.shadow_angle_z, "native_knuckles_shadow_angle_z"),
        state.shadow_attributes_top,
        field(state.shadow_attributes_bottom, "native_knuckles_shadow_attributes_bottom"),
        field(state.shadow_top_y, "native_knuckles_shadow_top_y"),
        field(state.shadow_bottom_y, "native_knuckles_shadow_bottom_y"),
        field(state.shadow_prior_attributes_top, "native_knuckles_shadow_prior_attributes_top"),
        field(state.shadow_prior_attributes_bottom,
              "native_knuckles_shadow_prior_attributes_bottom"),
        field(state.shadow_scale, "native_knuckles_shadow_scale")};
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
                missing("native_knuckles_shadow_land_list");
            return {};
        }
        fidelity::ExactFloat horizontal_distance(const NinjaVector &) override {
            missing("native_knuckles_shadow_distance");
        }
        shadow::ShadowHit get_zx_shadow(const NinjaVector &,
                                        const shadow::LandCandidate &) override {
            missing("native_knuckles_shadow_water_polygon");
        }
        shadow::ShadowHit polygon_test(const NinjaVector &,
                                       const shadow::LandCandidate &) override {
            missing("native_knuckles_shadow_polygon");
        }
        void splash() override { missing("native_knuckles_shadow_splash"); }
        void ripple(std::uint8_t, const NinjaVector &,
                    const std::array<std::int32_t, 3> &) override {
            missing("native_knuckles_shadow_ripple");
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
                missing("native_knuckles_shadow_matrix_capacity");
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
                missing("native_knuckles_shadow_asset_owner");
            const auto shadow_object = context.players.gamma_assets->shadow_object;
            if (!shadow_object)
                missing("native_knuckles_shadow_object");
            if (!context.model_environment)
                missing("native_knuckles_shadow_environment");
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
                missing("native_knuckles_shadow_model_queue");
            if (queued.route != LateModelRoute::queued)
                missing("native_late_model_immediate");
        }
    } services(context, body, work);
    shadow::update(body, work, services, context.rendering.arithmetic());
}
void set_event_camera(Context &, std::int16_t, std::int8_t) {
    missing("native_knuckles_camera_event");
}
void play_one_shot(Context &, std::int32_t, std::int32_t, std::int32_t, std::int32_t) {
    missing("native_knuckles_ds_play_oneshot");
}
void play_loop(Context &, std::int32_t, std::int32_t, std::int32_t, std::int32_t) {
    missing("native_knuckles_ds_play_iloop");
}
void stop_id(Context &) { missing("native_knuckles_ds_stop_id"); }
void start_idle_camera_effect(Context &) { missing("native_knuckles_camera_idle_start"); }
void reset_idle_camera_effect(Context &) { missing("native_knuckles_camera_idle_reset"); }
void start_held_camera_effect(Context &) { missing("native_knuckles_camera_held_start"); }
void chao_setup(Context &, const TaskWorkHandle &, const Player &, std::uint8_t) {
    missing("native_knuckles_chao_setup");
}
bool check_damage(Context &, Primary task, const Player &player) {
    // The damage checker returns 0 when playerwk is null (the pre-initialization frame),
    // or when neither the task hurt bit nor the player's damage-contact
    // attribute (0x10000) is set, or in MD_KNUCK_ROLL. Only that branch stores
    // MD_KNUCK_DAMG, so the guard below is the full source zero branch.
    if (!player)
        return false;
    const auto flags = task.status;
    const auto attributes = physics(player).attributes;
    if ((((flags & kHurt) == 0) && ((attributes & kDamageAttribute) == 0)) ||
        task.mode == kRollMode)
        return false;
    missing("native_knuckles_check_damage");
}
void hurt_reaction(Context &, const TaskWorkHandle &) {
    missing("native_knuckles_hurt_reaction");
}
void scatter_rings(Context &, std::uint8_t) { missing("native_knuckles_scatter_rings"); }
bool check_next_action(Context &, Primary, const TaskWorkHandle &, const Player &) {
    // The next-action checker's ewp move/path prefix and its kNextAction/smode table can both
    // store a new mode, including the smode 0x0c case the preview injects.
    missing("native_knuckles_check_next_action");
}
bool check_fall(Context &, const TaskWorkHandle &) {
    // The fall checker stores MD_KNUCK_FALL and reqaction 0x13 whenever the status low
    // bits are clear, which is the preview's first stand condition.
    missing("native_knuckles_check_fall");
}
bool check_dig(Context &context, const TaskWorkHandle &, const Player &player) {
    // The dig checker's whole body is gated on playerwk+0x10 bit 0x40; the preview's
    // status word starts at zero and no contact owner sets that bit.
    if ((player_flags(context, player) & kDigStatus) == 0)
        return false;
    missing("native_knuckles_check_dig");
}
bool check_jump(Context &, Primary, const Player &) {
    // The jump checker calls JumpAllowed and stores MD_KNUCK_JUMP / HJMP / PNJP.
    missing("native_knuckles_check_jump");
}
bool check_grab(Context &, Primary, const TaskWorkHandle &) {
    // The grab checker reads the grab mask, PCheckHoldObject and the wall form, then
    // stores MD_KNUCK_TOCH / PICK.
    missing("native_knuckles_check_grab");
}
bool check_attack(Context &context, Primary task) {
    // The attack checker tests the pad held word against the AttackButtons mask before
    // any store; the selection preview holds no attack button.
    if ((held_buttons(context, task.player_index) & attack_buttons(context)) == 0)
        return false;
    missing("native_knuckles_check_attack");
}
bool stage_pickup(Context &context, Primary, const TaskWorkHandle &, const Player &) {
    // The stage pickup owner returns zero outside the Chao stages before any other test.
    if (context.stage < 39 || context.stage > 42)
        return false;
    missing("native_knuckles_stage_pickup");
}
bool stage_action(Context &context, Primary, const TaskWorkHandle &, const Player &,
                  std::uint32_t) {
    // The stage action owner returns zero outside the Chao stages before any other test.
    if (context.stage < 39 || context.stage > 42)
        return false;
    missing("native_knuckles_stage_action");
}
bool analog_input(Context &, Primary, const TaskWorkHandle &, const Player &) {
    // The recovered GetAnalog owner exists, but this path's normalized-input
    // and event-work facts are not composed in the Knuckles context yet.
    missing("native_knuckles_analog_input");
}
void unrecovered_knuckles_auxiliary_task(TaskScheduler &, const TaskHandle &) {
    missing("native_knuckles_auxiliary_task");
}
void initialize_auxiliary(Context &context, std::uint8_t index) {
    const auto created = context.scheduler.create_element(2, 2, unrecovered_knuckles_auxiliary_task);
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
            missing("native_knuckles_auxiliary_motion_copy");
    }
}
character_pack::ObjectResource &equipment_object(Context &context, unsigned table_index) {
    if (table_index < 58 || table_index >= 74)
        missing("native_knuckles_equipment_object_row");
    return context.scene.objects.object(assets(context).equipment_objects[table_index - 58]);
}
character_pack::ResourceId equipment_model(Context &context, unsigned table_index) {
    if (table_index >= 23)
        missing("native_knuckles_equipment_model_row");
    return assets(context).equipment_models[table_index];
}
void stand_tail(Context &, const TaskWorkHandle &) { missing("native_knuckles_stand_tail"); }
void update_held_task(Context &, const TaskWorkHandle &) { missing("native_knuckles_held_task"); }
void update_collision_shape(Context &context, Primary view, const Player &player) {
    // Knuckles_CollisionShape: outside the glide animation range it enables
    // every collision record and rewrites the first damage byte; the glide
    // shape rewrite stays a seam below.
    const auto requested = requested_animation(context, player);
    if (requested >= kFirstGlideAnimation && requested <= kLastGlideAnimation)
        return;
    if (!has_collision(context, context.task->primary_work)) {
        if (requested == kGlideAnimation)
            missing("native_knuckles_collision_shape_info");
        return;
    }
    if (requested == kGlideAnimation)
        missing("native_knuckles_collision_shape_glide");
    constexpr std::uint32_t kEnabled = 16;
    collision_attributes(context, context.task->primary_work, 1) |= kEnabled;
    collision_attributes(context, context.task->primary_work, 2) |= kEnabled;
    collision_attributes(context, context.task->primary_work, 3) |= kEnabled;
    std::uint8_t low = 0;
    std::uint8_t klass = 0;
    if ((item_word(context, player) & kItemHighBit) != 0) {
        low = 3;
        klass = 3;
    } else if ((view.status & kStatusHighBit) != 0) {
        low = 1;
        klass = 1;
    } else if (view.mode == kPunchMode) {
        low = 3;
        klass = 1;
    } else if (view.mode == kDigMode) {
        klass = 1;
    }
    auto &damage = collision_info(context, context.task->primary_work, 0).damage;
    damage = static_cast<std::uint8_t>((klass << 2) | (damage & kDamageLowNibble) | low);
}
void update_reference_position(Context &context, Primary view) {
    auto &arithmetic = context.rendering.arithmetic();
    auto &reference = state(context).position_reference;
    const auto wrapped_x = wrap_reference_axis(context, view.position.x);
    const auto wrapped_z = wrap_reference_axis(context, view.position.z);
    const auto dx = fidelity::ExactFloat(arithmetic, view.position.x) - reference.x;
    const auto dy = fidelity::ExactFloat(arithmetic, view.position.y) - reference.y;
    const auto dz = fidelity::ExactFloat(arithmetic, view.position.z) - reference.z;
    // Named in source order because the two sides of + are not sequenced.
    const auto xx = dx * dx;
    const auto yy = dy * dy;
    const auto zz = dz * dz;
    const auto squared = ((xx + yy) + zz).rounded();
    const auto distance = fidelity::ExactFloat(arithmetic, squared).square_root();
    // The source stores the root before comparing the retained extended value,
    // then reloads the binary32 word for the near-distance decision and ramp.
    const auto stored_distance = distance.rounded();
    if (distance.compare(kReferenceMiddleDistance) == X87Comparison::less) {
        const auto stored = fidelity::ExactFloat(arithmetic, stored_distance);
        if (stored.compare(kReferenceNearDistance) == X87Comparison::less) {
            for (std::size_t index = 0; index < reference.levels.size(); ++index)
                reference.levels[index] =
                    (fidelity::ExactFloat(arithmetic, kReferenceLevels[index]) *
                     fidelity::ExactFloat(arithmetic, 0.0f))
                        .rounded();
            return;
        }
        const auto scale = (stored - kReferenceNearDistance) * kReferenceLevelRamp;
        for (std::size_t index = 0; index < reference.levels.size(); ++index)
            reference.levels[index] =
                (fidelity::ExactFloat(arithmetic, kReferenceLevels[index]) * scale).rounded();
        return;
    }
    const auto wrapped_distance =
        wrapped_offset_distance(context, view, wrapped_x, wrapped_z);
    if (wrapped_distance.compare(kReferenceWrappedFarDistance) != X87Comparison::less) {
        for (std::size_t index = 0; index < reference.levels.size(); ++index)
            reference.levels[index] = kReferenceLevels[index];
        return;
    }
    if (wrapped_distance.compare(kReferenceWrappedNearDistance) == X87Comparison::less) {
        for (std::size_t index = 0; index < reference.levels.size(); ++index)
            reference.levels[index] = kReferenceWrappedNearLevels[index];
        return;
    }
    for (std::size_t index = 0; index < reference.levels.size(); ++index)
        reference.levels[index] = kReferenceWrappedMiddleLevels[index];
}
bool is_in_death_zone(Context &context, Primary) {
    const auto stage_index = static_cast<std::uint8_t>(stage(context));
    if (stage_index >= kDeathZoneCharacters.size())
        missing("native_knuckles_death_zone_stage");
    const auto act_index = static_cast<std::uint8_t>(act(context));
    if (act_index >= kDeathZoneCharacters[stage_index].size())
        missing("native_knuckles_death_zone_act");
    const auto characters = kDeathZoneCharacters[stage_index][act_index];
    if (characters == 0)
        return false;
    const auto player_bit =
        std::uint32_t{1} << (static_cast<unsigned>(context.bootstrap.player_number) & 31U);
    if ((characters & player_bit) == 0)
        return true;
    // TestObjectIntersect is the source's remaining body for a matching record.
    // The native scene has no killing-collision object for the source pointer,
    // so the hit test is the explicit boundary.
    missing("native_knuckles_death_zone_intersection");
}
void other_player_position(Context &, std::uint8_t, std::int8_t, NinjaVector &) {
    missing("native_knuckles_player_position");
}
bool check_view(Context &context, Primary view, float radius) {
    auto &display = context.rendering;
    view_check::Services services{[&display](const NinjaVector &point) {
        return display.calculate_current_point(point);
    }};
    return view_check::check_view(view.position, radius, display.view_check_state(), services,
                                  display.arithmetic());
}
[[noreturn]] void missing_display_submit(Context &) { missing("native_knuckles_display_submit"); }
void destroy(Context &, const TaskHandle &) { missing("native_knuckles_destruct"); }
void get_rotation(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_knuckles_get_rotation");
}
void get_acceleration(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_knuckles_get_acceleration");
}
void get_speed(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_knuckles_get_speed");
}
std::int32_t set_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_knuckles_set_position");
}
void reset_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_knuckles_reset_position");
}
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) {
    missing("native_knuckles_unrecovered_mode");
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
    case 2:
        flags = 2;
        callback = unrecovered_heat_attack;
        // The source also stores a pointer to the player idle timer at primary
        // +0x10 for this child. Its consumer is unrecovered and the idle timer
        // is not a task allocation, so no raw pointer is published.
        break;
    default:
        missing("native_knuckles_child_task_kind");
    }
    const auto created = context.scheduler.create_element(flags, level, callback);
    if (!created || !created.task)
        return {};
    store_child_index(context, created.task, index);
    return created.task;
}
TaskHandle create_radar_task(Context &context) {
    const auto created = context.scheduler.create_element(2, 6, unrecovered_radar);
    if (!created || !created.task)
        return {};
    return created.task;
}
} // namespace sadx::knuckles_selection

#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "engine/collision_component.hpp"
#include "player/player_underwater.hpp"
#include "game/game_mode.hpp"
#include "runtime/native_sonic_fields.hpp"
#include <stdexcept>

namespace sadx::big_selection {
namespace {
[[noreturn]] void missing(const char *boundary) {
    throw flow::UnrecoveredGameFlowCall(boundary);
}
TaskWorkArena &arena(Context &context) {
    const auto owner = context.scheduler.work_arena();
    if (!owner)
        missing("native_big_task_arena");
    return *owner;
}
native::BigActorAssets &assets(Context &context) {
    if (!context.players.big_assets)
        missing("native_big_actor_assets");
    return *context.players.big_assets;
}
PlayerPhysicsState &physics(const Player &player) {
    if (!player || !player->physics)
        missing("native_big_player_physics");
    return *player->physics;
}
std::shared_ptr<collision_component::Work> collision(Context &context,
                                                     const TaskWorkHandle &primary_work) {
    auto component = collision_component::component(arena(context), primary_work);
    if (!component)
        missing("native_big_null_collision_component");
    return component;
}
collision_component::Info &collision_info(Context &context, const TaskWorkHandle &primary_work,
                                          unsigned record) {
    auto component = collision(context, primary_work);
    if (record >= component->info_count || record >= component->info.size())
        missing("native_big_collision_info_record");
    return component->info[record];
}
// NOTSA: the typed free-work fields share the actual player allocation identity.
native::PlayerSystem::BigExtraWork &extra(Context &context, const Player &player) {
    if (!player)
        missing("native_big_null_player_free_work");
    for (auto &work : context.players.big_work)
        if (work.allocation == player->allocation)
            return work;
    context.players.big_work.push_back({player->allocation, 0, {}});
    return context.players.big_work.back();
}
constexpr unsigned kMotionPlayerField = 0, kCollisionField = 56;
} // namespace

State &state(Context &context) { return context.players.big; }
CurrentView current(Context &context) {
    return {context.players.current.primary, context.players.current.player};
}
Primary primary(Context &context, const TaskWorkHandle &work) {
    if (work != context.task->primary_work)
        missing("native_big_primary_identity");
    return player_initialization::bind_primary_view(context.task);
}
Player player_for_motion(Context &context, const TaskWorkHandle &motion) {
    const auto reference = arena(context).reference(motion, kMotionPlayerField);
    if (!reference)
        missing("native_big_motion_player_reference");
    if (!reference.work)
        return {};
    const auto found = context.players.registry.allocations.find(reference.work.get());
    if (found == context.players.registry.allocations.end())
        missing("native_big_player_allocation_identity");
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
std::uint16_t &previous_animation(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.previous_animation)
        missing("native_big_previous_animation");
    return *fields.previous_animation;
}
std::uint16_t &player_flags(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).flags;
}
std::uint16_t &equipment(Context &, const Player &player) {
    auto &value = physics(player).equipment;
    if (!value)
        missing("native_big_equipment");
    return *value;
}
std::uint16_t &item_word(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).flags;
}
std::uint32_t &free_flags(Context &context, const Player &player) {
    return extra(context, player).free_flags;
}
TaskHandle &lure_handle(Context &context, const Player &player) {
    return extra(context, player).lure;
}
std::uint16_t &idle_timer(Context &, const Player &player) {
    auto fields = sonic::native_fields::player(physics(player));
    if (!fields.idle_timer)
        missing("native_big_idle_timer");
    return *fields.idle_timer;
}
bool has_collision(Context &context, const TaskWorkHandle &primary_work) {
    const auto reference = arena(context).reference(primary_work, kCollisionField);
    if (!reference)
        missing("native_big_collision_reference");
    return bool(reference.work);
}
std::uint8_t &collision_damage(Context &context, const TaskWorkHandle &primary_work,
                               unsigned record) {
    return collision_info(context, primary_work, record).damage;
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
        missing("native_big_collision_task_backlink");
    return player_initialization::bind_primary_view(task).rotation.at(axis);
}
bool has_held_task(Context &, const Player &player) {
    return bool(player_initialization::bind_initialized_player(player).held_task);
}
float vertical_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work)
        missing("native_big_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.y;
}
float player_radius(Context &, const Player &player) {
    if (!player)
        missing("native_big_player_parameters");
    return player->parameters.rad;
}
TaskCallback display_callback(Context &context) {
    if (!context.display)
        missing("native_big_display_callback");
    return context.display;
}
TaskCallback destructor_callback(Context &context) {
    if (!context.destructor)
        missing("native_big_destructor_callback");
    return context.destructor;
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.rendering.arithmetic(), value};
}
std::int32_t game_mode(Context &context) { return context.bootstrap.game_mode; }
std::int16_t stage(Context &context) { return context.stage; }
std::uint16_t act(Context &context) { return context.act; }

void initialize_player(Context &context, std::int32_t index, const TaskHandle &task) {
    if (!context.players.gamma_assets)
        missing("native_big_parameters");
    player_initialization::NativeAccess access(context.players.gamma_assets->parameters,
                                               context.bootstrap.event_work, context.auxiliary);
    Player record;
    player_initialization::initialize(index, task,
        player_initialization::bind_existing_motion(task), context.scheduler,
        context.players.registry, access, context.rendering.arithmetic(), record);
}
void bind_animation_table(Context &context, const Player &player) {
    const auto &table = assets(context).animation_table;
    if (!table || table->records.size() != 105)
        missing("native_big_complete_animation_table");
    player_initialization::bind_initialized_player(player).animation_table = table;
}
void bind_weld_table(Context &context, const Player &player) {
    if (context.players.big_weld.rows.size() != 17)
        missing("native_big_imported_weld_table");
    player_initialization::bind_initialized_player(player).join_table = &context.players.big_weld;
}
void initialize_collision(Context &context, const TaskHandle &task, unsigned records,
                          std::uint8_t id) {
    auto view = player_initialization::bind_primary_view(task);
    (void)collision_component::initialize(arena(context), task, view.status,
        assets(context).collision_info, static_cast<std::int32_t>(records), id,
        context.rendering.arithmetic());
}
void load_character_textures(Context &context, std::uint8_t character) {
    if (!context.texture_state || !context.texture_services)
        missing("native_big_character_texture_owner");
    regular_textures::load_character(character, context.character_textures,
                                     *context.texture_state, *context.texture_services);
}
void load_big_pvm_textures(Context &) { missing("native_big_pvm_textures"); }
void calculate_collision_range(Context &context, Primary) {
    if (!context.task)
        missing("native_big_calculate_collision_range");
    auto work = collision(context, context.task->primary_work);
    collision_component::calculate_range(work.get(), context.rendering.arithmetic());
}
void enter_collision_list(Context &context, Primary view) {
    if (!context.task)
        missing("native_big_null_collision_task");
    if (!context.collision_entries)
        missing("native_big_collision_entries");
    auto work = collision(context, context.task->primary_work);
    collision_component::enter_list(*context.collision_entries, work, view.status, context.stage);
}
void underwater(Context &, Primary, const TaskWorkHandle &, const Player &) {
    // player_underwater::run is recovered; its product services are still
    // composed only by the Gamma binding.
    missing("native_big_underwater_binding");
}
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) {
    missing("native_big_unrecovered_mode");
}
void update_action_state(Context &, Primary, const TaskWorkHandle &) {
    missing("native_big_update_action_state");
}
TaskHandle create_flag_task(Context &, const TaskHandle &) {
    missing("native_big_create_flag_task");
}
void setup_for_player(Context &, std::uint8_t) { missing("native_big_setup_for_player"); }
void get_rotation(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_big_get_rotation");
}
void get_acceleration(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_big_get_acceleration");
}
void get_speed(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_big_get_speed");
}
std::int32_t set_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_big_set_position");
}
void reset_position(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_big_reset_position");
}
void set_motion(Context &, const Player &) { missing("native_big_set_motion"); }
void join_vertices(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_big_join_vertices");
}
void update_rod(Context &, const TaskWorkHandle &) { missing("native_big_update_rod"); }
void display(Context &, const TaskHandle &) { missing("native_big_display"); }
void destroy(Context &, const TaskHandle &) { missing("native_big_destruct"); }
void update_held_task(Context &, const TaskWorkHandle &) { missing("native_big_held_task"); }
void character_shadow(Context &, Primary, const Player &) { missing("native_big_character_shadow"); }
void stand_tail(Context &, const TaskWorkHandle &) { missing("native_big_stand_tail"); }
void update_equipment(Context &, const Player &) { missing("native_big_equipment"); }
TaskHandle create_child_task(Context &, unsigned, std::int32_t) {
    missing("native_big_child_task");
}
TaskHandle create_lure_task(Context &, const TaskHandle &) { missing("native_big_lure_task"); }
} // namespace sadx::big_selection

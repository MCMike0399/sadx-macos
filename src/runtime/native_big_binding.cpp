#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "engine/collision_component.hpp"
#include "player/player_vertex_weld.hpp"
#include "player/player_underwater.hpp"
#include "game/game_mode.hpp"
#include "runtime/native_sonic_fields.hpp"
#include <cstring>
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
constexpr unsigned kDirectMotionBytes = 0x88;
constexpr unsigned kCopiedWordsOffset = 0x58;
constexpr std::int32_t kHalfTurn = 0x8000;
void unrecovered_big_direct(TaskScheduler &, const TaskHandle &) {
    missing("native_big_direct_ahead");
}
void unrecovered_big_child_a(TaskScheduler &, const TaskHandle &) {
    missing("native_big_child_a");
}
void unrecovered_big_child_b(TaskScheduler &, const TaskHandle &) {
    missing("native_big_child_b");
}
void unrecovered_big_hud(TaskScheduler &, const TaskHandle &) {
    missing("native_big_hud_main");
}
// NOTSA: store the source player-index byte into a created child's primary.
void store_child_index(Context &context, const TaskHandle &child, std::uint8_t index) {
    player_initialization::establish_zeroed_primary(context.scheduler, child);
    player_initialization::bind_primary_view(child).player_index = index;
}
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
bool check_damage(Context &, const TaskWorkHandle &) { missing("native_big_check_damage"); }
void hurt_reaction(Context &, const TaskWorkHandle &) { missing("native_big_hurt_reaction"); }
void scatter_rings(Context &, std::uint8_t) { missing("native_big_scatter_rings"); }
bool stand_check_a(Context &, const TaskWorkHandle &) { missing("native_big_stand_check_a"); }
bool stand_check_b(Context &) { missing("native_big_stand_check_b"); }
bool stand_check_c(Context &, const TaskWorkHandle &) { missing("native_big_stand_check_c"); }
bool stand_check_d(Context &, const TaskWorkHandle &) { missing("native_big_stand_check_d"); }
bool stand_check_e(Context &, const TaskWorkHandle &) { missing("native_big_stand_check_e"); }
bool stand_check_f(Context &, const TaskWorkHandle &) { missing("native_big_stand_check_f"); }
bool stage_pickup(Context &context, Primary, const TaskWorkHandle &, const Player &) {
    // The recovered owner returns zero outside the Chao stages.
    if (context.stage < 39 || context.stage > 42)
        return false;
    missing("native_big_stage_pickup");
}
bool stage_action(Context &context, Primary, const Player &) {
    if (context.stage < 39 || context.stage > 42)
        return false;
    missing("native_big_stage_action");
}
void chao_setup(Context &, const TaskWorkHandle &, std::uint8_t) { missing("native_big_chao_setup"); }
bool analog_input(Context &, Primary) { missing("native_big_analog_input"); }
float analog_magnitude(Context &) { missing("native_big_analog_magnitude"); }
float contact_depth(Context &, const Player &player) {
    const auto &contact = physics(player).contact_work;
    if (!contact)
        missing("native_big_contact_depth");
    const auto &[direction, radius, center, displacement, angle_x, angle_z, count, selected,
                 v1, v2, v3, v4, v5, v6] = *contact;
    (void)radius;
    (void)center;
    (void)displacement;
    (void)angle_x;
    (void)angle_z;
    (void)count;
    (void)selected;
    (void)v1;
    (void)v2;
    (void)v3;
    (void)v4;
    (void)v5;
    (void)v6;
    if (!direction)
        missing("native_big_contact_depth");
    return direction->y;
}
float &slide_threshold(Context &context, const Player &player) {
    return extra(context, player).slide_threshold;
}
std::uint16_t &wait_timer(Context &context) {
    try {
        return player_initialization::bind_wait_timer(context.task);
    } catch (const player_initialization::Error &) {
        missing("native_big_wait_timer");
    }
}
TaskHandle create_flag_task(Context &, const TaskHandle &) {
    missing("native_big_create_flag_task");
}
void setup_for_player(Context &context, const TaskWorkHandle &motion, std::uint8_t index) {
    const auto created = context.scheduler.create_element(2, 2, unrecovered_big_direct);
    if (!created || !created.task)
        return;
    auto allocation = arena(context).allocate(kDirectMotionBytes);
    if (!allocation) {
        (void)context.scheduler.destroy_task(created.task);
        return;
    }
    created.task->motion_work = allocation.work;
    store_child_index(context, created.task, index);
    // The source copies the parent's first three motion words into the new
    // motion work at +0x58: the player pointer word has no native identity
    // yet, so the typed speed words are copied and the pointer word stays
    // zero until the child callback that consumes it is recovered.
    const auto parent_motion = player_initialization::bind_existing_motion(context.task);
    const auto word_of = [](float value) {
        std::uint32_t word = 0;
        std::memcpy(&word, &value, sizeof(word));
        return word;
    };
    const std::array<std::uint32_t, 3> words{0, word_of(parent_motion->motion.speed.x),
                                             word_of(parent_motion->motion.speed.y)};
    for (unsigned word = 0; word < words.size(); ++word) {
        if (!arena(context).write(allocation.work, kCopiedWordsOffset + word * 4, &words[word],
                                  sizeof(words[word])))
            missing("native_big_setup_motion_copy");
    }
    (void)motion;
}
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
void set_motion(Context &context, const Player &player) {
    if (!context.players.big_motion_assets)
        missing("native_big_set_motion");
    animation::View view{physics(player), player_allocation(context, player)};
    animation::Runtime runtime{&arena(context), context.players.big_motion_assets->resources(),
                               &context.scene.objects};
    const auto result = animation::update(view, runtime, context.rendering.arithmetic());
    if (!result)
        missing("native_big_set_motion_failed");
}
void join_vertices(Context &context, Primary, const TaskWorkHandle &, const Player &player) {
    auto &join_mode = player_initialization::bind_join_vertex_mode(player);
    const auto mode = join_mode;
    if (mode == 1) {
        // Active processing reaches the real motion-matrix producer before it
        // consumes the join table, including when the table is empty.
        missing("native_big_join_vertices_CalcMMMatrix");
    }
    auto &table = physics(player).animation_tail.join_table;
    if (!table)
        missing("native_big_weld_table_field");
    if (!*table)
        missing("native_big_weld_null_table");
    if (mode == 0) {
        vertex_weld::initialize(**table, arena(context), context.scene);
        join_mode = 1;
    } else {
        vertex_weld::restore(**table, arena(context), context.scene);
    }
}
void update_rod(Context &context, const TaskWorkHandle &) {
    const auto view = player_initialization::bind_primary_view(context.task);
    auto work = collision(context, context.task->primary_work);
    if (work->info.size() < 2)
        missing("native_big_rod_collision_info");
    auto &display = context.rendering;
    if (!display.push_current_matrix())
        missing("native_big_rod_matrix_capacity");
    if (view.rotation[2] != 0)
        display.rotate_current_z(view.rotation[2]);
    if (view.rotation[0] != 0)
        display.rotate_current_x(view.rotation[0]);
    if (view.rotation[1] != kHalfTurn)
        display.rotate_current_y(view.rotation[1]);
    auto &first = work->info[0].center;
    first = display.calculate_current_vector(first);
    const auto add = [&display](float left, float right) {
        return fidelity::ExactFloat(display.arithmetic(), left) + right;
    };
    first.x = add(first.x, view.position.x).rounded();
    first.y = add(first.y, view.position.y).rounded();
    first.z = add(first.z, view.position.z).rounded();
    auto &second = work->info[1].center;
    second = display.calculate_current_vector(second);
    second.x = add(second.x, first.x).rounded();
    second.y = add(second.y, first.y).rounded();
    second.z = add(second.z, first.z).rounded();
    display.pop_current_matrix();
}
void display(Context &, const TaskHandle &) { missing("native_big_display"); }
void destroy(Context &, const TaskHandle &) { missing("native_big_destruct"); }
void update_held_task(Context &, const TaskWorkHandle &) { missing("native_big_held_task"); }
void character_shadow(Context &, Primary, const Player &) { missing("native_big_character_shadow"); }
void stand_tail(Context &, const TaskWorkHandle &) { missing("native_big_stand_tail"); }
character_pack::ObjectResource &equipment_object(Context &context, unsigned table_index) {
    if (table_index < 31 || table_index >= 37)
        missing("native_big_equipment_object_row");
    return context.scene.objects.object(assets(context).equipment_objects[table_index - 31]);
}
character_pack::ResourceId equipment_identity(Context &context, unsigned table_index) {
    if (table_index < 31 || table_index >= 37)
        missing("native_big_equipment_object_row");
    return assets(context).equipment_objects[table_index - 31];
}
character_pack::ResourceId equipment_model(Context &context, unsigned table_index) {
    if (table_index < 2 || table_index >= 4)
        missing("native_big_equipment_model_row");
    return assets(context).equipment_models[table_index - 2];
}
float &morph_amount(Context &context, const Player &player) {
    return extra(context, player).morph_amount;
}
bool check_morph(Context &, const TaskWorkHandle &) { missing("native_big_check_morph"); }
void morph_vertices(Context &, unsigned, float) { missing("native_big_morph_vertices"); }
TaskHandle create_child_task(Context &context, unsigned kind, std::int32_t level, std::uint8_t index) {
    TaskCallback callback = nullptr;
    std::uint32_t flags = 0;
    switch (kind) {
    case 0:
        flags = 2;
        callback = unrecovered_big_child_a;
        break;
    case 1:
        flags = 3;
        callback = unrecovered_big_child_b;
        break;
    case 2:
        flags = 2;
        callback = unrecovered_big_hud;
        break;
    default:
        missing("native_big_child_task_kind");
    }
    const auto created = context.scheduler.create_element(flags, level, callback);
    if (!created || !created.task)
        return {};
    store_child_index(context, created.task, index);
    return created.task;
}
TaskHandle create_lure_task(Context &, const TaskHandle &) { missing("native_big_lure_task"); }
} // namespace sadx::big_selection

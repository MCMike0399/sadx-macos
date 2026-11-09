#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "engine/collision_component.hpp"
#include "player/selection_player_queries.hpp"
#include "player/player_vertex_weld.hpp"
#include "player/gamma_input_buffer.hpp"
#include "player/gamma_fan.hpp"
#include "player/gamma_target_effects.hpp"
#include "game/game_mode.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::gamma_selection {
namespace {
[[noreturn]] void missing(const char *boundary) {
    throw flow::UnrecoveredGameFlowCall(boundary);
}
TaskWorkArena &arena(Context &context) {
    const auto owner = context.scheduler.work_arena();
    if (!owner) missing("native_gamma_task_arena");
    return *owner;
}
native::GammaActorAssets &assets(Context &context) {
    if (!context.players.gamma_assets) missing("native_gamma_actor_assets");
    return *context.players.gamma_assets;
}
PlayerPhysicsState &physics(const Player &player) {
    if (!player || !player->physics) missing("native_gamma_player_physics");
    return *player->physics;
}
// NOTSA: per-consumption access follows the actual primary allocation reference.
std::shared_ptr<collision_component::Work> collision(Context &context,
                                                     const TaskWorkHandle &primary_work) {
    auto component = collision_component::component(arena(context), primary_work);
    if (!component) missing("native_gamma_null_collision_component");
    return component;
}
// NOTSA: borrows the live mutable info record without a cached numeric copy.
collision_component::Info &collision_info(Context &context, const TaskWorkHandle &primary_work,
                                          unsigned record) {
    auto component = collision(context, primary_work);
    if (record >= component->info_count || record >= component->info.size())
        missing("native_gamma_collision_info_record");
    return component->info[record];
}
constexpr unsigned kMotionPlayerField = 0, kCollisionField = 56, kFreeWorkField = 124;
} // namespace
State &state(Context &context) { return context.players.gamma; }
CurrentView current(Context &context) {
    return {context.players.current.primary, context.players.current.player};
}
Primary primary(Context &context, const TaskWorkHandle &work) {
    if (work != context.task->primary_work) missing("native_gamma_primary_identity");
    return player_initialization::bind_primary_view(context.task);
}
Player player_for_motion(Context &context, const TaskWorkHandle &motion) {
    const auto reference = arena(context).reference(motion, kMotionPlayerField);
    if (!reference) missing("native_gamma_motion_player_reference");
    if (!reference.work) return {};
    const auto found = context.players.registry.allocations.find(reference.work.get());
    if (found == context.players.registry.allocations.end())
        missing("native_gamma_player_allocation_identity");
    return found->second;
}
TaskWorkHandle player_allocation(Context &, const Player &player) {
    return player ? player->allocation : TaskWorkHandle{};
}
TaskWorkHandle extra_work(Context &context, const Player &player) {
    if (!player) missing("native_gamma_null_player_free_work");
    const auto reference = arena(context).reference(player->allocation, kFreeWorkField);
    if (!reference) missing("native_gamma_player_free_work_reference");
    return reference.work;
}
void store_extra_work(Context &context, const Player &player, const TaskWorkHandle &work) {
    if (!player || !arena(context).set_reference(player->allocation, kFreeWorkField, work))
        missing("native_gamma_player_free_work_store");
}
ExtraView extra(Context &context, const TaskWorkHandle &allocation) {
    if (!allocation || allocation->released()) missing("native_gamma_null_E102WK");
    for (auto &work : context.players.gamma_work)
        if (work.allocation == allocation)
            return {work.laser_mode, work.general_timer, work.ground_y, work.ground_distance,
                    work.ground_attributes, work.walk_distance};
    missing("native_gamma_E102WK_allocation_identity");
}
std::uint16_t &player_timer(Context &, const Player &player) {
    // The source field is the unsigned view of playerwk.wtimer.
    return player_initialization::bind_initialized_player(player).timer;
}
std::uint16_t &player_flags(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).flags;
}
std::uint16_t &requested_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).requested_animation;
}
std::uint16_t &current_animation(Context &, const Player &player) {
    return player_initialization::bind_initialized_player(player).current_animation;
}
float &eyes_height(Context &, const Player &player) {
    if (!player) missing("native_gamma_player_parameters");
    return player->parameters.eyes_height;
}
float player_height(Context &, const Player &player) {
    if (!player) missing("native_gamma_player_parameters");
    return player->parameters.height;
}
float forward_speed(Context &, const Player &player) { return physics(player).motion.speed.x; }
float vertical_speed(Context &context, const TaskWorkHandle &motion) {
    if (motion != context.task->motion_work) missing("native_gamma_motion_identity");
    return player_initialization::bind_existing_motion(context.task)->motion.speed.y;
}
bool has_held_task(Context &, const Player &player) {
    return bool(player_initialization::bind_initialized_player(player).held_task);
}
bool has_collision(Context &context, const TaskWorkHandle &primary_work) {
    const auto reference = arena(context).reference(primary_work, kCollisionField);
    if (!reference) missing("native_gamma_collision_reference");
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
    if (!task) missing("native_gamma_collision_task_backlink");
    return player_initialization::bind_primary_view(task).rotation.at(axis);
}
SceneMaterial &material(Context &context, unsigned table_row, unsigned material_row) {
    // E102 reads its material-pointer table directly, independently of objects.
    for (const auto &row : assets(context).materials) {
        if (row.table_row != table_row || row.material_index != material_row)
            continue;
        if (!row.materials) missing("native_gamma_shared_material_binding");
        return context.scene.materials.material(row.materials, row.record_index);
    }
    missing("native_gamma_shared_material_binding");
}
ColorPointer line_colors(Context &context) { return assets(context).line_colors.data(); }
TaskCallback display_callback(Context &context) {
    if (!context.display) missing("native_gamma_display_callback");
    return context.display;
}
TaskCallback destructor_callback(Context &context) {
    if (!context.destructor) missing("native_gamma_destructor_callback");
    return context.destructor;
}
fidelity::ExactFloat exact(Context &context, float value) { return {context.rendering.arithmetic(), value}; }
void initialize_player(Context &context, std::int32_t index, const TaskHandle &task) {
    player_initialization::NativeAccess access(assets(context).parameters,
                                              context.bootstrap.event_work, context.auxiliary);
    Player record;
    player_initialization::initialize(index, task,
        player_initialization::bind_existing_motion(task), context.scheduler,
        context.players.registry, access, context.rendering.arithmetic(), record);
}
void bind_animation_table(Context &context, const Player &player) {
    const auto &table = assets(context).animation_table;
    if (!table || table->records.size() != 91) missing("native_gamma_complete_animation_table");
    player_initialization::bind_initialized_player(player).animation_table = table;
}
void bind_weld_table(Context &context, const Player &player) {
    if (context.players.gamma_weld.rows.size() != 5)
        missing("native_gamma_imported_weld_table");
    player_initialization::bind_initialized_player(player).join_table = &context.players.gamma_weld;
}
[[noreturn]] void unrecovered_mode(Context &, std::uint8_t) {
    missing("native_gamma_unrecovered_mode");
}
std::int32_t event_cancel(Context &context) {
    return selection_player_queries::event_cancel(context.bootstrap.cutscene_task);
}
std::uint8_t is_time_zero(Context &context) {
    return selection_player_queries::is_time_zero(context.bootstrap.frames,
        context.bootstrap.seconds, context.bootstrap.minutes);
}
std::int16_t stage(Context &context) { return context.stage; }
std::uint16_t act(Context &context) { return context.act; }
// NOTSA: native CCL_Init boundary; source seam declared in the Gamma core.
void initialize_collision(Context &context, const TaskHandle &task, unsigned records,
                           std::uint8_t id) {
    auto primary = player_initialization::bind_primary_view(task);
    // Gamma ignores the source return. A failed allocation remains null and
    // faults at the subsequent first-extent access; do not turn failure into success.
    (void)collision_component::initialize(arena(context), task, primary.status,
        assets(context).collision_info, static_cast<std::int32_t>(records), id,
        context.rendering.arithmetic());
}
TaskWorkHandle allocate_zero(Context &context, unsigned count, unsigned bytes) {
    constexpr unsigned kWordAlignmentMask = 3;
    const auto extent = ((bytes + kWordAlignmentMask) & ~kWordAlignmentMask) * count;
    auto allocation = arena(context).allocate(extent);
    if (!allocation) return {};
    if (!arena(context).fill(allocation.work, 0, 0, extent))
        missing("native_gamma_CAlloc_zero_fill");
    constexpr unsigned kExtraWorkBytes = 580;
    if (count == 1 && bytes == kExtraWorkBytes)
        context.players.gamma_work.push_back({allocation.work, 0, 0, 0, 0, 0, 0, {}, {}});
    return allocation.work;
}
// NOTSA: native boundary for GammaUpdateLaserMode; source seam declared in the Gamma core.
void update_laser_mode(Context &, const TaskWorkHandle &, std::uint8_t, bool) {
    missing("native_gamma_update_laser_mode");
}
// NOTSA: native boundary for CheckEditMode; source seam declared in the Gamma core.
std::int32_t check_edit_mode(Context &context) {
    if (!context.editor_state)
        missing("native_gamma_check_edit_mode");
    return editor::check_mode(*context.editor_state);
}
// NOTSA: native boundary for GammaProcessAction; source seam declared in the Gamma core.
void process_action(Context &, const TaskHandle &, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_process_action");
}
// NOTSA: native boundary for GammaUpdateActionState; source seam declared in the Gamma core.
void update_action_state(Context &, Primary, const Player &) {
    missing("native_gamma_update_action_state");
}
// NOTSA: native boundary for LoadCharTextures; source seam declared in the Gamma core.
void load_character_textures(Context &context, std::uint8_t character) {
    if (!context.texture_state || !context.texture_services)
        missing("native_gamma_character_texture_owner");
    regular_textures::load_character(character, context.character_textures,
                                    *context.texture_state, *context.texture_services);
}
// NOTSA: native boundary for GammaInitializeCursors; source seam declared in the Gamma core.
void initialize_cursors(Context &context, const TaskWorkHandle &allocation) {
    if (!allocation || allocation->released()) missing("native_gamma_null_E102WK_cursors");
    for (auto &work : context.players.gamma_work) {
        if (work.allocation == allocation) {
            gamma_cursors::initialize(work.cursor1, work.cursor2,
                                      context.players.gamma_cursor_templates, arena(context));
            return;
        }
    }
    missing("native_gamma_E102WK_cursor_allocation_identity");
}
// NOTSA: native boundary for GammaLoadSetupResource; source seam declared in the Gamma core.
void load_setup_resource(Context &context) {
    // Keep the source boundary explicit when an older imported pack lacks setup content.
    if (!context.players.gamma_setup_asset)
        missing("native_gamma_load_setup_resource");
    gamma_setup::load_resource(context.players.gamma_setup, context.sprites, arena(context));
}
// NOTSA: native boundary for CreateChildTask; source seam declared in the Gamma core.
void create_time_hud(Context &, unsigned, const TaskHandle &) {
    missing("native_gamma_create_time_hud");
}
// NOTSA: native boundary for GammaInitializeEffects; source seam declared in the Gamma core.
void initialize_effects(Context &context) {
    if (!context.players.gamma_effects_asset)
        missing("native_gamma_effects_import");
    // NOTSA: the source scheduler exists here; callbacks borrow this live FP owner.
    const auto existing = std::dynamic_pointer_cast<particles::Binding>(
        context.scheduler.callback_context(particles::update_task));
    if (existing) {
        if (&existing->arithmetic != &context.rendering.arithmetic())
            missing("native_particle_arithmetic_owner_conflict");
    } else if (!context.scheduler.register_callback_context(particles::update_task,
                   std::make_shared<particles::Binding>(context.rendering.arithmetic()))) {
        missing("native_particle_arithmetic_registration");
    }
    gamma_effects::initialize(context.players.gamma_effects, *context.players.gamma_effects_asset,
        context.scheduler, arena(context), context.rendering.arithmetic());
}
// NOTSA: native boundary for PSetMotion; source seam declared in the Gamma core.
void set_motion(Context &context, const Player &player) {
    if (!context.players.gamma_motion_assets)
        missing("native_gamma_set_motion");
    const auto result = native::update_gamma_motion({physics(player), player_allocation(context, player)},
        arena(context), *context.players.gamma_motion_assets, context.scene.objects,
        context.rendering.arithmetic());
    if (!result) {
        const auto boundary = "native_gamma_motion_phase_" +
            std::to_string(static_cast<unsigned>(result.phase)) + "_failure_" +
            std::to_string(static_cast<unsigned>(result.failure)) + ":" +
            result.unavailable_resource_reason;
        throw flow::UnrecoveredGameFlowCall(boundary.c_str());
    }
}
// NOTSA: native boundary for PJoinVertexes; source seam declared in the Gamma core.
void join_vertices(Context &context, Primary, const TaskWorkHandle &, const Player &player) {
    auto &join_mode = player_initialization::bind_join_vertex_mode(player);
    const auto mode = join_mode;
    if (mode == 1) {
        // Active processing reaches the real motion-matrix producer before it
        // consumes the join table, including when the table is empty.
        missing("native_gamma_join_vertices_CalcMMMatrix");
    }
    auto &table = physics(player).animation_tail.join_table;
    if (!table)
        missing("native_gamma_weld_table_field");
    if (!*table)
        missing("native_gamma_weld_null_table");
    if (mode == 0) {
        vertex_weld::initialize(**table, arena(context), context.scene);
        join_mode = 1;
    } else {
        vertex_weld::restore(**table, arena(context), context.scene);
    }
}
// NOTSA: input_buf remains in the actual E102WK arena allocation. No typed
// mirror is introduced; later buffer producers must write this same storage.
void update_laser_targets(Context &context, const TaskWorkHandle &work, std::uint8_t index) {
    struct InputAccess final : gamma_input_buffer::Access {
        Context &context;
        const TaskWorkHandle &work;
        InputAccess(Context &context, const TaskWorkHandle &work) : context(context), work(work) {}
        std::int32_t buffered_angle() override {
            constexpr unsigned kLatestAngle = 556;
            std::int32_t value;
            if (!arena(context).read(work, kLatestAngle, &value, sizeof(value)))
                missing("native_gamma_buffered_input_angle");
            return value;
        }
        NativeAnalogInput &destination(std::int32_t player) {
            if (!context.normalized_input)
                missing("native_gamma_normalized_input_owner");
            if (player < 0 || static_cast<unsigned>(player) >= context.normalized_input->size())
                missing("native_gamma_normalized_input_slot");
            return (*context.normalized_input)[static_cast<unsigned>(player)];
        }
        void store_angle(std::int32_t player, std::int32_t angle) override {
            destination(player).angle = angle;
        }
        std::uint32_t buffered_stroke_bits() override {
            constexpr unsigned kLatestStroke = 560;
            std::uint32_t bits;
            if (!arena(context).read(work, kLatestStroke, &bits, sizeof(bits)))
                missing("native_gamma_buffered_input_stroke");
            return bits;
        }
        void store_stroke_bits(std::int32_t player, std::uint32_t bits) override {
            std::memcpy(&destination(player).magnitude, &bits, sizeof(bits));
        }
    } access(context, work);
    gamma_input_buffer::restore(access, index);
}
// NOTSA: native boundary for GetMMMatrix; source seam declared in the Gamma core.
void get_model_matrix(Context &context, unsigned node, NinjaMatrix &destination) {
    if (!context.model_matrices)
        missing("native_gamma_get_model_matrix");
    (void)model_matrices::get(*context.model_matrices, node, destination);
}
// NOTSA: native boundary for njRotateZ; source seam declared in the Gamma core.
void rotate_z(Context &context, NinjaMatrix &matrix, std::int32_t angle) {
    context.rendering.rotate_matrix_z(matrix, angle);
}
// NOTSA: the fan borrows the passed initialized player and existing extra allocation.
void update_fan(Context &context, Primary, const Player &player, const TaskWorkHandle &work) {
    struct FanAccess final : gamma_fan::Access {
        Context &context;
        const Player player;
        const TaskWorkHandle work;
        FanAccess(Context &context, const Player &player, const TaskWorkHandle &work)
            : context(context), player(player), work(work) {}
        float lateral_speed() override { return physics(player).motion.speed.z; }
        float forward_speed() override { return physics(player).motion.speed.x; }
        std::uint32_t phase() override {
            constexpr unsigned kFanPhase = 28;
            std::uint32_t value;
            if (!arena(context).read(work, kFanPhase, &value, sizeof(value)))
                missing("native_gamma_fan_phase_read");
            return value;
        }
        void store_phase(std::uint32_t value) override {
            constexpr unsigned kFanPhase = 28;
            if (!arena(context).write(work, kFanPhase, &value, sizeof(value)))
                missing("native_gamma_fan_phase_store");
        }
        fidelity::ExactFloat sine(std::int32_t angle) override {
            return fidelity::ExactFloat::retained(context.rendering.arithmetic(),
                                                  context.rendering.sine(angle));
        }
        void store_height(float value) override {
            constexpr unsigned kFanHeight = 24;
            if (!arena(context).write(work, kFanHeight, &value, sizeof(value)))
                missing("native_gamma_fan_height_store");
        }
        void store_tilt(std::int32_t value) override {
            constexpr unsigned kFanTilt = 40;
            if (!arena(context).write(work, kFanTilt, &value, sizeof(value)))
                missing("native_gamma_fan_tilt_store");
        }
    } access(context, player, work);
    gamma_fan::update(access, context.rendering.arithmetic());
}
// NOTSA: native boundary for E102Destruct; source seam declared in the Gamma core.
void destroy(Context &, const TaskHandle &) {
    missing("native_gamma_destroy");
}
void unrecovered_search_task(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_gamma_search_task");
}
void unrecovered_aim_task(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_gamma_aim_task");
}
template <class T> T load_work(TaskWorkArena &memory, const TaskWorkHandle &work, std::uint32_t at) {
    T value{};
    if (!memory.read(work, at, &value, sizeof(value)))
        missing("native_gamma_target_effects_read");
    return value;
}
template <class T>
void store_work(TaskWorkArena &memory, const TaskWorkHandle &work, std::uint32_t at, T value) {
    if (!memory.write(work, at, &value, sizeof(value)))
        missing("native_gamma_target_effects_store");
}
// NOTSA: native boundary for GammaUpdateTargetEffects; source seam declared in the Gamma core.
void update_target_effects(Context &context, Primary task, const Player &,
                           const TaskWorkHandle &allocation) {
    native::GammaExtraWork *owned = nullptr;
    for (auto &candidate : context.players.gamma_work)
        if (candidate.allocation == allocation)
            owned = &candidate;
    if (!owned)
        missing("native_gamma_E102WK_allocation_identity");
    auto &memory = arena(context);
    gamma_target_effects::LaserTarget target;
    target.mode = static_cast<std::int8_t>(owned->laser_mode);
    target.gun_mode = static_cast<std::int8_t>(load_work<std::uint8_t>(memory, allocation, 1));
    target.lock_count = static_cast<std::int8_t>(load_work<std::uint8_t>(memory, allocation, 4));
    target.lock_combo = load_work<std::int16_t>(memory, allocation, 0x0a);
    target.shot_combo = load_work<std::int16_t>(memory, allocation, 0x0c);
    target.laser_timer = load_work<std::int16_t>(memory, allocation, 0x10);
    target.lock_timer = load_work<std::int16_t>(memory, allocation, 0x12);
    target.position_x = load_work<float>(memory, allocation, 0x34);
    target.position_y = load_work<float>(memory, allocation, 0x38);
    target.position_z = load_work<float>(memory, allocation, 0x3c);
    target.aim_x = load_work<float>(memory, allocation, 0x40);
    target.aim_y = load_work<float>(memory, allocation, 0x44);
    target.aim_z = load_work<float>(memory, allocation, 0x48);
    target.distance = load_work<float>(memory, allocation, 0x4c);
    target.scope = load_work<float>(memory, allocation, 0x84);
    target.search = owned->search_task ? reinterpret_cast<std::uintptr_t>(owned->search_task.get()) : 0;
    struct Product final : gamma_target_effects::Services {
        Context &context;
        native::GammaExtraWork &owned;
        TaskWorkHandle block;
        Product(Context &context, native::GammaExtraWork &owned) : context(context), owned(owned) {}
        std::uint8_t controller_slot() override { return 0; }
        std::uint32_t button_mask() override { return 0; }
        std::uint32_t held(std::uint8_t) override { return 0; }
        std::uint32_t pressed(std::uint8_t) override { return 0; }
        bool event_sounds_enabled() override { return state(context).event_enabled != 0; }
        std::uintptr_t create_search_task() override {
            const auto created = context.scheduler.create_element(0, 6, unrecovered_search_task);
            if (!created) {
                owned.search_task.reset();
                return 0;
            }
            owned.search_task = created.task;
            return reinterpret_cast<std::uintptr_t>(created.task.get());
        }
        std::uintptr_t allocate_search_block() override {
            block = allocate_zero(context, 1, 16);
            return block ? reinterpret_cast<std::uintptr_t>(block.get()) : 0;
        }
        void attach_search_block(std::uintptr_t, std::uintptr_t) override {
            if (!owned.search_task || !block)
                missing("native_gamma_target_effects_search_link");
            owned.search_task->auxiliary_work = block;
            if (!context.task || !context.task->primary_work)
                missing("native_gamma_target_effects_task_work");
            if (!arena(context).set_reference(block, 4, context.task->primary_work) ||
                !arena(context).set_reference(block, 8, owned.allocation))
                missing("native_gamma_target_effects_search_link");
        }
        void destroy_task(std::uintptr_t id) override {
            const auto task = id == reinterpret_cast<std::uintptr_t>(owned.search_task.get())
                                  ? owned.search_task
                              : id == reinterpret_cast<std::uintptr_t>(owned.aim_task.get())
                                  ? owned.aim_task
                                  : TaskHandle{};
            (void)context.scheduler.destroy_task(task);
        }
        void play_lock_sound() override {
            if (!context.play_effect)
                missing("native_gamma_target_effects_lock_sound");
            context.play_effect(0x343);
        }
        void stop_lock_sound() override { missing("native_gamma_stop_lock_sound"); }
        std::uintptr_t create_aim_task() override {
            const auto created = context.scheduler.create_element(2, 2, unrecovered_aim_task);
            if (!created || !created.task->primary_work) {
                owned.aim_task.reset();
                return 0;
            }
            owned.aim_task = created.task;
            return reinterpret_cast<std::uintptr_t>(created.task.get());
        }
        void store_aim(std::uintptr_t, const gamma_target_effects::AimPose &pose) override {
            if (!owned.aim_task || !owned.aim_task->primary_work)
                missing("native_gamma_target_effects_aim");
            const auto work = owned.aim_task->primary_work;
            auto &memory = arena(context);
            store_work(memory, work, 0x20, pose.x);
            store_work(memory, work, 0x24, pose.y);
            store_work(memory, work, 0x28, pose.z);
            store_work(memory, work, 0x18, pose.yaw);
            store_work(memory, work, 0x1c, pose.pitch);
            if (!memory.set_reference(work, 0x10, owned.allocation))
                missing("native_gamma_target_effects_aim");
        }
        std::int32_t read_lock_table(std::int8_t) override {
            missing("native_gamma_target_lock_input");
        }
        std::uintptr_t locked_subject(int) override { missing("native_gamma_target_release_lock"); }
        void release_locked_target(std::uintptr_t) override { missing("native_gamma_target_release_lock"); }
    } services(context, *owned);
    // The slot and button words are filled below; the overrides above are replaced
    // once the controller sample is known. A missing controller is the live boundary.
    struct Connected final : gamma_target_effects::Services {
        Product &product;
        Context &context;
        explicit Connected(Product &product, Context &context) : product(product), context(context) {}
        Context::ControllerSample sample(std::uint8_t slot) {
            if (!context.controller)
                missing("native_gamma_target_effects_controller");
            return context.controller(slot);
        }
        std::uint8_t controller_slot() override { return task_slot; }
        std::uint32_t button_mask() override { return sample(task_slot).mask; }
        std::uint32_t held(std::uint8_t slot) override { return sample(slot).held; }
        std::uint32_t pressed(std::uint8_t slot) override { return sample(slot).pressed; }
        bool event_sounds_enabled() override { return product.event_sounds_enabled(); }
        std::uintptr_t create_search_task() override { return product.create_search_task(); }
        std::uintptr_t allocate_search_block() override { return product.allocate_search_block(); }
        void attach_search_block(std::uintptr_t task, std::uintptr_t block) override {
            product.attach_search_block(task, block);
        }
        void destroy_task(std::uintptr_t task) override { product.destroy_task(task); }
        void play_lock_sound() override { product.play_lock_sound(); }
        void stop_lock_sound() override { product.stop_lock_sound(); }
        std::uintptr_t create_aim_task() override { return product.create_aim_task(); }
        void store_aim(std::uintptr_t task, const gamma_target_effects::AimPose &pose) override {
            product.store_aim(task, pose);
        }
        std::int32_t read_lock_table(std::int8_t mode) override { return product.read_lock_table(mode); }
        std::uintptr_t locked_subject(int index) override { return product.locked_subject(index); }
        void release_locked_target(std::uintptr_t subject) override {
            product.release_locked_target(subject);
        }
        std::uint8_t task_slot = 0;
    } connected(services, context);
    connected.task_slot = task.player_index;
    gamma_target_effects::update(target, static_cast<std::int8_t>(task.mode), connected,
                                 context.rendering.arithmetic());
    owned->laser_mode = static_cast<std::uint8_t>(target.mode);
    store_work(memory, allocation, 0, static_cast<std::uint8_t>(target.mode));
    store_work(memory, allocation, 1, static_cast<std::uint8_t>(target.gun_mode));
    store_work(memory, allocation, 4, static_cast<std::uint8_t>(target.lock_count));
    store_work(memory, allocation, 0x0a, target.lock_combo);
    store_work(memory, allocation, 0x0c, target.shot_combo);
    store_work(memory, allocation, 0x10, target.laser_timer);
    store_work(memory, allocation, 0x12, target.lock_timer);
    store_work(memory, allocation, 0x34, target.position_x);
    store_work(memory, allocation, 0x38, target.position_y);
    store_work(memory, allocation, 0x3c, target.position_z);
    store_work(memory, allocation, 0x40, target.aim_x);
    store_work(memory, allocation, 0x44, target.aim_y);
    store_work(memory, allocation, 0x48, target.aim_z);
    store_work(memory, allocation, 0x4c, target.distance);
    store_work(memory, allocation, 0x84, target.scope);
    if (target.search == 0)
        owned->search_task.reset();
}
// NOTSA: native boundary for GammaUpdateGunEffects; source seam declared in the Gamma core.
void update_gun_effects(Context &, const TaskHandle &, const Player &, const TaskWorkHandle &) {
    missing("native_gamma_update_gun_effects");
}
// NOTSA: native boundary for GammaUpdateHeldTask; source seam declared in the Gamma core.
void update_held_task(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_update_held_task");
}
// NOTSA: native boundary for Shadow; source seam declared in the Gamma core.
fidelity::ExactFloat shadow(Context &, Primary, float) {
    missing("native_gamma_shadow");
}
// NOTSA: native boundary for GammaGroundAttributes; source seam declared in the Gamma core.
std::uint32_t ground_attributes(Context &, Primary) {
    missing("native_gamma_ground_attributes");
}
// NOTSA: native boundary for GammaRunningEffects; source seam declared in the Gamma core.
void running_effects(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_running_effects");
}
// NOTSA: native boundary for CCL_CalcRange; the owner is collision_component::calculate_range.
void calculate_collision_range(Context &context, Primary) {
    if (!context.task)
        missing("native_gamma_calculate_collision_range");
    auto work = collision(context, context.task->primary_work);
    collision_component::calculate_range(work.get(), context.rendering.arithmetic());
}
// NOTSA: native boundary for EntryColliList; source seam declared in the Gamma core.
void enter_collision_list(Context &, Primary) {
    missing("native_gamma_enter_collision_list");
}
// NOTSA: native boundary for RunUnderwaterStuff; source seam declared in the Gamma core.
void underwater(Context &, Primary, const TaskWorkHandle &, const Player &) {
    missing("native_gamma_underwater");
}
// NOTSA: native boundary for LoopTaskC; the owner is TaskScheduler::run_children.
void run_children(Context &context, const TaskHandle &parent) {
    const auto result = context.scheduler.run_children(parent);
    if (result)
        return;
    // A child that throws keeps its own failure. A tagged dispatch failure is
    // recovered task behavior, not the old unrecovered seam.
    throw std::runtime_error("native gamma child dispatch failed");
}
} // namespace sadx::gamma_selection

namespace sadx::regular_textures {
// NOTSA: native fail-loud binding for the unrecovered Amy-specific source seam.
void load_egg_robo_textures(State &, Services &) {
    throw flow::UnrecoveredGameFlowCall("native_LoadEggRoboTextures");
}
} // namespace sadx::regular_textures

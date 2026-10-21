#include "player/player_initialization.hpp"
#include <algorithm>
#include <cstring>

namespace sadx::player_initialization {
namespace {
// NOTSA: exact source calloc arguments over the existing native arena.
TaskWorkResult allocate_zero(TaskWorkArena &arena, std::uint32_t count, std::uint32_t size) {
    // This owner supplies only the actual constant call arguments. The source
    // CAlloc rounds the per-element size to four bytes, then multiplies.
    const auto bytes = ((size + 3) & ~std::uint32_t{3}) * count;
    auto result = arena.allocate(bytes);
    if (result)
        result = arena.fill(result.work, 0, 0, bytes);
    return result;
}
// NOTSA: the nullable contact allocation establishes these live zero values.
PlayerContactWork zero_contact() {
    PlayerContactWork result;
    const NinjaVector zero{0, 0, 0};
    result.direction_at_1c = zero;
    result.radius_at_0 = 0;
    result.center_at_4 = zero;
    result.displacement_at_10 = zero;
    result.angle_x_at_34 = 0;
    result.angle_z_at_38 = 0;
    result.contact_count_at_28 = 0;
    result.selected_contact_at_2a = 0;
    result.vector_at_3c = zero;
    result.vector_at_48 = zero;
    result.vector_at_54 = zero;
    result.vector_at_60 = zero;
    result.vector_at_6c = zero;
    result.vector_at_78 = zero;
    return result;
}
// NOTSA: the source calloc and explicit shadow stores share these live fields.
void store_animation_shadow(PlayerPhysicsState &player, float shadow) {
    player.animation_mode_at_120 = 0;
    player.shadow_scale_at_1fc = shadow;
    player.join_vertex_mode_at_122 = 0;
}
// NOTSA: fields established by the successful source player calloc.
void initialize_physics(PlayerPhysicsState &player) {
    player.floor_normal = NinjaVector{0, 0, 0};
    player.source_vector_at_44 = NinjaVector{0, 0, 0};
    player.prior_attributes = 0;
    player.path_distance = 0;
    player.held_task_at_68 = TaskHandle{};
    player.flags_at_10 = 0;
    player.no_control_time_at_a = 0;
    player.source_flags_at_6 = 0;
    player.timer_at_7e = 0;
    player.source_charge_at_0 = 0;
    player.jump_timer_at_8 = 0;
    player.idle_timer_at_e = 0;
    player.timer_at_80 = 0;
    player.timer_at_82 = 0;
    player.requested_animation_at_124 = 0;
    player.current_animation_at_126 = 0;
    player.previous_animation_at_128 = 0;
    // The actual 512-byte calloc includes these mtnjvwk fields. Keep them
    // unknown in generic state; only this source producer establishes zero/null.
    player.animation_tail.next_at_12a = 0;
    player.animation_tail.timer_at_12c = 0;
    player.animation_tail.flags_at_12e = 0;
    player.animation_tail.frame_at_130 = 0;
    player.animation_tail.start_at_134 = 0;
    player.animation_tail.table_at_140 = std::shared_ptr<const PlayerAnimationTable>{};
    player.animation_tail.buffer_at_148 = std::shared_ptr<PlayerAnimationAllocatedAction>{};
    player.moving_task_at_6c = TaskHandle{};
    player.cached_land_object_at_74 = SceneResourceReference{};
    player.equipment_at_4 = 0;
    player.island_number_at_12 = 0;
    store_animation_shadow(player, 0);
}
struct EventStore {
    std::uint16_t index, mask;
};
const EventStore sonic[] = {{141, 2}, {142, 1}, {159, 4}, {205, 8}, {219, 16}};
const EventStore tails[] = {{205, 8}, {219, 16}};
const EventStore knuckles[] = {{281, 64}, {282, 32}};
const EventStore amy[] = {{338, 256}, {350, 128}};
const EventStore gamma[] = {{393, 1024}, {394, 512}, {464, 4096}, {465, 2048}};
const EventStore big[] = {{464, 4096}, {465, 2048}};
// NOTSA: arena initialization provenance is serialized from typed imported
// content.
std::array<std::uint8_t, 132> parameter_bytes(const PlayerParameters &p) {
    std::array<std::uint8_t, 132> bytes{};
    std::memcpy(bytes.data(), &p.jump2_timer, 4);
    std::memcpy(bytes.data() + 4, &p.pos_error, 4);
    std::memcpy(bytes.data() + 8, &p.lim_h_spd, 4);
    std::memcpy(bytes.data() + 12, &p.lim_v_spd, 4);
    std::memcpy(bytes.data() + 16, &p.max_x_spd, 4);
    std::memcpy(bytes.data() + 20, &p.max_psh_spd, 4);
    std::memcpy(bytes.data() + 24, &p.jmp_y_spd, 4);
    std::memcpy(bytes.data() + 28, &p.nocon_speed, 4);
    std::memcpy(bytes.data() + 32, &p.slide_speed, 4);
    std::memcpy(bytes.data() + 36, &p.jog_speed, 4);
    std::memcpy(bytes.data() + 40, &p.run_speed, 4);
    std::memcpy(bytes.data() + 44, &p.rush_speed, 4);
    std::memcpy(bytes.data() + 48, &p.crash_speed, 4);
    std::memcpy(bytes.data() + 52, &p.dash_speed, 4);
    std::memcpy(bytes.data() + 56, &p.jmp_addit, 4);
    std::memcpy(bytes.data() + 60, &p.run_accel, 4);
    std::memcpy(bytes.data() + 64, &p.air_accel, 4);
    std::memcpy(bytes.data() + 68, &p.slow_down, 4);
    std::memcpy(bytes.data() + 72, &p.run_break, 4);
    std::memcpy(bytes.data() + 76, &p.air_break, 4);
    std::memcpy(bytes.data() + 80, &p.air_resist_air, 4);
    std::memcpy(bytes.data() + 84, &p.air_resist, 4);
    std::memcpy(bytes.data() + 88, &p.air_resist_y, 4);
    std::memcpy(bytes.data() + 92, &p.air_resist_z, 4);
    std::memcpy(bytes.data() + 96, &p.grd_frict, 4);
    std::memcpy(bytes.data() + 100, &p.grd_frict_z, 4);
    std::memcpy(bytes.data() + 104, &p.lim_frict, 4);
    std::memcpy(bytes.data() + 108, &p.rat_bound, 4);
    std::memcpy(bytes.data() + 112, &p.rad, 4);
    std::memcpy(bytes.data() + 116, &p.height, 4);
    std::memcpy(bytes.data() + 120, &p.weight, 4);
    std::memcpy(bytes.data() + 124, &p.eyes_height, 4);
    std::memcpy(bytes.data() + 128, &p.center_height, 4);
    return bytes;
}
// NOTSA: ordered source resource-binding phase.
void bind_work(Player &record, PlayerPhysicsState &player, const TaskHandle &task,
               TaskWorkArena &memory) {
    if (!task->motion_work)
        throw Error(Boundary::MotionWork);
    const auto motion_bound = memory.set_reference(task->motion_work, 0, record.allocation);
    if (!motion_bound)
        throw Error(Boundary::MotionBinding, motion_bound.failure);
    record.stage = Stage::MotionBound;
    auto object = memory.reference(task->primary_work, 60);
    if (!object)
        throw Error(Boundary::ObjectReference, object.failure);
    if (!object.work) {
        object = allocate_zero(memory, 1, 124);
        const auto stored = memory.set_reference(task->primary_work, 60, object.work);
        if (!stored)
            throw Error(Boundary::ObjectStore, stored.failure);
        if (!object)
            throw Error(Boundary::ObjectInitialization, object.failure);
        const std::uint8_t mode = 3;
        const auto initialized = memory.write(object.work, 0, &mode, 1);
        if (!initialized)
            throw Error(Boundary::ObjectInitialization, initialized.failure);
    }
    record.stage = Stage::ObjectBound;
    auto contact = allocate_zero(memory, 1, 132);
    record.contact = contact.work;
    const auto contact_stored = memory.set_reference(record.allocation, 100, record.contact);
    if (!contact_stored)
        throw Error(Boundary::ContactBinding, contact_stored.failure);
    if (contact.work)
        player.contact_work = zero_contact(); // a failed nullable allocation stays null
    record.stage = Stage::ContactBound;
}

// NOTSA: the four actual global registration stores, in source order.
void register_player(std::int32_t index, const TaskHandle &task,
                     const std::shared_ptr<PlayerTaskState> &primary_owner, Player &record,
                     const PlayerHandle &output, Registry &registry, Access &access) {
    if (index < 0 || static_cast<std::uint32_t>(index) >= registry.tasks.size())
        throw Error(Boundary::RegistrationSlot);
    const auto slot = static_cast<std::size_t>(index);
    registry.tasks[slot] = task;
    record.stage = Stage::RegistrationTask;
    registry.primary[slot] = primary_owner;
    record.stage = Stage::RegistrationPrimary;
    registry.motion[slot] = access.live_motion(task, record.motion);
    registry.players[slot] = output;
    record.stage = Stage::Registered;
}

// NOTSA: exact source shadow and live speed/self alias stores.
void bind_aliases(Player &record, PlayerPhysicsState &player, TaskWorkArena &memory,
                  X87Context &fp) {
    const auto shadow = player_initial_shadow_scale(record.parameters.rad, fp);
    store_animation_shadow(player, shadow);
    const auto speed_alias =
        memory.set_reference_view(record.allocation, 312, record.allocation, 56);
    if (!speed_alias)
        throw Error(Boundary::SpeedAlias, speed_alias.failure);
    const auto self_alias = memory.set_reference(record.allocation, 316, record.allocation);
    if (!self_alias)
        throw Error(Boundary::SelfAlias, self_alias.failure);
    // Typed speed_at_138/self_at_13c expose the same authoritative live state;
    // validated allocation aliases are identity facts, not raw vector reads.
    record.stage = Stage::ShadowStored;
}

// NOTSA: named equipment phase; reads save flags only when the source does.
void initialize_equipment(Player &record, PlayerPhysicsState &player, std::uint8_t character,
                          Access &access) {
    const EventStore *events = nullptr;
    std::size_t event_count = 0;
    switch (character) {
    case 0:
        events = sonic;
        event_count = 5;
        break;
    case 2:
        events = tails;
        event_count = 2;
        break;
    case 3:
        events = knuckles;
        event_count = 2;
        break;
    case 5:
        events = amy;
        event_count = 2;
        break;
    case 6:
        events = gamma;
        event_count = 4;
        break;
    case 7:
        events = big;
        event_count = 2;
        break;
    default:
        break;
    }
    record.stage = Stage::EventFlags;
    for (std::size_t i = 0; i < event_count; ++i) {
        if (access.event_flag(events[i].index))
            player.equipment_at_4 =
                static_cast<std::uint16_t>(*player.equipment_at_4 | events[i].mask);
    }
}

// NOTSA: source task-ID/work/scale/auxiliary-creation tail.
void initialize_auxiliary(std::int32_t index, const TaskHandle &task, Player &record,
                          PlayerPhysicsState &player, TaskWorkArena &memory,
                          TaskScheduler &scheduler, Access &access, X87Context &fp) {
    player.motion.speed = {0, 0, 0};
    record.stage = Stage::SpeedCleared;
    access.live_motion(task, record.motion)->friction_scale = 1;
    record.stage = Stage::TaskId;
    auto &id = access.task_id(task);
    if (id == 0)
        id = 1;
    auto extra = allocate_zero(memory, 15, 32);
    record.auxiliary = extra.work;
    const auto extra_stored = memory.set_reference(record.allocation, 120, record.auxiliary);
    if (!extra_stored)
        throw Error(Boundary::AuxiliaryBinding, extra_stored.failure);
    player.island_number_at_12 = 0;
    record.stage = Stage::AuxiliaryWork;
    access.store_scale(task, NinjaVector{1, 1, 1});
    record.stage = Stage::TaskScale;
    const auto callback = access.auxiliary_initializer();
    auto created = scheduler.create_element(2, 2, callback);
    record.auxiliary_task = created.task;
    if (!created) {
        Error error(Boundary::TaskCreation);
        error.task_failure = created;
        throw error;
    }
    record.stage = Stage::AuxiliaryTask;
    // The auxiliary callback uses a different taskwk union: +8 becomes a
    // table pointer in the auxiliary initializer, not player_index/character_id. Keep its work in
    // the generic native byte owner until its real shadow view is recovered.
    const float encoded_index = fp.to_float(fp.from_int64(index));
    const auto stored_index =
        memory.write(record.auxiliary_task->primary_work, 52, &encoded_index, 4);
    if (!stored_index)
        throw Error(Boundary::AuxiliaryIndex, stored_index.failure);
}
} // namespace
void initialize(std::int32_t index, const TaskHandle &task,
                std::shared_ptr<PlayerMotionState> motion, TaskScheduler &scheduler,
                Registry &registry, Access &access, X87Context &fp, PlayerHandle &output) {
    output = std::make_shared<Player>();
    auto &record = *output;
    record.task = task;
    record.motion = std::move(motion);
    if (!task)
        throw Error(Boundary::Task);
    const auto arena = scheduler.work_arena();
    if (!arena)
        throw Error(Boundary::Arena);
    auto allocated = allocate_zero(*arena, 1, 512);
    record.allocation = allocated.work;
    if (!allocated)
        throw Error(Boundary::PlayerAllocation,
                    allocated.failure); // source null destination copy
    record.stage = Stage::PlayerAllocated;
    record.physics = std::make_shared<PlayerPhysicsState>();
    auto &player = *record.physics;
    registry.allocations.emplace(record.allocation.get(), output);
    initialize_physics(player);
    if (!task->primary_work)
        throw Error(Boundary::Primary);
    const auto primary_owner = access.primary(task);
    auto &primary = *primary_owner;
    const auto &parameters = access.parameters(primary.character_id);
    const auto raw = parameter_bytes(parameters);
    const auto copied = arena->write(record.allocation, 156, raw.data(), raw.size());
    if (!copied)
        throw Error(Boundary::ParameterStore, copied.failure);
    record.parameters = parameters;
    record.stage = Stage::ParametersCopied;
    bind_work(record, player, task, *arena);
    register_player(index, task, primary_owner, record, output, registry, access);
    bind_aliases(record, player, *arena, fp);
    initialize_equipment(record, player, primary.character_id, access);
    initialize_auxiliary(index, task, record, player, *arena, scheduler, access, fp);
    record.stage = Stage::Complete;
    return;
}
} // namespace sadx::player_initialization

namespace sadx::player_initialization {
namespace {
// NOTSA: optional availability belongs only at this legacy-owner adapter
// boundary.
template <class T> T &required(std::optional<T> &value, Boundary boundary) {
    if (!value)
        throw Error(boundary);
    return *value;
}
std::shared_ptr<PlayerTaskState> required_primary(const TaskHandle &task) {
    if (!task)
        throw Error(Boundary::Task);
    if (!task->primary_work || !task->primary_state)
        throw Error(Boundary::Primary);
    return task->primary_state;
}
// NOTSA: share the late legacy-optional check without another mutable ID owner.
std::uint8_t &required_task_id(const TaskHandle &task) {
    return required(required_primary(task)->task_id_at_2, Boundary::TaskId);
}

} // namespace
PrimaryView bind_primary_view(const TaskHandle &task) {
    auto &primary = *required_primary(task);
    return {required(primary.action_at_0, Boundary::PrimaryMode),
            required_task_id(task),
            primary.player_index,
            primary.character_id,
            primary.orientation.flags,
            primary.orientation.angles,
            required(primary.position, Boundary::PrimaryPosition),
            required(primary.scale_at_2c, Boundary::PrimaryScale)};
}
void establish_zeroed_primary(TaskScheduler &scheduler, const TaskHandle &task) {
    if (!task)
        throw Error(Boundary::Task);
    if (!task->primary_work)
        throw Error(Boundary::Primary);
    if (task->auxiliary_effect_work)
        throw Error(Boundary::PrimaryConflict);
    if (task->primary_state)
        return;
    const auto arena = scheduler.work_arena();
    if (!arena)
        throw Error(Boundary::Arena);
    std::array<std::uint8_t, 64> bytes{};
    const auto read = arena->read(task->primary_work, 0, bytes.data(), bytes.size());
    if (!read)
        throw Error(Boundary::PrimaryNotZero, read.failure);
    if (!std::all_of(bytes.begin(), bytes.end(), [](auto value) { return value == 0; }))
        throw Error(Boundary::PrimaryNotZero);
    auto primary = std::make_shared<PlayerTaskState>();
    primary->orientation = {0, {0, 0, 0}};
    primary->player_index = 0;
    primary->character_id = 0;
    primary->position = NinjaVector{0, 0, 0};
    primary->path_resource = std::shared_ptr<const PlayerPathResource>{};
    primary->task_id_at_2 = 0;
    primary->scale_at_2c = NinjaVector{0, 0, 0};
    primary->action_at_0 = 0;
    primary->secondary_mode_at_1 = 0;
    primary->control_timer_at_6 = 0;
    primary->source_counter_at_c = 0;
    task->primary_state = std::move(primary);
}
void establish_zeroed_motion(TaskScheduler &scheduler, const TaskHandle &task) {
    if (!task)
        throw Error(Boundary::Task);
    if (!task->motion_work)
        throw Error(Boundary::MotionWork);
    if (task->motion_state)
        return;
    const auto arena = scheduler.work_arena();
    if (!arena)
        throw Error(Boundary::Arena);
    std::array<std::uint8_t, 64> bytes{};
    const auto read = arena->read(task->motion_work, 0, bytes.data(), bytes.size());
    if (!read)
        throw Error(Boundary::MotionNotZero, read.failure);
    if (!std::all_of(bytes.begin(), bytes.end(), [](auto value) { return value == 0; }))
        throw Error(Boundary::MotionNotZero);
    auto motion = std::make_shared<PlayerMotionState>();
    motion->motion = {{0, 0, 0}, {0, 0, 0}};
    motion->target_yaw = 0;
    motion->source_field_34 = 0;
    motion->source_field_38 = 0;
    motion->friction_scale = 0;
    motion->angle_x_at_1c = 0;
    motion->angle_z_at_24 = 0;
    task->motion_state = std::move(motion);
}
std::shared_ptr<PlayerMotionState> bind_existing_motion(const TaskHandle &task) {
    if (!task)
        throw Error(Boundary::Task);
    if (!task->motion_work || !task->motion_state)
        throw Error(Boundary::MotionState);
    return task->motion_state;
}
std::shared_ptr<PlayerTaskState> NativeAccess::primary(const TaskHandle &task) {
    return required_primary(task);
}
const PlayerParameters &NativeAccess::parameters(std::uint8_t character) {
    if (character >= table.size())
        throw Error(Boundary::ParameterRow);
    return table[character];
}
bool NativeAccess::event_flag(std::uint16_t index) { return events.at(index) != 0; }
std::shared_ptr<PlayerMotionState>
NativeAccess::live_motion(const TaskHandle &task,
                          const std::shared_ptr<PlayerMotionState> &candidate) {
    const auto canonical = bind_existing_motion(task);
    if (candidate != canonical)
        throw Error(Boundary::MotionState);
    return canonical;
}
std::uint8_t &NativeAccess::task_id(const TaskHandle &task) { return required_task_id(task); }
void NativeAccess::store_scale(const TaskHandle &task, NinjaVector value) {
    required_primary(task)->scale_at_2c = value;
}
TaskCallback NativeAccess::auxiliary_initializer() {
    if (!auxiliary)
        throw Error(Boundary::AuxiliaryCallback);
    return auxiliary;
}
} // namespace sadx::player_initialization

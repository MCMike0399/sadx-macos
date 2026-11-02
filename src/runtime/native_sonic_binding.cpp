#include "player/ground_acceleration.hpp"
#include "player/sonic_player_internal.hpp"
#include "player/sonic_player_seams.hpp"
#include "runtime/native_sonic_context.hpp"
#include "runtime/native_sonic_fields.hpp"

namespace sadx::sonic {
namespace {
[[noreturn]] void missing(const char *name) { throw Unavailable(name); }
template <class T> T &require(std::optional<T> &value, const char *name) {
    if (!value)
        missing(name);
    return *value;
}
template <class T> const T &require(const std::optional<T> &value, const char *name) {
    if (!value)
        missing(name);
    return *value;
}
// NOTSA: distinguish an observed null link from a released native owner.
template <class T> std::shared_ptr<T> lock_link(const std::weak_ptr<T> &link, const char *name) {
    auto value = link.lock();
    const std::weak_ptr<T> empty;
    if (!value && (link.owner_before(empty) || empty.owner_before(link)))
        missing(name);
    return value;
}
struct Binding {
    using Result = Reply;
    // NOTSA: a source helper borrows its dependency owner once, while separate
    // later helper calls may see a runtime binding replaced by a callback.
    Motion *captured_motion = nullptr;
    bool motion_pinned = false;
    Motion *motion_dependencies() { return motion_pinned ? captured_motion : context.motion; }
    struct MotionScope {
        Binding &binding;
        Motion *previous;
        bool pinned;
        explicit MotionScope(Binding &binding)
            : binding(binding), previous(binding.captured_motion), pinned(binding.motion_pinned) {
            binding.captured_motion = binding.motion_dependencies();
            binding.motion_pinned = true;
        }
        MotionScope(const MotionScope &) = delete;
        ~MotionScope() {
            binding.captured_motion = previous;
            binding.motion_pinned = pinned;
        }
    };
    MotionScope capture_motion() { return MotionScope(*this); }

    using Record = player_initialization::PlayerHandle;
    using Player = PlayerBorrow<player_initialization::Player>;
    Context &context;
    // NOTSA: source locals retain the callback argument and captured work views.
    const TaskHandle object;
    TaskWorkHandle primary_allocation, motion_allocation;
    std::shared_ptr<PlayerTaskState> primary_state;
    std::shared_ptr<PlayerMotionState> motion_state;
    Record captured;
    bool resolved = false;
    explicit Binding(Context &context) : context(context), object(context.task) {}
    TaskWorkArena &arena() {
        auto value = context.scheduler.work_arena();
        if (!value)
            missing("sonic_task_arena");
        return *value;
    }
    PlayerTaskState &task() {
        if (!primary_state) {
            if (!object || !object->primary_state || !object->primary_work)
                missing("sonic_primary_state");
            primary_state = object->primary_state;
            primary_allocation = object->primary_work;
        }
        return *primary_state;
    }
    std::shared_ptr<PlayerMotionState> world() {
        if (!motion_state) {
            motion_state = player_initialization::bind_existing_motion(object);
            motion_allocation = object->motion_work;
        }
        return motion_state;
    }
    Record resolve() {
        (void)world();
        if (!motion_allocation)
            missing("sonic_motion_work");
        const auto ref = arena().reference_view(motion_allocation, 0);
        if (!ref || ref.byte_offset)
            missing("sonic_motion_player_reference");
        if (!ref.work)
            return {};
        if (ref.work->released())
            missing("sonic_released_player");
        const auto found = context.players.registry.allocations.find(ref.work.get());
        if (found == context.players.registry.allocations.end())
            missing("sonic_player_allocation");
        auto value = found->second;
        if (value && value->motion && value->motion != world())
            missing("sonic_conflicting_motion");
        return value;
    }
    Result done(std::optional<std::uint32_t> value = {}) { return {value, {}}; }
    Player player(Site) {
        if (!resolved) {
            captured = resolve();
            resolved = true;
        }
        return {captured.get(), captured ? captured->physics.get() : nullptr};
    }
    Player player_unchecked() {
        return {captured.get(), captured ? captured->physics.get() : nullptr};
    }
    Player require_player(Site site) {
        auto value = player(site);
        if (!value || !value.physics)
            missing("sonic_player_physics");
        return value;
    }
    fidelity::Context &fp() { return context.arithmetic; }
    TaskScheduler &scheduler() { return context.scheduler; }
    PlayerOrientation &orientation() { return task().orientation; }
    std::uint8_t index() { return task().player_index; }
    void require_task(Site) { (void)task(); }
    void require_world(Site) { (void)world(); }
    void require_speed_frame(Site site) {
        require_task(site);
        require_world(site);
    }
    void require_dispatch_frame(Site site) { require_speed_frame(site); }
    void require_motion(Site, Operation = Operation::SetMotion) {
        if (!motion_dependencies())
            missing("sonic_motion_dependencies");
    }
    void entry() {
        if (!object)
            missing("sonic_task");
        (void)task();
        (void)world();
        captured = resolve();
        resolved = true;
        actor_current::set_primary(context.players.current, primary_allocation);
        actor_current::set_player(context.players.current,
                                  captured ? captured->allocation : TaskWorkHandle{});
    }
    void publish_super(std::uint8_t value) {
        actor_current::set_super_sonic(context.players.current, value);
    }
    std::uint8_t mode(Site) { return require(native_fields::primary(task()).mode, "sonic_mode"); }
    std::uint8_t mode_unchecked() {
        return require(native_fields::primary(task()).mode, "sonic_mode");
    }
    void mode(std::uint8_t value) { native_fields::primary(task()).mode = value; }
    std::uint8_t submode(Site) {
        return require(native_fields::primary(task()).submode, "sonic_submode");
    }
    std::uint8_t task_id(Site) {
        return require(native_fields::primary(task()).id, "sonic_task_id");
    }
    std::uint16_t control_timer(Site) {
        return require(native_fields::primary(task()).control_timer, "sonic_control_timer");
    }
    void control_timer(std::uint16_t value) {
        native_fields::primary(task()).control_timer = value;
    }
    std::uint8_t event_mode(Site) {
        if (!context.read_event_mode)
            missing("sonic_event_mode_binding");
        return context.read_event_mode();
    }
    void event_mode(std::uint8_t value) {
        if (!context.write_event_mode)
            missing("sonic_event_mode_store_binding");
        context.write_event_mode(value);
    }
    bool event_link(Site) {
        if (!context.read_event_link)
            missing("sonic_event_link_binding");
        return context.read_event_link();
    }
    std::uint16_t stage(Site) {
        if (!context.stage)
            missing("sonic_stage");
        return static_cast<std::uint16_t>(*context.stage);
    }
    std::uint8_t alternate(Site) {
        if (!context.alternate_model)
            missing("sonic_alternate_model");
        return *context.alternate_model;
    }
    std::int16_t editor(Site) {
        if (!context.read_editor)
            missing("sonic_check_edit_mode_editor");
        return context.read_editor();
    }
    std::int16_t forced(Site) {
        if (!context.read_forced)
            missing("sonic_check_edit_mode_forced");
        return context.read_forced();
    }
    PlayerParameters &parameters(const Player &p, Site) {
        if (!p || p.record->stage < player_initialization::Stage::ParametersCopied)
            missing("sonic_player_parameters");
        return p.record->parameters;
    }
    PlayerPhysicsState &physics(const Player &p) {
        if (!p || !p.physics)
            missing("sonic_player_physics");
        return *p.physics;
    }
    NinjaVector &speed(const Player &p) { return physics(p).motion.speed; }
    std::uint32_t attributes(const Player &p) { return physics(p).attributes; }
    std::uint16_t word(const Player &p, Word field, Site) {
        auto v = native_fields::player(physics(p));
        switch (field) {
#define U(field, name)                                                                             \
    case Word::field:                                                                              \
        return static_cast<std::uint16_t>(require(v.name, "sonic_" #field))
            U(Equipment, equipment);
            U(Flags, flags);
            U(JumpTimer, jump_timer);
            U(IdleTimer, idle_timer);
            U(Timer, timer);
            U(ReleaseTimer, release_timer);
            U(ReleasePhase, release_phase);
            U(RequestedAnimation, requested_animation);
            U(CurrentAnimation, current_animation);
            U(PreviousAnimation, previous_animation);
#undef U
        }
        missing("sonic_player_word");
    }
    void word(const Player &p, Word field, std::uint16_t value) {
        auto v = native_fields::player(physics(p));
        switch (field) {
#define U(field, name)                                                                             \
    case Word::field:                                                                              \
        v.name = value;                                                                            \
        return
#define I(field, name)                                                                             \
    case Word::field:                                                                              \
        v.name = static_cast<std::int16_t>(value);                                                 \
        return
            U(Equipment, equipment);
            U(Flags, flags);
            I(JumpTimer, jump_timer);
            U(IdleTimer, idle_timer);
            U(Timer, timer);
            I(ReleaseTimer, release_timer);
            I(ReleasePhase, release_phase);
            U(RequestedAnimation, requested_animation);
            U(CurrentAnimation, current_animation);
            U(PreviousAnimation, previous_animation);
#undef U
#undef I
        }
    }
    float charge(const Player &p, Site) {
        return require(native_fields::player(physics(p)).charge, "sonic_charge");
    }
    void charge(const Player &p, float value) { native_fields::player(physics(p)).charge = value; }
    TaskHandle moving_task(const Player &p, Site) {
        return require(native_fields::player(physics(p)).moving, "sonic_moving_task");
    }
    void held_task(const Player &p, TaskHandle value) {
        native_fields::player(physics(p)).held = std::move(value);
    }
    const NinjaVector &contact_direction(const Player &p, Site) {
        auto &contact = require(physics(p).contact_work, "sonic_contact_work");
        return require(native_fields::contact_direction(contact), "sonic_contact_direction");
    }
    const NinjaVector &floor_normal(const Player &p, Site) {
        return require(physics(p).floor_normal, "sonic_floor_normal");
    }
    std::uint32_t prior_attributes(const Player &p, Site) {
        return require(physics(p).prior_attributes, "sonic_previous_attributes");
    }
    void alignment(const Player &p, float value) { physics(p).dotp = value; }
    NinjaVector &world_speed() { return world()->motion.speed; }
    std::int32_t &target_yaw() { return world()->target_yaw; }
    std::int32_t world_angle(unsigned axis, Site) {
        auto value = world();
        return require(native_fields::motion_angle(*value, axis), "sonic_motion_angle");
    }
    void world_angle(unsigned axis, std::int32_t value) {
        auto state = world();
        (native_fields::motion_angle(*state, axis)) = value;
    }
    const NinjaVector &world_gravity(Site) {
        require_motion(Site::GravityVector);
        return require(motion_dependencies()->acceleration.world_gravity, "sonic_world_gravity");
    }
    Player registered_player(std::uint8_t index, Site) {
        if (index >= context.players.registry.players.size())
            missing("sonic_registered_player_index");
        const auto &record = context.players.registry.players[index];
        return {record.get(), record ? record->physics.get() : nullptr};
    }
    NinjaVector &registered_speed(const Player &p, Site) { return speed(p); }
    NinjaVector moving_force(const TaskHandle &moving, Site) {
        NinjaVector vector;
        if (!arena().read(moving->force_work, 16, &vector, 12))
            missing("sonic_moving_force");
        return vector;
    }
    NinjaVector &registered_force(std::uint8_t index, Site, Site) {
        if (index >= context.players.registry.motion.size())
            missing("sonic_registered_motion_index");
        auto &motion = context.players.registry.motion[index];
        if (!motion)
            missing("sonic_registered_motion");
        return motion->motion.acceleration;
    }
    void require_collision_reference(Site) {
        if (!object || !primary_allocation)
            missing("sonic_collision_primary");
        auto ref = arena().reference(primary_allocation, 56);
        if (!ref)
            missing("sonic_collision_reference");
    }
    bool has_collision(Site site) {
        require_collision_reference(site);
        return static_cast<bool>(arena().reference(primary_allocation, 56).work);
    }
    std::shared_ptr<collision_component::Work> collision(Site site) {
        require_collision_reference(site);
        auto work = collision_component::component(arena(), primary_allocation);
        if (!work)
            missing("sonic_collision_component");
        return work;
    }
    using Collision = std::shared_ptr<collision_component::Work>;
    Collision infos(const Collision &work, Site) {
        if (!work)
            missing("sonic_collision_info");
        return work;
    }
    collision_component::Info *info(const Collision &work, std::int16_t index, Site) {
        if (index < 0 || static_cast<std::size_t>(index) >= work->info.size())
            missing("sonic_collision_info_index");
        return &work->info[index];
    }
    std::uint8_t damage(collision_component::Info *record, Site) { return record->damage; }
    void damage(collision_component::Info *record, std::uint8_t value) { record->damage = value; }
    std::uint32_t info_attributes(collision_component::Info *record, Site) {
        return record->attributes;
    }
    void info_attributes(collision_component::Info *record, std::uint32_t value) {
        record->attributes = value;
    }
    void center(collision_component::Info *record, NinjaVector value) { record->center = value; }
    NinjaVector &center(collision_component::Info *record) { return record->center; }
    std::uint16_t collision_flags(const Collision &work, Site) { return work->flags; }
    std::int16_t collision_index(const Collision &work, Site) { return work->own_number; }
    bool has_hit(const Collision &work, Site) {
        return static_cast<bool>(lock_link(work->other, "sonic_released_hit_collision"));
    }
    Collision required_hit(const Collision &work, Site, Site) {
        auto other = lock_link(work->other, "sonic_released_hit_collision");
        if (!other)
            missing("sonic_hit_collision");
        return other;
    }
    TaskHandle empty_actor() { return {}; }
    TaskHandle hit_actor(const Collision &work, Site read, Site task, Site primary) {
        return collision_actor(required_hit(work, read, task), read, task, primary);
    }
    TaskHandle collision_actor(const Collision &work, Site, Site, Site) {
        auto owner = lock_link(work->task, "sonic_released_collision_actor");
        if (!owner || !owner->primary_work || !owner->primary_state)
            missing("sonic_collision_actor");
        return owner;
    }
    PlayerTaskState &actor(const TaskHandle &owner) {
        if (!owner || !owner->primary_state)
            missing("sonic_actor_primary");
        return *owner->primary_state;
    }
    std::uint8_t actor_id(const TaskHandle &owner, Site) {
        return require(native_fields::primary(actor(owner)).id, "sonic_actor_id");
    }
    std::uint16_t actor_timer(const TaskHandle &owner, Site) {
        return require(native_fields::primary(actor(owner)).control_timer,
                       "sonic_actor_control_timer");
    }
    std::int32_t actor_counter(const TaskHandle &owner, Site) {
        return require(native_fields::primary(actor(owner)).counter, "sonic_actor_counter");
    }
    std::int32_t actor_angle(const TaskHandle &owner, unsigned axis, Site) {
        return actor(owner).orientation.angles[axis];
    }
    TaskHandle hit_task(const Collision &work, Site read, Site required, Site) {
        return lock_link(required_hit(work, read, required)->task,
                         "sonic_released_collision_actor");
    }
    NinjaVector position(Site) { return require(task().position, "sonic_position"); }
    NinjaVector position(const std::shared_ptr<const PlayerTaskState> &work, Site) {
        return require(work->position, "sonic_collision_position");
    }
    std::shared_ptr<const PlayerTaskState> collision_return(const Result &r, Site) {
        return r.task_work;
    }
    bool pad(bool held, Button button, Site, Site) {
        if (!context.button_masks || !context.pads)
            missing("sonic_controls");
        auto &[roll_mask, jump_mask, hold_mask, action_mask, pickup_mask] = *context.button_masks;
        const std::optional<std::uint32_t> *mask = nullptr;
        switch (button) {
        case Button::Roll:
            mask = &roll_mask;
            break;
        case Button::Jump:
            mask = &jump_mask;
            break;
        case Button::Hold:
            mask = &hold_mask;
            break;
        case Button::StageAction:
            mask = &action_mask;
            break;
        case Button::StagePickup:
            mask = &pickup_mask;
            break;
        }
        if (index() >= context.pads->records.size())
            missing("sonic_pad_index");
        const auto value = context.pads->records[index()].read_u32(
            held ? GameLogicController::held_offset : GameLogicController::pressed_offset);
        return (require(value, "sonic_pad_word") & require(*mask, "sonic_button_mask")) != 0;
    }
    Input input(Site site) {
        require_motion(site);
        std::int32_t angle = 0;
        const auto result = get_player_input(
            input_task_view(task()), motion_dependencies()->acceleration.input, fp(), &angle,
            nullptr, motion_dependencies()->acceleration.forced_angle);
        if (!result || !result.active)
            missing("sonic_get_player_input");
        return {*result.active, (result.outputs_written & 1) != 0, angle};
    }
    Result input_reply(bool active) { return done(active ? 1 : 0); }
    void require_angle(const Input &input, Site) {
        if (!input.angle_written)
            missing("sonic_input_angle");
    }
    // STUB: SONIC 0x43b9c0 PCheckHoldObject native collision/registry composition remains open
    Result hold(Site) { missing("sonic_get_hold"); }
    // STUB: SONIC 0x41bed0 FUN_0041bed0 native position composition remains open
    NinjaVector normal(Site) { missing("sonic_collision_normal"); }
    void attach_normal(Result &) {}
    double angle_units() {
        require_motion(Site::WallNormal);
        return motion_dependencies()->position_constants.angle_units;
    }
    double positive_factor() {
        require_motion(Site::WallNormal);
        return motion_dependencies()->position_constants.positive_factor;
    }
    std::uint8_t character(Site site) {
        require_motion(site);
        return require(motion_dependencies()->acceleration.input.current_character,
                       "sonic_current_character");
    }
    void require_value(const Result &r, Site, Operation = Operation::SetMotion) {
        if (!r.value)
            missing("sonic_callee_return");
    }
    void require_argument(const std::optional<std::uint32_t> &value, Site) {
        if (!value)
            missing("sonic_packed_action");
    }
    Result set_motion(Site site) {
        if (!context.animation)
            missing("sonic_set_motion");
        auto p = require_player(site);
        animation::View view{physics(p), p.record->allocation};
        const auto result = animation::update(view, *context.animation, fp());
        if (!result)
            missing("sonic_set_motion_failed");
        return done();
    }

    Result event_effect(Site) {
        calls::event_effect(context);
        return done();
    }
    Result upgrade_release(Site) { return done(calls::upgrade_release(context)); }
    Result play_sound(std::uint32_t id, Site) {
        calls::play_sound(context, id);
        return done();
    }
    Result play_attached_sound(std::uint32_t id, Site) {
        calls::play_attached_sound(context, id);
        return done();
    }
    Result stop_sound(std::uint32_t id, Site) {
        calls::stop_sound(context, id);
        return done();
    }
    Result drop_held(Site) {
        calls::drop_held(context);
        return done();
    }
    Result special_jump(Site) {
        calls::special_jump(context);
        return done();
    }
    Result special_jump_animation(Site) {
        calls::special_jump_animation(context);
        return done();
    }
    Result play_voice(Site) {
        calls::play_voice(context);
        return done();
    }
    Result wall_turn(Site) { return done(calls::wall_turn(context)); }
    Result notify_damage(std::uint8_t index, Site) {
        calls::notify_damage(context, index);
        return done();
    }
    Result stage_query(Site) { return done(calls::stage_query(context)); }
    Result stage_apply(std::uint8_t index, std::uint32_t value, Site) {
        calls::stage_apply(context, index, value);
        return done();
    }
    Result idle_animation(Site) {
        calls::idle_animation(context);
        return done();
    }
    Result bounce(Site) {
        calls::bounce(context);
        return done();
    }
    Result walk_animation(Site) {
        calls::walk_animation(context);
        return done();
    }
    Result find_collision(std::int32_t yaw, Site) {
        auto r = done();
        r.task_work = calls::find_collision(context, yaw);
        return r;
    }
    void event_camera(Site site) {
        calls::event_camera(context);
        (void)site;
    }
    [[noreturn]] void unrecovered(Site, Operation) { missing("sonic_unrecovered_body"); }
    [[noreturn]] void missing_model(Site) { missing("sonic_model_graph"); }
    void character_id(std::uint8_t value) { task().character_id = value; }
    void initialize(Site) {
        if (!context.parameters || !context.events)
            missing("sonic_initialization_inputs");
        player_initialization::NativeAccess access(*context.parameters, *context.events,
                                                   context.auxiliary);
        auto motion = world();
        try {
            player_initialization::initialize(index(), object, motion, scheduler(),
                                              context.players.registry, access, fp(), captured);
            resolved = true;
        } catch (...) {
            resolved = true;
            throw;
        }
    }
    void bind_animation(const Player &p, Site) {
        if (!context.animation_table || !*context.animation_table)
            missing("sonic_animation_table");
        native_fields::animation_table(physics(p).animation_tail) = *context.animation_table;
    }
    TaskCallback callback(Operation op, Site) {
        auto cb = op == Operation::SpinEffect      ? context.spin_effect
                  : op == Operation::ReleaseEffect ? context.release_effect
                                                   : context.release_destructor;
        if (!cb)
            missing(op == Operation::SpinEffect      ? "sonic_spin_effect"
                    : op == Operation::ReleaseEffect ? "sonic_release_effect"
                                                     : "sonic_release_destructor");
        return cb;
    }
    void check_creation(const TaskResult &result, Site) {
        if (!result && result.failure != TaskFailure::SourcePoolExhausted)
            missing("sonic_effect_creation");
    }
    void effect_counter(const TaskHandle &owner, std::int32_t value, Site) {
        if (!owner->primary_work)
            missing("sonic_effect_primary");
        if (owner->primary_state)
            native_fields::primary(*owner->primary_state).counter = value;
        else if (!arena().write(owner->primary_work, 12, &value, 4))
            missing("sonic_effect_counter");
    }
    void effect_primary(const TaskHandle &owner, Site) {
        if (!object || !primary_allocation ||
            !arena().set_reference(owner->auxiliary_work, 0, primary_allocation))
            missing("sonic_effect_primary_reference");
    }
    void effect_player(const TaskHandle &owner, const Player &p, Site) {
        if (!p.record->allocation ||
            !arena().set_reference(owner->auxiliary_work, 4, p.record->allocation))
            missing("sonic_effect_player_reference");
    }
    void effect_speed(const TaskHandle &owner, float value, Site) {
        if (!arena().write(owner->auxiliary_work, 8, &value, 4))
            missing("sonic_effect_saved_speed");
    }
    PlayerCoordinates &coordinates() {
        require_motion(Site::CopyTransform);
        return motion_dependencies()->coordinates;
    }
    void coordinate_result(const CoordinateResult &result, Site) {
        if (!result)
            missing("sonic_coordinate_transform");
    }
    void world_to_player(NinjaVector &vector, Site site) {
        coordinate_result(coordinates().world_to_player(orientation(), vector, fp()), site);
    }
    void task_to_world(NinjaVector &vector, Site site) {
        coordinate_result(coordinates().task_to_world(&orientation(), vector, fp()), site);
    }
    NormalAngles floor_angles(const NinjaVector &normal, Site site) {
        require_motion(site);
        auto result = floor_normal_angles(normal, motion_dependencies()->floor_constants,
                                          motion_dependencies()->inverse, fp());
        if (!result.angles || result.failure != CrtAsinFailure::none)
            missing("sonic_floor_angles");
        return *result.angles;
    }
    std::array<std::int16_t, 2> raw_axes(Site site) {
        require_motion(site);
        if (!motion_dependencies()->surface)
            missing("sonic_raw_axes");
        const auto &axes = motion_dependencies()->surface->raw_axes;
        if (index() >= axes.size())
            missing("sonic_raw_axes_index");
        return require(axes[index()], "sonic_raw_axes");
    }
    std::array<std::int32_t, 2> camera_angles(Site) {
        return require(motion_dependencies()->surface->camera_pitch_yaw, "sonic_camera_angles");
    }
    const CoordinateAxisConstants &axis() {
        if (!motion_dependencies()->axis)
            missing("sonic_coordinate_axis");
        return *motion_dependencies()->axis;
    }
    double surface_angle_units() { return motion_dependencies()->position_constants.angle_units; }
    double surface_positive_factor() {
        return motion_dependencies()->position_constants.positive_factor;
    }
    std::int32_t project_angle(NinjaVector first, NinjaVector second, const NinjaVector &normal,
                               Site) {
        if (!motion_dependencies()->projection)
            missing("sonic_projection_constants");
        auto result = projection_vector_angle(first, second, &normal, axis(),
                                              *motion_dependencies()->projection,
                                              motion_dependencies()->inverse, fp());
        if (!result)
            missing("sonic_projection_angle");
        return *result.angle;
    }
    void reset_ground_direction(const Player &, Site) { missing("sonic_reset_ground_direction"); }
    void passive_rotation(const Player &, Site) { missing("sonic_passive_rotation"); }
    void blended_rotation(const Player &, std::int32_t, Site) { missing("sonic_blended_rotation"); }
    void regular_rotation(const Player &, std::int32_t, Site) { missing("sonic_regular_rotation"); }
    void ground_acceleration(const Player &p, PlayerParameters &parameters, Site site) {
        require_motion(site);
        auto motion = world();
        auto &v = physics(p);
        physics::GroundMotion state{orientation(),
                                    motion->motion,
                                    v.motion,
                                    v.dotp,
                                    v.attributes,
                                    motion->target_yaw,
                                    motion->source_field_34,
                                    motion->source_field_38,
                                    motion->friction_scale};
        physics::GroundEnvironment environment{{}, {}, &coordinates(), {}};
        // Alternate owners stop before the ordinary branch consumes gravity.
        if (!(orientation().flags & detail::kDisableControl) &&
            !(v.attributes & (detail::kProjectedInput | detail::kAlternateFloor)))
            environment.gravity = world_gravity(site);
        environment.query_input = [&](fidelity::Context &) {
            std::int32_t angle = 0;
            float magnitude = 0;
            const auto result = get_player_input(
                input_task_view(task()), motion_dependencies()->acceleration.input, fp(), &angle,
                &magnitude, motion_dependencies()->acceleration.forced_angle);
            if (!result || !result.active)
                missing("sonic_ground_input");
            if (*result.active && (result.outputs_written & 3) != 3)
                missing("sonic_ground_input_outputs");
            // Contact is consumed after input, so callback mutations remain visible.
            environment.contact_direction = contact_direction(p, site);
            return physics::GroundInput{angle, magnitude, *result.active};
        };
        physics::calculate_ground_acceleration(state, parameters, environment, fp());
    }
    void air_acceleration(const Player &p, PlayerParameters &parameters, Site site) {
        require_motion(site);
        auto motion = world();
        const auto result = get_player_acceleration_air(
            task(), *motion, physics(p), parameters, motion_dependencies()->air_constants,
            motion_dependencies()->acceleration, motion_dependencies()->air_callees, fp());
        if (!result)
            missing("sonic_air_acceleration");
    }
    void gravity_acceleration(const Player &p, PlayerParameters &parameters, Site site) {
        const auto result = get_gravity(
            orientation(), world()->motion, physics(p).motion, world_gravity(site),
            parameters.weight,
            [&](const PlayerOrientation &orientation, NinjaVector &vector) {
                const auto result = coordinates().world_to_player(orientation, vector, fp());
                return PlayerMotionResult{
                    result ? PlayerMotionFailure::none
                           : PlayerMotionFailure::world_to_player_transform_unavailable};
            },
            fp());
        if (!result)
            missing("sonic_gravity_acceleration");
    }
    void calculate_speed(const Player &p, PlayerParameters &parameters, Site site) {
        require_motion(site);
        auto result = run_player_movement(task(), motion_dependencies()->acceleration.input,
                                          world()->motion, physics(p), parameters, coordinates(),
                                          motion_dependencies()->floor_constants,
                                          motion_dependencies()->inverse, fp());
        if (!result)
            missing("sonic_calculate_speed");
    }
    // NOTSA: PSetPosition is recovered against a reference resource scene; native
    // collision composition must be bound without creating a snapshot of that scene.
    std::uint32_t set_position(const Player &, PlayerParameters &, Site) {
        missing("sonic_set_player_position");
    }
    Site predicate_site(Predicate) { return Site::DispatchMode; }
    Site animation_site(Predicate, unsigned) { return Site::DispatchMode; }
    Site body_site(std::uint8_t) { return Site::DispatchMode; }
    void dispatching(std::uint8_t) {}
    Site character_sound_site(unsigned) { return Site::DispatchMode; }
    TaskHandle reset_task(unsigned index) {
        if (!context.reset_tasks)
            missing("sonic_reset_task_table");
        return (*context.reset_tasks)[index];
    }
    void clear_task_mode(const TaskHandle &owner, unsigned) {
        if (!owner->primary_work)
            missing("sonic_reset_task_primary");
        if (owner->primary_state)
            native_fields::primary(*owner->primary_state).mode = 0;
        else {
            std::uint8_t value = 0;
            if (!arena().write(owner->primary_work, 0, &value, 1))
                missing("sonic_reset_task_store");
        }
    }
};
} // namespace
void run(Context &context) {
    Binding binding(context);
    detail::run(binding);
}
Reply action_checks(Context &context) {
    Binding binding(context);
    return detail::action_checks(binding);
}
Reply action_predicate(Context &context, Predicate predicate,
                       std::optional<std::uint32_t> argument) {
    Binding binding(context);
    return detail::predicate(binding, predicate, argument);
}
Reply dispatch(Context &context) {
    Binding binding(context);
    return detail::dispatch(binding);
}
Reply ground_input(Context &context) {
    Binding binding(context);
    return detail::ground_input(binding);
}
Reply copy_local_speed(Context &context) {
    Binding binding(context);
    return detail::copy_local_speed(binding);
}
Reply update_collision_center(Context &context) {
    Binding binding(context);
    return detail::update_collision_center(binding);
}
} // namespace sadx::sonic

namespace sadx::sonic::calls {
void play_sound(Context &, std::uint32_t) { throw Unavailable("sonic_play_sound"); }
void play_attached_sound(Context &, std::uint32_t) {
    throw Unavailable("sonic_play_attached_sound");
}
void stop_sound(Context &, std::uint32_t) { throw Unavailable("sonic_stop_sound"); }
void event_effect(Context &) { throw Unavailable("sonic_event_effect"); }
std::uint32_t upgrade_release(Context &) { throw Unavailable("sonic_upgrade_release"); }
void event_camera(Context &) { throw Unavailable("sonic_event_camera"); }
void drop_held(Context &) { throw Unavailable("sonic_drop_held"); }
void special_jump(Context &) { throw Unavailable("sonic_special_jump"); }
void special_jump_animation(Context &) { throw Unavailable("sonic_special_jump_animation"); }
std::shared_ptr<const PlayerTaskState> find_collision(Context &, std::int32_t) {
    throw Unavailable("sonic_find_collision");
}
void play_voice(Context &) { throw Unavailable("sonic_play_voice"); }
std::uint32_t wall_turn(Context &) { throw Unavailable("sonic_wall_turn"); }
void notify_damage(Context &, std::uint8_t) { throw Unavailable("sonic_notify_damage"); }
std::uint32_t stage_query(Context &) { throw Unavailable("sonic_stage_query"); }
void stage_apply(Context &, std::uint8_t, std::uint32_t) { throw Unavailable("sonic_stage_apply"); }
void idle_animation(Context &) { throw Unavailable("sonic_idle_animation"); }
void bounce(Context &) { throw Unavailable("sonic_bounce"); }
void walk_animation(Context &) { throw Unavailable("sonic_walk_animation"); }
} // namespace sadx::sonic::calls

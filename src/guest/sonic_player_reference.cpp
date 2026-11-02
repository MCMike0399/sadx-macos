#include "guest/sonic_player_reference.hpp"
#include "guest/sonic_player_sites.hpp"
#include "player/player_hold.hpp"
#include "player/sonic_player_internal.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx {
namespace {
SonicPlayerResult fail(SonicPlayerFailure f, std::uint32_t b, std::uint32_t callee = 0) {
    SonicPlayerResult r;
    r.failure = f;
    r.source_boundary = b;
    r.callee_address = callee;
    return r;
}
SonicPlayerResult done(std::optional<std::uint32_t> value = {}) {
    SonicPlayerResult r;
    r.completed = true;
    r.value = value;
    return r;
}
PlayerStartupPlayer *player(SonicPlayerFrame &f) {
    return f.player && *f.player ? f.player->get() : nullptr;
}
SonicPlayerResult require_player(SonicPlayerFrame &f, std::uint32_t b) {
    if (!f.player)
        return fail(SonicPlayerFailure::unknown_player_binding, b);
    if (!player(f) || !player(f)->player)
        return fail(SonicPlayerFailure::null_required_resource, b);
    return done();
}
struct Stop {
    SonicPlayerResult result;
};
void check(SonicPlayerResult r) {
    if (!r)
        throw Stop{std::move(r)};
}
} // namespace
SonicPlayerFrameBinding bind_sonic_player_frame(const TaskHandle &object,
                                                const TaskWorkArena &arena,
                                                std::shared_ptr<PlayerMotionState> world,
                                                const PlayerStartupRegistry &players,
                                                PlayerPositionCollisionScene *collision) {
    SonicPlayerFrameBinding binding;
    auto stop = [&](SonicPlayerFailure f, std::uint32_t b) {
        binding.result = fail(f, b);
        return binding;
    };
    if (!object)
        return stop(SonicPlayerFailure::null_required_resource, 0x49a9b8);
    if (!object->motion_work)
        return stop(SonicPlayerFailure::null_required_resource, 0x49a9c0);
    const auto reference = arena.reference_view(object->motion_work, 0);
    if (!reference) {
        binding.result = fail(SonicPlayerFailure::work_binding_failed, 0x49a9c0);
        binding.result.work_failure = reference.failure;
        return binding;
    }
    if (reference.byte_offset) {
        binding.result = fail(SonicPlayerFailure::work_binding_failed, 0x49a9c0);
        binding.result.work_failure = TaskWorkFailure::InteriorReferenceRequiresView;
        return binding;
    }
    std::shared_ptr<PlayerStartupPlayer> player;
    if (reference.work) {
        player = players.lookup_player_work(reference.work);
        if (!player)
            return stop(SonicPlayerFailure::unresolved_resource, 0x49a9c0);
    }
    if (!world)
        return stop(SonicPlayerFailure::unknown_motion_binding, 0x49a9bd);
    if (player && player->motion && player->motion != world)
        return stop(SonicPlayerFailure::conflicting_live_binding, 0x49a9bd);
    if (!object->primary_state)
        return stop(SonicPlayerFailure::unknown_field, 0x49a9d0);
    if (!object->primary_work)
        return stop(SonicPlayerFailure::null_required_resource, 0x49a9d0);
    if (object->auxiliary_effect_work)
        return stop(SonicPlayerFailure::conflicting_live_binding, 0x49a9d0);
    binding.frame = SonicPlayerFrame{object, object->primary_state, std::move(world),
                                     std::move(player), collision};
    binding.result = done();
    return binding;
}
float SonicPlayerConstants::at(std::uint32_t a) const {
    for (std::size_t i = 0; i < addresses.size(); ++i)
        if (addresses[i] == a) {
            float v;
            std::memcpy(&v, &bits[i], 4);
            return v;
        }
    throw std::out_of_range("Sonic source constant");
}
std::optional<SonicPlayerConstants> decode_sonic_player_constants(const std::uint8_t *b,
                                                                  std::size_t n) {
    if (!b || n != 52)
        return {};
    SonicPlayerConstants c;
    for (std::size_t i = 0; i < 13; ++i)
        c.bits[i] = std::uint32_t(b[4 * i]) | (std::uint32_t(b[4 * i + 1]) << 8) |
                    (std::uint32_t(b[4 * i + 2]) << 16) | (std::uint32_t(b[4 * i + 3]) << 24);
    return c;
}
std::optional<std::uint32_t> sonic_action_target(std::uint8_t action) {
    static constexpr std::uint32_t table[] = {
        0x49ab30, 0x49ad2d, 0x49ad69, 0x49ae04, 0x49ae2f, 0x49aebb, 0x49aec8, 0x49af4b, 0x49b324,
        0x49af71, 0x49af7e, 0x49af9f, 0x49b324, 0x49b083, 0x49afac, 0x49aff3, 0x49b072, 0x49b083,
        0x49b083, 0x49b0ab, 0x49be04, 0x49b11e, 0x49b11e, 0x49b24d, 0x49b262, 0x49b28c, 0x49b277,
        0x49b083, 0x49b2e2, 0x49b324, 0x49af71, 0x49af7e, 0x49b366, 0x49b3be, 0x49b3f6, 0x49b406,
        0x49b0c0, 0x49b0c0, 0x49b0de, 0x49b0de, 0x49b0fc, 0x49be04, 0x49b11e, 0x49b12b, 0x49b200,
        0x49be04, 0x49b438, 0x49b506, 0x49b6af, 0x49b5ca, 0x49b771, 0x49b7d1, 0x49b79a, 0x49b7e3,
        0x49b7f3, 0x49b828, 0x49b872, 0x49b816, 0x49b83e, 0x49b828, 0x49b866, 0x49b888, 0x49b8bf,
        0x49be04, 0x49b9b1, 0x49b9e1, 0x49b9e1, 0x49b9f6, 0x49b9b1, 0x49ba36, 0x49bb62, 0x49bc36,
        0x49b24d, 0x49b24d, 0x49b24d, 0x49b24d, 0x49bc43, 0x49bca6, 0x49bcde, 0x49af7e, 0x49af71,
        0x49bcf3, 0x49bcde, 0x49b083, 0x49bd53, 0x49bd61, 0x49bdb4, 0x49bdf0};
    static_assert(sizeof(table) / 4 == 88);
    if (action >= 88)
        return {};
    return table[action];
}
namespace sonic_reference {
using namespace sonic;
struct Context {
    using Result = SonicPlayerResult;
    // NOTSA: a source helper borrows its dependency owner once, while separate
    // later helper calls may see a runtime binding replaced by a callback.
    const SonicPlayerNativeMotion *captured_motion = nullptr;
    bool motion_pinned = false;
    const SonicPlayerNativeMotion *motion_dependencies() {
        return motion_pinned ? captured_motion : rt.motion;
    }
    struct MotionScope {
        Context &binding;
        const SonicPlayerNativeMotion *previous;
        bool pinned;
        explicit MotionScope(Context &binding)
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

    using Player = sonic::PlayerBorrow<PlayerStartupPlayer>;
    SonicPlayerFrame &f;
    SonicPlayerRuntime &rt;
    X87Context &arithmetic;
    std::optional<PlayerInputResult> last_input{};
    std::optional<PositionNormalResult> last_normal{};
    std::optional<PlayerPositionResult> movement_position{};
    std::optional<PlayerAnimationResult> animation_diagnostics{};
    std::optional<std::uint8_t> dispatched{};
    Context(SonicPlayerFrame &f, SonicPlayerRuntime &rt, X87Context &arithmetic)
        : f(f), rt(rt), arithmetic(arithmetic) {}
    [[noreturn]] void stop(SonicPlayerFailure failure, Site site, std::uint32_t callee = 0) {
        throw Stop{fail(failure, source_site(site), callee)};
    }
    Result done(std::optional<std::uint32_t> v = {}) { return sadx::done(v); }
    Result finish(Result r) {
        if (dispatched) {
            if (!r.position && movement_position)
                r.position = movement_position;
            if (!r.animation && animation_diagnostics)
                r.animation = animation_diagnostics;
            r.dispatched_action = dispatched;
            r.dispatch_target = sonic_action_target(*dispatched).value_or(0x49be04);
        }
        return r;
    }
    X87Context &fp() { return arithmetic; }
    TaskScheduler &scheduler() { return rt.scheduler; }
    PlayerOrientation &orientation() { return f.task->orientation; }
    std::uint8_t index() { return f.task->player_index; }
    void require_task(Site site) {
        if (!f.task)
            stop(SonicPlayerFailure::null_required_resource, site);
    }
    void require_world(Site site) {
        if (!f.world)
            stop(SonicPlayerFailure::null_required_resource, site);
    }
    void require_speed_frame(Site site) {
        if (!f.task || !f.world)
            stop(SonicPlayerFailure::null_required_resource, site);
    }
    void require_dispatch_frame(Site site) { require_speed_frame(site); }
    void require_motion(Site site, Operation op = Operation::SetMotion) {
        if (!motion_dependencies())
            stop(SonicPlayerFailure::missing_native_owner, site,
                 op == Operation::Speed ? 0x443f50 : 0);
    }
    Player player(Site site) {
        if (!f.player)
            stop(SonicPlayerFailure::unknown_player_binding, site);
        const auto &record = *f.player;
        return {record.get(), record ? record->player.get() : nullptr};
    }
    Player player_unchecked() {
        const auto &record = *f.player;
        return {record.get(), record ? record->player.get() : nullptr};
    }
    Player require_player(Site site) {
        check(sadx::require_player(f, source_site(site)));
        return player_unchecked();
    }
    void entry() {
        if (!f.object)
            stop(SonicPlayerFailure::null_required_resource, Site::EntryObject);
        if (!f.task)
            stop(SonicPlayerFailure::unknown_field, Site::EntryObject);
        if (!f.world)
            stop(SonicPlayerFailure::unknown_motion_binding, Site::EntryWorld);
        if (!f.player)
            stop(SonicPlayerFailure::unknown_player_binding, Site::EntryPlayerBinding);
        actor_current::set_primary(rt.globals.current, f.object->primary_work,
                                   rt.globals.current_observation);
        actor_current::set_player(rt.globals.current,
                                  sadx::player(f) ? sadx::player(f)->player_work : TaskWorkHandle{},
                                  rt.globals.current_observation);
    }
    void publish_super(std::uint8_t value) {
        actor_current::set_super_sonic(rt.globals.current, value, rt.globals.current_observation);
    }
    std::uint8_t mode(Site site) {
        if (!f.task->action_at_0)
            stop(SonicPlayerFailure::unknown_field, site);
        return *f.task->action_at_0;
    }
    std::uint8_t mode_unchecked() { return *f.task->action_at_0; }
    void mode(std::uint8_t value) { f.task->action_at_0 = value; }
    std::uint8_t submode(Site site) {
        if (!f.task->secondary_mode_at_1)
            stop(SonicPlayerFailure::unknown_field, site);
        return *f.task->secondary_mode_at_1;
    }
    std::uint8_t task_id(Site site) {
        if (!f.task->task_id_at_2)
            stop(SonicPlayerFailure::unknown_field, site);
        return *f.task->task_id_at_2;
    }
    std::uint16_t control_timer(Site site) {
        if (!f.task->control_timer_at_6)
            stop(SonicPlayerFailure::unknown_field, site);
        return *f.task->control_timer_at_6;
    }
    void control_timer(std::uint16_t value) { f.task->control_timer_at_6 = value; }
    std::uint8_t event_mode(Site site) {
        if (!f.task->event_move || !f.task->event_move->mode)
            stop(SonicPlayerFailure::unknown_field, site);
        return *f.task->event_move->mode;
    }
    void event_mode(std::uint8_t value) { f.task->event_move->mode = value; }
    // The source tests eventwk+0x4c. The old field name is retained ABI, not proof
    // that this is eventwk.action.list (New.h places that separate link at +0x40).
    bool event_link(Site site) {
        if (!f.task->event_move->action_list_at_4c)
            stop(SonicPlayerFailure::unknown_field, site);
        return f.task->event_move->action_list_at_4c->id.has_value();
    }
    std::uint16_t stage(Site site) {
        if (!rt.globals.stage_at_3b22dcc)
            stop(SonicPlayerFailure::unknown_field, site);
        return *rt.globals.stage_at_3b22dcc;
    }
    std::uint8_t alternate(Site site) {
        if (!rt.globals.alternate_model_at_3b18db5)
            stop(SonicPlayerFailure::unknown_field, site);
        return *rt.globals.alternate_model_at_3b18db5;
    }
    std::int16_t editor(Site site) {
        if (!rt.globals.forced_at_3b29d0c)
            stop(SonicPlayerFailure::unknown_field, site);
        return *rt.globals.forced_at_3b29d0c;
    }
    std::int16_t forced(Site site) {
        if (!rt.globals.forced_at_3b29d08 || !*rt.globals.forced_at_3b29d08)
            stop(SonicPlayerFailure::unknown_field, site);
        return static_cast<std::int16_t>(**rt.globals.forced_at_3b29d08);
    }
    PlayerParameters &parameters(const Player &, Site site) {
        // The legacy frame contract reloads this owner after intervening calls;
        // the physics reference captured by the algorithm remains independent.
        const auto current = sadx::player(f);
        if (!current || !current->parameters)
            stop(SonicPlayerFailure::unknown_field, site);
        return *current->parameters;
    }
    NinjaVector &speed(const Player &p) { return p.physics->motion.speed; }
    std::uint32_t attributes(const Player &p) { return p.physics->attributes; }
    std::uint16_t word(const Player &p, Word field, Site site) {
        if (!p.physics)
            stop(SonicPlayerFailure::null_required_resource, site);
        auto &v = *p.physics;
        switch (field) {
#define U(field, name)                                                                             \
    case Word::field:                                                                              \
        if (!v.name)                                                                               \
            stop(SonicPlayerFailure::unknown_field, site);                                         \
        return static_cast<std::uint16_t>(*v.name)
            U(Equipment, equipment_at_4);
            U(Flags, source_flags_at_6);
            U(JumpTimer, jump_timer_at_8);
            U(IdleTimer, idle_timer_at_e);
            U(Timer, timer_at_7e);
            U(ReleaseTimer, timer_at_80);
            U(ReleasePhase, timer_at_82);
            U(RequestedAnimation, requested_animation_at_124);
            U(CurrentAnimation, current_animation_at_126);
            U(PreviousAnimation, previous_animation_at_128);
#undef U
        }
        throw std::logic_error("Sonic player word");
    }
    void word(const Player &p, Word field, std::uint16_t value) {
        auto &v = *p.physics;
        switch (field) {
#define U(field, name)                                                                             \
    case Word::field:                                                                              \
        v.name = value;                                                                            \
        return
#define I(field, name)                                                                             \
    case Word::field:                                                                              \
        v.name = static_cast<std::int16_t>(value);                                                 \
        return
            U(Equipment, equipment_at_4);
            U(Flags, source_flags_at_6);
            I(JumpTimer, jump_timer_at_8);
            U(IdleTimer, idle_timer_at_e);
            U(Timer, timer_at_7e);
            I(ReleaseTimer, timer_at_80);
            I(ReleasePhase, timer_at_82);
            U(RequestedAnimation, requested_animation_at_124);
            U(CurrentAnimation, current_animation_at_126);
            U(PreviousAnimation, previous_animation_at_128);
#undef U
#undef I
        }
    }
    float charge(const Player &p, Site site) {
        if (!p.physics->source_charge_at_0)
            stop(SonicPlayerFailure::unknown_field, site);
        return *p.physics->source_charge_at_0;
    }
    void charge(const Player &p, float value) { p.physics->source_charge_at_0 = value; }
    TaskHandle moving_task(const Player &p, Site site) {
        if (!p.physics->moving_task_at_6c)
            stop(SonicPlayerFailure::unknown_field, site);
        return *p.physics->moving_task_at_6c;
    }
    void held_task(const Player &p, TaskHandle task) {
        p.physics->held_task_at_68 = std::move(task);
    }
    const NinjaVector &contact_direction(const Player &p, Site site) {
        if (!p.physics->contact_work || !p.physics->contact_work->direction_at_1c)
            stop(SonicPlayerFailure::unknown_field, site);
        return *p.physics->contact_work->direction_at_1c;
    }
    const NinjaVector &floor_normal(const Player &p, Site site) {
        if (!p.physics->floor_normal)
            stop(SonicPlayerFailure::unknown_field, site);
        return *p.physics->floor_normal;
    }
    std::uint32_t prior_attributes(const Player &p, Site site) {
        if (!p.physics->prior_attributes)
            stop(SonicPlayerFailure::unknown_field, site);
        return *p.physics->prior_attributes;
    }
    void alignment(const Player &p, float value) { p.physics->dotp = value; }
    NinjaVector &world_speed() { return f.world->motion.speed; }
    std::int32_t &target_yaw() { return f.world->target_yaw; }
    std::int32_t world_angle(unsigned axis, Site site) {
        auto &value = axis == 0 ? f.world->angle_x_at_1c : f.world->angle_z_at_24;
        if (!value)
            stop(SonicPlayerFailure::unknown_field, site);
        return *value;
    }
    void world_angle(unsigned axis, std::int32_t value) {
        (axis == 0 ? f.world->angle_x_at_1c : f.world->angle_z_at_24) = value;
    }
    const NinjaVector &world_gravity(Site site) {
        if (!motion_dependencies()->acceleration_runtime.world_gravity)
            stop(SonicPlayerFailure::unknown_field, site);
        return *motion_dependencies()->acceleration_runtime.world_gravity;
    }
    Player registered_player(std::uint8_t index, Site site) {
        if (index >= rt.players.player_work.size() || !rt.players.player_work[index])
            stop(SonicPlayerFailure::unknown_player_binding, site);
        const auto &record = *rt.players.player_work[index];
        return {record.get(), record ? record->player.get() : nullptr};
    }
    NinjaVector &registered_speed(const Player &p, Site site) {
        if (!p.physics)
            stop(SonicPlayerFailure::unknown_player_binding, site);
        return p.physics->motion.speed;
    }
    NinjaVector moving_force(const TaskHandle &moving, Site site) {
        auto arena = rt.scheduler.work_arena();
        if (!arena)
            stop(SonicPlayerFailure::missing_native_owner, site);
        NinjaVector vector;
        const auto result = arena->read(moving->force_work, 16, &vector, 12);
        if (!result) {
            auto r = fail(SonicPlayerFailure::work_binding_failed, source_site(site));
            r.work_failure = result.failure;
            throw Stop{r};
        }
        return vector;
    }
    NinjaVector &registered_force(std::uint8_t index, Site read, Site required) {
        if (index >= rt.players.motion_work.size() || !rt.players.motion_work[index])
            stop(SonicPlayerFailure::unknown_motion_binding, read);
        auto world = *rt.players.motion_work[index];
        if (!world)
            stop(SonicPlayerFailure::null_required_resource, required);
        return world->motion.acceleration;
    }
    void require_collision_reference(Site site) {
        if (!f.collision || !f.collision->task_collision)
            stop(SonicPlayerFailure::unknown_field, site);
    }
    bool has_collision(Site site) {
        require_collision_reference(site);
        return f.collision->task_collision->id.has_value();
    }
    PositionCollisionWork *collision(Site site) {
        require_collision_reference(site);
        auto ref = *f.collision->task_collision;
        if (!ref.id)
            stop(SonicPlayerFailure::null_required_resource, site);
        auto it = f.collision->works.find(*ref.id);
        if (it == f.collision->works.end())
            stop(SonicPlayerFailure::unresolved_resource, site);
        return &it->second;
    }
    PositionInfoTable *infos(PositionCollisionWork *work, Site site) {
        if (!work->info)
            stop(SonicPlayerFailure::unknown_field, site);
        if (!work->info->id)
            stop(SonicPlayerFailure::null_required_resource, site);
        auto it = f.collision->info_tables.find(*work->info->id);
        if (it == f.collision->info_tables.end())
            stop(SonicPlayerFailure::unresolved_resource, site);
        return &it->second;
    }
    PositionCollisionInfo *info(PositionInfoTable *table, std::int16_t index, Site site) {
        auto it = table->records.find(index);
        if (it == table->records.end())
            stop(SonicPlayerFailure::unresolved_resource, site);
        return &it->second;
    }
    std::uint8_t damage(PositionCollisionInfo *record, Site site) {
        if (!record->damage)
            stop(SonicPlayerFailure::unknown_field, site);
        return *record->damage;
    }
    void damage(PositionCollisionInfo *record, std::uint8_t value) { record->damage = value; }
    std::uint32_t info_attributes(PositionCollisionInfo *record, Site site) {
        if (!record->attributes)
            stop(SonicPlayerFailure::unknown_field, site);
        return *record->attributes;
    }
    void info_attributes(PositionCollisionInfo *record, std::uint32_t value) {
        record->attributes = value;
    }
    void center(PositionCollisionInfo *record, NinjaVector value) { record->center = value; }
    NinjaVector &center(PositionCollisionInfo *record) { return *record->center; }
    std::uint16_t collision_flags(PositionCollisionWork *work, Site site) {
        if (!work->flags)
            stop(SonicPlayerFailure::unknown_field, site);
        return *work->flags;
    }
    std::int16_t collision_index(PositionCollisionWork *work, Site site) {
        if (!work->my_info_index)
            stop(SonicPlayerFailure::unknown_field, site);
        return *work->my_info_index;
    }
    bool has_hit(PositionCollisionWork *own, Site site) {
        if (!own->hit_work)
            stop(SonicPlayerFailure::unknown_field, site);
        return own->hit_work->id.has_value();
    }
    PositionCollisionWork *required_hit(PositionCollisionWork *own, Site read, Site required) {
        if (!has_hit(own, read))
            stop(SonicPlayerFailure::null_required_resource, required);
        auto it = f.collision->works.find(*own->hit_work->id);
        if (it == f.collision->works.end())
            stop(SonicPlayerFailure::unresolved_resource, required);
        return &it->second;
    }
    PositionHitActor *empty_actor() { return nullptr; }
    PositionHitActor *resolve_actor(PositionResourceRef ref, Site task_site, Site work_site) {
        if (!ref.id)
            stop(SonicPlayerFailure::null_required_resource, work_site);
        auto object = f.collision->tasks.find(*ref.id);
        if (object == f.collision->tasks.end())
            stop(SonicPlayerFailure::unresolved_resource, work_site);
        if (!object->second.work)
            stop(SonicPlayerFailure::unknown_field, work_site);
        if (!object->second.work->id)
            throw Stop{
                fail(SonicPlayerFailure::null_required_resource, source_site(work_site) + 3)};
        auto actor = f.collision->actors.find(*object->second.work->id);
        if (actor == f.collision->actors.end())
            throw Stop{fail(SonicPlayerFailure::unresolved_resource, source_site(work_site) + 3)};
        (void)task_site;
        return &actor->second;
    }
    PositionHitActor *hit_actor(PositionCollisionWork *own, Site hit, Site task, Site work) {
        auto other = required_hit(own, hit, task);
        if (!other->owning_task)
            stop(SonicPlayerFailure::unknown_field, task);
        return resolve_actor(*other->owning_task, task, work);
    }
    PositionHitActor *collision_actor(PositionCollisionWork *own, Site read, Site task, Site work) {
        if (!own->owning_task)
            stop(SonicPlayerFailure::unknown_field, read);
        if (!own->owning_task->id)
            stop(SonicPlayerFailure::null_required_resource, task);
        auto object = f.collision->tasks.find(*own->owning_task->id);
        if (object == f.collision->tasks.end())
            stop(SonicPlayerFailure::unresolved_resource, task);
        if (!object->second.work)
            stop(SonicPlayerFailure::unknown_field, task);
        if (!object->second.work->id)
            stop(SonicPlayerFailure::null_required_resource, work);
        auto actor = f.collision->actors.find(*object->second.work->id);
        if (actor == f.collision->actors.end())
            stop(SonicPlayerFailure::unresolved_resource, work);
        return &actor->second;
    }
    std::uint8_t actor_id(PositionHitActor *actor, Site site) {
        auto v = position_actor_task_id(*actor);
        if (!v)
            stop(SonicPlayerFailure::unknown_field, site);
        return *v;
    }
    std::uint16_t actor_timer(PositionHitActor *actor, Site site) {
        auto v = position_actor_control_timer(*actor);
        if (!v)
            stop(SonicPlayerFailure::unknown_field, site);
        return *v;
    }
    std::int32_t actor_counter(PositionHitActor *actor, Site site) {
        auto v = position_actor_source_counter(*actor);
        if (!v)
            stop(SonicPlayerFailure::unknown_field, site);
        return *v;
    }
    std::int32_t actor_angle(PositionHitActor *actor, unsigned axis, Site site) {
        auto v = position_actor_angle(*actor, axis);
        if (!v)
            stop(SonicPlayerFailure::unknown_field, site);
        return *v;
    }
    TaskHandle hit_task(PositionCollisionWork *own, Site hit, Site task, Site lookup) {
        auto other = required_hit(own, hit, task);
        if (!other->owning_task)
            stop(SonicPlayerFailure::unknown_field, task);
        if (!other->owning_task->id)
            return {};
        if (!rt.collision_tasks)
            stop(SonicPlayerFailure::unresolved_resource, lookup);
        auto object = rt.collision_tasks->find(*other->owning_task->id);
        if (object == rt.collision_tasks->end())
            stop(SonicPlayerFailure::unresolved_resource, lookup);
        return object->second;
    }
    NinjaVector position(Site site) {
        if (!f.task->position)
            stop(SonicPlayerFailure::unknown_field, site);
        return *f.task->position;
    }
    NinjaVector position(const std::shared_ptr<const PlayerTaskState> &task, Site site) {
        if (!task->position)
            stop(SonicPlayerFailure::unknown_field, site);
        return *task->position;
    }
    std::shared_ptr<const PlayerTaskState> collision_return(const Result &r, Site site) {
        if (!r.task_work_return)
            stop(SonicPlayerFailure::native_owner_failed, site, 0x41c0c0);
        return *r.task_work_return;
    }
    bool pad(bool held, Button button, Site mask_site, Site pad_site) {
        if (!f.task)
            stop(SonicPlayerFailure::null_required_resource, pad_site);
        if (!rt.controls || !rt.controls->masks)
            stop(SonicPlayerFailure::unknown_control, mask_site);
        auto &masks = *rt.controls->masks;
        const std::optional<std::uint32_t> *mask = nullptr;
        switch (button) {
        case Button::Roll:
            mask = &masks.mask_at_909f94;
            break;
        case Button::Jump:
            mask = &masks.mask_at_909f98;
            break;
        case Button::Hold:
            mask = &masks.mask_at_909fa4;
            break;
        case Button::StageAction:
            mask = &masks.mask_at_909fa8;
            break;
        case Button::StagePickup:
            mask = &masks.mask_at_909fac;
            break;
        }
        if (!*mask)
            stop(SonicPlayerFailure::unknown_control, mask_site);
        auto pads = rt.controls->engine_pads;
        if (!pads || index() >= pads->records.size())
            stop(SonicPlayerFailure::unknown_control, pad_site);
        auto value = pads->records[index()].read_u32(held ? GameLogicController::held_offset
                                                          : GameLogicController::pressed_offset);
        if (!value)
            stop(SonicPlayerFailure::unknown_control, pad_site);
        return (*value & **mask) != 0;
    }
    Input input(Site site) {
        if (!motion_dependencies())
            stop(SonicPlayerFailure::missing_native_owner, site, 0x43bdc0);
        std::int32_t angle = 0;
        auto q = get_player_input(
            input_task_view(*f.task), motion_dependencies()->acceleration_runtime.input, fp(),
            &angle, nullptr, motion_dependencies()->acceleration_runtime.forced_angle);
        last_input = q;
        if (!q || !q.active) {
            auto r = fail(SonicPlayerFailure::input_failed, source_site(site), 0x43bdc0);
            r.input = q;
            if (site != Site::GroundInput)
                r.nested_source_boundary = q.source_boundary;
            throw Stop{r};
        }
        return {*q.active, (q.outputs_written & 1) != 0, angle};
    }
    Result input_reply(bool active) {
        auto r = done(active ? 1 : 0);
        r.input = last_input;
        return r;
    }
    void require_angle(const Input &input, Site site) {
        if (!input.angle_written) {
            auto r = fail(SonicPlayerFailure::input_failed, source_site(site), 0x43bdc0);
            if (site == Site::GroundAngle)
                r.input = last_input;
            throw Stop{r};
        }
    }
    Result hold(Site site) {
        if (!f.collision)
            stop(SonicPlayerFailure::unknown_field, site);
        PlayerHoldRuntime context{[&](std::uint8_t i) { return rt.players.lookup_player(i); },
                                  rt.scheduler.work_arena()};
        auto q = query_player_hold(*f.task, *f.collision, context);
        PositionHoldResult held{q ? PlayerPositionFailure::none
                                  : PlayerPositionFailure::hold_owner_failed,
                                q.flags, q.source_boundary, std::make_shared<PlayerHoldResult>(q)};
        if (!q || !q.flags) {
            auto r = fail(SonicPlayerFailure::native_owner_failed, source_site(site), 0x43b9c0);
            r.hold = held;
            r.nested_source_boundary = q.source_boundary;
            throw Stop{r};
        }
        auto r = done(*q.flags);
        r.hold = held;
        return r;
    }
    NinjaVector normal(Site site) {
        if (!motion_dependencies() || !motion_dependencies()->position_callees.normal)
            stop(SonicPlayerFailure::missing_native_owner, site, 0x41bed0);
        if (!f.collision)
            stop(SonicPlayerFailure::unknown_field, site);
        auto normal = motion_dependencies()->position_callees.normal(*f.task, *f.collision, fp());
        last_normal = normal;
        if (normal.failure != PlayerPositionFailure::none || !normal.normal) {
            auto r = fail(SonicPlayerFailure::native_owner_failed, source_site(site), 0x41bed0);
            r.normal = normal;
            r.nested_source_boundary = normal.source_boundary;
            throw Stop{r};
        }
        return *normal.normal;
    }
    void attach_normal(Result &r) { r.normal = last_normal; }
    double angle_units() { return motion_dependencies()->position_constants.angle_units; }
    double positive_factor() { return motion_dependencies()->position_constants.positive_factor; }
    std::uint8_t character(Site site) {
        if (!motion_dependencies() ||
            !motion_dependencies()->acceleration_runtime.input.current_character)
            stop(SonicPlayerFailure::unknown_field, site);
        return *motion_dependencies()->acceleration_runtime.input.current_character;
    }
    void require_value(const Result &r, Site site, Operation operation = Operation::SetMotion) {
        if (!r.value)
            stop(SonicPlayerFailure::native_owner_failed, site,
                 operation == Operation::SetMotion ? 0 : callee(operation));
    }
    void require_argument(const std::optional<std::uint32_t> &a, Site site) {
        if (!a)
            stop(SonicPlayerFailure::unknown_field, site);
    }
    static std::uint32_t callee(Operation op) {
        switch (op) {
        case Operation::WallForm:
            return 0x43bfe0;
        case Operation::SetMotion:
            return 0x44a800;
        case Operation::EventEffect:
            return 0x45a870;
        case Operation::UpgradeRelease:
            return 0x492710;
        case Operation::PlaySound:
            return 0x423d70;
        case Operation::PlayAttachedSound:
            return 0x423f50;
        case Operation::StopSound:
            return 0x424210;
        case Operation::EventCamera:
            return 0x437d20;
        case Operation::DropHeld:
            return 0x473b00;
        case Operation::SpecialJump:
            return 0x4b8470;
        case Operation::SpecialJumpAnimation:
            return 0x495c80;
        case Operation::FindCollision:
            return 0x41c0c0;
        case Operation::PlayVoice:
            return 0x423be0;
        case Operation::WallTurn:
            return 0x43c060;
        case Operation::NotifyDamage:
            return 0x4bcc10;
        case Operation::StageQuery:
            return 0x491550;
        case Operation::StageApply:
            return 0x442720;
        case Operation::IdleAnimation:
            return 0x491660;
        case Operation::Bounce:
            return 0x440790;
        case Operation::WalkAnimation:
            return 0x495cd0;
        case Operation::SpinEffect:
            return 0x495a50;
        case Operation::ReleaseEffect:
            return 0x4940e0;
        case Operation::ReleaseDestructor:
            return 0x494120;
        default:
            return 0;
        }
    }
    Result call(Operation op, Site site, std::array<std::optional<std::uint32_t>, 5> arguments = {},
                std::optional<std::uint8_t> task_argument = {}) {
        const auto target = callee(op), boundary = source_site(site);
        if (op == Operation::SetMotion) {
            if (!rt.animation_runtime && !rt.native_animation_runtime)
                stop(SonicPlayerFailure::missing_native_owner, site, target);
            auto p = require_player(site);
            PlayerAnimationView view{*p.physics, p.physics->animation_tail, p.record->player_work};
            auto a = rt.native_animation_runtime
                         ? update_player_animation(view, *rt.native_animation_runtime, fp())
                         : update_player_animation(view, *rt.animation_runtime, fp());
            auto r = a ? done() : fail(SonicPlayerFailure::animation_failed, boundary, target);
            r.animation = a;
            if (dispatched)
                animation_diagnostics = a;
            if (!a) {
                r.nested_source_boundary = a.source_boundary;
                throw Stop{r};
            }
            return r;
        }
        if (!rt.external)
            stop(SonicPlayerFailure::missing_native_owner, site, target);
        auto r = rt.external({target, boundary, arguments, task_argument}, f, fp());
        if (!r) {
            r.nested_source_boundary = r.source_boundary;
            r.source_boundary = boundary;
            r.callee_address = target;
            if (r.failure == SonicPlayerFailure::none)
                r.failure = SonicPlayerFailure::native_owner_failed;
            throw Stop{r};
        }
        return r;
    }
    Result set_motion(Site site) { return call(Operation::SetMotion, site); }
    Result event_effect(Site site) { return call(Operation::EventEffect, site); }
    Result upgrade_release(Site site) { return call(Operation::UpgradeRelease, site); }
    Result play_sound(std::uint32_t id, Site site) {
        return call(Operation::PlaySound, site, {id, 0, 0, 0});
    }
    Result play_attached_sound(std::uint32_t id, Site site) {
        return call(Operation::PlayAttachedSound, site, {id, {}, 1, 0, 2}, 1);
    }
    Result stop_sound(std::uint32_t id, Site site) {
        return call(Operation::StopSound, site, {id});
    }
    Result drop_held(Site site) { return call(Operation::DropHeld, site); }
    Result special_jump(Site site) { return call(Operation::SpecialJump, site); }
    Result special_jump_animation(Site site) { return call(Operation::SpecialJumpAnimation, site); }
    Result play_voice(Site site) { return call(Operation::PlayVoice, site, {1, 0x5d, 0, 0x1f}); }
    Result wall_turn(Site site) { return call(Operation::WallTurn, site); }
    Result notify_damage(std::uint8_t index, Site site) {
        return call(Operation::NotifyDamage, site, {index, 7, 0x3b, 6});
    }
    Result stage_query(Site site) { return call(Operation::StageQuery, site); }
    Result stage_apply(std::uint8_t index, std::uint32_t value, Site site) {
        return call(Operation::StageApply, site, {index, value, {}, {}});
    }
    Result idle_animation(Site site) { return call(Operation::IdleAnimation, site); }
    Result bounce(Site site) { return call(Operation::Bounce, site); }
    Result walk_animation(Site site) { return call(Operation::WalkAnimation, site); }
    Result find_collision(std::int32_t yaw, Site site) {
        return call(Operation::FindCollision, site,
                    {std::nullopt, 0x4000U - static_cast<std::uint32_t>(yaw), 0x2000, 0x92}, 0);
    }
    void event_camera(Site site) { call(Operation::EventCamera, site, {0x464df0, 0, 0}); }
    [[noreturn]] void unrecovered(Site site, Operation) {
        std::uint32_t target = 0;
        if (site == Site::RemainingChecks)
            target = 0x496f50;
        if (site == Site::DamageSuffix)
            target = 0x492d70;
        if (site == Site::EventRemaining)
            target = 0x495fa0;
        stop(SonicPlayerFailure::unimplemented_action, site, target);
    }
    [[noreturn]] void initialize_model(Site site) {
        stop(SonicPlayerFailure::unresolved_resource, site);
    }
    void character_id(std::uint8_t value) { f.task->character_id = value; }
    void initialize(Site site) {
        if (!rt.startup_runtime)
            stop(SonicPlayerFailure::missing_native_owner, site, 0x442750);
        auto startup = initialize_player_startup(index(), f.object, f.world, rt.scheduler,
                                                 rt.players, *rt.startup_runtime, fp());
        if (startup.partial)
            f.player = startup.partial;
        if (!startup) {
            auto r = fail(SonicPlayerFailure::startup_failed, source_site(site), 0x442750);
            r.startup = startup;
            throw Stop{r};
        }
    }
    void bind_animation(const Player &p, Site site) {
        if (!rt.initialized_animation_table)
            stop(SonicPlayerFailure::unresolved_resource, site);
        if (!*rt.initialized_animation_table)
            stop(SonicPlayerFailure::null_required_resource, site);
        p.physics->animation_tail.table_at_140 = *rt.initialized_animation_table;
    }
    TaskCallback callback(Operation op, Site site) {
        auto cb = op == Operation::SpinEffect      ? rt.spin_effect_callback_495a50
                  : op == Operation::ReleaseEffect ? rt.spin_release_effect_callback_4940e0
                                                   : rt.spin_release_destructor_494120;
        if (!cb)
            stop(SonicPlayerFailure::missing_native_owner, site, callee(op));
        return cb;
    }
    void check_creation(const TaskResult &result, Site site) {
        effect_result = result;
        if (!result && result.failure != TaskFailure::SourcePoolExhausted) {
            auto r = fail(SonicPlayerFailure::native_owner_failed, source_site(site), 0x40b860);
            r.task_creation = result;
            throw Stop{r};
        }
    }
    void effect_counter(const TaskHandle &task, std::int32_t value, Site site) {
        if (!task->primary_work)
            stop(SonicPlayerFailure::null_required_resource, site);
        if (task->primary_state) {
            task->primary_state->source_counter_at_c = value;
            return;
        }
        auto arena = rt.scheduler.work_arena();
        if (!arena)
            stop(SonicPlayerFailure::missing_native_owner, site);
        auto w = arena->write(task->primary_work, 0xc, &value, 4);
        if (!w) {
            auto r = fail(SonicPlayerFailure::work_binding_failed, source_site(site));
            r.work_failure = w.failure;
            throw Stop{r};
        }
    }
    // The source saves the actual task result only in these auxiliary-work failures.
    TaskResult effect_result{};
    void effect_primary(const TaskHandle &task, Site site) {
        auto arena = rt.scheduler.work_arena();
        if (!arena)
            stop(SonicPlayerFailure::missing_native_owner, site);
        if (!f.object || !f.object->primary_work)
            stop(SonicPlayerFailure::unknown_field, site);
        auto w = arena->set_reference(task->auxiliary_work, 0, f.object->primary_work);
        if (!w) {
            auto r = fail(SonicPlayerFailure::work_binding_failed, source_site(site));
            r.work_failure = w.failure;
            r.task_creation = effect_result;
            throw Stop{r};
        }
    }
    void effect_player(const TaskHandle &task, const Player &, Site site) {
        const auto current = sadx::player(f);
        if (!current || !current->player_work)
            stop(SonicPlayerFailure::unknown_field, site);
        const auto write =
            rt.scheduler.work_arena()->set_reference(task->auxiliary_work, 4, current->player_work);
        if (!write) {
            auto r = fail(SonicPlayerFailure::work_binding_failed, source_site(site));
            r.work_failure = write.failure;
            r.task_creation = effect_result;
            throw Stop{r};
        }
    }
    void effect_speed(const TaskHandle &task, float value, Site site) {
        auto w = rt.scheduler.work_arena()->write(task->auxiliary_work, 8, &value, 4);
        if (!w) {
            auto r = fail(SonicPlayerFailure::work_binding_failed, source_site(site));
            r.work_failure = w.failure;
            r.task_creation = effect_result;
            throw Stop{r};
        }
    }
    TaskHandle reset_task(unsigned index) {
        const auto &field = rt.globals.tasks_at_3c5a27c[index];
        if (!field)
            throw Stop{fail(SonicPlayerFailure::unknown_field, 0x4b8320 + 15 * index)};
        return *field;
    }
    void clear_task_mode(const TaskHandle &task, unsigned index) {
        const auto address = 0x4b8320 + 15 * index + 12;
        if (!task->primary_work)
            throw Stop{fail(SonicPlayerFailure::null_required_resource, address)};
        if (task->primary_state)
            task->primary_state->action_at_0 = 0;
        else {
            auto arena = rt.scheduler.work_arena();
            if (!arena)
                throw Stop{fail(SonicPlayerFailure::missing_native_owner, address)};
            std::uint8_t zero = 0;
            auto result = arena->write(task->primary_work, 0, &zero, 1);
            if (!result) {
                auto r = fail(SonicPlayerFailure::work_binding_failed, address);
                r.work_failure = result.failure;
                throw Stop{r};
            }
        }
    }
    Site predicate_site(Predicate p) {
        switch (p) {
        case Predicate::Event:
            return Site::PredicateEvent;
        case Predicate::RollRelease:
            return Site::PredicateRollRelease;
        case Predicate::Fall:
            return Site::PredicateFall;
        case Predicate::Jump:
            return Site::PredicateJump;
        case Predicate::Hold:
            return Site::PredicateHold;
        case Predicate::Charge:
            return Site::PredicateCharge;
        case Predicate::StagePickup:
            return Site::PredicateStagePickup;
        case Predicate::StageAction:
            return Site::PredicateStageAction;
        case Predicate::JumpPress:
            return Site::PredicateJumpPress;
        case Predicate::JumpClassify:
            return Site::PredicateJumpClassify;
        case Predicate::ClearTasks:
            return Site::PredicateClearTasks;
        case Predicate::Idle:
            return Site::PredicateIdle;
        case Predicate::TurnBrake:
            return Site::PredicateTurnBrake;
        case Predicate::SpinAnimation:
            return Site::PredicateSpinAnimation;
        case Predicate::JumpAnimation:
            return Site::PredicateJumpAnimation;
        case Predicate::WalkAnimation:
            return Site::PredicateWalkAnimation;
        case Predicate::WallForm:
            return Site::PredicateWallForm;
        case Predicate::SkidEnd:
            return Site::PredicateSkidEnd;
        case Predicate::ChargeTick:
            return Site::PredicateChargeTick;
        }
        throw std::logic_error("Sonic predicate");
    }
    Site animation_site(Predicate p, unsigned step) {
        if (p == Predicate::SpinAnimation) {
            static constexpr Site values[] = {Site::SpinAnimation0, Site::SpinAnimation1,
                                              Site::SpinAnimation2, Site::SpinAnimation3,
                                              Site::SpinAnimation4};
            return values[step];
        }
        if (p == Predicate::JumpAnimation) {
            static constexpr Site values[] = {Site::JumpAnimation0, Site::JumpAnimation1,
                                              Site::JumpAnimation2, Site::JumpAnimation3,
                                              Site::JumpAnimation4};
            return values[step];
        }
        if (p == Predicate::WalkAnimation) {
            static constexpr Site values[] = {Site::WalkAnimation0, Site::WalkAnimation1,
                                              Site::WalkAnimation2, Site::WalkAnimation3,
                                              Site::WalkAnimation4};
            return values[step];
        }
        throw std::logic_error("Sonic animation request");
    }
    Site body_site(std::uint8_t mode) {
        static constexpr Site values[] = {Site::BodyInitialize,
                                          Site::BodyStand,
                                          Site::BodyWalk,
                                          Site::BodyWalkPath,
                                          Site::BodySpin,
                                          Site::BodySpinDash,
                                          Site::BodyLightSpeedDash,
                                          Site::BodySpinPath,
                                          Site::BodyJump,
                                          Site::BodySpring,
                                          Site::BodyLaunch,
                                          Site::BodyParachute,
                                          Site::BodyFall,
                                          Site::BodyBrake,
                                          Site::BodySpinAttack,
                                          Site::BodyPush,
                                          Site::BodyDamage,
                                          Site::BodyBack,
                                          Site::BodyCrash,
                                          Site::BodySlide,
                                          Site::BodyPlaceOn,
                                          Site::BodyPick,
                                          Site::BodyPut,
                                          Site::BodyHold,
                                          Site::BodyTouch,
                                          Site::BodyHoldWalk,
                                          Site::BodyThrow,
                                          Site::BodyHoldBrake,
                                          Site::BodyHoldFall,
                                          Site::BodyHoldJump,
                                          Site::BodyHoldSpring,
                                          Site::BodyHoldLaunch,
                                          Site::BodyPull,
                                          Site::BodyThrough,
                                          Site::BodyShake,
                                          Site::BodyEscapeShake,
                                          Site::BodyHangWait,
                                          Site::BodyHangGo,
                                          Site::BodyLadderWait,
                                          Site::BodyLadderGo,
                                          Site::BodyPanelJump,
                                          Site::BodyPanelWait,
                                          Site::BodyCapture,
                                          Site::BodyIce,
                                          Site::BodyLightSpeedAttack,
                                          Site::BodyCart,
                                          Site::BodySandWalk,
                                          Site::BodySandSpinDash,
                                          Site::BodySandFall,
                                          Site::BodySandJump,
                                          Site::BodySandCrash,
                                          Site::BodySnowWait,
                                          Site::BodySnowSlide,
                                          Site::BodySnowRotateLeft,
                                          Site::BodySnowRotateRight,
                                          Site::BodyIceWait,
                                          Site::BodyIceSlip,
                                          Site::BodyIceRotateLeft,
                                          Site::BodyIceRotateRight,
                                          Site::BodyIceJump,
                                          Site::BodyIceClimb,
                                          Site::BodyIceFly,
                                          Site::BodyIceSlide,
                                          Site::BodyIceFastSlide,
                                          Site::BodyIceBrake,
                                          Site::BodyIceFall,
                                          Site::BodyIceAirJump,
                                          Site::BodyIceSpecial,
                                          Site::BodyIceCrash,
                                          Site::BodyPinball,
                                          Site::BodyPinballFall,
                                          Site::BodyPetStart,
                                          Site::BodyPetLoop,
                                          Site::BodyPetEnd,
                                          Site::BodyWhistle,
                                          Site::BodySuperStand,
                                          Site::BodySuperWalk,
                                          Site::BodySuperPath,
                                          Site::BodySuperFall,
                                          Site::BodySuperLaunch,
                                          Site::BodySuperSpring,
                                          Site::BodySuperSpinAttack,
                                          Site::BodySuperJump,
                                          Site::BodyPose,
                                          Site::BodyEvent,
                                          Site::BodySleep,
                                          Site::BodyKill,
                                          Site::BodyEdit};
        return mode < 88 ? values[mode] : Site::BodyPlaceOn;
    }
    void dispatching(std::uint8_t mode) { dispatched = mode; }
    Site character_sound_site(unsigned character) {
        switch (character) {
        case 0:
            return Site::StageSonicSound;
        case 2:
            return Site::StageMilesSound;
        case 3:
            return Site::StageKnucklesSound;
        case 5:
            return Site::StageAmySound;
        case 7:
            return Site::StageBigSound;
        default:
            throw std::logic_error("Sonic stage sound");
        }
    }
    PlayerCoordinates &coordinates() { return motion_dependencies()->coordinates; }
    void coordinate_result(const CoordinateResult &c, Site site) {
        if (!c) {
            const auto target = site == Site::CopyTransform ? 0x43ec00
                                : (site == Site::CenterTransform || site == Site::GroundTaskVector)
                                    ? 0x43ec90
                                    : 0x784d60;
            auto r = fail(SonicPlayerFailure::coordinate_failed, source_site(site), target);
            r.coordinate = c;
            throw Stop{r};
        }
    }
    void world_to_player(NinjaVector &v, Site site) {
        coordinate_result(coordinates().world_to_player(orientation(), v, fp()), site);
    }
    void task_to_world(NinjaVector &v, Site site) {
        coordinate_result(coordinates().task_to_world(&orientation(), v, fp()), site);
    }
    NormalAngles floor_angles(const NinjaVector &normal, Site site) {
        auto a = floor_normal_angles(normal, motion_dependencies()->floor_constants,
                                     motion_dependencies()->inverse_math, fp());
        if (!a.angles || a.failure != CrtAsinFailure::none) {
            auto r = fail(SonicPlayerFailure::coordinate_failed, source_site(site), 0x4b8e80);
            r.floor_angles = a;
            throw Stop{r};
        }
        return *a.angles;
    }
    std::array<std::int16_t, 2> raw_axes(Site site) {
        if (!motion_dependencies()->ground_input_projection)
            stop(SonicPlayerFailure::missing_native_owner, site);
        const auto &values = motion_dependencies()->ground_input_projection->surface.raw_axes;
        if (index() >= values.size() || !values[index()])
            stop(SonicPlayerFailure::unknown_field, site);
        return *values[index()];
    }
    std::array<std::int32_t, 2> camera_angles(Site site) {
        const auto &v = motion_dependencies()->ground_input_projection->surface.camera_pitch_yaw;
        if (!v)
            stop(SonicPlayerFailure::unknown_field, site);
        return *v;
    }
    const CoordinateAxisConstants &axis() {
        return motion_dependencies()->ground_input_projection->axis;
    }
    double surface_angle_units() {
        return motion_dependencies()->ground_input_projection->angle_units;
    }
    double surface_positive_factor() {
        return motion_dependencies()->ground_input_projection->positive_factor;
    }
    std::int32_t project_angle(NinjaVector a, NinjaVector b, const NinjaVector &normal, Site site) {
        auto &m = *motion_dependencies();
        const auto &g = *m.ground_input_projection;
        auto result =
            projection_vector_angle(a, b, &normal, g.axis, g.projection, m.inverse_math, fp());
        if (!result) {
            auto r = fail(SonicPlayerFailure::coordinate_failed, source_site(site), 0x438410);
            r.projection = result;
            throw Stop{r};
        }
        return *result.angle;
    }
    void ground_mutation(const Player &p, const GroundAccelerationMutation &fn, Site site,
                         std::uint32_t target) {
        if (!fn)
            stop(SonicPlayerFailure::missing_native_owner, site, target);
        auto &parameters = this->parameters(p, site);
        auto result = fn(*f.task, *f.world, *p.physics, parameters, fp());
        if (!result) {
            auto r = fail(SonicPlayerFailure::acceleration_failed, source_site(site), target);
            r.acceleration = result;
            throw Stop{r};
        }
    }
    void reset_ground_direction(const Player &p, Site site) {
        ground_mutation(p, motion_dependencies()->ground_callees.direction_reset, site, 0x43c130);
    }
    void passive_rotation(const Player &p, Site site) {
        ground_mutation(p, motion_dependencies()->ground_callees.passive_rotation, site, 0x4491e0);
    }
    void ground_yaw(const Player &p, const GroundYawMutation &fn, std::int32_t yaw, Site site,
                    std::uint32_t target) {
        if (!fn)
            stop(SonicPlayerFailure::missing_native_owner, site, target);
        auto &parameters = this->parameters(p, site);
        auto result = fn(*f.task, *f.world, *p.physics, parameters, yaw, fp());
        if (!result) {
            auto r = fail(SonicPlayerFailure::acceleration_failed, source_site(site), target);
            r.acceleration = result;
            throw Stop{r};
        }
    }
    void blended_rotation(const Player &p, std::int32_t yaw, Site site) {
        ground_yaw(p, motion_dependencies()->ground_callees.yaw_blended, yaw, site, 0x443e60);
    }
    void regular_rotation(const Player &p, std::int32_t yaw, Site site) {
        ground_yaw(p, motion_dependencies()->ground_callees.yaw_regular, yaw, site, 0x443c50);
    }
    void ground_acceleration(const Player &p, PlayerParameters &params, Site site) {
        auto &m = *motion_dependencies();
        auto a = get_player_acceleration_ground(*f.task, *f.world, *p.physics, params,
                                                m.ground_constants, m.acceleration_runtime,
                                                m.ground_callees, fp());
        if (!a) {
            auto r = fail(SonicPlayerFailure::acceleration_failed, source_site(site), 0x44c270);
            r.nested_source_boundary = a.source_boundary;
            r.acceleration = a;
            throw Stop{r};
        }
    }
    void air_acceleration(const Player &p, PlayerParameters &params, Site site) {
        auto &m = *motion_dependencies();
        auto a = get_player_acceleration_air(*f.task, *f.world, *p.physics, params, m.air_constants,
                                             m.acceleration_runtime, m.air_callees, fp());
        if (!a) {
            auto r = fail(SonicPlayerFailure::acceleration_failed, source_site(site), 0x44b9c0);
            r.nested_source_boundary = a.acceleration.source_boundary;
            r.air = a;
            throw Stop{r};
        }
    }
    void gravity_acceleration(const Player &p, PlayerParameters &params, Site site) {
        auto &m = *motion_dependencies();
        auto g = get_gravity(
            orientation(), f.world->motion, p.physics->motion, world_gravity(Site::GravityVector),
            params.weight,
            [&](const PlayerOrientation &t, NinjaVector &v) {
                auto c = coordinates().world_to_player(t, v, fp());
                return PlayerMotionResult{
                    c ? PlayerMotionFailure::none
                      : PlayerMotionFailure::world_to_player_transform_unavailable};
            },
            fp());
        if (!g)
            stop(SonicPlayerFailure::acceleration_failed, site, 0x443800);
        (void)m;
    }
    void calculate_speed(const Player &p, PlayerParameters &params, Site site) {
        auto &m = *motion_dependencies();
        auto s =
            run_player_movement(*f.task, m.acceleration_runtime.input, f.world->motion, *p.physics,
                                params, coordinates(), m.floor_constants, m.inverse_math, fp());
        if (!s) {
            auto r = fail(SonicPlayerFailure::speed_failed, source_site(site), 0x443f50);
            r.nested_source_boundary = s.speed.source_boundary;
            r.movement = s;
            throw Stop{r};
        }
    }
    std::uint32_t set_position(const Player &p, PlayerParameters &params, Site site) {
        auto &m = *motion_dependencies();
        if (!f.collision)
            stop(SonicPlayerFailure::unknown_field, site);
        auto pos = set_player_position(
            *f.task, f.world.get(), *p.physics, params, *f.collision, m.position_constants,
            m.rotation_constants, m.position_runtime, m.position_callees, &coordinates(), fp());
        if (dispatched)
            movement_position = pos;
        if (!pos || !pos.outcome) {
            auto r = fail(SonicPlayerFailure::position_failed, source_site(site), 0x44cdf0);
            r.nested_source_boundary = pos.source_boundary;
            r.position = pos;
            throw Stop{r};
        }
        return static_cast<std::uint32_t>(*pos.outcome);
    }
};

template <class Function> SonicPlayerResult execute(Context &context, Function fn) {
    try {
        return context.finish(fn());
    } catch (const Stop &stop) {
        return context.finish(stop.result);
    }
}
} // namespace sonic_reference

SonicPlayerResult sonic_action_checks(SonicPlayerFrame &f, SonicPlayerRuntime &rt, X87Context &fp) {
    sonic_reference::Context c(f, rt, fp);
    return sonic_reference::execute(c, [&] { return sonic::detail::action_checks(c); });
}
SonicPlayerResult dispatch_sonic_player_action(SonicPlayerFrame &f, SonicPlayerRuntime &rt,
                                               X87Context &fp) {
    sonic_reference::Context c(f, rt, fp);
    return sonic_reference::execute(c, [&] { return sonic::detail::dispatch(c); });
}
SonicPlayerResult run_sonic_player(SonicPlayerFrame &f, SonicPlayerRuntime &rt, X87Context &fp) {
    sonic_reference::Context c(f, rt, fp);
    return sonic_reference::execute(c, [&] { return sonic::detail::run(c); });
}
SonicPlayerResult sonic_action_predicate(SonicPlayerPredicateEntry entry, SonicPlayerFrame &f,
                                         SonicPlayerRuntime &rt, X87Context &fp,
                                         std::optional<std::uint32_t> argument) {
    sonic_reference::Context c(f, rt, fp);
    sonic::Predicate predicate;
    switch (entry) {
    case SonicPlayerPredicateEntry::event_action_495fa0:
        predicate = sonic::Predicate::Event;
        break;
    case SonicPlayerPredicateEntry::roll_release_492f90:
        predicate = sonic::Predicate::RollRelease;
        break;
    case SonicPlayerPredicateEntry::fall_494f70:
        predicate = sonic::Predicate::Fall;
        break;
    case SonicPlayerPredicateEntry::jump_495e60:
        predicate = sonic::Predicate::Jump;
        break;
    case SonicPlayerPredicateEntry::hold_493120:
        predicate = sonic::Predicate::Hold;
        break;
    case SonicPlayerPredicateEntry::charge_496ee0:
        predicate = sonic::Predicate::Charge;
        break;
    case SonicPlayerPredicateEntry::stage_pickup_442680:
        predicate = sonic::Predicate::StagePickup;
        break;
    case SonicPlayerPredicateEntry::stage_action_442570:
        predicate = sonic::Predicate::StageAction;
        break;
    case SonicPlayerPredicateEntry::jump_press_43bf40:
        predicate = sonic::Predicate::JumpPress;
        break;
    case SonicPlayerPredicateEntry::jump_classify_4b83f0:
        predicate = sonic::Predicate::JumpClassify;
        break;
    case SonicPlayerPredicateEntry::clear_tasks_4b8320:
        predicate = sonic::Predicate::ClearTasks;
        break;
    case SonicPlayerPredicateEntry::idle_494ff0:
        predicate = sonic::Predicate::Idle;
        break;
    case SonicPlayerPredicateEntry::turn_break_4429c0:
        predicate = sonic::Predicate::TurnBrake;
        break;
    case SonicPlayerPredicateEntry::request_spin_animation_495c30:
        predicate = sonic::Predicate::SpinAnimation;
        break;
    case SonicPlayerPredicateEntry::request_jump_animation_495b50:
        predicate = sonic::Predicate::JumpAnimation;
        break;
    case SonicPlayerPredicateEntry::request_walk_animation_495ba0:
        predicate = sonic::Predicate::WalkAnimation;
        break;
    case SonicPlayerPredicateEntry::wall_form_43bfe0:
        predicate = sonic::Predicate::WallForm;
        break;
    case SonicPlayerPredicateEntry::skid_end_4930d0:
        predicate = sonic::Predicate::SkidEnd;
        break;
    case SonicPlayerPredicateEntry::charge_tick_495080:
        predicate = sonic::Predicate::ChargeTick;
        break;
    default:
        return fail(SonicPlayerFailure::unimplemented_action, static_cast<unsigned>(entry));
    }
    return sonic_reference::execute(
        c, [&] { return sonic::detail::predicate(c, predicate, argument); });
}
SonicPlayerResult sonic_ground_input(SonicPlayerFrame &f, const SonicPlayerNativeMotion &motion,
                                     const SonicPlayerConstants &constants, X87Context &fp) {
    SonicPlayerGlobals globals;
    TaskScheduler scheduler;
    PlayerStartupRegistry players;
    SonicPlayerRuntime runtime{globals, scheduler, players, nullptr, &motion, constants, {}};
    sonic_reference::Context c(f, runtime, fp);
    return sonic_reference::execute(c, [&] { return sonic::detail::ground_input(c); });
}
SonicPlayerResult sonic_copy_local_speed(SonicPlayerFrame &f, const SonicPlayerNativeMotion &motion,
                                         const SonicPlayerConstants &constants, X87Context &fp) {
    SonicPlayerGlobals globals;
    TaskScheduler scheduler;
    PlayerStartupRegistry players;
    SonicPlayerRuntime runtime{globals, scheduler, players, nullptr, &motion, constants, {}};
    sonic_reference::Context c(f, runtime, fp);
    return sonic_reference::execute(c, [&] { return sonic::detail::copy_local_speed(c); });
}
SonicPlayerResult sonic_finalize_local_speed(SonicPlayerFrame &f, NinjaVector &vector,
                                             const SonicPlayerConstants &constants,
                                             const std::optional<NinjaVector> &gravity,
                                             X87Context &fp) {
    SonicPlayerGlobals globals;
    TaskScheduler scheduler;
    PlayerStartupRegistry players;
    // This direct suffix does not consume matrix state; gravity is a separate consumed fact.
    NinjaMatrix unit{};
    NinjaMatrixStack stack({unit, unit}, 1, 0);
    PlayerCoordinates coordinates({}, unit, stack);
    PlayerAccelerationRuntime acceleration;
    acceleration.world_gravity = gravity;
    GroundAccelerationConstants gc{};
    GroundAccelerationCallees gcall;
    AirAccelerationConstants ac{};
    AirAccelerationCallees acall;
    FloorAngleConstants fc{};
    CrtAsinRuntime inverse;
    PlayerPositionConstants pc{};
    PlayerRotationConstants rc{};
    PlayerPositionRuntime pr;
    PlayerPositionCallees pcall;
    SonicPlayerNativeMotion motion{coordinates, acceleration, gc, gcall, ac, acall,
                                   fc,          inverse,      pc, rc,    pr, pcall};
    SonicPlayerRuntime runtime{globals, scheduler, players, nullptr, &motion, constants, {}};
    sonic_reference::Context c(f, runtime, fp);
    return sonic_reference::execute(c,
                                    [&] { return sonic::detail::finalize_local_speed(c, vector); });
}
SonicPlayerResult sonic_update_collision_center(SonicPlayerFrame &f, PlayerCoordinates &coordinates,
                                                X87Context &fp) {
    SonicPlayerGlobals globals;
    TaskScheduler scheduler;
    PlayerStartupRegistry players;
    SonicPlayerConstants constants{};
    PlayerAccelerationRuntime acceleration;
    GroundAccelerationConstants gc{};
    GroundAccelerationCallees gcall;
    AirAccelerationConstants ac{};
    AirAccelerationCallees acall;
    FloorAngleConstants fc{};
    CrtAsinRuntime inverse;
    PlayerPositionConstants pc{};
    PlayerRotationConstants rc{};
    PlayerPositionRuntime pr;
    PlayerPositionCallees pcall;
    SonicPlayerNativeMotion motion{coordinates, acceleration, gc, gcall, ac, acall,
                                   fc,          inverse,      pc, rc,    pr, pcall};
    SonicPlayerRuntime runtime{globals, scheduler, players, nullptr, &motion, constants, {}};
    sonic_reference::Context c(f, runtime, fp);
    return sonic_reference::execute(c, [&] { return sonic::detail::update_collision_center(c); });
}
} // namespace sadx

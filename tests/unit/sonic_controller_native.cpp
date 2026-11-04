#include "player/sonic_player_internal.hpp"
#include "runtime/native_sonic_context.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace sadx;
// Deliberately throws if scheduled; this test only verifies stored callback identity.
void auxiliary(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("test auxiliary invoked");
}
struct Fixture {
    native::PlayerSystem players;
    TaskScheduler scheduler;
    std::shared_ptr<TaskWorkArena> arena = std::make_shared<TaskWorkArena>();
    TaskHandle task;
    X87Context fp{127};
    std::int16_t stage = 0;
    std::uint8_t alternate = 0;
    player_initialization::ParameterTable parameters{};
    PlayerEventFlags events{};
    std::shared_ptr<const animation::TableStorage> table = std::make_shared<animation::Table>();
    sonic::Context context;
    Fixture() : task(), context{players, scheduler, task, fp} {
        assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}));
        task = scheduler.create_element(7, 3, nullptr).task;
        assert(task);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        context.stage = &stage;
        context.alternate_model = &alternate;
        context.parameters = &parameters;
        context.events = &events;
        context.auxiliary = auxiliary;
        context.animation_table = &table;
        context.read_editor = [] { return std::int16_t{0}; };
        context.read_forced = [] { return std::int16_t{0}; };
        parameters[0].rad = 4;
        parameters[0].height = 10;
        parameters[0].weight = 0.125f;
    }
    std::string run_stop() {
        try {
            sonic::run(context);
        } catch (const sonic::Unavailable &e) {
            return e.what();
        }
        throw std::runtime_error("native Sonic unexpectedly completed");
    }
    player_initialization::PlayerHandle initialize() {
        assert(run_stop() == "sonic_model_graph");
        auto record = players.registry.players[0];
        assert(record);
        assert(record == players.registry.allocations.at(record->allocation.get()));
        return record;
    }
};
void compiler_constants_match_retained_source_bytes() {
    // Source evidence: BASELINE.json constants bytes, all thirteen original words.
    const std::array<float, 13> values{
        {0.0f, sonic::detail::kGroundInputThreshold, sonic::detail::kTinyForwardSpeed,
         -sonic::detail::kTinyForwardSpeed, sonic::detail::kTinyAirSpeed,
         -sonic::detail::kTinyAirSpeed, sonic::detail::kBounceSpeedMultiplier, 2.0f,
         sonic::detail::kSlideSlopeThreshold, sonic::detail::kJumpActorTimerLimit,
         sonic::detail::kPickupHeight, sonic::detail::kMaximumCharge,
         sonic::detail::kChargeIncrement}};
    const std::array<std::uint32_t, 13> expected{{0, 0x3f666666, 0x3c23d70a, 0xbc23d70a, 0x3a83126f,
                                                  0xba83126f, 0xbe99999a, 0x40000000, 0x3f451eb8,
                                                  0x41f00000, 0x3fbfdf3b, 0x41200000, 0x3ecccccd}};
    std::array<std::uint32_t, 13> actual{};
    std::memcpy(actual.data(), values.data(), sizeof(values));
    assert(actual == expected);
}
void initialization_uses_canonical_registry() {
    Fixture f;
    const auto primary = f.task->primary_state;
    const auto motion = f.task->motion_state;
    auto record = f.initialize();
    assert(record->stage == player_initialization::Stage::Complete);
    assert(record->task == f.task && record->motion == motion && f.task->primary_state == primary);
    assert(f.arena->reference(f.task->motion_work, 0).work == record->allocation);
    assert(record->physics->animation_tail.table_at_140 == f.table);
    assert(record->physics->requested_animation_at_124 == 0);
    assert(f.players.current.primary == f.task->primary_work && !f.players.current.player);
    // The first callback captures a genuine null player before allocating it.
    // A later callback publishes the actual same registered allocation.
    f.task->primary_state->action_at_0 = 86;
    assert(f.run_stop() == "sonic_unrecovered_body");
    assert(f.players.current.player == record->allocation);
    assert(f.players.registry.allocations.size() == 1);
}
void forced_reads_are_ordered_even_when_editor_is_nonzero() {
    Fixture f;
    f.initialize();
    f.task->primary_state->action_at_0 = 86;
    std::vector<int> reads;
    f.context.read_editor = [&] {
        reads.push_back(1);
        return std::int16_t{-1};
    };
    f.context.read_forced = [&] {
        reads.push_back(2);
        return std::int16_t{0};
    };
    assert(f.run_stop() == "sonic_unrecovered_body");
    assert((reads == std::vector<int>{1, 2}));
    assert(f.task->primary_state->action_at_0 == 87);
    f.context.read_forced = {};
    f.task->primary_state->action_at_0 = 86;
    reads.clear();
    assert(f.run_stop() == "sonic_check_edit_mode_forced");
    assert((reads == std::vector<int>{1}));
    assert(f.task->primary_state->action_at_0 == 86);
}
void collision_borrows_actual_records_and_actor() {
    Fixture f;
    auto record = f.initialize();
    f.task->primary_state->action_at_0 = 86;
    std::vector<collision_component::Info> info(2);
    info[0].damage = 0xac;
    info[1].attributes = 0x87654321;
    auto result = collision_component::initialize(
        *f.arena, f.task, f.task->primary_state->orientation.flags, info, 2, 0, f.fp);
    (void)result;
    auto work = collision_component::component(*f.arena, f.task->primary_work);
    assert(work);
    record->physics->source_flags_at_6 = 0x8000;
    assert(f.run_stop() == "sonic_unrecovered_body");
    assert(work->info[0].damage == 0xaf && work->info[1].attributes == 0x87654331);
    auto owner = f.scheduler.create_element(2, 3, nullptr).task;
    player_initialization::establish_zeroed_primary(f.scheduler, owner);
    owner->primary_state->orientation.angles = {123, 456, 789};
    work->task = owner;
    f.task->primary_state->orientation.flags = 32;
    f.task->motion_state->motion.speed.y = -1;
    assert(f.run_stop() == "sonic_unrecovered_body");
    assert(f.task->primary_state->orientation.angles[0] == 123 &&
           f.task->primary_state->orientation.angles[2] == 789);
    owner->primary_state->orientation.angles[0] = -4321;
    assert(f.run_stop() == "sonic_unrecovered_body");
    assert(f.task->primary_state->orientation.angles[0] == -4321);
}
void upgraded_charge_mutates_the_same_parameters_before_a_missing_sound() {
    Fixture f;
    auto record = f.initialize();
    record->physics->timer_at_7e = 120;
    record->physics->equipment = 3;
    record->parameters.max_x_spd = 9.25f;
    f.context.release_effect = auxiliary;
    f.context.release_destructor = auxiliary;
    bool stopped = false;
    try {
        sonic::action_predicate(f.context, sonic::Predicate::ChargeTick);
    } catch (const sonic::Unavailable &error) {
        stopped = std::string(error.what()) == "sonic_stop_sound";
    }
    assert(stopped);
    assert(record->parameters.max_x_spd == 0.5f);
    const auto effect = f.scheduler.list_head(0);
    assert(effect && effect->exec == auxiliary && effect->destructor == auxiliary);
    assert(f.arena->reference(effect->auxiliary_work, 0).work == f.task->primary_work);
    assert(f.arena->reference(effect->auxiliary_work, 4).work == record->allocation);
    float saved = 0;
    assert(f.arena->read(effect->auxiliary_work, 8, &saved, 4));
    assert(saved == 9.25f);
}
void failed_initialization_keeps_the_canonical_partial_allocation() {
    Fixture f;
    f.context.auxiliary = nullptr;
    bool stopped = false;
    try {
        sonic::run(f.context);
    } catch (const player_initialization::Error &error) {
        stopped = error.boundary == player_initialization::Boundary::AuxiliaryCallback;
    }
    assert(stopped);
    auto record = f.players.registry.players[0];
    assert(record && record->stage == player_initialization::Stage::TaskScale);
    assert(f.players.registry.allocations.at(record->allocation.get()) == record);
    assert(f.arena->reference(f.task->motion_work, 0).work == record->allocation);
}
void event_link_requires_an_independent_borrowed_binding() {
    Fixture f;
    f.task->primary_state->event_move = PlayerEventMoveData{};
    f.task->primary_state->event_move->mode = 0;
    f.task->primary_state->event_move->action_list_at_4c = PlayerEventActionListReference{1};
    bool stopped = false;
    try {
        sonic::action_predicate(f.context, sonic::Predicate::Event);
    } catch (const sonic::Unavailable &error) {
        stopped = std::string(error.what()) == "sonic_event_mode_binding";
    }
    assert(stopped);
    f.context.read_event_mode = [] { return std::uint8_t{0}; };
    stopped = false;
    try {
        sonic::action_predicate(f.context, sonic::Predicate::Event);
    } catch (const sonic::Unavailable &error) {
        stopped = std::string(error.what()) == "sonic_event_link_binding";
    }
    assert(stopped);
    // A value in the legacy field does not supply the canonical native event seam.
    f.context.read_event_link = [] { return false; };
    assert(sonic::action_predicate(f.context, sonic::Predicate::Event).value == 0);
    f.context.read_event_link = [] { return true; };
    assert(sonic::action_predicate(f.context, sonic::Predicate::Event).value == 1);
    assert(f.task->primary_state->action_at_0 == 84);
}
void released_collision_link_is_not_a_known_null_hit() {
    Fixture f;
    f.initialize();
    std::vector<collision_component::Info> info(2);
    collision_component::initialize(*f.arena, f.task, f.task->primary_state->orientation.flags,
                                    info, 2, 0, f.fp);
    auto own = collision_component::component(*f.arena, f.task->primary_work);
    std::array<TaskHandle, 10> resets{};
    f.context.reset_tasks = &resets;
    assert(sonic::action_predicate(f.context, sonic::Predicate::JumpClassify).value == 0);
    {
        auto other = std::make_shared<collision_component::Work>();
        own->other = other;
    }
    bool stopped = false;
    try {
        sonic::action_predicate(f.context, sonic::Predicate::JumpClassify);
    } catch (const sonic::Unavailable &error) {
        stopped = std::string(error.what()) == "sonic_released_hit_collision";
    }
    assert(stopped);
}
void event_binding_replacement_does_not_change_the_captured_primary() {
    Fixture f;
    auto original = f.task->primary_state;
    auto replacement = std::make_shared<PlayerTaskState>();
    replacement->action_at_0 = 17;
    replacement->orientation.flags = 1234;
    const auto old_work = f.task->primary_work;
    f.context.read_event_mode = [] { return std::uint8_t{0}; };
    f.context.read_event_link = [&] {
        f.task->primary_state = replacement;
        f.task->primary_work = f.arena->allocate(64).work;
        return true;
    };
    assert(sonic::action_predicate(f.context, sonic::Predicate::Event).value == 1);
    assert(original->action_at_0 == 84);
    assert(replacement->action_at_0 == 17 && replacement->orientation.flags == 1234);
    assert(f.task->primary_work != old_work);
}
void null_and_conflicting_player_references_are_not_guessed() {
    Fixture f;
    auto record = f.initialize();
    assert(f.arena->set_reference(f.task->motion_work, 0, {}));
    f.task->primary_state->action_at_0 = 86;
    assert(f.run_stop() == "sonic_player_physics");
    assert(!f.players.current.player && f.players.registry.players[0] == record);
    assert(f.arena->set_reference(f.task->motion_work, 0, record->allocation));
    record->motion = std::make_shared<PlayerMotionState>();
    assert(f.run_stop() == "sonic_conflicting_motion");
}
} // namespace
int main() {
    compiler_constants_match_retained_source_bytes();
    event_binding_replacement_does_not_change_the_captured_primary();
    released_collision_link_is_not_a_known_null_hit();
    event_link_requires_an_independent_borrowed_binding();
    initialization_uses_canonical_registry();
    forced_reads_are_ordered_even_when_editor_is_nonzero();
    collision_borrows_actual_records_and_actor();
    null_and_conflicting_player_references_are_not_guessed();
    upgraded_charge_mutates_the_same_parameters_before_a_missing_sound();
    failed_initialization_keeps_the_canonical_partial_allocation();
    std::cout << "Controlled native Sonic ownership/order harness passed\n";
}

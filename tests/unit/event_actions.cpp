#include "runtime/native_event_actions_context.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

namespace e = sadx::event_actions;
namespace p = sadx::selection_preview;
namespace {
using Bytes = std::vector<std::uint8_t>;
void word(Bytes &bytes, std::uint32_t value) {
    for (unsigned n = 0; n < 4; ++n) bytes.push_back(static_cast<std::uint8_t>(value >> (8 * n)));
}
void record(Bytes &bytes, unsigned kind, const Bytes &payload) {
    word(bytes, kind); word(bytes, static_cast<unsigned>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}
sadx::character_pack::CharacterPack pack() {
    Bytes bytes{'S','D','X','A','C','T','0','1'};
    for (auto value : {5,2,100,1,101,4}) word(bytes, value);
    Bytes action; word(action, 2); word(action, 3);
    record(bytes, 1, action); record(bytes, 2, Bytes(52));
    Bytes motion; for (auto value : {5,1,131075,1}) word(motion, value);
    record(bytes, 11, motion); record(bytes, 1, action);
    Bytes tracks(20); tracks[0] = 1; record(bytes, 12, tracks);
    return sadx::character_pack::CharacterPack::decode(bytes);
}
sadx::TaskHandle actor(sadx::TaskScheduler &scheduler) {
    const auto result = scheduler.create_element(2, 1, sadx::TaskScheduler::source_no_op);
    assert(result);
    auto task = result.task;
    task->primary_state = std::make_shared<sadx::PlayerTaskState>();
    task->primary_state->orientation = {0, {0,0,0}};
    task->primary_state->secondary_mode_at_1 = 0;
    task->primary_state->source_counter_at_c = -1;
    return task;
}
struct Alias final : e::ActionSource {
    sadx::PlayerEventActionNode *node = nullptr;
    std::uint32_t object(sadx::ActionHandle) override { return static_cast<std::uint8_t>(node->mode) | (static_cast<std::uint8_t>(node->link_frames) << 8); }
    std::uint32_t motion(sadx::ActionHandle) override { return node->action.object; }
};
}
int main() {
    auto arena = std::make_shared<sadx::TaskWorkArena>();
    sadx::TaskScheduler scheduler;
    assert(scheduler.initialize_source_pool(arena, {}));
    sadx::player_initialization::Registry registry;
    const auto resources = pack();
    e::State state;
    const sadx::native::EventPrimaryFields fields{
        [](sadx::PlayerTaskState &primary, std::uint8_t mode) { primary.secondary_mode_at_1 = mode; },
        [](sadx::PlayerTaskState &primary) -> std::int32_t & {
            if (!primary.source_counter_at_c)
                throw e::Error(e::Boundary::RegisteredCounter);
            return *primary.source_counter_at_c;
        }};
    sadx::native::EventActionsContext context(*arena, state, registry, resources, fields);
    sadx::native::CharacterSelectionAssets assets;
    std::array<sadx::regular_textures::List, 6> lists{};
    std::array<sadx::regular_textures::List *, 6> textures{};
    p::Work work;
    p::ActionState actions;
    const auto idle = resources.action(sadx::character_pack::StartupAction::ClearPose);
    const auto selected = resources.action(sadx::character_pack::StartupAction::ClearPoseTransition);
    for (std::size_t n = 0; n < 6; ++n) {
        textures[n] = &lists[n];
        assets.sequences[n].steps = {{{},idle,0},{{},selected,130},{{},idle,-129}};
        work.actors[n] = actor(scheduler);
        registry.primary[n] = work.actors[n]->primary_state;
        e::initialize(context.core, work.actors[n]->primary_work);
    }
    // PInitialize already creates this same event allocation in bind_work. A
    // legacy-created event needs no second owner or synthetic list to admit/clear.
    const auto startup_actor = actor(scheduler);
    const auto startup_event = arena->allocate(124).work;
    assert(startup_event && arena->fill(startup_event,0,0,124));
    assert(arena->set_reference(startup_actor->primary_work,60,startup_event));
    e::initialize(context.core,startup_actor->primary_work);
    assert(e::event_work(*arena,startup_actor->primary_work)==startup_event);
    e::set_action(context.core,startup_actor,idle,&lists[0],.25f,1,0);
    auto *startup_node = e::action_head(*arena,startup_actor->primary_work);
    assert(startup_node && startup_node->action.object==2);
    e::clear_action(context.core,startup_actor);
    assert(!e::live(*startup_node) && !e::action_head(*arena,startup_actor->primary_work));
    const auto miles = work.actors[1]->primary_state;
    registry.primary[1] = std::make_shared<sadx::PlayerTaskState>(*miles);
    registry.primary[1]->source_counter_at_c = -1;
    sadx::native::PreviewActionCalls calls(context, assets, textures);
    std::uint8_t selection = 1;
    p::update_actions(work, actions, selection, calls);
    for (std::size_t n = 0; n < 6; ++n) {
        auto *node = e::action_head(*arena, work.actors[n]->primary_work);
        assert(node && actions.tails[n] == node && e::live(*node));
        assert(node->action.object == 2 && node->action.motion == 3);
        assert(node->textures == &lists[n] && node->mode == 1 && node->link_frames == 0);
        assert(node->speed == (n == 1 ? .5f : .25f));
        assert(!e::next_action(*arena,node));
    }
    assert(registry.primary[1]->secondary_mode_at_1 == 49);
    assert((static_cast<std::uint32_t>(*registry.primary[1]->source_counter_at_c) & (1U<<29)) == 0);
    assert(miles->secondary_mode_at_1 == 0 && miles->source_counter_at_c == -1);
    work.selected_motion = 1;
    p::update_actions(work, actions, selection, calls);
    auto *first = e::action_head(*arena, work.actors[1]->primary_work);
    auto *second = e::next_action(*arena, first);
    auto *third = e::next_action(*arena, second);
    assert(second->mode == 0 && second->link_frames == -126);
    assert(third->mode == 1 && third->link_frames == 127 && actions.tails[1] == third);
    assert(work.motion_counts[1] == 3 && work.selected_motion == 0);

    // A lower allocation callback replaces the task's primary. Tail reads retain
    // the primary captured before SetInputP/EV_SetAction, with its live event link.
    const auto captured = sadx::native::capture_event_primary(work.actors[0]);
    const auto replacement = actor(scheduler);
    e::initialize(context.core, replacement->primary_work);
    const auto original_tail = actions.tails[0];
    bool rebound = false;
    arena->set_payload_transport({}, [&](std::uint32_t, const void *, std::uint32_t bytes) {
        if (bytes == 24 && !rebound) {
            rebound = true;
            work.actors[0]->primary_state = replacement->primary_state;
            work.actors[0]->primary_work = replacement->primary_work;
        }
        return true;
    });
    work.selected_motion = 1; selection = 0;
    p::update_actions(work, actions, selection, calls);
    arena->set_payload_transport({}, {});
    assert(rebound && actions.tails[0] != original_tail);
    assert(actions.tails[0] == e::next_action(*arena, original_tail));
    // First queued selected action went to captured primary; second went to
    // replacement after reentry, so captured tail is the first appended node.
    assert(e::action_head(*arena, replacement->primary_work)->mode == 1);

    // The captured primary's event pointer remains live across a lower call.
    const auto live_actor = actor(scheduler);
    const auto event_replacement = actor(scheduler);
    e::initialize(context.core,live_actor->primary_work);
    e::initialize(context.core,event_replacement->primary_work);
    e::set_action(context.core,event_replacement,idle,&lists[0],.25f,1,0);
    const auto old_event = e::event_work(*arena,live_actor->primary_work);
    const auto new_event = e::event_work(*arena,event_replacement->primary_work);
    auto *replacement_tail = e::action_head(*arena,event_replacement->primary_work);
    p::Work live_work;
    live_work.actors[0] = live_actor;
    p::ActionState live_actions;
    bool event_rebound = false;
    arena->set_payload_transport({},[&](std::uint32_t,const void *,std::uint32_t bytes) {
        if(bytes==24 && !event_rebound) {
            event_rebound = true;
            assert(arena->set_reference(live_actor->primary_work,60,new_event));
        }
        return true;
    });
    p::update_actions(live_work,live_actions,selection,calls);
    arena->set_payload_transport({},{});
    assert(event_rebound && live_actions.tails[0] == replacement_tail);
    const auto queued_old = arena->reference(old_event,64);
    assert(queued_old && queued_old.work && queued_old.work->event_state->node.get() != replacement_tail);
    assert(live_work.motion_counts[0] == 1);

    // Copies action fields in source order, including overlap with a new node.
    Alias alias;
    e::Context alias_context{*arena,state,alias};
    const auto alias_task = actor(scheduler);
    e::initialize(alias_context, alias_task->primary_work);
    arena->set_metadata_writer([&](std::uint32_t, std::optional<std::uint32_t>) {
        if (!state.node_identities.empty()) alias.node = state.node_identities.back()->event_state->node.get();
    });
    e::set_action(alias_context,alias_task,{1},nullptr,-0.0f,258,259);
    arena->set_metadata_writer({});
    auto *overlap = e::action_head(*arena,alias_task->primary_work);
    assert(overlap->action.object == 770 && overlap->action.motion == 770);
    std::uint32_t bits = 0; std::memcpy(&bits,&overlap->speed,4); assert(bits == 0x80000000U);

    // No fixture handle may keep a freed head alive for the old=head quirk.
    const auto alias_old_actor = actor(scheduler);
    e::initialize(context.core,alias_old_actor->primary_work);
    e::set_action(context.core,alias_old_actor,idle,&lists[0],.25f,1,0);
    auto *alias_old = e::action_head(*arena,alias_old_actor->primary_work);
    const auto alias_event = e::event_work(*arena,alias_old_actor->primary_work);
    assert(arena->set_reference(alias_event,68,alias_old->allocation.lock()));
    bool duplicate_stop = false;
    try { e::clear_action(context.core,alias_old_actor); }
    catch (const e::Error &failure) {
        duplicate_stop = failure.boundary==e::Boundary::OldRelease &&
                         failure.work_failure==sadx::TaskWorkFailure::ReleasedWork;
    }
    assert(duplicate_stop && !e::live(*alias_old));
    assert(!arena->reference(alias_event,64).work); // Head was cleared before old free.
    assert(arena->reference(alias_event,68).failure==sadx::TaskWorkFailure::ReleasedWork);

    const auto retained = actions.tails;
    e::clear_actions(context.core,captured.allocation);
    assert(actions.tails == retained && !e::live(*retained[0]));
    bool stale = false;
    try { e::next_action(*arena, retained[0]); } catch (const e::Error &) { stale = true; }
    assert(stale);
    // Actual scheduler cleanup walks the same arena links and retires real nodes.
    assert(scheduler.destroy_task(work.actors[1]));
    assert(!e::live(*retained[1]) && actions.tails == retained);
    const auto again = actor(scheduler);
    e::initialize(context.core,again->primary_work);
    e::set_action(context.core,again,idle,&lists[0],1,1,0);
    assert(!e::live(*retained[1]));
    p::Work reentry;
    p::update_actions(reentry,actions,selection,calls);
    assert(actions.tails == retained);
    e::set_action(context.core,{}, {},nullptr,0,0,0);
    e::clear_action(context.core,{});
    registry.primary[0].reset();
    e::set_input(registry.primary,0,-1,fields.store_secondary_mode);
    // Missing scalar binding stops after the source flag store.
    registry.primary[0] = again->primary_state;
    bool input_stop = false;
    try { e::set_input(registry.primary,0,12,{}); }
    catch (const e::Error &failure) { input_stop = failure.boundary == e::Boundary::InputMode; }
    assert(input_stop && (again->primary_state->orientation.flags & 4096));
    const auto failed_actor = actor(scheduler);
    e::initialize(context.core, failed_actor->primary_work);
    const auto free_blocks = arena->blocks();
    std::vector<std::uint32_t> requests;
    auto free_offset = arena->free_head();
    while (free_offset) {
        for (const auto &block : free_blocks) if (block.offset == *free_offset) {
            if (block.physical_next && *block.physical_next-block.offset > 32)
                requests.push_back(*block.physical_next-block.offset-32);
            free_offset = block.free_next;
            break;
        }
    }
    for (auto bytes : requests) assert(arena->allocate(bytes));
    assert(!arena->allocate(24));
    p::Work failed_work;
    failed_work.actors[0] = failed_actor;
    bool allocation_stop = false;
    try { p::update_actions(failed_work,actions,selection,calls); }
    catch (const e::Error &failure) { allocation_stop = failure.boundary == e::Boundary::NodeMode; }
    assert(allocation_stop && failed_work.motion_counts[0] == 0 && actions.tails == retained);
    assert(!e::action_head(*arena,failed_actor->primary_work));
    failed_work.motion_counts[0] = 1;
    failed_work.selected_motion = 1;
    try { p::update_actions(failed_work,actions,selection,calls); assert(false); }
    catch (const e::Error &failure) { assert(failure.boundary == e::Boundary::NodeMode); }
    assert(failed_work.motion_counts[0] == 2 && failed_work.selected_motion == 1);
    assert(actions.tails == retained);
    std::cout << "event actions: native scheduler/arena/registry/preview composition passed\n";
}

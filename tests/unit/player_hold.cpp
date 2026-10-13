#include "player/player_hold.hpp"
#include <cassert>
#include <iostream>
namespace {
void check(const sadx::PlayerHoldResult& r,std::uint32_t flags){assert(r && r.flags && *r.flags==flags);}
void stopped(const sadx::PlayerHoldResult& r,sadx::PlayerHoldFailure f,std::uint32_t at){assert(!r && !r.flags && r.failure==f && r.source_boundary==at);}
}
int main(){using namespace sadx;
    TaskScheduler scheduler;auto arena=std::make_shared<TaskWorkArena>();
    auto held=scheduler.append_task(0,&TaskScheduler::source_no_op).task;
    held->primary_work=arena->allocate(64).work;held->motion_work=arena->allocate(64).work;
    auto cwp=arena->allocate(1).work;assert(arena->set_reference(held->primary_work,0x38,cwp));
    PlayerTaskState task{{0x800,{}},255,0,{}};PlayerPhysicsState player{};player.held_task_at_68=held;
    unsigned lookups=0;
    PlayerHoldRuntime runtime{[&](std::uint8_t index)->std::optional<const PlayerPhysicsState*>{++lookups;assert(index==255);return &player;},arena};
    PlayerPositionCollisionScene scene;
    constexpr unsigned golden[8][4]={{512,1024,0,2048},{0,0,0,0},{512,1024,0,2048},
        {512,1024,0,2048},{0,0,0,0},{512,1024,0,2048},{512,1024,0,2048},{512,1536,1536,2048}};
    for(unsigned character=0;character<8;++character)for(unsigned id=4;id<8;++id){
        task.character_id=character;const auto byte=static_cast<std::uint8_t>(id);
        assert(arena->write(held->primary_work,2,&byte,1));check(query_player_hold(task,scene,runtime),golden[character][id-4]);
    }
    task.character_id=255;check(query_player_hold(task,scene,runtime),0);task.character_id=0;
    held->primary_state=std::make_shared<PlayerTaskState>(task);
    stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::unknown_actor_id,0x43ba34);
    held->primary_state->task_id_at_2=5;check(query_player_hold(task,scene,runtime),0x400); // arena still ID7
    held->primary_state->task_id_at_2=6;check(query_player_hold(task,scene,runtime),0);
    held->primary_state.reset();
    player.held_task_at_68.reset();stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::unknown_held_task,0x43b9ea);
    player.held_task_at_68=TaskHandle{};check(query_player_hold(task,scene,runtime),0);player.held_task_at_68=held;
    held->exec=&TaskScheduler::destroy_callback;runtime.work_arena.reset();check(query_player_hold(task,scene,runtime),0);
    held->exec=nullptr;check(query_player_hold(task,scene,runtime),0);
    held->exec=&TaskScheduler::source_no_op;
    stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::missing_work_arena,0x43ba22);
    runtime.work_arena=arena;assert(arena->set_reference(held->primary_work,0x38,{}));check(query_player_hold(task,scene,runtime),0);
    assert(arena->set_reference(held->primary_work,0x38,cwp));
    auto unknown=arena->allocate(64).work;auto old=held->primary_work;held->primary_work=unknown;
    auto unobserved=query_player_hold(task,scene,runtime);assert(!unobserved && unobserved.source_boundary==0x43ba22 && unobserved.work_failure!=TaskWorkFailure::none);
    held->primary_work=old;

    task.orientation.flags=0;const auto before=lookups;
    stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::unknown_collision_reference,0x43ba61);
    scene.task_collision=PositionResourceRef{1};scene.works[1].flags=0;
    check(query_player_hold(task,scene,runtime),0);assert(lookups==before);
    auto& own=scene.works[1];own.flags=5;own.hit_work=PositionResourceRef{};
    check(query_player_hold(task,scene,runtime),0);
    own.hit_work=PositionResourceRef{2};scene.works[2].owning_task=PositionResourceRef{};
    stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::null_required_owning_task,0x43ba8f);
    scene.works[2].owning_task=PositionResourceRef{3};scene.tasks[3].work=PositionResourceRef{4};
    auto& actor=scene.actors[4];actor.flags_bytes_at_4[0]=0;
    check(query_player_hold(task,scene,runtime),0); // high byte and ID remain unknown
    actor.flags_bytes_at_4[0]=0x40;
    stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::unknown_actor_id,0x43ba98);
    actor.task_id=5;stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::unknown_actor_flags,0x43ba9b);
    actor.flags_bytes_at_4[1]=0;check(query_player_hold(task,scene,runtime),0x400);
    PlayerTaskState live=task;live.orientation.flags=0x40;actor.live_task=&live;
    stopped(query_player_hold(task,scene,runtime),PlayerHoldFailure::unknown_actor_id,0x43ba98);
    live.task_id_at_2=4;check(query_player_hold(task,scene,runtime),0x200); // snapshot still5
    live.orientation.flags=0x1040;check(query_player_hold(task,scene,runtime),0);
    live.orientation.flags=0x40;live.task_id_at_2=5;
    const auto bound=bind_player_hold(runtime);
    for(unsigned pc:{0U,0x200U,0x300U})for(unsigned rc:{0U,0x400U,0x800U,0xc00U}){
        X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));
        (void)fp.divide(fp.from_int64(1),fp.from_int64(3));const auto flags=fp.exception_flags(),cw=fp.control_word();
        auto result=bound(task,player,scene,fp);assert(result.failure==PlayerPositionFailure::none && result.flags==0x400);
        assert(result.hold_diagnostics && *result.hold_diagnostics);
        assert(fp.control_word()==cw && fp.exception_flags()==flags);
    }
    live.task_id_at_2.reset();X87Context fp(0x37f);const auto failed=bound(task,player,scene,fp);
    assert(failed.failure==PlayerPositionFailure::hold_owner_failed && failed.hold_diagnostics);
    assert(failed.hold_diagnostics->failure==PlayerHoldFailure::unknown_actor_id && failed.source_boundary==0x43ba98);
    live.task_id_at_2=5;X87Context pending(0x37f);
    (void)pending.divide(pending.from_int64(1),pending.from_int64(3));pending.set_control_word(0x035f);
    const auto integer_only=bound(task,player,scene,pending);
    assert(integer_only.failure==PlayerPositionFailure::none && integer_only.flags==0x400);
    assert(pending.control_word()==0x035f && (pending.exception_flags()&0x20)); // no invented FWAIT
    std::cout<<"Player hold query passed source routes, live authority, availability order and12 untouched FP profiles\n";
}

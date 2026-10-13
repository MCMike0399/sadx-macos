#include "player/player_startup.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>

using namespace sadx;
namespace {
template<class T> void check(const T& v){if(!static_cast<bool>(v))throw std::runtime_error("startup assertion");}
TaskHandle make_task(TaskScheduler& scheduler,std::uint8_t character,std::uint8_t id=0) {
    auto task=scheduler.create_element(3,1,nullptr).task;check(task);
    task->primary_state=std::make_shared<PlayerTaskState>();
    task->primary_state->character_id=character;task->primary_state->task_id_at_2=id;
    task->primary_state->position=NinjaVector{3,4,5};task->primary_state->scale_at_2c=NinjaVector{7,8,9};
    return task;
}
void callback_boundary_token(TaskScheduler&,const TaskHandle&) {
    throw std::runtime_error("Unrecovered auxiliary callback must not execute in this owner test");
}
void generic_auxiliary_work(const PlayerStartupParameterTable& table) {
    TaskScheduler scheduler;auto arena=std::make_shared<TaskWorkArena>();
    check(scheduler.initialize_source_pool(arena,std::shared_ptr<void>{}));
    auto task=make_task(scheduler,1);auto motion=std::make_shared<PlayerMotionState>();
    PlayerStartupRegistry registry;X87Context fp(0x37f);
    // Explicit uninvoked callback token tests source pointer storage/creation,
    // not4a2b60 execution. It fails if called; no successful callback is faked.
    const PlayerStartupRuntime runtime{&table,nullptr,&callback_boundary_token};
    auto result=initialize_player_startup(3,task,motion,scheduler,registry,runtime,fp);
    check(result && result.partial->stage==PlayerStartupStage::complete);
    const auto auxiliary=result.partial->auxiliary_task;
    check(auxiliary && auxiliary->exec==&callback_boundary_token && !auxiliary->primary_state);
    float encoded=0;check(arena->read(auxiliary->primary_work,0x34,&encoded,4));check(encoded==3);
    check(arena->set_reference(auxiliary->primary_work,8,result.partial->player_work));
    check(arena->reference(auxiliary->primary_work,8).work==result.partial->player_work);
    check(!auxiliary->primary_state); // opaque union pointer stays outside player fields
}
void checked_prefix(const PlayerStartupParameterTable& table) {
    const PlayerPhysicsState unproduced{};
    check(!unproduced.animation_tail.next_at_12a && !unproduced.animation_tail.timer_at_12c && !unproduced.animation_tail.flags_at_12e);
    check(!unproduced.animation_tail.frame_at_130 && !unproduced.animation_tail.start_at_134);
    check(!unproduced.animation_tail.table_at_140 && !unproduced.animation_tail.buffer_at_148);
    auto arena=std::make_shared<TaskWorkArena>();TaskScheduler scheduler;
    check(scheduler.initialize_source_pool(arena,std::shared_ptr<void>{}));
    PlayerStartupRegistry registry;PlayerEventFlags events;events.fill(2);
    PlayerStartupRuntime runtime{&table,&events,nullptr};X87Context fp(0x27f);
    const std::uint16_t expected[]={0x1f,0,0x18,0x60,0,0x180,0x1e00,0x1800};
    for(std::uint8_t row=0;row<8;++row) {
        auto task=make_task(scheduler,row,row==0?0:6);
        auto motion=std::make_shared<PlayerMotionState>();motion->motion.speed={9,8,7};
        auto result=initialize_player_startup(row,task,motion,scheduler,registry,runtime,fp);
        check(result.failure==PlayerStartupFailure::missing_auxiliary_effect_callback && result.source_boundary==0x44297b && result.callee_boundary==0x4a2b60);
        const auto player=result.partial;check(player && player->stage==PlayerStartupStage::task_scale);
        check(player->player_work && player->contact_work && player->auxiliary_work && !player->auxiliary_task);
        check(player->player->equipment_at_4==expected[row] && player->player->island_number_at_12==0);
        check(task->primary_state->task_id_at_2==(row==0?1:6) && task->primary_state->scale_at_2c->z==1);
        check(task->primary_state->position->x==3 && motion->motion.speed.x==9 && motion->friction_scale==1);
        check(player->player->floor_normal->x==0 && player->player->contact_work->radius_at_0==0);
        check(player->player->held_task_at_68 && !*player->player->held_task_at_68);
        check(player->player->flags_at_10==0 && player->player->no_control_time_at_a==0);
        check(player->player->source_flags_at_6==0 && player->player->timer_at_7e==0);
        check(player->player->source_charge_at_0==0 && player->player->jump_timer_at_8==0 && player->player->idle_timer_at_e==0);
        check(player->player->timer_at_80==0 && player->player->timer_at_82==0);
        check(player->player->requested_animation_at_124==0 && player->player->current_animation_at_126==0 && player->player->previous_animation_at_128==0);
        const auto& tail=player->player->animation_tail;
        check(tail.next_at_12a==0 && tail.timer_at_12c==0 && tail.flags_at_12e==0);
        check(tail.frame_at_130==0 && tail.start_at_134==0);
        check(tail.table_at_140 && !*tail.table_at_140 && tail.buffer_at_148 && !*tail.buffer_at_148);
        check(player->player->moving_task_at_6c && !*player->player->moving_task_at_6c);
        check(player->player->cached_land_object_at_74 && !player->player->cached_land_object_at_74->id);
        check(registry.lookup_player(row) && *registry.lookup_player(row)==player->player.get());
        check(registry.lookup_player_work(player->player_work)==player);
        check(arena->reference(task->motion_work,0).work==player->player_work);
        const auto speed=arena->reference_view(player->player_work,0x138);
        check(speed && speed.work==player->player_work && speed.byte_offset==0x38);
        check(arena->reference(player->player_work,0x138).failure==TaskWorkFailure::interior_reference_requires_view);
        check(arena->reference(player->player_work,0x13c).work==player->player_work);
        check(player->self_at_13c()==player->player.get());
        player->player->motion.speed.x=17;check(player->speed_at_138()->x==17);
        player->speed_at_138()->y=18;check(player->player->motion.speed.y==18);
        const auto object=arena->reference(task->primary_work,0x3c);check(object && object.work);
        std::uint8_t mode=0;check(arena->read(object.work,0,&mode,1) && mode==3);
    }
    check(!registry.lookup_player(9));
    auto generic=scheduler.create_element(2,0,nullptr).task;check(generic);
    const std::uint8_t action=0x53;check(arena->write(generic->primary_work,0,&action,1));
    const std::uint8_t secondary=0x27;const std::uint16_t timer=0x1234;
    check(arena->write(generic->primary_work,1,&secondary,1));check(arena->write(generic->primary_work,6,&timer,2));
    const std::uint32_t counter=0x89abcdef;check(arena->write(generic->primary_work,0x0c,&counter,4));
    check(bind_player_task_primary(scheduler,generic));check(generic->primary_state->action_at_0==0x53);
    check(generic->primary_state->secondary_mode_at_1==0x27 && generic->primary_state->control_timer_at_6==0x1234);
    check(generic->primary_state->source_counter_at_c==-1985229329 && !generic->primary_state->event_move);
    generic->primary_state->action_at_0.reset();check(bind_player_task_primary(scheduler,generic));
    check(!generic->primary_state->action_at_0); // existing live unknown never falls back
    generic->primary_state->source_counter_at_c.reset();check(bind_player_task_primary(scheduler,generic));
    check(!generic->primary_state->source_counter_at_c); // unknown live value stays unknown
}
void unknown_and_null(const PlayerStartupParameterTable& table) {
    TaskScheduler scheduler;auto arena=std::make_shared<TaskWorkArena>();
    check(scheduler.initialize_source_pool(arena,std::shared_ptr<void>{}));
    PlayerStartupRegistry registry;PlayerStartupRuntime missing{&table,nullptr,nullptr};X87Context fp(0x37f);
    auto task=make_task(scheduler,0);auto motion=std::make_shared<PlayerMotionState>();
    auto events=initialize_player_startup(0,task,motion,scheduler,registry,missing,fp);
    check(events.failure==PlayerStartupFailure::missing_events && events.source_boundary==0x44281a);
    check(events.partial->player->shadow_scale_at_1fc && events.partial->player->equipment_at_4==0);
    check(registry.lookup_player_work(events.partial->player_work)==events.partial && registry.player_work[0]);
    auto eggman=make_task(scheduler,1);auto no_events=initialize_player_startup(1,eggman,motion,scheduler,registry,missing,fp);
    check(no_events.failure==PlayerStartupFailure::missing_auxiliary_effect_callback); // no source event reads
    auto unknown_id=make_task(scheduler,1);unknown_id->primary_state->task_id_at_2.reset();
    auto id=initialize_player_startup(2,unknown_id,motion,scheduler,registry,missing,fp);
    check(id.failure==PlayerStartupFailure::missing_task_id && id.source_boundary==0x44294c && motion->friction_scale==1);
    auto missing_motion=make_task(scheduler,1);
    auto live=initialize_player_startup(3,missing_motion,{},scheduler,registry,missing,fp);
    check(live.failure==PlayerStartupFailure::missing_live_motion && live.source_boundary==0x4427bf && registry.tasks[3] && registry.task_work[3] && !registry.player_work[3]);
    auto unsupported=make_task(scheduler,1);
    auto index=initialize_player_startup(8,unsupported,motion,scheduler,registry,missing,fp);
    check(index.failure==PlayerStartupFailure::unsupported_registration_slot && index.partial->contact_work);
    auto no_parameters=initialize_player_startup(0,unsupported,motion,scheduler,registry,{},fp);
    check(no_parameters.failure==PlayerStartupFailure::missing_parameters && no_parameters.partial->player_work && !no_parameters.partial->parameters);
    auto nullable=make_task(scheduler,1);auto object=arena->allocate(124).work;check(arena->fill(object,0,0,124));
    check(arena->set_reference(nullable->primary_work,0x3c,object));
    // Leave precisely512 payload +32 header, so contact and auxiliary alloc
    // return null; source reaches the required auxiliary-effect creation boundary.
    const auto free=*arena->free_head();const auto remaining=TaskWorkArena::usable_bytes-free;
    check(arena->allocate(remaining-544-32));
    auto nulls=initialize_player_startup(4,nullable,motion,scheduler,registry,missing,fp);
    check(nulls.failure==PlayerStartupFailure::missing_auxiliary_effect_callback && !nulls.partial->contact_work && !nulls.partial->auxiliary_work);
    check(arena->reference(nulls.partial->player_work,0x64) && !arena->reference(nulls.partial->player_work,0x64).work);
}
}
int main(int argc,char** argv){try{
    check(argc==2);PlayerStartupParameterTable table{};std::ifstream file(argv[1],std::ios::binary);
    file.read(reinterpret_cast<char*>(table.data()),1056);check(file.gcount()==1056 && file.peek()==EOF);
    checked_prefix(table);unknown_and_null(table);generic_auxiliary_work(table);
    std::cout<<"Player startup: real allocation/registration prefix, live aliases, source events and retained boundaries passed.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

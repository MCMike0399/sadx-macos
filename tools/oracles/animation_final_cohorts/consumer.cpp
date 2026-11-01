#include "player/sonic_player.hpp"
#include "player/player_initialization.hpp"
#include <sstream>
#include <cstring>
extern "C" const char* final_consumer(const std::uint8_t* constants,std::size_t size,const std::uint32_t* in) {
    using namespace sadx;
    static std::string result;
    auto k=decode_sonic_player_constants(constants,size);if(!k)return nullptr;
    TaskScheduler scheduler;TaskWorkArena arena;
    auto object=scheduler.append_task(0,&TaskScheduler::source_no_op).task;
    object->primary_work=arena.allocate(64).work;
    auto task=std::make_shared<PlayerTaskState>();object->primary_state=task;
    task->action_at_0=static_cast<std::uint8_t>(in[0]);task->task_id_at_2=0;task->control_timer_at_6=0;
    auto world=std::make_shared<PlayerMotionState>();
    auto player=std::make_shared<PlayerStartupPlayer>();player->player=std::make_shared<PlayerPhysicsState>();player->player_work=arena.allocate(512).work;
    auto& p=*player->player;auto table=std::make_shared<PlayerAnimationTable>();
    for(unsigned i=0;i<2;++i) table->records[i]={AnimationResourceReference{i},0,3,static_cast<std::uint16_t>(in[3]+i),0,0};
    PlayerAnimationResources resources;for(unsigned i=0;i<2;++i){resources.actions[i].motion=AnimationResourceReference{i};resources.motions[i].frame_count=10;}
    PlayerAnimationConstants c{0,1,4294967296.0F};PlayerAnimationRuntime animation{&arena,resources,c};
    p.animation_mode_at_120=3;p.requested_animation_at_124=0;p.current_animation_at_126=0;
    p.animation_tail.table_at_140=table;p.animation_tail.frame_at_130=0;p.animation_tail.start_at_134=0;p.animation_tail.timer_at_12c=static_cast<std::uint16_t>(in[2]);p.animation_tail.flags_at_12e=0;
    p.timer_at_80=0;p.source_flags_at_6=0;p.held_task_at_68=TaskHandle{};p.animation_tail.join_table=static_cast<PlayerJoinTable*>(nullptr);
    // This is the exact initialized view used by the Gamma table binding. Sharing
    // is observed through it; PSetMotion is not called or replaced.
    auto initialized=std::make_shared<player_initialization::Player>();initialized->physics=player->player;
    auto view=player_initialization::bind_initialized_player(initialized);
    view.animation_table=table;view.requested_animation=static_cast<std::uint16_t>(in[4]&1);
    view.current_animation=view.requested_animation;
    PlayerPositionCollisionScene scene;SonicPlayerFrame frame{object,task,world,player,&scene};
    SonicPlayerGlobals globals;PlayerStartupRegistry registry;SonicPlayerRuntime runtime{globals,scheduler,registry,nullptr,nullptr,*k,{}};runtime.animation_runtime=&animation;
    if(in[5]==1)p.animation_tail.timer_at_12c.reset();
    if(in[5]==2)runtime.animation_runtime=nullptr;
    X87Context fp(static_cast<std::uint16_t>(in[1]));auto r=dispatch_sonic_player_action(frame,runtime,fp);
    std::ostringstream out;out<<"{\"result\":["<<static_cast<unsigned>(r.failure)<<','<<r.source_boundary<<','<<r.callee_address<<','<<r.nested_source_boundary<<','<<r.completed<<"],\"animation\":[";
    if(r.animation)out<<static_cast<unsigned>(r.animation->failure)<<','<<r.animation->source_boundary;else out<<"null,null";
    out<<"],\"identity\":["<<(view.animation_table.get()==table.get())<<','<<(p.animation_tail.table_at_140&&(*p.animation_tail.table_at_140).get()==table.get())<<','<<(&view.requested_animation==&*p.requested_animation_at_124)<<','<<(&view.current_animation==&*p.current_animation_at_126)<<"],\"state\":["<<*p.requested_animation_at_124<<','<<*p.current_animation_at_126<<',';
    if(p.animation_tail.timer_at_12c)out<<*p.animation_tail.timer_at_12c;else out<<"null";
    out<<','<<fp.exception_flags()<<','<<fp.control_word()<<"],\"PSetMotion\":\"UNRECOVERED\"}";
    result=out.str();return result.c_str();
}

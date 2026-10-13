#include "player/sonic_player.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
namespace   {
    std::vector<std::uint8_t> read(const char* p)  {
        std::ifstream f(p,std::ios::binary);
        return {std::istreambuf_iterator<char>(f),{}};
    }
    void stopped(const sadx::SonicPlayerResult& r,sadx::SonicPlayerFailure failure,std::uint32_t at)  {
        if(r||r.completed||r.failure!=failure||r.source_boundary!=at)std::cerr<<"expected "<<static_cast<unsigned>(failure)<<" @"<<std::hex<<at<<" got "<<static_cast<unsigned>(r.failure)<<" @"<<r.source_boundary<<" complete "<<r.completed<<"\n";
        assert(!r&&!r.completed&&r.failure==failure&&r.source_boundary==at);
    }
    struct Fixture {
    sadx::TaskScheduler scheduler;
    std::shared_ptr<sadx::TaskWorkArena> arena=std::make_shared<sadx::TaskWorkArena>();
    sadx::TaskHandle object=scheduler.append_task(0,&sadx::TaskScheduler::source_no_op).task;
    std::shared_ptr<sadx::PlayerTaskState> task=std::make_shared<sadx::PlayerTaskState>();
    std::shared_ptr<sadx::PlayerMotionState> world=std::make_shared<sadx::PlayerMotionState>();
    std::shared_ptr<sadx::PlayerStartupPlayer> player=std::make_shared<sadx::PlayerStartupPlayer>();
    sadx::PlayerPositionCollisionScene scene;
    std::optional<std::uint16_t> forced_3b29d08;sadx::SonicPlayerGlobals globals;
    sadx::PlayerStartupRegistry players;
    Fixture(const Fixture&)=delete; // globals borrow forced_3b29d08
    Fixture()  {
        object->primary_work=arena->allocate(64).work;
        object->primary_state=task;
        player->player_work=arena->allocate(512).work;
        player->player=std::make_shared<sadx::PlayerPhysicsState>();
        player->parameters=sadx::PlayerParameters{};
        task->action_at_0=0;
        task->task_id_at_2=2;
        task->control_timer_at_6=0;
        world->angle_x_at_1c=0x12345678;
        world->angle_z_at_24=0xabcdef12;
        world->target_yaw=0xabcd8765;
        scene.task_collision=sadx::PositionResourceRef{};
        globals.forced_at_3b29d0c=0;
        forced_3b29d08=0;globals.forced_at_3b29d08=&forced_3b29d08;
        globals.stage_at_3b22dcc=0;
        player->player->source_flags_at_6=0;
        player->player->equipment_at_4=0;
        player->player->timer_at_7e=0;
        player->player->prior_attributes=0;
    }
    sadx::SonicPlayerFrame frame()  {
        return {object,task,world,player,&scene};
    }
    };
}
int main(int argc,char** argv)  {
    using namespace sadx;
    assert(argc==5);
    const auto bytes=read(argv[1]),dispatch=read(argv[2]);
    const auto k=decode_sonic_player_constants(bytes.data(),bytes.size());
    assert(k&&dispatch.size()==352);
    const auto tablebytes=read(argv[3]),unitbytes=read(argv[4]);
    assert(tablebytes.size()==4100&&unitbytes.size()==64);
    NinjaMath::SineTable table;
    NinjaMatrix unit;
    std::memcpy(table.data(),tablebytes.data(),4100);
    std::memcpy(unit.data(),unitbytes.data(),64);
    assert(!decode_sonic_player_constants(bytes.data(),51));
    for(unsigned a=0;a<256;++a)  {
        auto target=sonic_action_target(a);
        if(a>=88)assert(!target);
        else  {
            const auto b=dispatch.data()+4*a;
            const auto actual=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
            assert(target==actual);
        }
    }
    for(unsigned pc:{0U,0x200U,0x300U})for(unsigned rc:{0U,0x400U,0x800U,0xc00U})  {
        X87Context fp(pc|rc|0x7f);
        Fixture f;
        auto frame=f.frame();
        SonicPlayerRuntime rt{f.globals,f.scheduler,f.players,nullptr,nullptr,*k,{}};
        auto binding=bind_sonic_player_frame(f.object,*f.arena,f.world,f.players,&f.scene);
        stopped(binding.result,SonicPlayerFailure::null_required_resource,0x49a9c0);
        f.object->motion_work=f.arena->allocate(64).work;
        binding=bind_sonic_player_frame(f.object,*f.arena,f.world,f.players,&f.scene);
        stopped(binding.result,SonicPlayerFailure::work_binding_failed,0x49a9c0);
        assert(binding.result.work_failure);
        assert(f.arena->set_reference(f.object->motion_work,0,{}));
        binding=bind_sonic_player_frame(f.object,*f.arena,f.world,f.players,&f.scene);
        assert(binding&&binding.frame->player&&!*binding.frame->player&&binding.frame->task==f.task);
        assert(f.arena->set_reference(f.object->motion_work,0,f.player->player_work));
        binding=bind_sonic_player_frame(f.object,*f.arena,f.world,f.players,&f.scene);
        stopped(binding.result,SonicPlayerFailure::unresolved_resource,0x49a9c0);
        f.players.allocations[f.player->player_work.get()]=f.player;
        f.player->motion=f.world;
        binding=bind_sonic_player_frame(f.object,*f.arena,f.world,f.players,&f.scene);
        assert(binding&&binding.frame->player==f.player);
        binding=bind_sonic_player_frame(f.object,*f.arena,std::make_shared<PlayerMotionState>(),f.players,&f.scene);
        stopped(binding.result,SonicPlayerFailure::conflicting_live_binding,0x49a9bd);
        frame.player.reset();
        stopped(sonic_action_checks(frame,rt,fp),SonicPlayerFailure::unknown_player_binding,0x496f84);
        frame.player=std::shared_ptr<PlayerStartupPlayer>{};
        auto checks=sonic_action_checks(frame,rt,fp);
        assert(checks&&checks.value==0xffffffff);
        // pre-init null is a real closed branch
        f.task->task_id_at_2.reset();
        stopped(sonic_action_checks(frame,rt,fp),SonicPlayerFailure::unknown_field,0x492d70);
        f.task->task_id_at_2=2;
        f.task->control_timer_at_6=1;
        f.task->orientation.flags=0xffff;
        checks=sonic_action_checks(frame,rt,fp);
        assert(checks&&*f.task->control_timer_at_6==0&&f.task->orientation.flags==0xfffb);
        f.task->orientation.flags=0;
        f.task->control_timer_at_6=0;
        f.task->character_id=5;
        auto start=run_sonic_player(frame,rt,fp);
        stopped(start,SonicPlayerFailure::missing_native_owner,0x49ab3e);
        assert(start.callee_address==0x442750&&f.task->character_id==0);
        assert(f.globals.current_task_at_3abdf60==f.object->primary_work&&f.globals.current_player_at_3abdf64&&!*f.globals.current_player_at_3abdf64);
        frame.player=f.player;
        f.task->task_id_at_2=0;
        f.task->orientation.flags=4;
        auto timer=sonic_action_checks(frame,rt,fp);
        stopped(timer,SonicPlayerFailure::missing_native_owner,0x492da6);
        assert(timer.callee_address==0x4bcc10&&f.task->control_timer_at_6==120);
        f.task->orientation.flags=0;
        f.task->control_timer_at_6=55;
        f.player->player->source_flags_at_6=0x8000;
        checks=sonic_action_checks(frame,rt,fp);
        assert(checks&&f.task->orientation.flags==0x8000&&f.task->control_timer_at_6==0);
        f.player->player->source_flags_at_6=0;
        f.task->action_at_0=1;
        f.task->task_id_at_2=2;
        stopped(sonic_action_checks(frame,rt,fp),SonicPlayerFailure::unknown_field,0x495fac);
        f.task->action_at_0=86;
        f.task->orientation.flags=0x20;
        f.world->motion.speed.y=-1;
        f.scene.task_collision=PositionResourceRef{1};
        f.scene.works[1].owning_task=PositionResourceRef{2};
        f.scene.tasks[2].work=PositionResourceRef{3};
        auto& actor=f.scene.actors[3];
        actor.angle_x_at_14=0x12345678;
        auto carry=run_sonic_player(frame,rt,fp);
        stopped(carry,SonicPlayerFailure::unknown_field,0x49aa0f);
        assert(f.task->orientation.flags&2);
        assert(f.task->orientation.angles[0]==0x12345678);
        PlayerTaskState live{};
        live.orientation.angles={0x1111,0x2222,0x3333};
        actor.live_task=&live;
        f.scene.works[1].info=PositionResourceRef{4};
        f.scene.info_tables[4].records[0].damage=0xa0;
        f.scene.info_tables[4].records[1].attributes=0;
        f.task->orientation.flags=0x120;
        f.player->player->source_flags_at_6=0x8000;
        f.player->player->timer_at_7e=65535;
        carry=run_sonic_player(frame,rt,fp);
        stopped(carry,SonicPlayerFailure::unimplemented_action,0x49bdb4);
        assert(f.task->orientation.angles[0]==0x1111&&f.task->orientation.angles[2]==0x3333);
        assert(f.scene.info_tables[4].records[0].damage==0xaf&&f.scene.info_tables[4].records[1].attributes==0x10&&f.player->player->timer_at_7e==0);
        assert(f.globals.range_at_3c55d45==0);
        f.globals.forced_at_3b29d0c=1;
        f.forced_3b29d08.reset();
        stopped(run_sonic_player(frame,rt,fp),SonicPlayerFailure::unknown_field,0x4258f7);
        assert(f.task->action_at_0==86);
        // signed OR still reads both globals
        f.task->orientation.flags=0;
        f.player->parameters.reset();
        NinjaVector v{0,7,0};
        auto local=sonic_finalize_local_speed(frame,v,*k,{},fp);
        assert(local&&f.player->player->motion.speed.y==7&&f.player->player->dotp==1);
        // air suffix does not read parameters/priorattrs/gravity
        f.task->orientation.angles={static_cast<std::int32_t>(0xffff0123),0x76543210,0x1111abcd};
        f.world->angle_x_at_1c.reset();
        v={4,5,6};
        auto partial=sonic_finalize_local_speed(frame,v,*k,{},fp);
        stopped(partial,SonicPlayerFailure::unknown_field,0x43efc0);
        assert(f.player->player->motion.speed.x==4&&f.task->orientation.angles[0]==0x123&&f.task->orientation.angles[2]==0xabcd);
        assert(f.world->target_yaw==0x8765);
        f.world->angle_x_at_1c=0xdeadbeef;
        f.world->angle_z_at_24=0xcafebabe;
        f.task->orientation.flags=1;
        f.player->player->prior_attributes=4;
        f.player->player->attributes=0;
        f.player->player->floor_normal=NinjaVector{0,1,0};
        v={3,5,7};
        local=sonic_finalize_local_speed(frame,v,*k,NinjaVector{0,-1,0},fp);
        assert(local&&f.player->player->motion.speed.x==3&&f.player->player->motion.speed.y==0&&f.player->player->dotp==1);
        f.player->player->prior_attributes=0;
        stopped(sonic_finalize_local_speed(frame,v,*k,NinjaVector{0,-1,0},fp),SonicPlayerFailure::unknown_field,0x43eeef);
        NinjaMatrixStack matrix({unit,unit,unit},1,0);
        PlayerCoordinates coords(table,unit,matrix);
        PlayerAccelerationRuntime ar;
        ar.input.stage=0;
        ar.input.act=0;
        ar.input.current_character=0;
        ar.input.no_control_timer[0]=0;
        ar.input.normalized[0]=NormalizedPlayerInput{0x12345678,.5F};
        GroundAccelerationConstants gc{};
        GroundAccelerationCallees gcall;
        AirAccelerationConstants ac{};
        AirAccelerationCallees acall;
        FloorAngleConstants fc{};
        CrtAsinRuntime inverse;
        PlayerPositionConstants pc{};
        PlayerRotationConstants rotations{};
        PlayerPositionRuntime pr;
        PlayerPositionCallees pcall;
        SonicPlayerNativeMotion motion{coords,ar,gc,gcall,ac,acall,fc,inverse,pc,rotations,pr,pcall};
        f.task->orientation.flags=0x4000;
        auto absent=frame;
        absent.player.reset();
        absent.world.reset();
        assert(sonic_ground_input(absent,motion,*k,fp));
        assert(matrix.depth()==1);
        // source early return consumes only task flags
        f.task->orientation.flags=0;
        f.task->event_move=PlayerEventMoveData{};
        f.task->event_move->mode=0;
        auto heading=sonic_ground_input(frame,motion,*k,fp);
        stopped(heading,SonicPlayerFailure::missing_native_owner,0x44bd5a);
        assert(heading.callee_address==0x443c50&&f.world->target_yaw==0x12345678);
        f.task->event_move->mode=1;
        heading=sonic_ground_input(frame,motion,*k,fp);
        stopped(heading,SonicPlayerFailure::missing_native_owner,0x44bd5a);
        assert(f.world->target_yaw==0x12345678);
        // event-mode early GetAnalog path is guarded out by this owner's4000 return
        f.task->event_move->mode=0;
        f.player->player->motion.speed.x=0;
        ar.input.normalized[0]=NormalizedPlayerInput{0x12345678,0};
        f.task->orientation.flags=1;
        heading=sonic_ground_input(frame,motion,*k,fp);
        stopped(heading,SonicPlayerFailure::missing_native_owner,0x44bbed);
        assert(heading.callee_address==0x4491e0);
        f.task->orientation.flags=0x2000;
        heading=sonic_ground_input(frame,motion,*k,fp);
        stopped(heading,SonicPlayerFailure::missing_native_owner,0x44bb98);
        assert(heading.callee_address==0x43c130);
        f.task->orientation={0,{0,0,0}};
        f.player->parameters=PlayerParameters{};
        f.player->parameters->center_height=4;
        f.scene.task_collision=PositionResourceRef{1};
        f.scene.works[1].info=PositionResourceRef{4};
        f.task->position=NinjaVector{1,2,3};
        auto center=sonic_update_collision_center(frame,coords,fp);
        assert(center&&matrix.depth()==1);
        auto vcenter=*f.scene.info_tables[4].records[0].center;
        assert(vcenter.x==1&&vcenter.y==6&&vcenter.z==3);
        f.task->position.reset();
        center=sonic_update_collision_center(frame,coords,fp);
        stopped(center,SonicPlayerFailure::unknown_field,0x491638);
        vcenter=*f.scene.info_tables[4].records[0].center;
        assert(vcenter.x==0&&vcenter.y==4&&vcenter.z==0&&matrix.depth()==1);
        f.scene.task_collision=PositionResourceRef{};
        f.player->parameters.reset();
        center=sonic_update_collision_center(frame,coords,fp);
        stopped(center,SonicPlayerFailure::unknown_field,0x4915f6);
        f.player->parameters=PlayerParameters{};
        center=sonic_update_collision_center(frame,coords,fp);
        stopped(center,SonicPlayerFailure::null_required_resource,0x4915fd);
        f.world->motion.speed={3,4,5};
        f.player->player->motion.speed={};
        auto copied=sonic_copy_local_speed(frame,motion,*k,fp);
        assert(copied&&f.player->player->motion.speed.x==3&&f.player->player->motion.speed.y==4&&f.player->player->motion.speed.z==5&&matrix.depth()==1);
        // Known source animation owner cannot be bypassed by a callback.
        f.task->action_at_0=255;
        rt.external=[](const SonicPlayerCall&,SonicPlayerFrame&,X87Context&) {
            assert(false);return SonicPlayerResult{};
        };
        stopped(dispatch_sonic_player_action(frame,rt,fp),SonicPlayerFailure::missing_native_owner,0x49be0b);
        PlayerAnimationResources animation_resources;
        PlayerAnimationConstants animation_constants{0,1,4294967296.0F};
        PlayerAnimationRuntime animation_runtime{f.arena.get(),animation_resources,animation_constants};
        rt.animation_runtime=&animation_runtime;
        auto animation_table=std::make_shared<PlayerAnimationTable>();
        auto& row=animation_table->records[0];row.action=AnimationResourceReference{0};row.frame_step=0;
        animation_resources.actions[0].motion=AnimationResourceReference{0};
        animation_resources.motions[0].frame_count=10;
        auto& physics=*f.player->player;
        physics.current_animation_at_126=0;physics.requested_animation_at_124=0;
        physics.animation_mode_at_120=3;physics.animation_tail.table_at_140=animation_table;
        physics.animation_tail.frame_at_130=0;
        auto unavailable=dispatch_sonic_player_action(frame,rt,fp);
        stopped(unavailable,SonicPlayerFailure::animation_failed,0x49be0b);
        assert(unavailable.animation&&unavailable.animation->failure==PlayerAnimationFailure::unknown_field&&unavailable.nested_source_boundary==0x44a842);
        physics.animation_tail.timer_at_12c=0;
        auto advanced=dispatch_sonic_player_action(frame,rt,fp);
        stopped(advanced,SonicPlayerFailure::unimplemented_action,0x49be10);
        assert(physics.animation_tail.timer_at_12c==1&&physics.animation_tail.frame_at_130==0);
        // Successful animation does not certify omitted model/render tail.

    }
    std::cout<<"Sonic control dispatch and source availability tests passed across 12 explicit CW profiles\n";
}

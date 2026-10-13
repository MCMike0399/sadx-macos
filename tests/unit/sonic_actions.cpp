#include "player/sonic_player.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
namespace {
void stopped(const sadx::SonicPlayerResult& r,sadx::SonicPlayerFailure f,std::uint32_t address){if(r||r.failure!=f||r.source_boundary!=address)std::cerr<<"expected "<<unsigned(f)<<"@"<<std::hex<<address<<" got "<<unsigned(r.failure)<<"@"<<r.source_boundary<<"\n";assert(!r&&!r.completed&&r.failure==f&&r.source_boundary==address);}
struct Fixture {
    sadx::TaskScheduler scheduler;
    std::shared_ptr<sadx::TaskWorkArena> arena=std::make_shared<sadx::TaskWorkArena>();
    sadx::TaskHandle object=scheduler.append_task(0,&sadx::TaskScheduler::source_no_op).task;
    std::shared_ptr<sadx::PlayerTaskState> task=std::make_shared<sadx::PlayerTaskState>();
    std::shared_ptr<sadx::PlayerMotionState> world=std::make_shared<sadx::PlayerMotionState>();
    std::shared_ptr<sadx::PlayerStartupPlayer> record=std::make_shared<sadx::PlayerStartupPlayer>();
    sadx::PlayerPositionCollisionScene scene;
    sadx::SonicPlayerGlobals globals;
    sadx::GameLogicEnginePads pads;
    sadx::GameControllerButtonMasks masks;
    sadx::SonicPlayerControls controls{&pads,&masks};
    sadx::PlayerStartupRegistry players;
    sadx::NinjaMatrix unit{};
    sadx::NinjaMatrixStack stack{{unit,unit},1,0};
    sadx::PlayerCoordinates coordinates{{},unit,stack};
    sadx::PlayerAccelerationRuntime acceleration;
    sadx::GroundAccelerationConstants ground{};
    sadx::GroundAccelerationCallees ground_calls;
    sadx::AirAccelerationConstants air{};
    sadx::AirAccelerationCallees air_calls;
    sadx::FloorAngleConstants floor{};
    sadx::CrtAsinRuntime inverse;
    sadx::PlayerPositionConstants position{};
    sadx::PlayerRotationConstants rotation{};
    sadx::PlayerPositionRuntime position_runtime;
    sadx::PlayerPositionCallees position_calls;
    sadx::SonicPlayerNativeMotion motion{coordinates,acceleration,ground,ground_calls,air,air_calls,floor,inverse,position,rotation,position_runtime,position_calls};
    explicit Fixture(){object->primary_work=arena->allocate(64).work;object->primary_state=task;task->orientation={3,{}};task->action_at_0=1;task->secondary_mode_at_1=0;task->task_id_at_2=2;task->control_timer_at_6=0;task->event_move=sadx::PlayerEventMoveData{};task->event_move->mode=0;task->event_move->action_list_at_4c=sadx::PlayerEventActionListReference{};
        record->player=std::make_shared<sadx::PlayerPhysicsState>();record->parameters=sadx::PlayerParameters{};record->player_work=arena->allocate(512).work;auto& p=*record->player;p.source_flags_at_6=0;p.timer_at_7e=0;p.equipment_at_4=0;p.source_charge_at_0=2;p.requested_animation_at_124=14;p.jump_timer_at_8=12;p.idle_timer_at_e=12;p.moving_task_at_6c=sadx::TaskHandle{};p.held_task_at_68=sadx::TaskHandle{};p.contact_work=sadx::PlayerContactWork{};p.contact_work->direction_at_1c=sadx::NinjaVector{0,1,0};record->parameters->jmp_y_spd=6;record->parameters->jog_speed=1;record->parameters->run_speed=2;record->parameters->slide_speed=.1F;
        globals.stage_at_3b22dcc=0;globals.alternate_model_at_3b18db5=0;for(auto& field:globals.tasks_at_3c5a27c)field=sadx::TaskHandle{};
        pads.records[0].write_u32(8,0);pads.records[0].write_u32(0x10,0);masks.mask_at_909f94=2;masks.mask_at_909f98=4;masks.mask_at_909fa4=0x20;masks.mask_at_909fa8=0x40;masks.mask_at_909fac=0x80;
        acceleration.input.stage=0;acceleration.input.act=0;acceleration.input.current_character=0;acceleration.input.normalized[0]=sadx::NormalizedPlayerInput{0,0};acceleration.input.no_control_timer[0]=0;
        scene.task_collision=sadx::PositionResourceRef{1};scene.works[1].flags=0;scene.works[1].hit_work=sadx::PositionResourceRef{};scene.works[1].my_info_index=1;players.player_work[0]=record;players.motion_work[0]=world;
    }
    sadx::SonicPlayerFrame frame(){return {object,task,world,record,&scene};}
    sadx::SonicPlayerRuntime runtime(const sadx::SonicPlayerConstants& c){sadx::SonicPlayerRuntime result{globals,scheduler,players,nullptr,&motion,c,{}};result.controls=&controls;return result;}
};
}
int main(int argc,char** argv){using namespace sadx;assert(argc==2);std::ifstream file(argv[1],std::ios::binary);std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(file),{}};const auto constants=decode_sonic_player_constants(data.data(),data.size());assert(constants);
    for(unsigned pc:{0U,0x200U,0x300U})for(unsigned rc:{0U,0x400U,0x800U,0xc00U}){X87Context fp(pc|rc|0x7f);
        Fixture f;auto frame=f.frame();auto rt=f.runtime(*constants);auto& p=*f.record->player;
        auto result=sonic_action_checks(frame,rt,fp);assert(result&&f.task->action_at_0==1); // genuine neutral grounded path completes its predicate chain
        f.acceleration.input.normalized[0]=NormalizedPlayerInput{0,.5F};result=sonic_action_checks(frame,rt,fp);assert(result&&f.task->action_at_0==2&&p.requested_animation_at_124==9);
        f.task->action_at_0=1;f.acceleration.input.normalized[0]=NormalizedPlayerInput{0,0};p.contact_work->direction_at_1c=NinjaVector{0,0,0};p.motion.speed={3,0,0};result=sonic_action_checks(frame,rt,fp);assert(result&&f.task->action_at_0==19&&p.requested_animation_at_124==31);
        f.task->action_at_0=1;f.pads.records[0].write_u32(0x10,2);p.timer_at_7e=77;
        // Inspect the real five-slot native call contract, then fail the owner.
        bool saw_sound=false;rt.external=[&](const SonicPlayerCall& call,SonicPlayerFrame& current,X87Context&){assert(call.address==0x423f50&&call.call_site==0x496f33);assert(call.integer_arguments[0]==0x2ff&&!call.integer_arguments[1]&&call.integer_arguments[2]==1&&call.integer_arguments[3]==0&&call.integer_arguments[4]==2&&call.task_work_argument_index==1);assert(current.task==f.task);saw_sound=true;SonicPlayerResult r;r.failure=SonicPlayerFailure::native_owner_failed;return r;};
        result=sonic_action_checks(frame,rt,fp);stopped(result,SonicPlayerFailure::native_owner_failed,0x496f33);assert(saw_sound&&f.task->action_at_0==4&&p.requested_animation_at_124==14&&p.source_charge_at_0==3&&p.timer_at_7e==0&&(f.task->orientation.flags&0x500)==0x500);rt.external={};
        f.task->orientation.flags=3;f.task->action_at_0=1;f.pads.records[0].write_u32(0x10,4);result=sonic_action_checks(frame,rt,fp);stopped(result,SonicPlayerFailure::missing_native_owner,0x495eaa);assert(f.task->action_at_0==8&&p.motion.speed.y==6&&p.requested_animation_at_124==14&&p.source_charge_at_0==5&&p.jump_timer_at_8==0&&f.task->orientation.flags==0x501);
        f.pads.records[0].write_u32(0x10,0);f.task->orientation.flags=0;f.task->action_at_0=1;result=sonic_action_checks(frame,rt,fp);assert(result&&f.task->action_at_0==12&&p.requested_animation_at_124==18);
        f.task->orientation.flags=3;f.task->action_at_0=2;f.acceleration.input.normalized[0]=NormalizedPlayerInput{0,.5F};result=sonic_action_checks(frame,rt,fp);assert(result&&f.task->action_at_0==2);
        f.acceleration.input.normalized[0]=NormalizedPlayerInput{0,0};p.motion.speed.x=0;result=sonic_action_checks(frame,rt,fp);stopped(result,SonicPlayerFailure::missing_native_owner,0x49506b);assert(f.task->action_at_0==1&&p.idle_timer_at_e==0);
        f.task->action_at_0=3;f.task->orientation.flags=0x2503;p.motion.speed.x=.5F;result=sonic_action_checks(frame,rt,fp);assert(result&&f.task->action_at_0==2&&p.requested_animation_at_124==9&&f.task->orientation.flags==3);
        f.task->action_at_0=1;f.pads.records[0].forget(0xa,1);result=sonic_action_checks(frame,rt,fp);stopped(result,SonicPlayerFailure::unknown_control,0x492f9d);f.pads.records[0].write_u32(8,0);
        f.task->event_move->action_list_at_4c.reset();result=sonic_action_predicate(SonicPlayerPredicateEntry::event_action_495fa0,frame,rt,fp);stopped(result,SonicPlayerFailure::unknown_field,0x495fc7);
        f.task->event_move->action_list_at_4c=PlayerEventActionListReference{77};f.task->orientation.flags=0xffff;result=sonic_action_predicate(SonicPlayerPredicateEntry::event_action_495fa0,frame,rt,fp);assert(result&&result.value==1&&f.task->action_at_0==84&&f.task->orientation.flags==0xdaff);f.task->event_move->action_list_at_4c=PlayerEventActionListReference{};
        f.globals.alternate_model_at_3b18db5=1;p.requested_animation_at_124=14;assert(sonic_action_predicate(SonicPlayerPredicateEntry::request_spin_animation_495c30,frame,rt,fp));p.requested_animation_at_124=13;result=sonic_action_predicate(SonicPlayerPredicateEntry::request_spin_animation_495c30,frame,rt,fp);stopped(result,SonicPlayerFailure::missing_native_owner,0x495c51);assert(p.requested_animation_at_124==13);f.globals.alternate_model_at_3b18db5=0;
        f.task->orientation.flags=3;f.scene.works[1].hit_work=PositionResourceRef{2};f.scene.works[2].owning_task=PositionResourceRef{3};f.scene.tasks[3].work=PositionResourceRef{4};auto& actor=f.scene.actors[4];actor.task_id=8;actor.control_timer_at_6=1;actor.source_counter_at_c=0;PlayerTaskState live{};live.task_id_at_2=8;actor.live_task=&live;
        result=sonic_action_predicate(SonicPlayerPredicateEntry::jump_classify_4b83f0,frame,rt,fp);stopped(result,SonicPlayerFailure::unknown_field,0x4b8433);live.control_timer_at_6=1;result=sonic_action_predicate(SonicPlayerPredicateEntry::jump_classify_4b83f0,frame,rt,fp);stopped(result,SonicPlayerFailure::unknown_field,0x4b844e);live.source_counter_at_c=0;result=sonic_action_predicate(SonicPlayerPredicateEntry::jump_classify_4b83f0,frame,rt,fp);assert(result&&result.value==1);
        f.globals.tasks_at_3c5a27c[0]=f.object;f.task->action_at_0=7;f.globals.tasks_at_3c5a27c[1].reset();result=sonic_action_predicate(SonicPlayerPredicateEntry::clear_tasks_4b8320,frame,rt,fp);stopped(result,SonicPlayerFailure::unknown_field,0x4b832f);assert(f.task->action_at_0==0);f.globals.tasks_at_3c5a27c[1]=TaskHandle{};
        f.task->orientation.flags=3;f.pads.records[0].write_u32(8,2);p.timer_at_7e=1;p.equipment_at_4=0;p.source_charge_at_0=2;result=sonic_action_predicate(SonicPlayerPredicateEntry::charge_tick_495080,frame,rt,fp);stopped(result,SonicPlayerFailure::missing_native_owner,0x495286);assert(*p.source_charge_at_0>2); // arithmetic survives unavailable loop sound
        p.timer_at_7e=120;p.equipment_at_4=3;result=sonic_action_predicate(SonicPlayerPredicateEntry::charge_tick_495080,frame,rt,fp);stopped(result,SonicPlayerFailure::missing_native_owner,0x4950b6);assert(f.task->orientation.flags&0x200); // required real effect callback
        p.timer_at_7e=300;p.equipment_at_4=0;p.motion.speed.x=-1;result=sonic_action_predicate(SonicPlayerPredicateEntry::charge_tick_495080,frame,rt,fp);stopped(result,SonicPlayerFailure::missing_native_owner,0x495190);assert(f.task->action_at_0==1&&p.timer_at_7e==0&&p.requested_animation_at_124==29&&p.idle_timer_at_e==0);
        f.task->orientation.flags=0;auto absent=frame;absent.player=std::shared_ptr<PlayerStartupPlayer>{};result=sonic_action_predicate(SonicPlayerPredicateEntry::fall_494f70,absent,rt,fp);stopped(result,SonicPlayerFailure::null_required_resource,0x494fd5);assert(f.task->action_at_0==12);
        f.pads.records[0].write_u32(0x10,0);f.scene.works[1].hit_work=PositionResourceRef{};result=sonic_action_predicate(SonicPlayerPredicateEntry::jump_495e60,absent,rt,fp);assert(result&&result.value==0); // no-button branch does not consume nullable player pointer
    }
    std::cout<<"Grounded/running/jump/spindash predicate and partial-owner tests passed across12 explicit CW profiles\n";
}

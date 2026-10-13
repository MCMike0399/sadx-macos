#include "player/sonic_player.hpp"
#include <cstring>
namespace   {
    float scalar(std::uint32_t bits)  {
        float v;
        std::memcpy(&v,&bits,4);
        return v;
    }
    std::uint32_t bits(float v)  {
        std::uint32_t b;
        std::memcpy(&b,&v,4);
        return b;
    }
}
// Fully known captured facts. This probe never substitutes a successful
// animation/collision owner, and does not execute Sonic as a playable game.
extern "C" int sadx_native_sonic_player(const std::uint8_t* constants,std::size_t size,
const std::uint32_t* in,std::uint32_t* out)  {
    if(!in||!out)return 1;
    auto k=sadx::decode_sonic_player_constants(constants,size);
    if(!k)return 2;
    std::memset(out,0,40*4);
    sadx::X87Context fp(static_cast<std::uint16_t>(in[1]));
    sadx::TaskScheduler scheduler;
    auto object=scheduler.append_task(0,&sadx::TaskScheduler::source_no_op).task;
    auto arena=std::make_shared<sadx::TaskWorkArena>();
    object->primary_work=arena->allocate(64).work;
    auto task=std::make_shared<sadx::PlayerTaskState>();
    object->primary_state=task;
    task->orientation={static_cast<std::uint16_t>(in[2]),{static_cast<std::int32_t>(in[14]),static_cast<std::int32_t>(in[15]),static_cast<std::int32_t>(in[16])}};
    task->action_at_0=static_cast<std::uint8_t>(in[3]);
    task->task_id_at_2=static_cast<std::uint8_t>(in[4]);
    task->control_timer_at_6=static_cast<std::uint16_t>(in[5]);
    auto world=std::make_shared<sadx::PlayerMotionState>();
    world->motion.speed={scalar(in[11]),scalar(in[12]),scalar(in[13])};
    world->angle_x_at_1c=static_cast<std::int32_t>(in[17]);
    world->target_yaw=static_cast<std::int32_t>(in[18]);
    world->angle_z_at_24=static_cast<std::int32_t>(in[19]);
    auto player=std::make_shared<sadx::PlayerStartupPlayer>();
    player->player=std::make_shared<sadx::PlayerPhysicsState>();
    player->player_work=arena->allocate(512).work;
    player->parameters=sadx::PlayerParameters{};
    auto& p=*player->player;
    p.source_flags_at_6=static_cast<std::uint16_t>(in[6]);
    p.equipment_at_4=static_cast<std::uint16_t>(in[7]);
    p.timer_at_7e=static_cast<std::uint16_t>(in[8]);
    p.attributes=in[9];
    p.prior_attributes=in[10];
    p.floor_normal=sadx::NinjaVector{scalar(in[21]),scalar(in[22]),scalar(in[23])};
    player->parameters->run_accel=scalar(in[20]);
    p.motion.speed={scalar(0x42a20000),scalar(0x42a40000),scalar(0x42a60000)};
    p.dotp=scalar(0x42a80000);
    sadx::PlayerPositionCollisionScene scene;
    scene.task_collision=sadx::PositionResourceRef{in[30]?std::optional<std::size_t>(1):std::nullopt};
    scene.works[1].info=sadx::PositionResourceRef{2};
    scene.works[1].owning_task=sadx::PositionResourceRef{3};
    scene.tasks[3].work=sadx::PositionResourceRef{4};
    scene.actors[4].angle_x_at_14=static_cast<std::int32_t>(in[33]);
    scene.actors[4].angle_z_at_1c=static_cast<std::int32_t>(in[34]);
    scene.info_tables[2].records[0].damage=static_cast<std::uint8_t>(in[31]);
    scene.info_tables[2].records[1].attributes=in[32];
    sadx::SonicPlayerFrame frame{object,task,world,in[35]?player:std::shared_ptr<sadx::PlayerStartupPlayer>{},&scene};
    sadx::SonicPlayerGlobals globals;
    globals.stage_at_3b22dcc=static_cast<std::uint16_t>(in[27]);
    globals.forced_at_3b29d0c=static_cast<std::int16_t>(in[28]);
    std::optional<std::uint16_t> forced_3b29d08=static_cast<std::uint16_t>(in[29]);globals.forced_at_3b29d08=&forced_3b29d08;
    sadx::PlayerStartupRegistry registry;
    sadx::SonicPlayerRuntime rt{globals,scheduler,registry,nullptr,nullptr,*k,{}};
    sadx::SonicPlayerResult r;
    if(in[0]==0)  {
        auto v=world->motion.speed;
        r=sadx::sonic_finalize_local_speed(frame,v,*k,sadx::NinjaVector{scalar(in[24]),scalar(in[25]),scalar(in[26])},fp);
    }
    else if(in[0]==1)r=sadx::sonic_action_checks(frame,rt,fp);
    else if(in[0]==2)r=sadx::run_sonic_player(frame,rt,fp);
    else return 3;
    out[0]=static_cast<std::uint32_t>(r.failure);
    out[1]=r.source_boundary;
    out[2]=task->orientation.flags;
    out[3]=*task->action_at_0;
    out[4]=*task->control_timer_at_6;
    out[5]=*p.source_flags_at_6;
    out[6]=*p.timer_at_7e;
    out[7]=bits(p.motion.speed.x);
    out[8]=bits(p.motion.speed.y);
    out[9]=bits(p.motion.speed.z);
    for(std::size_t i=0;i<3;++i)out[10+i]=static_cast<std::uint32_t>(task->orientation.angles[i]);
    out[13]=static_cast<std::uint32_t>(*world->angle_x_at_1c);
    out[14]=static_cast<std::uint32_t>(world->target_yaw);
    out[15]=static_cast<std::uint32_t>(*world->angle_z_at_24);
    out[16]=bits(p.dotp);
    out[17]=fp.exception_flags();
    out[18]=fp.control_word();
    out[19]=globals.current_task_at_3abdf60.has_value();
    out[20]=globals.current_player_at_3abdf64.has_value();
    out[21]=globals.range_at_3c55d45.has_value();
    out[22]=globals.range_at_3c55d45.value_or(0);
    out[23]=*scene.info_tables[2].records[0].damage;
    out[24]=*scene.info_tables[2].records[1].attributes;
    out[25]=r.dispatch_target.value_or(0);
    out[26]=r.callee_address;
    out[27]=r.completed;
    out[28]=r.value.value_or(0);
    return 0;
}
// Prefix-only44bb60: all mandatory rotation/normal-projection owners are absent,
// so matrix/trig resources below are never consumed by the compared paths.
extern "C" int sadx_native_sonic_ground_input(const std::uint8_t* constants,std::size_t size,const std::uint32_t* in,std::uint32_t* out)  {
    if(!in||!out)return 1;
    auto k=sadx::decode_sonic_player_constants(constants,size);
    if(!k)return 2;
    sadx::X87Context fp(in[0]);
    auto task=std::make_shared<sadx::PlayerTaskState>();
    task->orientation={static_cast<std::uint16_t>(in[1]),{0,static_cast<std::int32_t>(in[12]),0}};
    task->event_move=sadx::PlayerEventMoveData{};
    task->event_move->mode=static_cast<std::uint8_t>(in[2]);
    auto world=std::make_shared<sadx::PlayerMotionState>();
    world->target_yaw=static_cast<std::int32_t>(in[10]);
    auto p=std::make_shared<sadx::PlayerStartupPlayer>();
    p->player=std::make_shared<sadx::PlayerPhysicsState>();
    p->player->attributes=in[8];
    p->player->motion.speed.x=scalar(in[9]);
    sadx::SonicPlayerFrame f{{},task,world,p,nullptr};
    sadx::PlayerAccelerationRuntime ar;
    ar.input.stage=static_cast<std::int16_t>(in[6]);
    ar.input.act=static_cast<std::int16_t>(in[7]);
    ar.input.current_character=0;
    ar.input.camera_yaw=static_cast<std::int32_t>(in[11]);
    ar.input.normalized[0]=sadx::NormalizedPlayerInput{static_cast<std::int32_t>(in[4]),scalar(in[3])};
    ar.input.no_control_timer[0]=static_cast<std::uint16_t>(in[5]);
    sadx::NinjaMatrix matrix{};
    sadx::NinjaMatrixStack stack({matrix,matrix},1,0);
    sadx::PlayerCoordinates coords({},matrix,stack);
    sadx::GroundAccelerationConstants gc{};
    sadx::GroundAccelerationCallees gcall;
    sadx::AirAccelerationConstants ac{};
    sadx::AirAccelerationCallees acall;
    sadx::FloorAngleConstants fc{};
    sadx::CrtAsinRuntime inverse;
    sadx::PlayerPositionConstants pc{};
    sadx::PlayerRotationConstants rc{};
    sadx::PlayerPositionRuntime pr;
    sadx::PlayerPositionCallees pcall;
    sadx::SonicPlayerNativeMotion motion{coords,ar,gc,gcall,ac,acall,fc,inverse,pc,rc,pr,pcall};
    auto result=sadx::sonic_ground_input(f,motion,*k,fp);
    out[0]=static_cast<std::uint32_t>(result.failure);
    out[1]=result.source_boundary;
    out[2]=result.callee_address;
    out[3]=static_cast<std::uint32_t>(world->target_yaw);
    out[4]=fp.exception_flags();
    out[5]=fp.control_word();
    out[6]=task->orientation.flags;
    out[7]=result.completed;
    return 0;
}

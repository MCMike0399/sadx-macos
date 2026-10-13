#include "player/player_acceleration_air.hpp"
#include <cstddef>
#include <cstring>
namespace {
float value(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
sadx::NinjaVector vec(const std::uint32_t* w){return {value(w[0]),value(w[1]),value(w[2])};}
}
extern "C" int sadx_native_air_acceleration(const std::uint8_t* table,std::size_t ts,
    const std::uint8_t* unit,std::size_t us,const std::uint8_t* constants,std::size_t cs,
    const std::uint32_t* w,std::size_t wc,std::uint16_t cw,std::uint32_t* out,std::size_t oc){
    if(!table||ts!=4100||!unit||us!=64||!constants||cs!=88||!w||wc!=63||!out||oc!=74)return 1;
    try{
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,ts);
        sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,us);
        sadx::NinjaMatrixStack stack({identity,identity,identity},1,0);
        sadx::PlayerCoordinates coordinates(sine,identity,stack);
        sadx::PlayerRotationConstants c;std::memcpy(c.float_bits.data(),constants,64);std::memcpy(c.double_bits.data(),constants+64,24);
        const auto p=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(w+30),132);if(!p)return 2;
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(w[0]),{integer(w[1]),integer(w[2]),integer(w[3])}},0,0,{},{}};
        sadx::PlayerMotionState world{{vec(w+12),vec(w+15)},integer(w[8]),value(w[9]),value(w[10]),value(w[11])};
        sadx::PlayerPhysicsState player{{vec(w+21),vec(w+18)},value(w[6]),{0,0,0},{},w[7],0,sadx::PlayerContactWork{vec(w+27)}};
        sadx::PlayerAccelerationRuntime runtime;runtime.world_gravity=vec(w+24);
        runtime.input.stage=1;runtime.input.no_control_timer[0]=0;runtime.input.normalized[0]=sadx::NormalizedPlayerInput{integer(w[4]),value(w[5])};
        sadx::PlayerRotationRuntime rotation;rotation.input=runtime.input;rotation.passed_player_no_control_timer=0;
        sadx::AirAccelerationCallees callees;std::uint32_t calls=0;
        callees.world_to_player=[&](const auto& t,auto& v,auto& fp){return coordinates.world_to_player(t,v,fp);};
        callees.yaw_regular=[&](auto& t,auto& m,auto& v,std::int32_t target,auto& fp){++calls;return sadx::rotate_player_regular(t,m,v,target,c,rotation,coordinates,fp);};
        sadx::X87Context fp(cw);const auto result=sadx::get_player_acceleration_air(task,world,player,*p.parameters,{c.single(0x7dcb2c)},runtime,callees,fp);
        const auto a=player.motion.acceleration,s=player.motion.speed;
        out[0]=bits(a.x);out[1]=bits(a.y);out[2]=bits(a.z);out[3]=bits(s.x);out[4]=bits(s.y);out[5]=bits(s.z);
        out[6]=static_cast<std::uint32_t>(world.target_yaw);
        for(std::size_t i=0;i<3;++i)out[7+i]=static_cast<std::uint32_t>(task.orientation.angles[i]);
        out[10]=calls;out[11]=stack.depth();out[12]=fp.exception_flags();out[13]=fp.control_word();out[14]=static_cast<std::uint32_t>(result.acceleration.failure);
        out[15]=bits(world.source_field_34);out[16]=bits(world.source_field_38);out[17]=bits(world.friction_scale);
        out[18]=bits(world.motion.speed.x);out[19]=bits(world.motion.speed.y);out[20]=bits(world.motion.speed.z);
        out[21]=bits(world.motion.acceleration.x);out[22]=bits(world.motion.acceleration.y);out[23]=bits(world.motion.acceleration.z);
        out[24]=task.orientation.flags;out[25]=player.attributes;
        // All matrix slots, including popped source writes, remain observable.
        for(std::size_t i=0;i<3;++i)for(std::size_t j=0;j<16;++j)out[26+i*16+j]=bits(stack.slots()[i][j]);
        return 0;
    }catch(...){return 3;}
}

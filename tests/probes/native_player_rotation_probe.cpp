#include "player/player_rotation.hpp"
#include <cstddef>
#include <cstring>
#include <utility>

namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
extern "C" int sadx_native_player_rotation(const std::uint8_t* table,std::size_t table_size,
    const std::uint8_t* unit,std::size_t unit_size,const std::uint8_t* constants,std::size_t constants_size,
    const std::uint32_t* w,std::size_t count,std::uint16_t cw,std::uint32_t* out,std::size_t out_count) {
    if(!table||table_size!=4100||!unit||unit_size!=64||!constants||constants_size!=88||
       !w||count!=154||!out||out_count!=107||w[0]>3)return 1;
    try {
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);
        sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::PlayerRotationConstants c;std::memcpy(c.float_bits.data(),constants,64);
        std::memcpy(c.double_bits.data(),constants+64,24);
        std::vector<sadx::NinjaMatrix> frames(6);
        for(std::size_t r=0;r<6;++r)for(std::size_t i=0;i<16;++i)frames[r][i]=number(w[58+r*16+i]);
        sadx::NinjaMatrixStack stack(std::move(frames),w[56],w[57]);
        sadx::PlayerCoordinates coordinates(sine,identity,stack);
        const auto parameters=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(w+23),132);
        if(!parameters)return 2;
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(w[1]),{integer(w[2]),integer(w[3]),integer(w[4])}},0,0,{}, {}};
        sadx::PlayerMotionState motion{{vector(w+13),{0,0,0}},integer(w[16]),0,0,number(w[17])};
        sadx::PlayerPhysicsState player{{{0,0,0},vector(w+6)},number(w[9]),{0,0,0},{},w[10],0,{}};
        sadx::PlayerRotationRuntime runtime;
        if(w[11])runtime.passed_player_no_control_timer=static_cast<std::uint16_t>(w[12]);
        if(w[22])runtime.world_gravity=vector(w+18);
        runtime.input.stage=1;runtime.input.no_control_timer[0]=static_cast<std::uint16_t>(w[12]);
        runtime.input.normalized[0]=sadx::NormalizedPlayerInput{{},number(w[21])};
        sadx::X87Context fp(cw);sadx::PlayerRotationResult result;
        switch(w[0]) {
        case 0: result=sadx::rotate_player_regular(task,motion,player,integer(w[5]),c,runtime,coordinates,fp);break;
        case 1: result=sadx::rotate_player_blended(task,player,*parameters.parameters,integer(w[5]),c,coordinates,fp);break;
        case 2: result=sadx::rotate_player_passive(task,motion,player,*parameters.parameters,c,runtime,coordinates,fp);break;
        case 3: result=sadx::rotate_player_attribute(task,motion,player,*parameters.parameters,c,runtime,coordinates,fp);break;
        }
        for(std::size_t i=0;i<3;++i)out[i]=static_cast<std::uint32_t>(task.orientation.angles[i]);
        out[3]=bits(player.motion.speed.x);out[4]=bits(player.motion.speed.y);out[5]=bits(player.motion.speed.z);
        out[6]=static_cast<std::uint32_t>(motion.target_yaw);out[7]=stack.depth();out[8]=fp.exception_flags();
        out[9]=static_cast<std::uint32_t>(result.failure);out[10]=result.source_boundary;
        for(std::size_t r=0;r<6;++r)for(std::size_t i=0;i<16;++i)out[11+r*16+i]=bits(stack.slots()[r][i]);
        return 0;
    } catch(const sadx::X87Error&){return 3;} catch(const sadx::CoordinateError&){return 4;}
}

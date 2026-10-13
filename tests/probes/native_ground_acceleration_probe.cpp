// Integration comparison adapter: real native coordinate/input/cross helpers,
// explicitly controlled rotation mutation boundaries, never gameplay defaults.
#include "player/player_acceleration_ground.hpp"
#include "player/player_heading.hpp"
#include <cstring>

namespace {
float number(std::uint32_t word){float result;std::memcpy(&result,&word,4);return result;}
std::uint32_t bits(float value){std::uint32_t result;std::memcpy(&result,&value,4);return result;}
std::int32_t integer(std::uint32_t word){std::int32_t result;std::memcpy(&result,&word,4);return result;}
sadx::NinjaVector vector(const std::uint32_t* words){return {number(words[0]),number(words[1]),number(words[2])};}
void store(std::uint32_t* words,const sadx::NinjaVector& value){words[0]=bits(value.x);words[1]=bits(value.y);words[2]=bits(value.z);}
}
extern "C" const char* sadx_native_ground_acceleration_compiler(){
#if defined(__clang__) && defined(__aarch64__)
    return "Clang " __clang_version__ "; target arm64; native integer x87 arithmetic";
#else
    return "Unsupported compiler/target";
#endif
}
extern "C" int sadx_native_ground_acceleration(const std::uint8_t* table,std::size_t table_size,
    const std::uint8_t* unit,std::size_t unit_size,const std::uint32_t* constants,std::size_t constant_count,
    const std::uint32_t* words,std::size_t word_count,std::uint16_t cw,std::uint32_t* output,std::size_t output_count) {
    if(!table || table_size!=4100 || !unit || unit_size!=64 || !constants || constant_count!=24 ||
       !words || word_count!=63 || !output || output_count!=43)return 1;
    std::memset(output,0,43*4);
    try {
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,table_size);
        sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,unit_size);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3,identity),1,0);
        sadx::PlayerCoordinates coordinates(sine,identity,stack);
        sadx::GroundAccelerationConstants values;std::memcpy(values.bits.data(),constants,24*4);
        const auto parameters=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(words+30),132);
        if(!parameters)return 2;
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(words[0]),
            {integer(words[1]),integer(words[2]),integer(words[3])}},0,0,{}};
        sadx::PlayerMotionState world{{vector(words+12),vector(words+15)},integer(words[8]),
            number(words[9]),number(words[10]),number(words[11])};
        sadx::PlayerPhysicsState player{{vector(words+21),vector(words+18)},number(words[6]),
            {0,0,0},{},words[7],0,sadx::PlayerContactWork{vector(words+27)}};
        sadx::PlayerAccelerationRuntime runtime;runtime.world_gravity=vector(words+24);
        runtime.input.stage=1;runtime.input.no_control_timer[0]=0;
        runtime.input.normalized[0]=sadx::NormalizedPlayerInput{integer(words[4]),number(words[5])};
        sadx::GroundAccelerationCallees callees;
        callees.world_to_player=[&](const auto& orientation,auto& value,auto& fp){return coordinates.world_to_player(orientation,value,fp);};
        std::vector<std::uint32_t> calls;
        const auto control=[&](std::uint32_t target){return [&,target](auto&,auto&,auto&,const auto&,auto&){
            calls.push_back(target);return sadx::PlayerAccelerationResult{sadx::PlayerAccelerationFailure::none};};};
        const auto yaw=[&](std::uint32_t target){return [&,target](auto&,auto&,auto&,const auto&,std::int32_t,auto&){
            calls.push_back(target);return sadx::PlayerAccelerationResult{sadx::PlayerAccelerationFailure::none};};};
        callees.passive_rotation=control(0x4491e0);callees.direction_reset=control(0x43c130);
        callees.attribute_rotation=control(0x449380);
        callees.yaw_fast=[&](auto& selected_task,auto&,auto& selected_player,const auto&,std::int32_t target,auto& fp){
            calls.push_back(0x443df0);
            const auto result=sadx::turn_player_heading(selected_task.orientation,selected_player.motion,target,coordinates,fp);
            return sadx::PlayerAccelerationResult{result ? sadx::PlayerAccelerationFailure::none : sadx::PlayerAccelerationFailure::coordinate_unavailable};
        };
        callees.yaw_blended=yaw(0x443e60);callees.yaw_regular=yaw(0x443c50);
        sadx::X87Context fp(cw);
        const auto result=sadx::get_player_acceleration_ground(task,world,player,*parameters.parameters,values,runtime,callees,fp);
        store(output,player.motion.acceleration);store(output+3,player.motion.speed);store(output+6,world.motion.speed);
        std::memcpy(output+9,&world.target_yaw,4);output[10]=bits(world.source_field_34);output[11]=bits(world.source_field_38);
        output[12]=static_cast<std::uint32_t>(result.failure);output[13]=result.source_boundary;output[14]=result.callee_address;
        output[15]=fp.exception_flags();output[16]=fp.control_word();output[17]=calls.size();
        for(std::size_t i=0;i<calls.size() && i<16;++i)output[18+i]=calls[i];
        if(result.temporary_acceleration){output[34]=1;store(output+35,*result.temporary_acceleration);}
        output[38]=static_cast<std::uint32_t>(result.input_failure);output[39]=static_cast<std::uint32_t>(result.coordinate_failure);
        std::memcpy(output+40,task.orientation.angles.data(),12);
        return 0;
    } catch(const sadx::X87Error&){return 3;}catch(...){return 4;}
}

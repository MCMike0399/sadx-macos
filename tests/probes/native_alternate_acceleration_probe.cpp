#include "player/player_acceleration_alternate.hpp"
#include "player/player_acceleration_alternate_math.hpp"
#include "player/player_rotation.hpp"
#include <cstring>

namespace {
float number(std::uint32_t word){float result;std::memcpy(&result,&word,4);return result;}
std::uint32_t bits(float value){std::uint32_t result;std::memcpy(&result,&value,4);return result;}
std::int32_t integer(std::uint32_t word){std::int32_t result;std::memcpy(&result,&word,4);return result;}
sadx::NinjaVector vector(const std::uint32_t* words){return {number(words[0]),number(words[1]),number(words[2])};}
void store(std::uint32_t* words,const sadx::NinjaVector& value){words[0]=bits(value.x);words[1]=bits(value.y);words[2]=bits(value.z);}
sadx::PlayerAccelerationResult rotation_result(const sadx::PlayerRotationResult& rotation){
    sadx::PlayerAccelerationResult result;result.rotation=rotation;
    if(!rotation){result.failure=sadx::PlayerAccelerationFailure::callee_unavailable;result.source_boundary=rotation.source_boundary;
        result.coordinate_failure=rotation.coordinate_failure;result.input_failure=rotation.input_failure;
        result.nested_source_boundary=rotation.nested_source_boundary;result.temporary_acceleration=rotation.temporary_vector;}
    return result;
}
}
extern "C" int sadx_native_event_vector_add(const std::uint32_t* destination,
    const std::uint32_t* other,std::uint16_t cw,std::uint32_t* output) {
    if(!destination || !other || !output)return 1;
    sadx::X87Context fp(cw);auto result=vector(destination);
    sadx::event_vector_add(result,vector(other),fp);store(output,result);
    output[3]=fp.exception_flags();return 0;
}
extern "C" const char* sadx_native_alternate_acceleration_compiler(){
#if defined(__clang__) && defined(__aarch64__)
    return "Clang " __clang_version__ "; target arm64; source-specific software x87";
#else
    return "Unsupported compiler/target profile";
#endif
}
extern "C" int sadx_native_alternate_acceleration(const std::uint8_t* table,std::size_t table_size,
    const std::uint8_t* unit,std::size_t unit_size,const std::uint32_t* constants,std::size_t constant_count,
    const std::uint8_t* rotation_bytes,std::size_t rotation_size,
    const std::uint8_t* event_bytes,std::size_t event_size,
    const std::uint8_t* projection_bytes,std::size_t projection_size,
    const std::uint32_t* words,std::size_t word_count,std::uint16_t cw,std::uint32_t* output,std::size_t output_count) {
    if(!table || table_size!=4100 || !unit || unit_size!=64 || !constants || constant_count!=24 ||
       !rotation_bytes || rotation_size!=88 || !event_bytes || event_size!=20 ||
       !projection_bytes || projection_size!=28 || !words || word_count!=80 || !output || output_count!=58)return 1;
    std::memset(output,0,58*4);
    try {
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,table_size);
        sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,unit_size);
        sadx::NinjaMatrixStack stack(std::vector<sadx::NinjaMatrix>(3,identity),1,0);
        sadx::PlayerCoordinates coordinates(sine,identity,stack);
        sadx::GroundAccelerationConstants values;std::memcpy(values.bits.data(),constants,96);
        sadx::PlayerRotationConstants rotation;std::memcpy(rotation.float_bits.data(),rotation_bytes,64);
        std::memcpy(rotation.double_bits.data(),rotation_bytes+64,24);
        sadx::EventAccelerationConstants event_constants;
        std::memcpy(&event_constants.target_stop_distance,event_bytes,4);
        std::memcpy(&event_constants.angle_units,event_bytes+4,8);
        std::memcpy(&event_constants.positive_factor,event_bytes+12,8);
        sadx::CoordinateAxisConstants axis;std::memcpy(&axis.zero,projection_bytes,4);std::memcpy(&axis.one,projection_bytes+4,4);
        sadx::ProjectionAngleConstants projection_constants;std::memcpy(&projection_constants.negative_one,projection_bytes+8,4);
        std::memcpy(&projection_constants.first_factor,projection_bytes+12,8);std::memcpy(&projection_constants.second_factor,projection_bytes+20,8);
        const auto parameters=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(words+30),132);
        if(!parameters)return 2;
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(words[0]),
            {integer(words[1]),integer(words[2]),integer(words[3])}},0,0,{}, {}};
        if(words[63]==1 || words[63]==2)task.event_move=sadx::PlayerEventMoveData{static_cast<std::uint8_t>(words[64]),vector(words+65),number(words[68]),number(words[69])};
        task.position=vector(words+70);
        sadx::PlayerMotionState world{{vector(words+12),vector(words+15)},integer(words[8]),
            number(words[9]),number(words[10]),number(words[11])};
        sadx::PlayerPhysicsState player{{vector(words+21),vector(words+18)},number(words[6]),
            {0,0,0},{},words[7],0,sadx::PlayerContactWork{vector(words+27)}};
        player.floor_normal=vector(words+77);
        sadx::PlayerAccelerationRuntime runtime;runtime.world_gravity=vector(words+24);
        runtime.input.stage=1;runtime.input.no_control_timer[0]=0;
        runtime.input.normalized[0]=sadx::NormalizedPlayerInput{integer(words[4]),number(words[5])};
        sadx::PlayerRotationRuntime rotation_runtime;rotation_runtime.input=runtime.input;
        rotation_runtime.passed_player_no_control_timer=0;rotation_runtime.world_gravity=runtime.world_gravity;
        sadx::GroundAccelerationCallees callees;std::vector<std::uint32_t> calls;
        callees.world_to_player=[&](const auto& orientation,auto& value,auto& fp){
            if(words[63]==1 || words[63]==2)store(output+45,value);
            const auto result=coordinates.world_to_player(orientation,value,fp);
            if(words[63]==1 || words[63]==2)store(output+48,value);
            return result;};
        callees.yaw_fast=[&](auto& t,auto&,auto& p,const auto&,std::int32_t target,auto& fp){
            calls.push_back(0x443df0);
            const auto heading=sadx::turn_player_heading(t.orientation,p.motion,target,coordinates,fp);sadx::PlayerAccelerationResult result;
            if(!heading){result.failure=sadx::PlayerAccelerationFailure::coordinate_unavailable;result.source_boundary=heading.source_boundary;
                result.coordinate_failure=heading.coordinate.failure;result.temporary_acceleration=heading.temporary_vector;}return result;};
        callees.yaw_regular=[&](auto& t,auto& w,auto& p,const auto&,std::int32_t target,auto& fp){
            calls.push_back(0x443c50);
            return rotation_result(sadx::rotate_player_regular(t,w,p,target,rotation,rotation_runtime,coordinates,fp));};
        callees.yaw_blended=[&](auto& t,auto&,auto& p,const auto& parameters,std::int32_t target,auto& fp){
            calls.push_back(0x443e60);
            return rotation_result(sadx::rotate_player_blended(t,p,parameters,target,rotation,coordinates,fp));};
        callees.passive_rotation=[&](auto& t,auto& w,auto& p,const auto& parameters,auto& fp){
            calls.push_back(0x4491e0);
            return rotation_result(sadx::rotate_player_passive(t,w,p,parameters,rotation,rotation_runtime,coordinates,fp));};
        sadx::X87Context fp(cw);
        sadx::PlayerAccelerationResult result;
        if(words[63]==0)result=sadx::get_player_acceleration_attribute_4000(task,world,player,*parameters.parameters,values,runtime,callees,fp);
        else if(words[63]==1)result=sadx::get_player_acceleration_event_ground(task,world,player,*parameters.parameters,values,event_constants,runtime,callees,fp);
        else if(words[63]==2)result=sadx::get_player_acceleration_event_air(task,world,player,*parameters.parameters,values,event_constants,runtime,callees,fp);
        else if(words[63]==3) {
            sadx::AlternateSurfaceRuntime surface;
            std::uint16_t xword=words[73],yword=words[74];std::int16_t x,y;std::memcpy(&x,&xword,2);std::memcpy(&y,&yword,2);
            surface.raw_axes[0]=std::array<std::int16_t,2>{x,y};surface.camera_pitch_yaw=std::array<std::int32_t,2>{integer(words[75]),integer(words[76])};
            const sadx::CrtAsinRuntime crt{1,{}}; // Explicit source-dispatcher bypass test seed.
            result=sadx::get_player_acceleration_attribute_20000(task,world,player,*parameters.parameters,values,event_constants,runtime,surface,callees,coordinates,axis,projection_constants,crt,fp);
        }
        else return 5;
        store(output,player.motion.acceleration);store(output+3,player.motion.speed);store(output+6,world.motion.speed);
        std::memcpy(output+9,&world.target_yaw,4);output[10]=bits(world.source_field_34);output[11]=bits(world.source_field_38);
        output[12]=static_cast<std::uint32_t>(result.failure);output[13]=result.source_boundary;output[14]=result.callee_address;
        output[15]=fp.exception_flags();output[16]=fp.control_word();output[17]=calls.size();
        for(std::size_t i=0;i<calls.size() && i<16;++i)output[18+i]=calls[i];
        if(result.temporary_acceleration){output[34]=1;store(output+35,*result.temporary_acceleration);}
        output[38]=static_cast<std::uint32_t>(result.input_failure);output[39]=static_cast<std::uint32_t>(result.coordinate_failure);
        std::memcpy(output+40,task.orientation.angles.data(),12);
        output[43]=task.orientation.flags;output[44]=task.event_move && task.event_move->mode ? *task.event_move->mode : 0x100U;
        if(result.floor_projection) {
            output[45]=1;
            if(result.floor_projection->camera_direction)store(output+46,*result.floor_projection->camera_direction);
            if(result.floor_projection->projected_direction)store(output+49,*result.floor_projection->projected_direction);
            if(result.floor_projection->forward_direction)store(output+52,*result.floor_projection->forward_direction);
        }
        if(result.projection && result.projection->angle){output[55]=1;std::memcpy(output+56,&*result.projection->angle,4);output[57]=bits(result.projection->stored_dot);}
        return 0;
    }catch(const sadx::X87Error&){return 3;}catch(...){return 4;}
}

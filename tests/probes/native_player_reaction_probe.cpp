#include "player/player_reaction.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
std::uint32_t word(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
void store(sadx::NinjaVector v,std::uint32_t* w){w[0]=word(v.x);w[1]=word(v.y);w[2]=word(v.z);}
}
// 21 input words: flags,yaw,target,speedXYZ,seedNormalXYZ,floorXYZ,
// initialPlayer44XYZ,collisionFlags,normalMode,floorKnown,CRTpolicyMode,
// collisionReferenceMode,motionPresent. Constant blob is40 exact source bytes.
extern "C" int sadx_native_player_reaction(const std::uint8_t* constants,std::size_t constant_size,
    const std::uint32_t* w,std::size_t input_size,std::uint16_t cw,
    std::uint32_t* out,std::size_t output_size){
    if(!constants||constant_size!=40||!w||input_size!=21||!out||output_size!=40)return 1;
    std::memset(out,0,40*4);
    try{
        const auto decoded=sadx::decode_player_reaction_constants(constants,constant_size);
        if(!decoded)return 3;
        const auto& c=*decoded;
        sadx::PlayerTaskState task{};task.orientation.flags=static_cast<std::uint16_t>(w[0]);task.orientation.angles[1]=integer(w[1]);
        sadx::PlayerMotionState world{};world.motion.speed=vector(w+3);world.target_yaw=integer(w[2]);
        sadx::PlayerPhysicsState player{};player.source_vector_at_44=vector(w+12);
        if(w[17])player.floor_normal=vector(w+9);
        sadx::PlayerPositionCollisionScene scene;
        if(w[19]!=1)scene.task_collision=sadx::PositionResourceRef{w[19]==2?std::optional<std::size_t>{}:std::optional<std::size_t>{1}};
        if(w[19]!=3){auto& work=scene.works[1];if(w[19]!=4)work.flags=static_cast<std::uint16_t>(w[15]);}
        sadx::CrtAsinRuntime runtime;if(w[18])runtime.matherr_control=w[18]-1;
        decltype(sadx::PlayerPositionCallees::normal) normal;
        if(w[16])normal=[&](auto&,auto&,auto&){
            ++out[9];
            if(w[16]==2)return sadx::PositionNormalResult{sadx::PlayerPositionFailure::normal_owner_failed,{},0x41bed0};
            if(w[16]==3)return sadx::PositionNormalResult{sadx::PlayerPositionFailure::none,{},0};
            return sadx::PositionNormalResult{sadx::PlayerPositionFailure::none,vector(w+6),0};
        };
        sadx::X87Context fp(cw);
        const auto result=sadx::react_player_to_contact(task,w[20]?&world:nullptr,player,scene,c,runtime,normal,fp);
        out[0]=task.orientation.flags;out[1]=static_cast<std::uint32_t>(task.orientation.angles[1]);out[2]=static_cast<std::uint32_t>(world.target_yaw);
        store(world.motion.speed,out+3);store(*player.source_vector_at_44,out+6);
        out[10]=static_cast<std::uint32_t>(result.failure);out[11]=result.source_boundary;out[12]=result.nested_source_boundary;
        if(result.angle){const auto& a=*result.angle;out[13]=1;out[14]=static_cast<std::uint32_t>(a.failure);out[15]=static_cast<std::uint32_t>(a.crt_failure);out[39]=a.nested_source_boundary;
            out[16]=a.angle.has_value();if(a.angle)out[17]=static_cast<std::uint32_t>(*a.angle);out[18]=word(a.stored_dot);
            if(a.cross){out[19]=1;store(*a.cross,out+20);}
            if(a.pending_stored_result){out[32]=1;std::memcpy(out+33,&*a.pending_stored_result,8);}
            if(a.pending_value){out[35]=1;out[36]=static_cast<std::uint32_t>(a.pending_value->significand());
                out[37]=static_cast<std::uint32_t>(a.pending_value->significand()>>32);out[38]=a.pending_value->sign_exponent();}
        }
        out[23]=fp.exception_flags();out[24]=fp.control_word();
        if(result.intersection){out[25]=1;store(result.intersection->point,out+26);store(result.intersection->normal,out+29);}
        return 0;
    }catch(const sadx::X87Error&){return 2;}
}

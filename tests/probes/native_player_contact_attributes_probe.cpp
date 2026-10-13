#include "player/player_contact_attributes.hpp"
#include <cstring>
namespace {
float number(std::uint32_t word){float value;std::memcpy(&value,&word,4);return value;}
std::uint32_t bits(float value){std::uint32_t word;std::memcpy(&word,&value,4);return word;}
}
extern "C" int sadx_native_contact_attributes(const std::uint8_t* bytes,const std::uint32_t* w,
        std::uint16_t cw,std::uint32_t* out){
    if(!bytes||!w||!out||w[0]>255)return 1;
    const auto c=sadx::decode_contact_attribute_constants(bytes,40);if(!c)return 1;
    sadx::PlayerTaskState task{};task.character_id=w[0];
    sadx::PlayerPhysicsState player{};player.shadow_attributes_top=w[1];player.attributes=w[2];
    sadx::PlayerParameters live{};live.weight=number(w[4]);live.air_resist_y=number(w[5]);live.run_accel=number(w[6]);
    sadx::PlayerMotionState world{};world.friction_scale=number(w[7]);
    sadx::ContactParameterTable defaults{};auto& row=defaults[task.character_id];
    if(w[11]&1)row.weight=number(w[8]);if(w[11]&2)row.air_resist_y=number(w[9]);if(w[11]&4)row.run_accel=number(w[10]);
    try{
        sadx::X87Context fp(cw);const auto result=sadx::apply_player_contact_attributes(task,w[12]?&world:nullptr,
            player,live,defaults,w[3],*c,fp);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=player.attributes;
        out[3]=bits(live.weight);out[4]=bits(live.air_resist_y);out[5]=bits(live.run_accel);out[6]=bits(world.friction_scale);
        out[7]=fp.exception_flags();out[8]=fp.control_word();return 0;
    }catch(const sadx::X87Error&){return 2;}
}

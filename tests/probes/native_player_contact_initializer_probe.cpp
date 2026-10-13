#include "player/player_contact_initializer.hpp"
#include <cstring>

namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
extern "C" int sadx_native_player_contact_initializer(const std::uint32_t* constants,
    const std::uint8_t* rotation,const std::uint32_t* words,std::uint16_t cw,std::uint32_t* output) {
    if(!constants || !rotation || !words || !output)return -1;
    try {
        sadx::PlayerContactInitializationConstants c;std::memcpy(c.bits.data(),constants,52);
        sadx::PlayerRotationConstants rc;std::memcpy(rc.float_bits.data(),rotation,64);std::memcpy(rc.double_bits.data(),rotation+64,24);
        const auto parameters=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(words+17),132);
        if(!parameters)return -2;
        sadx::PlayerPhysicsState player{{{0,0,0},vector(words+2)},0,{0,0,0},{},0,0,{}};player.prior_attributes=words[0];
        sadx::PlayerMotionState world{{vector(words+5),{0,0,0}},0,0,0,0};
        sadx::PlayerContactWork contact{vector(words+11)};
        auto offset=vector(words+8);sadx::X87Context fp(cw);
        const auto result=sadx::initialize_player_contact_offset(offset,world,player,contact,*parameters.parameters,
            static_cast<std::uint16_t>(words[1]),vector(words+14),c,rc,fp);
        std::memcpy(output,&offset,12);output[3]=static_cast<std::uint32_t>(result.failure);
        output[4]=result.source_boundary;output[5]=fp.exception_flags();output[6]=fp.control_word();return 0;
    }catch(const sadx::X87Error&){return -3;}
}
extern "C" const char* sadx_native_player_contact_initializer_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 software x87";}

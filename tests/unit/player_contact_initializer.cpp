#include "player/player_contact_initializer.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T exact(const char* path){T value;std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&value),sizeof(value));require(f.gcount()==sizeof(value)&&f.peek()==std::char_traits<char>::eof(),"Exact source constants required");return value;}
void tests(const sadx::PlayerParameters& p,const sadx::PlayerContactInitializationConstants& c,const sadx::PlayerRotationConstants& rc){
    sadx::PlayerPhysicsState player{{{0,0,0},{3,-1,0}},0,{0,0,0},{},0,0,{}};
    sadx::PlayerMotionState world{{{1,-1,2},{0,0,0}},0,0,0,0};
    sadx::PlayerContactWork contact;sadx::NinjaVector offset{99,1,98};sadx::X87Context fp(0x027f);
    auto run=[&](std::optional<std::uint16_t> flags,const std::optional<sadx::NinjaVector>& gravity){
        return sadx::initialize_player_contact_offset(offset,world,player,contact,p,flags,gravity,c,rc,fp);};
    auto result=run({},{});require(result.failure==sadx::PlayerContactInitializationFailure::unavailable_prior_attributes &&
        result.source_boundary==0x43c7e9 && offset.x==0 && offset.z==0 && offset.y==1,
        "X/Z stores precede missing prior attributes; Y must survive");
    player.prior_attributes=0x4000;offset={99,1,98};result=run({},{});
    require(static_cast<bool>(result)&&offset.x==0&&offset.z==0,"Attribute4000 negative-Y path does not require unused gravity/flags/direction");
    player.prior_attributes=0;offset={99,1,98};result=run({},{});
    require(result.failure==sadx::PlayerContactInitializationFailure::unavailable_gravity && offset.y==1,
        "Main path cannot fabricate gravity");
    result=run({},sadx::NinjaVector{0,-1,0});require(result.failure==sadx::PlayerContactInitializationFailure::unavailable_direction,
        "Contact direction is required only by the actual dot path");
    contact.direction_at_1c=sadx::NinjaVector{0,1,0};result=run({},sadx::NinjaVector{0,-1,0});
    require(result.failure==sadx::PlayerContactInitializationFailure::unavailable_prior_flags && result.source_boundary==0x43c89e,
        "Unknown prior flags survive without a made-up branch");
    player.motion.speed.y=1;offset={99,1,98};result=run(0,{});
    require(static_cast<bool>(result)&&offset.y==1,"Positive-Y branch with flags0 has no offsetY arithmetic");
    offset={99,1,98};result=run(0x2000,{});
    sadx::X87Context expected(0x027f);const auto y=expected.to_float(expected.subtract_float_memory(expected.from_float(1),p.pos_error));
    require(static_cast<bool>(result)&&std::memcmp(&offset.y,&y,4)==0,"Positive-Y2000 branch uses actual pos_error memory subtraction");
    player.motion.speed.y=-1;player.prior_attributes=8;offset={99,1,98};result=run({},{});
    require(static_cast<bool>(result),"Attribute8 center adjustment does not require unused gravity/flags");
}
}
int main(int argc,char** argv){
    try{
        require(argc==4,"usage: contact_initializer_test PARAMETER_ROWS CONSTANTS_52 ROTATION_CONSTANTS_88");
        std::array<std::uint8_t,132> bytes;std::ifstream f(argv[1],std::ios::binary);f.read(reinterpret_cast<char*>(bytes.data()),132);require(f.gcount()==132,"First parameter row required");
        const auto params=sadx::decode_player_parameters(bytes.data(),132);require(static_cast<bool>(params),"Parameters failed");
        sadx::PlayerContactInitializationConstants c{exact<std::array<std::uint32_t,13>>(argv[2])};
        const auto rotation=exact<std::array<std::uint8_t,88>>(argv[3]);sadx::PlayerRotationConstants rc;
        std::memcpy(rc.float_bits.data(),rotation.data(),64);std::memcpy(rc.double_bits.data(),rotation.data()+64,24);
        tests(*params.parameters,c,rc);std::cout<<"Contact initializer: exact local stores and lazy unknown-state failures passed.\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

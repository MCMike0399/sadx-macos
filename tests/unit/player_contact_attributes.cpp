#include "player/player_contact_attributes.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<std::size_t Size>std::array<std::uint8_t,Size> read(const char* path){
    std::array<std::uint8_t,Size> bytes{};std::ifstream input(path,std::ios::binary);
    input.read(reinterpret_cast<char*>(bytes.data()),Size);
    require(input.gcount()==Size&&input.peek()==std::char_traits<char>::eof(),"Exact original artifact size");return bytes;
}
std::uint32_t bits(float value){std::uint32_t word;std::memcpy(&word,&value,4);return word;}
}
int main(int argc,char** argv){
    try{
        require(argc==3,"usage: contact-attributes-test PARAMETERS_1056 CONSTANTS_40");
        const auto parameters=read<1056>(argv[1]);const auto bytes=read<40>(argv[2]);
        const auto decoded=sadx::decode_contact_attribute_constants(bytes.data(),bytes.size());require(bool(decoded),"Source material constants");
        sadx::ContactParameterTable table{};
        for(unsigned i=0;i<8;++i){const auto row=sadx::decode_player_parameters(parameters.data()+i*132,132);
            require(bool(row),"Source default parameter row");table[i]={row.parameters->weight,row.parameters->air_resist_y,row.parameters->run_accel};}
        for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
            sadx::X87Context fp(pc|rc|0x7f);sadx::PlayerTaskState task{};
            sadx::PlayerMotionState world{};sadx::PlayerPhysicsState player{};sadx::PlayerParameters live{};
            player.attributes=0x12340000;
            for(unsigned low=0;low<256;++low){
                const auto result=sadx::apply_player_contact_attributes(task,&world,player,live,table,low,*decoded,fp);
                const unsigned slot=(low&8)?0:(low&4)?1:(low&0x10)?2:(low&0x80)?3:4;
                require(result&&bits(world.friction_scale)==bits(decoded->friction[slot]),"Original low-byte friction priority");
            }
            sadx::apply_player_contact_attributes(task,&world,player,live,table,0x80000000,*decoded,fp);
            require(world.friction_scale==1&&player.attributes==0x923400ff,"Bit31 does not substitute for BL sign bit7");
            table[0]={2.0F,3.0F,4.0F};table[5]={2.0F,3.0F,4.0F};
            task.character_id=5;player.shadow_attributes_top=0x400002;
            require(sadx::apply_player_contact_attributes(task,&world,player,live,table,0,*decoded,fp)&&live.weight==1,
                    "Shadow bit2 precedes400000 and character5 selects source half factor");
            task.character_id=200;player.shadow_attributes_top=0;
            const auto unknown=sadx::apply_player_contact_attributes(task,&world,player,live,table,4,*decoded,fp);
            require(unknown.failure==sadx::ContactAttributeFailure::unavailable_source_parameter&&unknown.source_boundary==0x43cc6c&&
                    (player.attributes&4),"Unavailable row reports actual read after attribute OR");
        }
        sadx::X87Context fp(0x037e);sadx::PlayerTaskState task{};sadx::PlayerPhysicsState player{};sadx::PlayerParameters live{};
        std::uint32_t signaling=0x7f800123;float nan;std::memcpy(&nan,&signaling,4);table[0]={nan,nan,nan};
        const auto late=sadx::apply_player_contact_attributes(task,nullptr,player,live,table,0x80,*decoded,fp);
        require(late.failure==sadx::ContactAttributeFailure::null_motion&&late.source_boundary==0x43cccf&&
                bits(live.weight)==signaling&&bits(live.air_resist_y)==signaling&&bits(live.run_accel)==signaling&&fp.exception_flags()==0,
                "MOV restores retain signaling bits without FP operations before late null-motion boundary");
        std::cout<<"Contact attributes: live parameters, all low-byte friction priorities, partial reads and raw signaling stores passed.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

#include "player/sonic_animation_table.hpp"
#include <cstring>
extern "C" int sadx_native_sonic_animation_table(const std::uint32_t* identities,std::size_t count,std::uint8_t* bytes,std::size_t size){
    if(!identities||count!=149||!bytes||size!=147*16)return 1;auto exports=std::make_shared<sadx::SonicAnimationExport>();for(std::size_t i=0;i<count;++i)exports->push_back(sadx::AnimationResourceReference{identities[i]?std::optional<std::size_t>(identities[i]):std::nullopt});sadx::PlayerAnimationTable table;auto result=sadx::initialize_sonic_animation_table(table,exports);if(!result||result.completed_records!=147)return 2;
    for(std::size_t i=0;i<147;++i){const auto& r=table.records.at(i);std::uint32_t action=static_cast<std::uint32_t>(r.action->id.value_or(0));std::memcpy(bytes+16*i,&action,4);bytes[16*i+4]=*r.object_count;std::memcpy(bytes+16*i+5,&*r.mode,1);std::memcpy(bytes+16*i+6,&*r.next,2);std::memcpy(bytes+16*i+8,&*r.transition_step,4);std::memcpy(bytes+16*i+12,&*r.frame_step,4);}return 0;
}

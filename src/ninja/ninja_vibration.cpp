#include "ninja/ninja_vibration.hpp"
namespace sadx {
NinjaVibrationResult run_vibration_785190(NinjaVibrationRuntime& rt){
    NinjaVibrationResult r;auto& s=rt.state;
    const auto stop=[&](NinjaVibrationFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;return false;};
    const auto call=[&](std::uint32_t site,std::uint32_t callee,std::array<std::optional<std::uint32_t>,2> args,std::optional<std::uint32_t>* eax=nullptr){
        if(!rt.invoke)return stop(NinjaVibrationFailure::missing_owner,site);
        const auto [owner,value]=rt.invoke({site,callee,args});
        if(owner.state!=GameLogicOwnerState::completed)return stop(owner.state==GameLogicOwnerState::failed?NinjaVibrationFailure::owner_failed:NinjaVibrationFailure::missing_owner,site);
        if(eax)*eax=value;return true;};
    for(std::uint32_t i=0;i<4;++i){
        const auto entry=0x3d08428+0x18*i;auto& state=rt.state_at_3d08428[i];
        if(!state){stop(NinjaVibrationFailure::unknown_state,0x7851a0,entry);return r;}
        bool to_stop=false;
        if(*state==1||*state==2){
            if(!call(0x7851b1,0x793da0,{}))return r;
            std::optional<std::uint32_t> elapsed;if(!call(0x7851c0,0x793e00,{},&elapsed))return r;
            if(!s.duration_at_3d08434[i]){stop(NinjaVibrationFailure::unknown_state,0x7851c5,entry+0xc);return r;}
            if(!elapsed){stop(NinjaVibrationFailure::unknown_state,0x7851c8);return r;}
            if(*s.duration_at_3d08434[i]<=*elapsed){state=3;to_stop=true;}
        }
        if(!to_stop){
            if(*state==1){
                if(!s.strength_at_3d0842e[i]){stop(NinjaVibrationFailure::unknown_state,0x7851e0,entry+6);return r;}
                const auto v=static_cast<std::int32_t>(static_cast<std::int8_t>(*s.strength_at_3d0842e[i]));
                auto magnitude=static_cast<std::uint32_t>(v<0?-v:v)*0x594U;if(magnitude>0x2710)magnitude=0x2710;
                if(!call(0x7851fd,0x791f80,{i,magnitude}))return r;
                if(!s.counter_at_3d0843c[i]){stop(NinjaVibrationFailure::unknown_state,0x785202,entry+0x14);return r;}
                s.counter_at_3d0843c[i]=*s.counter_at_3d0843c[i]+1;
            }
            if(*state!=3)continue;
        }
        if(!call(0x785212,0x792000,{i}))return r;
        state=0;
    }
    return r;
}
} // namespace sadx

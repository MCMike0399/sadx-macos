#include "ninja/ninja_vibration.hpp"
#include <algorithm>
#include <vector>
// Native 785190 with the layout of tools/verify_original_vibration.py.
// in: [0] fail-at nested call [1..4] states [5..8] strength bytes [9..12] durations [13..16] counters [17..24] elapsed per 793e00 call
// out: [0] completed [1..4] states [5..8] counters [10] nested count [11..] (callee, site, a0, a1) x 24
extern "C" int sadx_native_vibration(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    NinjaVibrationState s;std::array<std::optional<std::uint32_t>,4> states;
    for(int i=0;i<4;++i){states[i]=in[1+i];s.strength_at_3d0842e[i]=static_cast<std::uint8_t>(in[5+i]);s.duration_at_3d08434[i]=in[9+i];s.counter_at_3d0843c[i]=in[13+i];}
    std::vector<std::array<std::uint32_t,4>> nested;std::size_t elapsed_calls=0;
    NinjaVibrationRuntime rt{s,states,[&](const NinjaVibrationCall& c)->std::pair<GameLogicOwnerResult,std::optional<std::uint32_t>>{
        nested.push_back({c.callee,c.source_call,c.arguments[0].value_or(0xdeadbeef),c.arguments[1].value_or(0xdeadbeef)});
        if(in[0]&&nested.size()==in[0])return {{GameLogicOwnerState::unavailable,c.callee,{}},std::nullopt};
        std::optional<std::uint32_t> eax;if(c.callee==0x793e00)eax=in[17+std::min<std::size_t>(elapsed_calls++,7)];
        return {{GameLogicOwnerState::completed,0,{}},eax};}};
    const auto r=run_vibration_785190(rt);
    std::fill(out,out+120,0);out[0]=r?1U:0U;
    for(int i=0;i<4;++i){out[1+i]=states[i].value_or(0xdeadbeef);out[5+i]=s.counter_at_3d0843c[i].value_or(0xdeadbeef);}
    out[10]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<24;++i)std::copy(nested[i].begin(),nested[i].end(),out+11+4*i);
    return 1;
}

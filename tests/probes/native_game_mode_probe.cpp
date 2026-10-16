#include "guest/game_mode_reference.hpp"
#include <algorithm>
#include <vector>
// Native 40c090 with the fixture of tools/verify_original_game_mode.py.
// in[0..15] globals, in[16..34] scripted EAX for kReturning, in[35..40] two
// mutators (callee, target, value), in[41] fail at call N (1-based, 0 = never).
namespace {
constexpr std::array<std::uint32_t,19> kReturning{{0x421f70,0x421f80,0x42e890,0x413c00,0x42f240,0x416610,0x4173b0,0x415af0,0x42f020,0x42caa0,
    0x5033b0,0x4144e0,0x412d20,0x642640,0x6428d0,0x413bc0,0x641c60,0x640f20,0x590650}};
}
extern "C" int sadx_native_game_mode(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=48||!out||capacity<256)return 0;
    sadx::GameLoopState loop;loop.mode_at_3b28114=in[1];loop.save_phase_at_3b291a4=static_cast<std::int32_t>(in[2]);loop.request_at_3b291ad=static_cast<std::uint8_t>(in[3]);loop.word_at_3abdcf8=in[4];
    std::optional<std::int32_t> mode=static_cast<std::int32_t>(in[0]);
    sadx::GameModeState st;st.byte_at_3abdf74=static_cast<std::uint8_t>(in[5]);st.byte_at_3abdcfc=static_cast<std::uint8_t>(in[6]);st.byte_at_3abdcfd=static_cast<std::uint8_t>(in[7]);
    st.byte_at_3b18db4=static_cast<std::uint8_t>(in[8]);st.byte_at_3b2a2fa=static_cast<std::uint8_t>(in[9]);st.byte_at_3b22e1e=static_cast<std::uint8_t>(in[10]);st.byte_at_3abdf78=static_cast<std::uint8_t>(in[11]);
    st.word_at_3b2c45c=in[12];st.word_at_3b290e4=in[13];if(!in[15])st.level_word_via_3b0ef4c=static_cast<std::int16_t>(static_cast<std::uint16_t>(in[14]));
    std::vector<std::array<std::uint32_t,5>> trace;
    sadx::GameModeRuntime rt{loop,mode,st,{}};
    rt.invoke=[&](const sadx::GameModeCall& c)->sadx::GameModeOwnerResult{
        trace.push_back({c.callee,c.source_call,c.arguments[0].value_or(0),c.arguments[1].value_or(0),c.arguments[2].value_or(0)});
        if(in[41]&&trace.size()==in[41])return {{sadx::GameLogicOwnerState::failed,0x123},{}};
        for(unsigned k=0;k<2;++k){const auto* mu=in+35+3*k;if(mu[0]!=c.callee)continue;
            switch(mu[1]){case 0:mode=static_cast<std::int32_t>(mu[2]);break;case 1:st.byte_at_3abdcfd=static_cast<std::uint8_t>(mu[2]);break;
                case 2:loop.mode_at_3b28114=mu[2];break;case 3:st.level_word_via_3b0ef4c=static_cast<std::int16_t>(static_cast<std::uint16_t>(mu[2]));break;case 4:loop.word_at_3abdcf8=mu[2];break;default:break;}}
        for(std::size_t i=0;i<kReturning.size();++i)if(kReturning[i]==c.callee)return {{sadx::GameLogicOwnerState::completed,0},in[16+i]};
        return {{sadx::GameLogicOwnerState::completed,0},0x5a5a5a5aU};
    };
    const auto r=sadx::run_game_mode_40c090(rt);std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.completed;
    const std::uint32_t g[]={static_cast<std::uint32_t>(*mode),*loop.mode_at_3b28114,static_cast<std::uint32_t>(*loop.save_phase_at_3b291a4),*loop.request_at_3b291ad,*loop.word_at_3abdcf8,
        *st.byte_at_3abdf74,*st.byte_at_3abdcfc,*st.byte_at_3abdcfd,*st.byte_at_3b18db4,*st.word_at_3b2c45c};
    std::copy(std::begin(g),std::end(g),out+3);out[13]=static_cast<std::uint32_t>(std::min<std::size_t>(trace.size(),48));
    for(std::size_t i=0;i<trace.size()&&i<48;++i)std::copy(trace[i].begin(),trace[i].end(),out+14+5*i);
    return 1;
}

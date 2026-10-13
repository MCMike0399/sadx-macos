#include "game/sound_manager.hpp"
#include <algorithm>
// Native 4250d0 with the layout of tools/verify_original_sound_manager.py.
// in: [0] 3b29ce0 [1] 3b29ce4 [2] 3b29ce8 [3] 3b2cbb0 [4] fail-at call [5] EAX base,
// [8..25] 3b292a8, then per record r (base 32+12r): timer,+4,flags,id,freq,vol,pan,3b29c28,3b29b90.
extern "C" int sadx_native_sound_manager(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=32+12*36||!out||capacity<600)return 0;
    sadx::AudioStartupState audio;audio.word_at_3b29ce0=in[0];audio.word_at_3b29ce4=in[1];audio.byte_at_3b29ce8=static_cast<std::uint8_t>(in[2]);
    for(int i=0;i<18;++i)audio.words_at_3b292a8[i]=in[8+i];
    std::array<std::optional<std::uint32_t>,36> flags{},entities{};sadx::SoundRecordFields f;f.listener_at_3b2cbb0=in[3];
    for(int r=0;r<36;++r){const auto* w=in+32+12*r;audio.record_words_at_3b292fc[r]=w[0];f.words_at_3b29300[r]=w[1];flags[r]=w[2];f.ids_at_3b29308[r]=w[3];
        f.frequency_at_3b2930c[r]=w[4];f.volume_at_3b29310[r]=w[5];f.pan_at_3b29318[r]=w[6];f.current_volume_at_3b29c28[r]=w[7];entities[r]=w[8];}
    std::vector<std::array<std::uint32_t,4>> trace;
    sadx::SoundManagerRuntime rt{audio,flags,entities,f,[&](const sadx::SoundManagerCall& c)->sadx::SoundManagerOwnerResult{
        trace.push_back({c.callee,c.source_call,c.arguments[0].value_or(0xdeadbeef),c.arguments[1].value_or(0xdeadbeef)});
        if(in[4]&&trace.size()==in[4])return {{sadx::GameLogicOwnerState::unavailable,c.callee,{}},std::nullopt};
        return {{sadx::GameLogicOwnerState::completed,0,{}},static_cast<std::uint32_t>(in[5]+trace.size()*0x101)};}};
    const auto r=sadx::run_sound_manager_4250d0(rt);
    std::fill(out,out+capacity,0);
    out[0]=r?1U:0U;out[1]=static_cast<std::uint32_t>(trace.size());out[2]=audio.byte_at_3b29ce8.value_or(0xef);
    for(int i=0;i<18;++i)out[8+i]=audio.words_at_3b292a8[i].value_or(0xdeadbeef);
    for(int k=0;k<36;++k){auto* w=out+32+12*k;const auto v=[](const std::optional<std::uint32_t>& x){return x.value_or(0xdeadbeef);};
        w[0]=v(audio.record_words_at_3b292fc[k]);w[1]=v(f.words_at_3b29300[k]);w[2]=v(flags[k]);w[3]=v(f.ids_at_3b29308[k]);w[4]=v(f.frequency_at_3b2930c[k]);
        w[5]=v(f.volume_at_3b29310[k]);w[6]=v(f.pan_at_3b29318[k]);w[7]=v(f.current_volume_at_3b29c28[k]);w[8]=v(entities[k]);}
    for(std::size_t i=0;i<trace.size()&&i<24;++i)std::copy(trace[i].begin(),trace[i].end(),out+464+4*i);
    return 1;
}

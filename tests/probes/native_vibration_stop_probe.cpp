#include "input/input_mod_pad.hpp"
#include <algorithm>
#include <vector>
// Native Input pdVibMxStop_r with the layout of tools/verify_original_vibration_stop.py.
// in: [0..15] tasks [16..23] pad rumble flags [24..31] pad rumble words [32] fail-at owner call
// out: [0] completed [1..16] tasks [17..24] flags [25..32] words [40] calls [41..] (kind, value): 1 free(task), 2 rumble(pad)
extern "C" int sadx_native_vibration_stop(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    InputModRumbleTasks tasks;for(int i=0;i<16;++i)tasks.tasks_at_1000ec60[i]=in[i];
    std::array<InputModDreamPad,8> pads{};for(int i=0;i<8;++i){pads[i].rumble_flags=static_cast<std::uint8_t>(in[16+i]);pads[i].rumble_word=in[24+i];}
    std::vector<std::uint32_t> calls;
    const auto step=[&](std::uint32_t kind,std::uint32_t v){calls.push_back(kind);calls.push_back(v);return !(in[32]&&calls.size()/2==in[32]);};
    const bool ok=input_mod_vibration_stop(tasks,pads,{[&](std::uint32_t t){return step(1,t);},[&](std::size_t p){return step(2,static_cast<std::uint32_t>(p));}});
    std::fill(out,out+80,0);out[0]=ok;
    for(int i=0;i<16;++i)out[1+i]=tasks.tasks_at_1000ec60[i];for(int i=0;i<8;++i){out[17+i]=pads[i].rumble_flags;out[25+i]=pads[i].rumble_word;}
    out[40]=static_cast<std::uint32_t>(calls.size()/2);for(std::size_t i=0;i<calls.size()&&i<38;++i)out[41+i]=calls[i];
    return 1;
}

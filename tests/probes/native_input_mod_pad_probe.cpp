#include "input/input_mod_pad.hpp"
#include <algorithm>
#include <cstring>
// Native Input 2.6.1 DreamPad::poll / convert_axes matching
// tools/verify_original_input_mod_pad.py. in (48 words):
// 0 connected,1 allow kb,2 deadzone_l,3 deadzone_r,4 radial_l,5 radial_r,6 threshold,7 demo,
// 8 half_press prior,9 e_held prior,10..15 gamepad axes,16 button bits(15),17 kb held,
// 18 kb rtrig,19 kb ltrig,20..23 kb lx,ly,rx,ry,24 kb normalized_l bits,25 kb normalized_r bits,
// 26 old buttons,27 mxcsr,28..40 prior record dwords (13),
// 41..45 DLL axes constants (1000bb6c,bbb0,bba0,bba4,bb7c),46 trigger scale 1000bba8,47 half 1000bb78
extern "C" int sadx_native_input_mod_pad(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=48||!out||capacity<24)return 0;
    sadx::InputModDreamPad pad;for(unsigned w=0;w<13;++w)pad.record.write_u32(w*4,in[28+w]);pad.record.write_u32(0x2c,in[26]);
    pad.connected=in[0]!=0;pad.half_press=in[8]!=0;pad.e_held=in[9]!=0;pad.normalized_l=sadx::Sse32{0xdeadbeef};pad.normalized_r=sadx::Sse32{0xdeadbeef};
    pad.settings={in[1]!=0,static_cast<std::int16_t>(in[2]),static_cast<std::int16_t>(in[3]),in[4]!=0,in[5]!=0,static_cast<std::uint8_t>(in[6])};
    sadx::InputModGamepadState gp;for(unsigned i=0;i<6;++i)gp.axes[i]=static_cast<std::int16_t>(in[10+i]);for(unsigned b=0;b<15;++b)gp.buttons[b]=(in[16]>>b)&1;
    sadx::InputModKeyboardPad kb;kb.held=in[17];kb.rtrigger=static_cast<std::uint16_t>(in[18]);kb.ltrigger=static_cast<std::uint16_t>(in[19]);
    kb.lx=static_cast<std::int16_t>(in[20]);kb.ly=static_cast<std::int16_t>(in[21]);kb.rx=static_cast<std::int16_t>(in[22]);kb.ry=static_cast<std::int16_t>(in[23]);kb.normalized_l={in[24]};kb.normalized_r={in[25]};
    const sadx::InputModPadConstants k{{{in[41]},{in[42]},{in[43]},{in[44]},{in[45]}},{in[46]},{in[47]}};
    sadx::SseContext sse(in[27]);const auto r=sadx::input_mod_poll(pad,kb,gp,in[7]!=0,k,sse);
    std::fill(out,out+capacity,0);out[0]=static_cast<std::uint32_t>(r.failure);
    for(unsigned w=0;w<13;++w)out[1+w]=pad.record.read_u32(w*4).value_or(0xdeadbeef);
    out[14]=pad.connected;out[15]=pad.e_held;out[16]=pad.half_press;out[17]=pad.normalized_l->bits;out[18]=pad.normalized_r->bits;out[19]=sse.mxcsr();
    return 1;
}

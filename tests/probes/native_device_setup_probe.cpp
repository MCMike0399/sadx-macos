#include "guest/frame_control_reference.hpp"
#include "engine/ninja_device_init.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 791610 with the layout of tools/verify_original_device_setup.py.
// in: [0] fail-at nested call [1] CW [2] Direct3DCreate8 result [3] 7943c0 installs a device (0/1) [4..9] viewport
//     [10] 3d1289c before [11] 3d08538 [12] 3d0853c (QPF frequency) [13..65] caps words (0xd4 bytes) [66..81] 389d318 matrix
//     [82] 3d1289c value the 794ec0 fixture leaves (bit 0 selects the stencil clear)
// out: [0] completed [1] 3d11f60 [2] 3d1289c [3..11] record heads [12..17] viewport [18] 3d12948 [19] 3d1294c [20..26] 3d0fd00..
//      [27..42] 3d0fd80 matrix [43..46] 3d0fd20..2c [47] 389d7dc [48] 3d084a8 [49] 3d084ac [50] device (1 live, 0 null)
//      [60] nested count [61..] (callee, site) x 16 [100] device-call count [101..] (method, a) x 8 ; caps out [120..172]
extern "C" int sadx_native_device_setup(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    NinjaDeviceState s;NinjaFrameCallState frame;frame.device_flags_at_3d1289c=in[10];for(auto& rec:frame.records_at_3d0fe00)rec={1u,2u,3u,4u};
    for(std::size_t i=0;i<s.block_at_3d0fae0.size;++i)s.block_at_3d0fae0.bytes[i]=0x55;
    for(int i=0;i<16;++i)s.matrix_at_389d318[i]=in[66+i];
    // Every compared field starts as the source memory does here: zero.
    s.direct3d_at_3d11f60=0;for(auto& b:s.caps_at_3d127c0.bytes)b=0;for(auto& w:s.viewport_at_3d12780)w=0;s.half_width_at_3d12948=0.0F;s.half_height_at_3d1294c=0.0F;
    for(auto& w:s.words_at_3d0fd00)w=0;for(auto& w:s.matrix_at_3d0fd80)w=0;s.left_at_3d0fd20=s.top_at_3d0fd24=s.right_at_3d0fd28=s.bottom_at_3d0fd2c=0.0F;
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{9});
    GameFrameRendererState renderer{};renderer.counter_frequency.low=in[11];renderer.counter_frequency.high=in[12];renderer.frame_interval_multiplier=0;renderer.frame_interval.low=0;renderer.frame_interval.high=0;
    X87Context fp(static_cast<std::uint16_t>(in[1]));
    std::vector<std::array<std::uint32_t,2>> nested,device;
    NinjaDeviceHost host{[&](std::uint32_t)->std::optional<std::uint32_t>{device.push_back({0xdc,0});return in[2];},
        [&](const D3D8FrameCommand&)->std::optional<std::vector<std::uint8_t>>{device.push_back({0x1c,0});std::vector<std::uint8_t> b(0xd4);std::memcpy(b.data(),in+13,0xd4);return b;},
        [&](const D3D8FrameCommand&)->std::optional<std::array<std::uint32_t,6>>{device.push_back({0xa4,0});return std::array<std::uint32_t,6>{in[4],in[5],in[6],in[7],in[8],in[9]};},
        [&](const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t,float,std::uint32_t)->std::optional<std::int32_t>{device.push_back({0x90,flags});return 0;}};
    NinjaDeviceInitRuntime rt{s,frame,render,renderer,fp,host,[&](const NinjaDeviceCall& c)->GameLogicOwnerResult{
        nested.push_back({c.callee,c.source_call});
        if(c.callee==0x7943c0&&in[3])render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
        if(c.callee==0x794ec0)frame.device_flags_at_3d1289c=in[82];
        return {in[0]&&nested.size()==in[0]?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,c.callee,{}};}};
    const auto r=run_device_setup_791610(rt);
    std::fill(out,out+200,0);out[0]=r?1U:0U;
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    const auto f=[](const std::optional<float>& x){std::uint32_t b=0xdeadbeef;if(x)std::memcpy(&b,&*x,4);return b;};
    out[1]=v(s.direct3d_at_3d11f60);out[2]=v(frame.device_flags_at_3d1289c);for(int i=0;i<9;++i)out[3+i]=v(frame.records_at_3d0fe00[i][0]);
    for(int i=0;i<6;++i)out[12+i]=v(s.viewport_at_3d12780[i]);out[18]=f(s.half_width_at_3d12948);out[19]=f(s.half_height_at_3d1294c);
    for(int i=0;i<7;++i)out[20+i]=v(s.words_at_3d0fd00[i]);for(int i=0;i<16;++i)out[27+i]=v(s.matrix_at_3d0fd80[i]);
    out[43]=f(s.left_at_3d0fd20);out[44]=f(s.top_at_3d0fd24);out[45]=f(s.right_at_3d0fd28);out[46]=f(s.bottom_at_3d0fd2c);
    out[47]=static_cast<std::uint32_t>(renderer.frame_interval_multiplier.value_or(0x7eadbeef));out[48]=v(renderer.frame_interval.low);out[49]=v(renderer.frame_interval.high);
    out[50]=render.device_at_3d128b0&&*render.device_at_3d128b0?1U:0U;
    out[60]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<16;++i){out[61+2*i]=nested[i][0];out[62+2*i]=nested[i][1];}
    out[100]=static_cast<std::uint32_t>(device.size());for(std::size_t i=0;i<device.size()&&i<8;++i){out[101+2*i]=device[i][0];out[102+2*i]=device[i][1];}
    for(std::size_t i=0;i<0xd4/4;++i){std::uint32_t w=0;for(int k=0;k<4;++k)w|=static_cast<std::uint32_t>(s.caps_at_3d127c0.bytes[4*i+k].value_or(0xef))<<(8*k);out[120+i]=w;}
    {bool zeroed=true;for(const auto& b:s.block_at_3d0fae0.bytes)zeroed=zeroed&&b==0;out[190]=zeroed;}
    return 1;
}

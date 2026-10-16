#include "guest/frame_control_reference.hpp"
#include "engine/ninja_screen.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 780c30 (+7899a0, 7912e0, 77ec60, 77ff70, 780090) with the layout of tools/verify_original_screen_init.py.
// in: [0] CW [1] flags [2] 3d0f804 word [3] 3d0f814 word (7899a0's argument) [4..9] viewport [10] 3d0fd1c
//     [11..26] 389d318 [27..42] 3d129c0 [43] 3d08538 [44] 3d0853c [45] device (1 live, 0 null) [46..67] 88ad64..88adbb
// out: [0] completed [1..60] the compared globals (see the oracle) [100] device-call count [101..] (method, a1, a2) x 8
//      [130..135] SetViewport words [140..155] SetTransform matrix [160..175] MultiplyTransform matrix
extern "C" int sadx_native_screen_init(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    NinjaScreenState s;NinjaDeviceState d;
    // Every compared field starts as the oracle's memory does: zero, except the seeded words below.
    s.invalid_at_3d0fd1c=in[10];
    for(int i=0;i<6;++i)d.viewport_at_3d12780[i]=in[4+i];
    for(int i=0;i<16;++i){d.matrix_at_389d318[i]=in[11+i];s.projection_at_3d129c0[i]=in[27+i];d.matrix_at_3d0fd80[i]=0;}
    d.left_at_3d0fd20=d.top_at_3d0fd24=d.right_at_3d0fd28=d.bottom_at_3d0fd2c=0.0F;
    NinjaRenderState render;if(in[45])render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});else render.device_at_3d128b0=NinjaRenderDeviceHandle{};
    std::uint8_t tables[88];std::memcpy(tables,in+46,88);const auto blend=decode_ninja_blend_tables(tables,88);render.blend_tables=blend?&*blend:nullptr;
    std::vector<std::array<std::uint32_t,3>> calls;std::array<std::uint32_t,6> vp_words{};D3dxMatrix set{},multiply{};
    render.set_render_state=[&](const NinjaRenderStateCommand& c){calls.push_back({0xc8,c.state,c.value});return NinjaRenderCallResult{true,0};};
    NinjaScreenDevice device;
    device.set_viewport=[&](const D3D8FrameCommand&,const std::array<std::uint32_t,6>& v)->std::optional<std::int32_t>{calls.push_back({0xa0,0,0});vp_words=v;return 0;};
    device.set_transform=[&](const D3D8FrameCommand&,std::uint32_t state,const D3dxMatrix& m)->std::optional<std::int32_t>{calls.push_back({0x94,state,0});set=m;return 0;};
    device.multiply_transform=[&](const D3D8FrameCommand&,std::uint32_t state,const D3dxMatrix& m)->std::optional<std::int32_t>{calls.push_back({0x9c,state,0});multiply=m;return 0;};
    GameFrameRendererState renderer{};renderer.counter_frequency.low=in[43];renderer.counter_frequency.high=in[44];renderer.frame_interval_multiplier=0;renderer.frame_interval.low=0;renderer.frame_interval.high=0;
    std::optional<float> width_3d0f9e4=0.0F;NinjaSpriteProjection projection;projection.center_x_at_3d0fa0c=0.0F;projection.center_y_at_3d0fa10=0.0F;
    NinjaCallbackList callbacks;callbacks.count_at_3ce712c=5;
    std::optional<std::uint32_t> w840=0x1234,w9c4=0x5678,pool=0x11,size=0x22;std::array<std::optional<std::uint32_t>,4> colors{1u,2u,3u,4u};
    X87Context fp(static_cast<std::uint16_t>(in[0]));
    NinjaScreenInitRuntime rt{NinjaViewportRuntime{s,d,render,device,fp},renderer,width_3d0f9e4,projection,callbacks,w840,w9c4,pool,size,colors};
    const auto r=run_screen_init_780c30(in[1],in[2],in[3],rt);
    std::fill(out,out+200,0);out[0]=r?1U:0U;
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    const auto f=[](const std::optional<float>& x){std::uint32_t b=0;if(x)std::memcpy(&b,&*x,4);return b;};
    out[1]=v(s.word_at_3d0f804);out[2]=v(s.flags_at_3d0f964);out[3]=v(s.word_at_3d0f814);out[4]=f(width_3d0f9e4);out[5]=s.width_at_3d0fa20.value_or(0);
    out[6]=f(s.height_at_3d0f9e8);out[7]=s.height_at_3d0f838.value_or(0);out[8]=f(s.width_at_3d0fa04);out[9]=f(s.height_at_3d0fa08);
    out[10]=f(projection.center_x_at_3d0fa0c);out[11]=f(projection.center_y_at_3d0fa10);out[12]=s.word_at_3ce7160.value_or(0);
    out[13]=f(s.left_at_3d0f820);out[14]=f(s.top_at_3d0f824);out[15]=f(s.right_at_3d0f828);out[16]=f(s.bottom_at_3d0f82c);
    out[17]=v(w840);out[18]=v(w9c4);out[19]=v(callbacks.count_at_3ce712c);out[20]=v(pool);out[21]=v(size);for(int i=0;i<4;++i)out[22+i]=v(colors[i]);
    for(int i=0;i<8;++i)out[26+i]=s.points_at_3ce7184[i].value_or(0);out[34]=s.points_at_3ce7164.value_or(0);out[35]=s.colors_at_3ce7168.value_or(0);out[36]=s.count_at_3ce7170.value_or(0);
    out[37]=v(s.invalid_at_3d0fd1c);for(int i=0;i<16;++i)out[38+i]=v(d.matrix_at_3d0fd80[i]);
    out[54]=f(d.left_at_3d0fd20);out[55]=f(d.top_at_3d0fd24);out[56]=f(d.right_at_3d0fd28);out[57]=f(d.bottom_at_3d0fd2c);
    out[58]=static_cast<std::uint32_t>(renderer.frame_interval_multiplier.value_or(0x7eadbeef));out[59]=v(renderer.frame_interval.low);out[60]=v(renderer.frame_interval.high);
    out[100]=static_cast<std::uint32_t>(calls.size());for(std::size_t i=0;i<calls.size()&&i<8;++i)for(int k=0;k<3;++k)out[101+3*i+k]=calls[i][k];
    for(int i=0;i<6;++i)out[130+i]=vp_words[i];for(int i=0;i<16;++i){out[140+i]=set[i];out[160+i]=multiply[i];}
    return 1;
}

#include "engine/ninja_screen.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 77dfc0 (+791940, 78bba0, 78b780, 78ea10, 78dbe0 -> D3DX 772615/774310/774150, 78b800) with the
// layout of tools/verify_original_screen_polygon.py. RCPPS is the division model (Unicorn's).
// in: [0] CW [1] MXCSR [2] count [3] argument (float bits) [4] flags [5] 3d0fdf4 [6] device (1 live, 0 null)
//     [7..18] six (x, y) points [19..24] six colours [25..30] viewport [31..46] 3d129c0 [47..62] 389d318
// out: [0] completed [1] 3d0fdf4 [2] 3d129a0 released (1) [3] device-call count [4..] (method, a1, a2) x 24
//      [80] primitive type [81] primitive count [82] stride [84..113] vertex words [120] MXCSR
constexpr std::uint32_t kBlock=0x10000800,kPoints=0x10000900,kColors=0x10000a00;
extern "C" int sadx_native_screen_polygon(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    NinjaScreenState s;NinjaDeviceState d;
    for(int i=0;i<6;++i)d.viewport_at_3d12780[i]=in[25+i];
    for(int i=0;i<16;++i){s.projection_at_3d129c0[i]=in[31+i];d.matrix_at_389d318[i]=in[47+i];}
    NinjaRenderState render;if(in[6])render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});else render.device_at_3d128b0=NinjaRenderDeviceHandle{};
    std::vector<std::array<std::uint32_t,3>> calls;std::optional<NinjaScreenPolygon> drawn;
    render.set_render_state=[&](const NinjaRenderStateCommand& c){calls.push_back({0xc8,c.state,c.value});return NinjaRenderCallResult{true,0};};
    render.set_texture_stage_state=[&](const NinjaRenderStageCommand& c){calls.push_back({0xfc,c.state,c.value});return NinjaRenderCallResult{true,0};};
    NinjaTextureSelectionState textures;textures.global_id_at_3d0fdf4=in[5];
    textures.active_memory_at_3d129a0=std::make_shared<const NinjaTextureMemoryReference>(NinjaTextureMemoryReference{nullptr,0,{},{},{},{},{},{}});
    NinjaTextureSelectionRuntime texture_runtime{render,nullptr,nullptr,[&](const NinjaTextureBindCommand& c){calls.push_back({0xf4,c.stage,c.memory?1u:0u});return NinjaRenderCallResult{true,0};}};
    NinjaScreenDevice device;
    device.set_vertex_shader=[&](const D3D8FrameCommand&,std::uint32_t shader)->std::optional<std::int32_t>{calls.push_back({0x130,shader,0});return 0;};
    device.draw_primitive_up=[&](const D3D8FrameCommand&,const NinjaScreenPolygon& p)->std::optional<std::int32_t>{calls.push_back({0x120,p.primitive_type,p.primitive_count});drawn=p;return 0;};
    const std::optional<std::uint32_t> buffer=0x10001000;
    X87Context fp(static_cast<std::uint16_t>(in[0]));SseContext sse(in[1]);
    NinjaPolygonRuntime rt{[&](std::uint32_t a)->std::optional<std::uint32_t>{
            if(a==kBlock)return kPoints;if(a==kBlock+4)return kColors;
            if(a>=kPoints&&a<kPoints+48)return in[7+(a-kPoints)/4];if(a>=kColors&&a<kColors+24)return in[19+(a-kColors)/4];return std::nullopt;},
        render,textures,texture_runtime,buffer,device,NinjaDepthRuntime{s,d,sse,fp,sse_reciprocal_by_division}};
    float argument;std::memcpy(&argument,in+3,4);
    const auto r=run_screen_polygon_77dfc0(kBlock,in[2],argument,in[4],rt);
    std::fill(out,out+200,0);out[0]=r?1U:0U;out[1]=textures.global_id_at_3d0fdf4.value_or(0xdeadbeef);
    out[2]=textures.active_memory_at_3d129a0&&!*textures.active_memory_at_3d129a0?1U:0U;
    out[3]=static_cast<std::uint32_t>(calls.size());for(std::size_t i=0;i<calls.size()&&i<24;++i)for(int k=0;k<3;++k)out[4+3*i+k]=calls[i][k];
    if(drawn){out[80]=drawn->primitive_type;out[81]=drawn->primitive_count;out[82]=drawn->stride;
        for(std::size_t i=0;i<drawn->vertex_bytes.size()/4&&i<30;++i)std::memcpy(out+84+i,drawn->vertex_bytes.data()+4*i,4);}
    out[120]=sse.mxcsr();
    return 1;
}

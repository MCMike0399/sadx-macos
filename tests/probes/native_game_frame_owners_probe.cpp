#include "game/game_frame_owners.hpp"
#include "guest/saved_fov_reference.hpp"
#include "guest/caption_frame_reference.hpp"
#include <algorithm>
#include <cstring>
// Native frame owners with the layout of tools/verify_original_game_frame_owners.py.
// in: [0] callee [1] argument [2] CW [3] fail-at nested call, [4..] globals (see GLOBALS),
// [32..47] colours 3b18230/3ab9864/3d0f7f0/(unused), [48..] edges 3b0e3e0.. (0x302 bytes as words).
namespace {float f(std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;}std::uint32_t b(float v){std::uint32_t x;std::memcpy(&x,&v,4);return x;}}
extern "C" int sadx_native_game_frame_owners(const std::uint32_t* in,const std::uint8_t* table,std::uint32_t* out){
    using namespace sadx;
    std::optional<std::uint8_t> reset=static_cast<std::uint8_t>(in[4]),keys=static_cast<std::uint8_t>(in[5]);std::optional<std::uint32_t> mode=0,draws=in[6],effect=in[7];
    GameLogicEdgeState edges;edges.bytes_at_3b0e3e0={static_cast<std::uint8_t>(in[48]),static_cast<std::uint8_t>(in[49])};
    for(int r=0;r<256;++r)for(int k=0;k<3;++k)edges.raw_at_3b0e3e2[r][k]=static_cast<std::uint8_t>(in[50+3*r+k]);
    std::optional<std::array<float,4>> color=std::array<float,4>{f(in[32]),f(in[33]),f(in[34]),f(in[35])};
    std::optional<std::array<float,4>> queue_color=std::array<float,4>{f(in[36]),f(in[37]),f(in[38]),f(in[39])},projection_color=std::array<float,4>{f(in[40]),f(in[41]),f(in[42]),f(in[43])};
    sadx::view::SavedFovState saved_fov;
    sadx::reference::SavedFovReference saved_fov_reference;
    saved_fov_reference.import_value(saved_fov,in[8]);
    GameViewState view{1.0F,1.0F,640,480,0.5F,639,479,reset,mode,0,0,0,0,0,0,0,0,in[10],in[9],saved_fov,[&] { saved_fov_reference.observe_write(); },{},color}; // 3aad0bc, 3ab98ec, 3b2cbb4
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
    std::vector<std::array<std::uint32_t,3>> trace;
    render.set_render_state=[&](const NinjaRenderStateCommand& c){trace.push_back({c.state,c.value,c.source_call});return NinjaRenderCallResult{true,{}};};
    std::array<std::uint32_t,8> depth{};std::memcpy(depth.data(),table+4100,32);
    std::optional<NinjaMath::SineTable> sine=NinjaMath::SineTable{};std::memcpy(sine->data(),table,4100);
    NinjaSpriteProjection projection{};projection.scale_at_3d0f9e0=f(in[11]);std::optional<float> width=f(in[12]);
    X87Context fp(static_cast<std::uint16_t>(in[2]));
    GameFrameOwnersState s;s.list_at_3b2c584=in[13];s.count_at_3b0e334=in[14];reference::CaptionFrameReference caption(s.caption);caption.observe_timer(in[15]);caption.observe_buffer(in[16]);
    std::vector<std::array<std::uint32_t,3>> nested;
    GameFrameOwnersRuntime rt{s,view,reset,draws,effect,keys,edges,render,&depth,sine,projection,width,color,NinjaQueueColorOwners{queue_color,projection_color,1.0F,-1.0F},fp,
        [&](const GameFrameOwnerCall& c){nested.push_back({c.callee,c.source_call,c.arguments[0].value_or(0xdeadbeef)});
            const bool stop=c.callee==0x40d520||c.callee==0x40eac0||c.callee==0x42f9cc||c.callee==0x40d3f5||(in[3]&&nested.size()==in[3]);
            return GameLogicOwnerResult{stop?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,c.callee,{}};},[&](const GameFrameOwnerInvoke& invoke){return caption.complete_frame(invoke);}};
    const auto r=run_game_frame_owner(in[0],in[1],rt);
    std::fill(out,out+128,0);out[0]=r?1U:0U;out[1]=r.eax.value_or(0xdeadbeef);out[2]=r.eax.has_value();
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    out[4]=v(reset);out[6]=v(draws);out[8]=v(saved_fov_reference.export_value(saved_fov));out[9]=v(view.fov_copy_at_3ab98ec);out[10]=v(view.fov_at_3aad0bc);out[11]=projection.scale_at_3d0f9e0?b(*projection.scale_at_3d0f9e0):0xdeadbeef;
    out[13]=v(s.list_at_3b2c584);out[14]=v(s.count_at_3b0e334);out[15]=v(caption.timer_word());out[16]=v(caption.buffer_word());
    for(int i=0;i<4;++i){out[36+i]=queue_color?b((*queue_color)[i]):0xdeadbeef;out[40+i]=projection_color?b((*projection_color)[i]):0xdeadbeef;}
    out[60]=static_cast<std::uint32_t>(trace.size());for(std::size_t i=0;i<trace.size()&&i<8;++i)std::copy(trace[i].begin(),trace[i].end(),out+64+3*i);
    out[90]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<8;++i)std::copy(nested[i].begin(),nested[i].end(),out+92+3*i);
    return 1;
}

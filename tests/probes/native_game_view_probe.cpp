#include "guest/frame_control_reference.hpp"
#include "game/game_view.hpp"
#include "guest/saved_fov_reference.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 4208e0 matching tools/verify_original_game_view.py.
// in: 0 cw,1 reset pending,2/3 screen ratios bits,4 3b28094,5 3aad0bc,6..9 colour 3b280ac,10..13 colour 3b18230,14 depth mode,15 fail call N,16 heap base
extern "C" int sadx_native_game_view(const std::uint8_t* k,std::size_t ks,const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!k||ks!=148||!in||count!=20||!out||capacity<120)return 0;
    const auto f=[](std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;};const auto bits=[](float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;};
    float c[7];std::memcpy(c,k,28);std::array<std::uint32_t,8> depth;std::memcpy(depth.data(),k+28,32); // 7dcb28,7dcb24,7dcc98,7dd594,7dd590,7dcb10,7dcb0c ; depth table
    std::vector<std::array<std::uint32_t,4>> trace;
    sadx::NinjaRenderState render;render.device_at_3d128b0=std::make_shared<sadx::NinjaRenderDevice>();render.packed_blend_at_3ab9890=0;
    const auto blend=*sadx::decode_ninja_blend_tables(k+60,88);render.blend_tables=&blend; // .rdata 88ad64..88adbb
    const auto fail=[&](){return in[15]&&trace.size()==in[15];};
    render.set_render_state=[&](const auto& cmd){trace.push_back({0xc8,cmd.source_call,cmd.state,cmd.value});return sadx::NinjaRenderCallResult{!fail(),{}};};
    auto arena=std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(0x88000,0xcd),0,0x88000,1);sadx::NinjaSpriteQueue queue(arena);
    sadx::NinjaSpriteQueueRuntime q;q.render_state=&render;q.color_at_3ab9864=std::array<float,4>{0,0,0,0};
    sadx::NinjaQueueConsumerState consumer;sadx::GameLogicTimingState timing;sadx::NinjaSpriteProjection projection{};
    const std::optional<std::array<float,4>> initial=std::array<float,4>{f(in[10]),f(in[11]),f(in[12]),f(in[13])},view_color=std::array<float,4>{f(in[6]),f(in[7]),f(in[8]),f(in[9])};
    const std::optional<std::uint32_t> depth_mode=in[14];const std::array<std::optional<std::uint32_t>,3> w840{{1,2,3}},w858{{4,5,6}};std::array<std::optional<std::uint32_t>,3> w898{{0,0,0}},w84c{{0,0,0}};
    sadx::NinjaQueueLifecycleOwners lifecycle{queue,q,consumer,timing,projection,initial,&depth,c[5],c[6],depth_mode,w840,w898,w858,w84c};
    std::optional<std::uint8_t> pending=static_cast<std::uint8_t>(in[1]);std::optional<std::uint32_t> mode28114=7;
    sadx::view::SavedFovState saved_fov;
    sadx::reference::SavedFovReference saved_fov_reference;
    sadx::GameViewState v{f(in[2]),f(in[3]),c[0],c[1],c[2],c[3],c[4],pending,mode28114,{},in[4],{},{},{},{},{},{},in[5],{},saved_fov,[&] { saved_fov_reference.observe_write(); },{},view_color};
    sadx::GameViewRuntime rt{v,lifecycle,in[16],[&](const sadx::GameViewCall& call){trace.push_back({call.callee,call.source_call,call.arguments[0].value_or(0),call.arguments[1].value_or(0)});
        return sadx::GameLogicOwnerResult{fail()?sadx::GameLogicOwnerState::failed:sadx::GameLogicOwnerState::completed,0,{}};}};
    sadx::X87Context fp(static_cast<std::uint16_t>(in[0]));const auto r=sadx::run_game_view_4208e0(rt,fp);std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.completed;out[2]=q.mode_at_3ab98ac.value_or(0xdeadbeef);out[3]=mode28114.value_or(0);out[4]=v.word_at_3abd9cc.value_or(0xdeadbeef);
    const auto o=[&](const std::optional<float>& x){return x?bits(*x):0xdeadbeefU;};out[5]=o(v.view_at_3b28098);out[6]=o(v.view_at_3b2809c);out[7]=o(v.view_at_3b280a0);out[8]=o(v.view_at_3b280a4);
    out[9]=projection.scale_at_3d0f9e0?bits(*projection.scale_at_3d0f9e0):0xdeadbeef;out[10]=v.projection_words_at_3d0f9ec.value_or(0xdeadbeef);out[11]=v.projection_words_at_3d0f9f0.value_or(0xdeadbeef);
    out[12]=v.fov_at_3aad0bc.value_or(0);out[13]=v.fov_copy_at_3ab98ec.value_or(0xdeadbeef);out[14]=saved_fov_reference.export_value(saved_fov).value_or(0xdeadbeef);
    for(unsigned i=0;i<4;++i){out[15+i]=v.clear_colors_at_3ce7174[i].value_or(0xdeadbeef);out[19+i]=q.color_at_3ab9864?bits((*q.color_at_3ab9864)[i]):0;out[23+i]=projection.color_argb_at_3d0f7f0?bits((*projection.color_argb_at_3d0f7f0)[i]):0xdeadbeef;}
    out[27]=q.captured_word_at_3d0f9c8.value_or(0xdeadbeef);out[28]=q.captured_word_at_3d0f840.value_or(0xdeadbeef);out[29]=q.captured_word_at_3d0f9c4.value_or(0xdeadbeef);out[30]=render.packed_blend_at_3ab9890.value_or(0);
    out[31]=static_cast<std::uint32_t>(trace.size());for(std::size_t i=0;i<trace.size()&&i<20;++i)std::copy(trace[i].begin(),trace[i].end(),out+32+4*i);
    out[112]=fp.exception_flags()&0x3f;return 1;
}

#include "guest/frame_control_reference.hpp"
#include "game/game_view.hpp"
#include "game/game_view_native.hpp"
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
    struct Stop { int unused = 0; };
    sadx::X87Context fp(static_cast<std::uint16_t>(in[0]));
    const float scale_x=f(in[2]),scale_y=f(in[3]);
    std::uint8_t pending=static_cast<std::uint8_t>(in[1]);
    std::uint32_t pause=7;
    std::int32_t animation=static_cast<std::int32_t>(0xdeadbeef),perspective=static_cast<std::int32_t>(in[5]),perspective_copy=animation,last_perspective=animation;
    sadx::GameViewInitialScreen descriptor{f(in[4]),f(0xdeadbeef),f(0xdeadbeef),f(0xdeadbeef),f(0xdeadbeef)};
    float sprite_scale=f(0xdeadbeef),center_x=f(0xdeadbeef),center_y=f(0xdeadbeef);
    std::array<std::uint32_t,4> clears{0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef};
    sadx::screen::State state{scale_x,scale_y,pending,pause,animation,descriptor,sprite_scale,center_x,center_y,perspective,perspective_copy,last_perspective,*view_color};
    auto device=[&](const sadx::NinjaRenderResult &r) {if(!r)throw Stop{};};
    auto color=[&](const std::optional<std::array<float,4>> &value){
        sadx::NinjaQueueColorOwners owners{q.color_at_3ab9864,projection.color_argb_at_3d0f7f0,lifecycle.one_at_7dcb10,lifecycle.negative_one_at_7dcb0c};
        sadx::NinjaQueueColorResult result;std::uint32_t site=0;
        if(!sadx::ninja_queue_color_402f40(value,owners,result,fp,site))throw Stop{};
    };
    auto call=[&](std::uint32_t target,std::uint32_t site,std::uint32_t first,std::uint32_t second){trace.push_back({target,site,first,second});if(fail())throw Stop{};};
    sadx::screen::Services services{
        [&]{q.mode_at_3ab98ac=2;},
        [&]{if(!sadx::ninja_queue_reset_407ab0(lifecycle,in[16],fp))throw Stop{};},
        [&]{clears.fill(0xff000000U);},
        [&](std::uint32_t destination,std::uint32_t mode){device(sadx::ninja_set_blend(destination,mode,render));},
        [&]{color(initial);},
        [&](float x,float y,float,float,float,std::uint32_t){call(0x4071c0,0x4208c3,bits(x),bits(y));},
        [&](std::int32_t angle){call(0x7815c0,0x402edf,static_cast<std::uint32_t>(angle),0);},
        [&](std::uint32_t value){q.captured_word_at_3d0f9c8=value;},
        [&](std::uint32_t mask,std::uint32_t attributes){q.captured_word_at_3d0f840=mask;q.captured_word_at_3d0f9c4=attributes;},
        [&](const std::array<float,4>&value){color(value);},
        [&](float near_distance,float far_distance){call(0x403180,0x4209e1,bits(near_distance),bits(far_distance));},
        [](std::uint32_t){},[](bool){}
    };
    bool completed=false;
    try {sadx::screen::setup(state,services,fp);completed=true;} catch(Stop) {}
    std::fill(out,out+capacity,0);
    out[0]=completed?0:1;out[1]=completed;out[2]=q.mode_at_3ab98ac.value_or(0xdeadbeef);out[3]=pause;out[4]=animation;
    out[5]=bits(descriptor.width);out[6]=bits(descriptor.height);out[7]=bits(descriptor.center_x);out[8]=bits(descriptor.center_y);
    out[9]=bits(sprite_scale);out[10]=bits(center_x);out[11]=bits(center_y);out[12]=perspective;out[13]=perspective_copy;out[14]=last_perspective;
    for(unsigned i=0;i<4;++i){out[15+i]=clears[i];out[19+i]=q.color_at_3ab9864?bits((*q.color_at_3ab9864)[i]):0;out[23+i]=projection.color_argb_at_3d0f7f0?bits((*projection.color_argb_at_3d0f7f0)[i]):0xdeadbeef;}
    out[27]=q.captured_word_at_3d0f9c8.value_or(0xdeadbeef);out[28]=q.captured_word_at_3d0f840.value_or(0xdeadbeef);out[29]=q.captured_word_at_3d0f9c4.value_or(0xdeadbeef);out[30]=render.packed_blend_at_3ab9890.value_or(0);
    out[31]=static_cast<std::uint32_t>(trace.size());for(std::size_t i=0;i<trace.size()&&i<20;++i)std::copy(trace[i].begin(),trace[i].end(),out+32+4*i);
    out[112]=fp.exception_flags()&0x3f;return 1;
}
extern "C" int sadx_native_screen_frame(std::uint32_t pending, std::uint32_t operation,
                                         std::uint32_t fail_call, std::uint32_t *out) {
    struct Stop { int unused = 0; };
    std::uint8_t reset_pending = static_cast<std::uint8_t>(pending);
    std::vector<std::array<std::uint32_t, 2>> calls;
    auto record = [&](std::uint32_t function, std::uint32_t argument) {
        calls.push_back({function, argument});
        if (fail_call && calls.size() == fail_call)
            throw Stop{};
    };
    sadx::screen::Services services{};
    services.set_depth_compare = [&](std::uint32_t value) { record(0x77ed00, value); };
    services.set_depth_write = [&](bool value) { record(0x77ed20, value); };
    services.reset_material = [&] { record(0x4128f0, 0); };
    bool complete = false;
    try {
        if (operation == 0)
            sadx::screen::begin_frame(reset_pending);
        else
            sadx::screen::end_frame(reset_pending, services);
        complete = true;
    } catch (Stop) {}
    std::fill(out, out + 10, 0);
    out[0] = reset_pending;
    out[1] = complete;
    out[2] = static_cast<std::uint32_t>(calls.size());
    for (std::size_t index = 0; index < calls.size(); ++index) {
        out[3 + index * 2] = calls[index][0];
        out[4 + index * 2] = calls[index][1];
    }
    return 1;
}

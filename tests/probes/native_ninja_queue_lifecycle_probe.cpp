#include "ninja/ninja_queue_lifecycle.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 408630 matching tools/verify_original_ninja_queue_lifecycle.py.
// in: 0 heap address, 1 cw, 2..5 colour 3b18230, 6 depth mode 8928c8, 7..9 900840..48, 10..12 900858..60,
//     13 initial 3b12208, 14 3abdbc0, 15 3ab98ac, 16 3b1117c, 17 3abd9d4, 18 3abd9dc, 19..22 3ab9864, 23 fail call N
extern "C" int sadx_native_ninja_queue_lifecycle(const std::uint8_t* tables,std::size_t table_size,const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!tables||table_size!=40||!in||count!=24||!out||capacity<80)return 0;
    std::array<std::uint32_t,8> depth;std::memcpy(depth.data(),tables,32);float one,neg;std::memcpy(&one,tables+32,4);std::memcpy(&neg,tables+36,4);
    const auto f=[](std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;};const auto bits=[](float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;};
    std::vector<std::array<std::uint32_t,3>> trace;
    sadx::NinjaRenderState render;auto device=std::make_shared<sadx::NinjaRenderDevice>();device->identity=1;render.device_at_3d128b0=device;
    render.set_render_state=[&](const auto& c){trace.push_back({c.source_call,c.state,c.value});if(in[23]&&trace.size()==in[23])return sadx::NinjaRenderCallResult{false,{}};return sadx::NinjaRenderCallResult{true,0};};
    auto arena=std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(64,0xcd),0,64,1);sadx::NinjaSpriteQueue queue(arena);
    sadx::NinjaSpriteQueueRuntime q;q.render_state=&render;q.flag_at_3b12208=in[13];q.mode_at_3ab98ac=in[15];q.color_at_3ab9864=std::array<float,4>{f(in[19]),f(in[20]),f(in[21]),f(in[22])};
    sadx::NinjaQueueConsumerState consumer;consumer.mode_byte_at_3abdbc0=static_cast<std::uint8_t>(in[14]);consumer.count_at_3abd9d4=static_cast<std::int32_t>(in[17]);consumer.count_at_3abd9dc=static_cast<std::int32_t>(in[18]);
    sadx::GameLogicTimingState timing;timing.word_at_3b1117c=static_cast<std::int32_t>(in[16]);sadx::NinjaSpriteProjection projection{};
    const std::optional<std::array<float,4>> initial=std::array<float,4>{f(in[2]),f(in[3]),f(in[4]),f(in[5])};const std::optional<std::uint32_t> depth_mode=in[6];
    const std::array<std::optional<std::uint32_t>,3> w840{{in[7],in[8],in[9]}},w858{{in[10],in[11],in[12]}};std::array<std::optional<std::uint32_t>,3> w898{{0xefefefefU,0xefefefefU,0xefefefefU}},w84c{{0xefefefefU,0xefefefefU,0xefefefefU}}; // harness pre-fill
    sadx::NinjaQueueLifecycleOwners o{queue,q,consumer,timing,projection,initial,&depth,one,neg,depth_mode,w840,w898,w858,w84c};
    sadx::X87Context fp(static_cast<std::uint16_t>(in[1]));const auto r=sadx::ninja_queue_allocate_408630(o,in[0],fp);
    std::fill(out,out+capacity,0);const auto& a=*queue.arena();
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.completed;out[3]=a.cursor();
    bool zero=true;for(auto b:a.bytes())zero&=b==0;out[4]=zero;out[5]=a.overflow_reported().value_or(0xff);
    for(unsigned i=0;i<4;++i){out[6+i]=q.color_at_3ab9864?bits((*q.color_at_3ab9864)[i]):0xdeadbeef;out[10+i]=projection.color_argb_at_3d0f7f0?bits((*projection.color_argb_at_3d0f7f0)[i]):0xdeadbeef;}
    for(unsigned i=0;i<3;++i){out[14+i]=w898[i].value_or(0xdeadbeef);out[17+i]=w84c[i].value_or(0xdeadbeef);}
    out[20]=q.flag_at_3b12208.value_or(0xdeadbeef);out[21]=consumer.mode_byte_at_3abdbc0.value_or(0xef);out[22]=q.mode_at_3ab98ac.value_or(0xdeadbeef);
    out[23]=static_cast<std::uint32_t>(timing.word_at_3b1117c.value_or(0));out[24]=static_cast<std::uint32_t>(consumer.count_at_3abd9d4.value_or(0));out[25]=static_cast<std::uint32_t>(consumer.count_at_3abd9dc.value_or(0));
    const auto front=queue.bucket_front(0);out[26]=bool(front)&&!front.record;out[27]=static_cast<std::uint32_t>(trace.size());
    for(std::size_t i=0;i<trace.size()&&i<8;++i)std::copy(trace[i].begin(),trace[i].end(),out+28+3*i);
    out[52]=fp.exception_flags()&0x3f;return 1;
}

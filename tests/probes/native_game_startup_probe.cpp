#include "guest/frame_control_reference.hpp"
#include "game/game_startup.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 40c030 over the six source pages used by tools/verify_original_game_startup.py.
// pages: 3abd000,3b0e000,3b29000,900000,903000,3d08000 (4096 bytes each).
namespace {
constexpr std::uint32_t kPages[6]={0x3abd000,0x3b0e000,0x3b29000,0x900000,0x903000,0x3d08000};
struct Memory {
    std::vector<std::uint8_t> bytes;
    std::uint8_t* at(std::uint32_t a){for(unsigned p=0;p<6;++p)if(a>=kPages[p]&&a<kPages[p]+4096)return bytes.data()+p*4096+(a-kPages[p]);return nullptr;}
    std::uint8_t b(std::uint32_t a){return *at(a);} std::uint32_t w(std::uint32_t a){std::uint32_t v;std::memcpy(&v,at(a),4);return v;}
    void put8(std::uint32_t a,std::optional<std::uint8_t> v){if(v)*at(a)=*v;} void put32(std::uint32_t a,std::optional<std::uint32_t> v){if(v)std::memcpy(at(a),&*v,4);}
};
}
extern "C" int sadx_native_game_startup(const std::uint8_t* pages,std::size_t size,const std::uint32_t* in,std::size_t count,std::uint8_t* out_pages,std::uint32_t* out,std::size_t capacity){
    if(!pages||size!=6*4096||!in||count!=4||!out_pages||!out||capacity<64)return 0;
    Memory m{std::vector<std::uint8_t>(pages,pages+size)};
    sadx::GameLogicEdgeState edges;for(unsigned r=0;r<256;++r)for(unsigned k=0;k<3;++k)edges.raw_at_3b0e3e2[r][k]=m.b(0x3b0e3e2+3*r+k);
    edges.bytes_at_3b0e3e0={m.b(0x3b0e3e0),m.b(0x3b0e3e1)};edges.special_at_3b0e3ba=m.b(0x3b0e3ba);edges.byte_at_3b0e3b9=m.b(0x3b0e3b9);
    std::optional<std::int32_t> save_phase;sadx::GameFrameInputState input{edges.soft_reset_edge(),{},{},save_phase};input.reset_input_mask=m.b(0x3b0e3b8);
    sadx::GameControllerInitGlobals g;for(unsigned i=0;i<4;++i){g.words_at_3b0e768[i]=m.w(0x3b0e768+4*i);g.words_at_3b0e344[i]=m.w(0x3b0e344+4*i);g.words_at_3b0e9b8[i]=m.w(0x3b0e9b8+4*i);g.bytes_at_3b0e9a4[i]=m.b(0x3b0e9a4+i);}
    for(unsigned i=0;i<5;++i)g.words_at_3b0e990[i]=m.w(0x3b0e990+4*i);
    sadx::GameLogicController tmpl;for(unsigned b=0;b<52;++b)tmpl.write_byte(b,m.b(0x3b0eaa8+b));
    std::array<std::shared_ptr<sadx::GameLogicController>,4> raw;for(unsigned r=0;r<4;++r){raw[r]=std::make_shared<sadx::GameLogicController>();for(unsigned b=0;b<52;++b)raw[r]->write_byte(b,m.b(0x3b0e9c8+0x34*r+b));}
    sadx::GameLogicControllers pointers;
    sadx::AudioStartupState audio;audio.word_at_3b29ce0=m.w(0x3b29ce0);audio.word_at_3b29ce4=m.w(0x3b29ce4);audio.word_at_3b29b7c=m.w(0x3b29b7c);audio.byte_at_3b29ce8=m.b(0x3b29ce8);
    for(unsigned i=0;i<36;++i)audio.record_words_at_3b292fc[i]=m.w(0x3b292fc+0x38*i);for(unsigned i=0;i<18;++i)audio.words_at_3b292a8[i]=m.w(0x3b292a8+4*i);
    sadx::MenuBlockState menu;for(unsigned i=0;i<48;++i){menu.block_at_900e88[i]=m.b(0x900e88+i);menu.block_at_9008b4[i]=m.b(0x9008b4+i);}menu.byte_at_3abdbc1=m.b(0x3abdbc1);menu.byte_at_903e58=m.b(0x903e58);
    sadx::ExitCallbackList exits;for(unsigned i=0;i<32;++i)exits.slots_at_3d084b0[i]=m.w(0x3d084b0+4*i);
    std::optional<std::int32_t> mode=static_cast<std::int32_t>(m.w(0x3abdc7c));std::optional<std::uint32_t> w80=m.w(0x3abdc80),w78=m.w(0x3abdc78),w90=m.w(0x3abdc90);
    std::vector<std::array<std::uint32_t,4>> trace;
    sadx::StartupRuntime rt{{edges,input,g,tmpl,raw,pointers,nullptr,audio,menu,exits,mode,w80,w78,w90},{}};
    rt.invoke=[&](const sadx::StartupCall& c){trace.push_back({c.callee,c.source_call,c.arguments[0].value_or(0),c.arguments[1].value_or(0)});
        if(in[0]&&trace.size()==in[0])return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::failed,0x123};return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0};};
    const auto r=sadx::run_startup_40c030(rt);
    for(unsigned r2=0;r2<256;++r2)for(unsigned k=0;k<3;++k)m.put8(0x3b0e3e2+3*r2+k,edges.raw_at_3b0e3e2[r2][k]);
    m.put8(0x3b0e3e0,edges.bytes_at_3b0e3e0[0]);m.put8(0x3b0e3e1,edges.bytes_at_3b0e3e0[1]);m.put8(0x3b0e3ba,edges.special_at_3b0e3ba);m.put8(0x3b0e3b9,edges.byte_at_3b0e3b9);m.put8(0x3b0e3b8,input.reset_input_mask);
    for(unsigned i=0;i<4;++i){m.put32(0x3b0e768+4*i,g.words_at_3b0e768[i]);m.put32(0x3b0e344+4*i,g.words_at_3b0e344[i]);m.put32(0x3b0e9b8+4*i,g.words_at_3b0e9b8[i]);m.put8(0x3b0e9a4+i,g.bytes_at_3b0e9a4[i]);}
    for(unsigned i=0;i<5;++i)m.put32(0x3b0e990+4*i,g.words_at_3b0e990[i]);
    for(unsigned rr=0;rr<4;++rr)for(unsigned b=0;b<52;++b)m.put8(0x3b0e9c8+0x34*rr+b,raw[rr]->read_byte(b));
    for(unsigned i=0;i<4;++i)if(pointers[i]&&*pointers[i])for(unsigned rr=0;rr<4;++rr)if(*pointers[i]==raw[rr])m.put32(0x3b0e77c+4*i,0x3b0e9c8+0x34*rr);
    m.put32(0x3b29ce0,audio.word_at_3b29ce0);m.put32(0x3b29ce4,audio.word_at_3b29ce4);m.put32(0x3b29b7c,audio.word_at_3b29b7c);m.put8(0x3b29ce8,audio.byte_at_3b29ce8);
    for(unsigned i=0;i<36;++i)m.put32(0x3b292fc+0x38*i,audio.record_words_at_3b292fc[i]);for(unsigned i=0;i<18;++i)m.put32(0x3b292a8+4*i,audio.words_at_3b292a8[i]);
    for(unsigned i=0;i<48;++i){m.put8(0x9008b4+i,menu.block_at_9008b4[i]);}for(unsigned i=0;i<48;++i)m.put8(0x900e88+i,menu.block_at_900e88[i]);m.put8(0x3abdbc1,menu.byte_at_3abdbc1);m.put8(0x903e58,menu.byte_at_903e58);
    for(unsigned i=0;i<32;++i)m.put32(0x3d084b0+4*i,exits.slots_at_3d084b0[i]);
    m.put32(0x3abdc7c,static_cast<std::uint32_t>(*mode));m.put32(0x3abdc80,w80);m.put32(0x3abdc78,w78);m.put32(0x3abdc90,w90);
    std::copy(m.bytes.begin(),m.bytes.end(),out_pages);std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.completed;out[3]=static_cast<std::uint32_t>(trace.size());
    for(std::size_t i=0;i<trace.size()&&i<12;++i)std::copy(trace[i].begin(),trace[i].end(),out+4+4*i);
    return 1;
}

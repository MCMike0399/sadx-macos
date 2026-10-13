#include "game/game_reset.hpp"
#include "game/game_reset_leaves.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 40bf40 + ported leaves over the eleven pages of tools/verify_original_game_reset_composed.py.
namespace {
constexpr std::uint32_t kPages[12]={0x3abd000,0x892000,0x3ab9000,0x912000,0x3b29000,0x3b11000,0x3c5e000,0x3b0e000,0x3d08000,0x389d000,0x2bcb000,0x3c4e000};
struct Memory {std::vector<std::uint8_t> b;
    std::uint8_t* at(std::uint32_t a){for(unsigned p=0;p<12;++p)if(a>=kPages[p]&&a<kPages[p]+4096)return b.data()+p*4096+(a-kPages[p]);return nullptr;}
    std::uint32_t w(std::uint32_t a){std::uint32_t v;std::memcpy(&v,at(a),4);return v;} std::uint16_t h(std::uint32_t a){std::uint16_t v;std::memcpy(&v,at(a),2);return v;}
    template<class T>void put(std::uint32_t a,const std::optional<T>& v){if(v){T x=*v;std::memcpy(at(a),&x,sizeof x);}}};
}
extern "C" int sadx_native_game_reset_composed(const std::uint8_t* pages,std::size_t size,std::uint32_t fail,std::uint8_t* out_pages,std::uint32_t* out,std::size_t capacity){
    if(size!=12*4096||capacity<200)return 0;Memory m{std::vector<std::uint8_t>(pages,pages+size)};
    std::optional<std::uint32_t> w80=m.w(0x3abdf80),depth=m.w(0x8928c8),qmode=m.w(0x3ab98ac),init=m.w(0x3b0ef2c);std::optional<std::int32_t> t18c=static_cast<std::int32_t>(m.w(0x3b1118c));
    std::array<std::optional<std::uint32_t>,36> rec;for(unsigned i=0;i<36;++i)rec[i]=m.w(0x3b292fc+0x38*i);
    sadx::CrtRandState rng{m.w(0x2bcbdf8)};sadx::GameResetState s{w80,m.h(0x3b29d08),m.w(0x3b290d0),m.w(0x3b0e330),rng};
    sadx::ResetLeafState L{m.w(0x3abdf90),m.w(0x3abdfac),depth,qmode,m.w(0x912694),m.w(0x912698),m.w(0x91269c),m.w(0x9126a8),m.w(0x3b29cf4),m.w(0x3b29cf8),
        m.w(0x3b11190),m.w(0x3c5e8b8),t18c,m.w(0x3b291b4),rec,{},{},init,m.w(0x3abdf94),m.w(0x3abdf9c),m.w(0x3abdfa0),m.w(0x3abdf98),m.w(0x3abdfa8),m.w(0x3abdfa4),m.w(0x3a7cf3c),m.w(0x38f6ec0),m.w(0x3b0e338),{},m.w(0x389d608),{},{},{},{},{}};
    for(unsigned i=0;i<36;++i){L.sound_record_flags_at_3b29304[i]=m.w(0x3b29304+0x38*i);L.words_at_3b29b90[i]=m.w(0x3b29b90+4*i);}
    for(unsigned i=0;i<4;++i)L.entry_words_at_3d08428[i]=m.w(0x3d08428+24*i);
    L.word_at_3c4e458=m.w(0x3c4e458);L.word_at_3c4e45c=m.w(0x3c4e45c);for(unsigned i=0;i<6;++i){L.set_files_at_3abdf40[i]=m.w(0x3abdf40+4*i);L.cam_files_at_3abdf28[i]=m.w(0x3abdf28+4*i);}
    std::vector<std::array<std::uint32_t,3>> trace;std::uint32_t failed_at=0;
    const auto fixture=[&](std::uint32_t callee,std::uint32_t site,std::uint32_t arg){trace.push_back({callee,site,arg});
        if(fail&&trace.size()==fail){failed_at=site;return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::failed,0x123,{}};}return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0,{}};};
    const sadx::ResetLeafNested nested=[&](const sadx::ResetLeafCall& c){return fixture(c.callee,c.source_call,c.this_object.value_or(c.argument.value_or(0)));};
    std::uint32_t leaf_stop=0;
    sadx::GameResetRuntime rt{s,[&](const sadx::GameResetCall& c){
        const auto leaf=sadx::run_reset_leaf(c.callee,c.argument,L,nested);
        if(leaf.outcome==sadx::ResetLeafOutcome::completed)return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0,{}};
        if(leaf.outcome==sadx::ResetLeafOutcome::not_a_leaf)return fixture(c.callee,c.source_call,c.argument.value_or(0));
        leaf_stop=leaf.source_boundary;return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::failed,leaf.source_boundary,{}};}};
    const auto r=sadx::run_game_reset_40bf40(rt);
    m.put(0x3abdf80,w80);m.put(0x8928c8,depth);m.put(0x3ab98ac,qmode);m.put(0x3b0ef2c,init);m.put(0x3b1118c,t18c);m.put(0x2bcbdf8,rng.seed_at_2bcbdf8);
    m.put(0x3b29d08,s.word_at_3b29d08);m.put(0x3b290d0,s.heap_at_3b290d0);m.put(0x3b0e330,s.heap_at_3b0e330);
    m.put(0x3abdf90,L.word_at_3abdf90);m.put(0x3abdfac,L.word_at_3abdfac);m.put(0x912694,L.word_at_912694);m.put(0x912698,L.word_at_912698);m.put(0x91269c,L.word_at_91269c);m.put(0x9126a8,L.word_at_9126a8);
    m.put(0x3b29cf4,L.word_at_3b29cf4);m.put(0x3b29cf8,L.word_at_3b29cf8);m.put(0x3b11190,L.word_at_3b11190);m.put(0x3b291b4,L.callback_at_3b291b4);
    m.put(0x3abdfa0,L.word_at_3abdfa0);m.put(0x3abdf98,L.word_at_3abdf98);m.put(0x3abdfa8,L.word_at_3abdfa8);m.put(0x3b0e338,L.word_at_3b0e338);
    for(unsigned i=0;i<36;++i){m.put(0x3b292fc+0x38*i,rec[i]);m.put(0x3b29304+0x38*i,L.sound_record_flags_at_3b29304[i]);m.put(0x3b29b90+4*i,L.words_at_3b29b90[i]);}
    for(unsigned i=0;i<4;++i)m.put(0x3d08428+24*i,L.entry_words_at_3d08428[i]);
    m.put(0x3c4e458,L.word_at_3c4e458);m.put(0x3c4e45c,L.word_at_3c4e45c);for(unsigned i=0;i<6;++i){m.put(0x3abdf40+4*i,L.set_files_at_3abdf40[i]);m.put(0x3abdf28+4*i,L.cam_files_at_3abdf28[i]);}
    std::copy(m.b.begin(),m.b.end(),out_pages);std::fill(out,out+capacity,0);
    out[0]=r.completed;out[1]=r.completed?0:(leaf_stop?leaf_stop:(failed_at?failed_at:r.source_boundary));out[2]=static_cast<std::uint32_t>(trace.size());
    for(std::size_t i=0;i<trace.size()&&i<60;++i)std::copy(trace[i].begin(),trace[i].end(),out+3+3*i);
    return 1;
}

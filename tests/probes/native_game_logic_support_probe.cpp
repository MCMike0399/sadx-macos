#include "game/game_logic_support.hpp"
#include <cstring>
namespace {
void diag(const sadx::GameLogicSupportResult& r,std::uint64_t* out){out[0]=static_cast<unsigned>(r.failure);out[1]=r.source_boundary;out[2]=r.completed;out[3]=r.byte_index?*r.byte_index:0xffffffffffffffffULL;out[4]=r.committed_bytes;out[5]=r.quotient?static_cast<std::uint32_t>(*r.quotient):0;out[6]=r.incremented_quotient?static_cast<std::uint32_t>(*r.incremented_quotient):0;out[7]=r.product?static_cast<std::uint32_t>(*r.product):0;out[8]=(r.quotient?1:0)|(r.incremented_quotient?2:0)|(r.product?4:0)|(r.clamped?8:0);out[9]=r.clamped&&*r.clamped;}
}
extern "C" int sadx_native_game_edges(std::int32_t mode,const std::uint8_t* bytes,const std::uint8_t* present,std::uint8_t* output,std::uint8_t* known,std::uint64_t* diagnostics){
    if(!bytes||!present||!output||!known||!diagnostics)return -1;sadx::GameLogicEdgeState state;
    for(std::size_t i=0;i<256;++i){for(std::size_t lane=0;lane<3;++lane)if(present[i*3+lane])state.raw_at_3b0e3e2[i][lane]=bytes[i*3+lane];if(present[768+i])state.aggregate_at_3b0eaf0[i]=bytes[768+i];}
    if(present[1024])state.special_at_3b0e3ba=bytes[1024];if(present[1025])state.aggregate_special_at_3b0eae8=bytes[1025];auto r=sadx::aggregate_game_logic_edges(mode,state);diag(r,diagnostics);
    const auto put=[&](const auto& value,std::size_t index){known[index]=value.has_value();output[index]=value?*value:0;};
    for(std::size_t i=0;i<256;++i){for(std::size_t lane=0;lane<3;++lane)put(state.raw_at_3b0e3e2[i][lane],i*3+lane);put(state.aggregate_at_3b0eaf0[i],768+i);}put(state.special_at_3b0e3ba,1024);put(state.aggregate_special_at_3b0eae8,1025);return 0;
}
extern "C" int sadx_native_game_ticks(const std::int32_t* words,const std::uint8_t* present,std::int32_t* output,std::uint8_t* known,std::uint64_t* diagnostics){
    if(!words||!present||!output||!known||!diagnostics)return -1;sadx::GameLogicTimingState s;
    std::optional<std::int32_t>* fields[]={&s.word_at_3b11178,&s.word_at_3b1117c,&s.word_at_3b11180,&s.word_at_3b11188,&s.word_at_3b1118c,&s.word_at_3b11198,&s.word_at_3b2a2e4};
    for(std::size_t i=0;i<7;++i)if(present[i])*fields[i]=words[i];auto r=sadx::calculate_game_logic_ticks(s);diag(r,diagnostics);for(std::size_t i=0;i<7;++i){known[i]=fields[i]->has_value();output[i]=known[i]?**fields[i]:0;}return 0;
}
extern "C" const char* sadx_native_game_logic_support_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 source byte edges and signed32 IDIV/INC/IMUL";}

#include "game/game_reset_leaves.hpp"
#include <algorithm>
// Native 40d0d0 (4253c0 thunk) with the layout of tools/verify_original_stream_players.py.
// in: 0 3b0ef2c, 1 3a7cf3c, 2 38f6ec0, 3..8 3abdf94/9c/a0/a4/98/a8.
extern "C" int sadx_native_stream_players(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=9||!out||capacity<10)return 0;
    std::optional<std::uint32_t> depth,qmode,init=in[0];std::optional<std::int32_t> t18c;std::array<std::optional<std::uint32_t>,36> rec{};
    sadx::ResetLeafState L{{},{},depth,qmode,{},{},{},{},{},{},{},{},t18c,{},rec,{},{},init,in[3],in[4],in[5],in[7],in[8],in[6],in[1],in[2],{},{},{},{},{},{},{},{}};
    std::uint32_t site=0;
    const auto r=sadx::run_reset_leaf(0x4253c0,{},L,[&](const sadx::ResetLeafCall& c){site=c.source_call;return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::unavailable,c.callee,{}};});
    std::fill(out,out+capacity,0);
    out[0]=r.outcome==sadx::ResetLeafOutcome::completed;out[1]=site;
    const std::optional<std::uint32_t>* words[]={&L.player_at_3abdf94,&L.player_at_3abdf9c,&L.word_at_3abdfa0,&L.word_at_3abdfa4,&L.word_at_3abdf98,&L.word_at_3abdfa8};
    for(int i=0;i<6;++i)out[2+i]=words[i]->value_or(0xdeadbeef);
    return 1;
}

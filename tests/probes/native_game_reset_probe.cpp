#include "game/game_reset.hpp"
#include <algorithm>
#include <vector>
// in: 0 3abdf80,1 3b29d08,2 3b290d0,3 3b0e330,4 seed,5 fail at call N
extern "C" int sadx_native_game_reset(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=6||!out||capacity<128)return 0;
    std::optional<std::uint32_t> w80=in[0];sadx::CrtRandState rng{in[4]};
    sadx::GameResetState s{w80,static_cast<std::uint16_t>(in[1]),in[2],in[3],rng};std::vector<std::array<std::uint32_t,3>> trace;
    sadx::GameResetRuntime rt{s,[&](const sadx::GameResetCall& c){trace.push_back({c.callee,c.source_call,c.argument.value_or(0)});
        if(in[5]&&trace.size()==in[5])return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::failed,0x123,{}};return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0,{}};}};
    const auto r=sadx::run_game_reset_40bf40(rt);std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.completed;out[3]=*w80;out[4]=*s.word_at_3b29d08;out[5]=*s.heap_at_3b290d0;out[6]=*s.heap_at_3b0e330;out[7]=*rng.seed_at_2bcbdf8;
    out[8]=static_cast<std::uint32_t>(trace.size());for(std::size_t i=0;i<trace.size()&&i<36;++i)std::copy(trace[i].begin(),trace[i].end(),out+9+3*i);
    return 1;
}

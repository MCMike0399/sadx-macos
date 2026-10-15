#include "game/game_loop.hpp"
#include <algorithm>
#include <vector>
// Native 411d00 matching tools/verify_original_frame_completion.py.
extern "C" int sadx_native_frame_completion(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=16||!out||capacity<40)return 0;
    sadx::GameLoopState loop;loop.resync_at_3abdf68=static_cast<std::uint8_t>(in[1]);loop.mode_at_3b28114=in[5];
    sadx::GameLogicTimingState timing;timing.frame_multiplier=static_cast<std::int32_t>(in[3]);timing.word_at_3b11198=static_cast<std::int32_t>(in[4]);timing.word_at_3b11180=static_cast<std::int32_t>(0x5a5a5a5a);
    sadx::GameLogicState logic{timing,{},static_cast<std::int32_t>(0x6b6b6b6b)};
    sadx::GameFrameRendererState renderer{};renderer.configured_frame_interval=static_cast<std::int32_t>(in[2]);
    std::vector<std::array<std::uint32_t,2>> trace;
    sadx::FrameCompletionRuntime runtime{loop,logic,renderer,in[0]?sadx::FrameCompletionPolicy::original_unported:sadx::FrameCompletionPolicy::speedrun_2_0,{}};
    runtime.invoke=[&](const sadx::FrameCompletionCall& c)->sadx::GameLogicOwnerResult{trace.push_back({c.callee,c.source_call});
        if(in[7]&&trace.size()==in[7])return {sadx::GameLogicOwnerState::failed,0x123};
        if(c.operation==sadx::FrameCompletionOperation::thunk_781030&&in[6])loop.mode_at_3b28114=in[6]-1;
        return {sadx::GameLogicOwnerState::completed,0};};
    const auto r=sadx::run_frame_completion(runtime);std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.nested_source_boundary;out[3]=r.completed;
    out[4]=static_cast<std::uint32_t>(timing.word_at_3b11180.value_or(0));out[5]=static_cast<std::uint32_t>(logic.word_at_3b11194.value_or(0));out[6]=static_cast<std::uint32_t>(timing.word_at_3b11198.value_or(0));
    out[7]=loop.mode_at_3b28114.value_or(0xdeadbeef);out[8]=loop.resync_at_3abdf68.value_or(0xef);out[9]=static_cast<std::uint32_t>(trace.size());
    for(std::size_t i=0;i<trace.size()&&i<8;++i){out[10+2*i]=trace[i][0];out[11+2*i]=trace[i][1];}
    return 1;
}

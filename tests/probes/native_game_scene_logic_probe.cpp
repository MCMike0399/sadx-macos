#include "game/game_scene_logic.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {std::int32_t signed_word(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}}
extern "C" int sadx_native_game_scene_logic(const std::uint32_t* input,std::size_t size,std::uint32_t* output,std::size_t capacity){
    if(!input||size!=8||!output||capacity<32)return 0;
    std::optional<std::uint16_t> stage=static_cast<std::uint16_t>(input[1]);
    std::optional<std::int32_t> mode=signed_word(input[0]);sadx::GameSceneLogicState state{stage,&mode,static_cast<std::uint8_t>(input[2]),static_cast<std::uint8_t>(input[3])};
    sadx::TaskScheduler scheduler;sadx::X87Context arithmetic(0x7f);std::vector<std::uint32_t> trace;
    sadx::GameSceneLogicRuntime runtime{state,scheduler,arithmetic,{}};
    runtime.invoke=[&](const sadx::GameSceneCall& call,sadx::X87Context&){
        trace.push_back(call.callee);
        if(trace.size()==input[5])return sadx::GameSceneOwnerResult{{sadx::GameLogicOwnerState::failed,call.callee},{}};
        if(call.operation==sadx::GameSceneOperation::conditional_426070&&input[6])stage=static_cast<std::uint16_t>(input[7]);
        return sadx::GameSceneOwnerResult{{sadx::GameLogicOwnerState::completed,0},call.operation==sadx::GameSceneOperation::query_42fb00?std::optional<std::int32_t>{signed_word(input[4])}:std::optional<std::int32_t>{}};
    };
    const auto result=sadx::run_game_scene_logic(runtime);std::fill(output,output+32,0);
    output[0]=static_cast<std::uint32_t>(result.failure);output[1]=result.source_boundary;output[2]=result.nested_source_boundary;
    output[3]=result.tasks_completed;output[4]=result.completed;output[5]=*state.byte_at_3b0ef40;output[6]=*state.byte_at_3b0f150;output[7]=*stage;output[8]=trace.size();
    std::copy(trace.begin(),trace.end(),output+9);return 1;
}

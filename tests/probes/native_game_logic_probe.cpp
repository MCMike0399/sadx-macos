#include "guest/frame_control_reference.hpp"
#include "engine/game_logic.hpp"
#include <cstring>
#include <vector>
namespace {
std::int32_t signed_word(std::uint32_t value){std::int32_t result;std::memcpy(&result,&value,4);return result;}
}
extern "C" int sadx_native_game_logic(const std::uint32_t* input,std::size_t count,
                                      std::uint32_t* output,std::size_t capacity){
    // This fixed fixture protocol bounds its instrumentation, not the native
    // routine. Production run_game_logic imposes no arbitrary tick count cap.
    if(!input||count!=18||!output||capacity<128||input[4]>8)return 0;
    sadx::GameLogicControllers pads;for(unsigned i=0;i<4;++i){auto pad=std::make_shared<sadx::GameLogicController>();pad->write_u32(0x10,input[8+i]);pads[i]=pad;}
    sadx::GameLogicTimingState timing;
    sadx::GameLogicState state{timing,{},{}};state.timing.word_at_3b11198=signed_word(input[0]);state.word_at_3b11194=signed_word(input[1]);
    state.timing.pending_logic_ticks=signed_word(input[2]);state.timing.logic_increment=signed_word(input[3]);
    state.word_at_3abdf70=input[5];state.timing.word_at_3b1118c=signed_word(input[6]);
    std::vector<std::uint32_t> trace;unsigned calls=0,polls=0;
    sadx::GameLogicRuntime runtime{pads,state,{},{} };
    runtime.invoke=[&](const sadx::GameLogicCall& call){
        ++calls;trace.push_back(call.callee);trace.push_back(call.first_argument?std::uint32_t(*call.first_argument):0xffffffffU);trace.push_back(call.second_argument?std::uint32_t(*call.second_argument):0xffffffffU);
        if(calls==input[7])return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::failed,call.callee};
        if(call.operation==sadx::GameLogicOperation::calculate_ticks){state.timing.pending_logic_ticks=signed_word(input[4]);state.timing.word_at_3b1118c=signed_word(input[4]);}
        if(call.operation==sadx::GameLogicOperation::poll_input){++polls;for(unsigned i=0;i<4;++i)(*pads[i])->write_u32(0x10,input[12+i]);}
        if(call.operation==sadx::GameLogicOperation::scene_logic)for(auto& pad:pads)(*pad)->write_u32(0x10,0);
        if(call.operation==sadx::GameLogicOperation::set_mode&&input[16]){auto pad=std::make_shared<sadx::GameLogicController>();pad->write_u32(0x10,input[17]);pads[0]=pad;}
        return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0};
    };
    const auto result=sadx::run_game_logic(runtime);if(trace.size()>112)return 0;
    std::fill(output,output+128,0);output[0]=static_cast<std::uint32_t>(result.failure);output[1]=result.source_boundary;output[2]=result.nested_source_boundary;output[3]=static_cast<std::uint32_t>(result.completed_scene_ticks);
    output[4]=*state.word_at_3abdf70;output[5]=std::uint32_t(*state.timing.word_at_3b11198);output[6]=std::uint32_t(*state.word_at_3b11194);
    output[7]=std::uint32_t(*state.timing.word_at_3b1118c);output[8]=std::uint32_t(*state.timing.pending_logic_ticks);output[9]=std::uint32_t(*state.timing.logic_increment);
    for(unsigned i=0;i<4;++i)output[10+i]=*(*pads[i])->read_u32(0x10);
    output[14]=calls;output[15]=polls;std::copy(trace.begin(),trace.end(),output+16);return 1;
}

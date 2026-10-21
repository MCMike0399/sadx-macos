#include "guest/caption_frame_reference.hpp"
#include <algorithm>
namespace {
std::uint32_t value(const std::optional<std::uint32_t> &word){return word.value_or(0xdeadbeef);}
}
extern "C" void sadx_native_caption_frame_reference_probe(const std::uint32_t *i,std::uint32_t *o){
    std::fill_n(o,64,0);
    sadx::menu_caption::State state;
    sadx::reference::CaptionFrameReference adapter(state);
    adapter.observe_timer(i[2]?std::optional<std::uint32_t>{i[0]}:std::nullopt);
    adapter.observe_buffer(i[3]?std::optional<std::uint32_t>{i[1]}:std::nullopt);
    unsigned count=0;
    sadx::GameFrameOwnerInvoke invoke;
    if(i[8])invoke=[&](const sadx::GameFrameOwnerCall &call){
        o[24+count*4]=call.source_call;o[25+count*4]=call.callee;
        o[26+count*4]=call.arguments[0].value_or(0xdeadbeef);
        o[27+count*4]=value(adapter.timer_word());++count;
        if(count==i[4])return sadx::GameLogicOwnerResult{
            i[9]?sadx::GameLogicOwnerState::failed:sadx::GameLogicOwnerState::unavailable,call.callee,{}};
        if(call.callee==0x40e2c0){
            if(i[5])adapter.observe_buffer(i[6]);
            if(i[7])adapter.observe_buffer(std::nullopt);
        }else if(i[10])adapter.observe_buffer(i[11]);
        return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,call.callee,{}};
    };
    const auto result=adapter.complete_frame(invoke);
    o[0]=static_cast<std::uint32_t>(result.failure);o[1]=result.source_boundary;
    o[2]=result.required_fact.value_or(0);o[3]=value(adapter.timer_word());o[4]=value(adapter.buffer_word());
    if(result.pending_call){o[5]=result.pending_call->source_call;o[6]=result.pending_call->callee;o[7]=result.pending_call->arguments[0].value_or(0xdeadbeef);}
    o[8]=count;
}

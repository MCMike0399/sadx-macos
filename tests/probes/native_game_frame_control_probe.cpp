#include "engine/game_frame_control.hpp"
#include <cstddef>
#include <cstring>
namespace {
std::int32_t signed_word(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}
}
extern "C" int sadx_native_game_frame_control(const std::uint32_t* input,std::size_t size,
                                              std::uint32_t* output,std::size_t capacity){
    if(!input||size!=19||!output||capacity<16||input[0]>3)return 0;
    const auto mask=input[1];sadx::GameFrameRendererState renderer;sadx::FrameRateModeState timing;std::optional<std::uint8_t> input_edge;
    std::optional<std::int32_t> save_phase;sadx::GameFrameInputState state{input_edge,{},{},save_phase};
    if(mask&1)renderer.counter_frequency.high=input[4];
    if(mask&2)renderer.counter_frequency.low=input[5];
    if(mask&4)renderer.frame_interval_multiplier=signed_word(input[6]);
    if(mask&8)renderer.configured_frame_interval=signed_word(input[7]);
    if(mask&16)renderer.frame_interval.low=input[8];
    if(mask&32)renderer.frame_interval.high=input[9];
    if(mask&64)renderer.input_callback=input[17]?&sadx::run_frame_input:nullptr;
    if(mask&128)state.soft_reset_flags=static_cast<std::uint8_t>(input[10]);
    if(mask&256)state.reset_input_mask=static_cast<std::uint8_t>(input[11]);
    if(mask&512)state.save_phase=signed_word(input[12]);
    if(mask&1024)input_edge=static_cast<std::uint8_t>(input[13]);
    if(mask&2048)timing.frame_multiplier=signed_word(input[14]);
    if(mask&4096)timing.logic_increment=signed_word(input[15]);
    if(mask&8192)timing.pending_logic_ticks=signed_word(input[16]);
    sadx::GameFrameControlResult result;
    if(input[0]==0)result=sadx::set_frame_interval_multiplier(signed_word(input[2]),renderer);
    if(input[0]==1)result=sadx::set_frame_rate_mode(signed_word(input[2]),signed_word(input[3]),timing,renderer);
    if(input[0]==2)result=sadx::update_soft_reset_flags(state);
    if(input[0]==3)result=sadx::run_frame_input(state);
    std::uint32_t present=0;
    const auto mark=[&](const auto& v,unsigned bit){if(v)present|=1U<<bit;};
    mark(renderer.counter_frequency.high,0);mark(renderer.counter_frequency.low,1);
    mark(renderer.frame_interval_multiplier,2);mark(renderer.configured_frame_interval,3);
    mark(renderer.frame_interval.low,4);mark(renderer.frame_interval.high,5);
    mark(renderer.input_callback,6);mark(state.soft_reset_flags,7);
    mark(state.reset_input_mask,8);mark(state.save_phase,9);mark(input_edge,10);
    mark(timing.frame_multiplier,11);mark(timing.logic_increment,12);mark(timing.pending_logic_ticks,13);
    output[0]=static_cast<std::uint32_t>(result.failure);output[1]=result.source_boundary;output[2]=result.nested_source_boundary;output[3]=result.completed;output[4]=present;
    output[5]=std::uint32_t(renderer.frame_interval_multiplier.value_or(0));output[6]=renderer.frame_interval.low.value_or(0);output[7]=renderer.frame_interval.high.value_or(0);
    output[8]=renderer.input_callback&&*renderer.input_callback?0x411e20:0;
    output[9]=state.soft_reset_flags.value_or(0);output[10]=std::uint32_t(timing.frame_multiplier.value_or(0));output[11]=std::uint32_t(timing.logic_increment.value_or(0));output[12]=std::uint32_t(timing.pending_logic_ticks.value_or(0));
    output[13]=std::uint32_t(state.save_phase.value_or(0));output[14]=input_edge.value_or(0);output[15]=state.reset_input_mask.value_or(0);return 1;
}

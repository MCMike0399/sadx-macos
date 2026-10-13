#include "engine/game_frame_control.hpp"
#include <cstring>
namespace {
std::int32_t signed_word(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}
}
extern "C" int sadx_native_game_frame_control(const std::uint32_t* input,std::size_t size,
                                              std::uint32_t* output,std::size_t capacity){
    if(!input||size!=19||!output||capacity<16||input[0]>3)return 0;
    const auto mask=input[1];sadx::GameFrameRendererState renderer;sadx::GameLogicTimingState timing;sadx::GameLogicEdgeState edges;
    std::optional<std::int32_t> save_phase;sadx::GameFrameInputState state{edges,{},{},save_phase};
    if(mask&1)renderer.frequency_high_at_3d0853c=input[4];
    if(mask&2)renderer.frequency_low_at_3d08538=input[5];
    if(mask&4)renderer.frame_multiplier_at_389d7dc=signed_word(input[6]);
    if(mask&8)renderer.render_factor_at_89295c=signed_word(input[7]);
    if(mask&16)renderer.interval_low_at_3d084a8=input[8];
    if(mask&32)renderer.interval_high_at_3d084ac=input[9];
    if(mask&64)renderer.input_callback_at_3d0fa34=input[17]?&sadx::run_game_frame_input:nullptr;
    if(mask&128)state.flags_at_3b0eaa0=static_cast<std::uint8_t>(input[10]);
    if(mask&256)state.mask_at_3b0e3b8=static_cast<std::uint8_t>(input[11]);
    if(mask&512)state.save_phase_at_3b291a4=signed_word(input[12]);
    if(mask&1024)edges.raw_at_3b0e3e2[59][0]=static_cast<std::uint8_t>(input[13]);
    if(mask&2048)timing.word_at_3b11188=signed_word(input[14]);
    if(mask&4096)timing.word_at_3b11178=signed_word(input[15]);
    if(mask&8192)timing.word_at_3b1117c=signed_word(input[16]);
    sadx::GameFrameControlResult result;
    if(input[0]==0)result=sadx::set_game_frame_multiplier(signed_word(input[2]),renderer);
    if(input[0]==1)result=sadx::set_game_logic_mode(signed_word(input[2]),signed_word(input[3]),timing,renderer);
    if(input[0]==2)result=sadx::update_game_frame_input_flags(state);
    if(input[0]==3)result=sadx::run_game_frame_input(state);
    std::uint32_t present=0;
    const auto mark=[&](const auto& v,unsigned bit){if(v)present|=1U<<bit;};
    mark(renderer.frequency_high_at_3d0853c,0);mark(renderer.frequency_low_at_3d08538,1);
    mark(renderer.frame_multiplier_at_389d7dc,2);mark(renderer.render_factor_at_89295c,3);
    mark(renderer.interval_low_at_3d084a8,4);mark(renderer.interval_high_at_3d084ac,5);
    mark(renderer.input_callback_at_3d0fa34,6);mark(state.flags_at_3b0eaa0,7);
    mark(state.mask_at_3b0e3b8,8);mark(state.save_phase_at_3b291a4,9);mark(edges.raw_at_3b0e3e2[59][0],10);
    mark(timing.word_at_3b11188,11);mark(timing.word_at_3b11178,12);mark(timing.word_at_3b1117c,13);
    output[0]=static_cast<std::uint32_t>(result.failure);output[1]=result.source_boundary;output[2]=result.nested_source_boundary;output[3]=result.completed;output[4]=present;
    output[5]=std::uint32_t(renderer.frame_multiplier_at_389d7dc.value_or(0));output[6]=renderer.interval_low_at_3d084a8.value_or(0);output[7]=renderer.interval_high_at_3d084ac.value_or(0);
    output[8]=renderer.input_callback_at_3d0fa34&&*renderer.input_callback_at_3d0fa34?0x411e20:0;
    output[9]=state.flags_at_3b0eaa0.value_or(0);output[10]=std::uint32_t(timing.word_at_3b11188.value_or(0));output[11]=std::uint32_t(timing.word_at_3b11178.value_or(0));output[12]=std::uint32_t(timing.word_at_3b1117c.value_or(0));
    output[13]=std::uint32_t(state.save_phase_at_3b291a4.value_or(0));output[14]=edges.raw_at_3b0e3e2[59][0].value_or(0);output[15]=state.mask_at_3b0e3b8.value_or(0);return 1;
}

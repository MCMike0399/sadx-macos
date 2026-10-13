#include "engine/game_frame_control.hpp"
#include "engine/speedrun.hpp"
#include <cstring>

namespace sadx {
GameFrameControlResult set_game_frame_multiplier(std::int32_t value,GameFrameRendererState& state){
    if(!state.frequency_high_at_3d0853c)return {GameFrameControlFailure::unknown_frequency_high,0x7899a0,0,false};
    if(!state.frequency_low_at_3d08538)return {GameFrameControlFailure::unknown_frequency_low,0x7899ab,0,false};
    const auto frequency=(std::uint64_t(*state.frequency_high_at_3d0853c)<<32)|*state.frequency_low_at_3d08538;
    state.frame_multiplier_at_389d7dc=value;
    // __allmul deliberately keeps the low64 bits. Unsigned arithmetic and a
    // bit-preserving signed view reproduce it without C++ signed overflow.
    const auto product=std::uint64_t(std::int64_t(value))*frequency;
    std::int64_t signed_product;std::memcpy(&signed_product,&product,8);
    const auto interval=signed_product/60;std::uint64_t bits;std::memcpy(&bits,&interval,8);
    state.interval_low_at_3d084a8=std::uint32_t(bits);state.interval_high_at_3d084ac=std::uint32_t(bits>>32);
    return {GameFrameControlFailure::none,0,0,true};
}
GameFrameControlResult set_game_logic_mode(std::int32_t render_ratio,std::int32_t logic_ratio,
    GameLogicTimingState& timing,GameFrameRendererState& renderer){
    timing.word_at_3b11188=render_ratio;timing.word_at_3b11178=logic_ratio;timing.word_at_3b1117c=0;
    std::optional<std::int32_t> multiplier;std::uint32_t call=0;
    if(render_ratio==1){
        if(!renderer.render_factor_at_89295c)return {GameFrameControlFailure::unknown_render_factor,0x411e72,0,false};
        multiplier=*renderer.render_factor_at_89295c;call=0x411e79;
    }else if(render_ratio==2){
        if(!renderer.render_factor_at_89295c)return {GameFrameControlFailure::unknown_render_factor,0x411e53,0,false};
        if(*renderer.render_factor_at_89295c==1){multiplier=2;call=0x411e5e;}
    }
    if(multiplier){auto result=set_game_frame_multiplier(*multiplier,renderer);
        if(!result){result.nested_source_boundary=result.source_boundary;result.source_boundary=call;return result;}}
    //77ff50 stores this native callable. No guest code address is ever cast.
    renderer.input_callback_at_3d0fa34=&run_game_frame_input;
    return {GameFrameControlFailure::none,0,0,true};
}
GameFrameControlResult update_game_frame_input_flags(GameFrameInputState& state){
    //40f010's initial MOV from3b0e760 contributes only an incidental EAX
    // return, unused by this void callback; it has no modeled state effect.
    if(!state.flags_at_3b0eaa0)return {GameFrameControlFailure::unknown_flags,0x40f015,0,false};
    if(*state.flags_at_3b0eaa0!=0){
        if(!state.save_phase_at_3b291a4)return {GameFrameControlFailure::unknown_save_phase,0x40f01e,0,false};
        if(*state.save_phase_at_3b291a4==0)*state.flags_at_3b0eaa0|=1;
        return {GameFrameControlFailure::none,0,0,true};
    }
    //3b0e493 is the existing strided raw byte owner:3b0e3e2+59*3.
    const auto& edge=state.edges.raw_at_3b0e3e2[59][0];
    if(!edge)return {GameFrameControlFailure::unknown_edge,0x40f037,0,false};
    if(*edge!=0){
        if(!state.mask_at_3b0e3b8)return {GameFrameControlFailure::unknown_mask,0x40f040,0,false};
        if((*state.mask_at_3b0e3b8&0x44)!=0){*state.flags_at_3b0eaa0|=4;return {GameFrameControlFailure::none,0,0,true};}
    }
    *state.flags_at_3b0eaa0&=0xfb;return {GameFrameControlFailure::none,0,0,true};
}
GameFrameControlResult run_game_frame_input(GameFrameInputState& state){
    // Actual792f60 is XOR EAX,EAX; RET.411e20 then tail-jumps40f010.
    return update_game_frame_input_flags(state);
}
SpeedrunLimiterResult run_speedrun_frame_limiter(GameFrameRendererState& renderer,const GameLogicTimingState& timing,std::int32_t cadence,const SpeedrunLimiterClock& clock){
    SpeedrunLimiterResult r;const auto stop=[&](SpeedrunLimiterFailure f,std::uint32_t at){r.failure=f;r.source_boundary=at;return r;};
    if(!clock.frequency||!clock.counter)return stop(SpeedrunLimiterFailure::missing_clock,0x1000110c);
    const auto frequency=clock.frequency();
    if(!frequency){ // 10001195: QueryPerformanceCounter(last)
        r.counter_reads=1;const auto now=clock.counter();if(!now)return stop(SpeedrunLimiterFailure::counter_unavailable,0x1000119b);
        renderer.last_counter_at_3d08550=*now;r.completed=true;return r;}
    r.frequency_available=true;
    renderer.frequency_low_at_3d08538=static_cast<std::uint32_t>(static_cast<std::uint64_t>(*frequency));
    renderer.frequency_high_at_3d0853c=static_cast<std::uint32_t>(static_cast<std::uint64_t>(*frequency)>>32);
    if(!timing.word_at_3b11188)return stop(SpeedrunLimiterFailure::unknown_frame_multiplier,0x1000111c);
    r.interval=speedrun::limiter_interval(*timing.word_at_3b11188,*frequency,cadence);
    if(!r.interval)return stop(SpeedrunLimiterFailure::divide_fault,0x1000113d);
    for(;;){
        const auto now=clock.counter();++r.counter_reads;if(!now)return stop(SpeedrunLimiterFailure::counter_unavailable,0x10001155);
        if(!renderer.last_counter_at_3d08550)return stop(SpeedrunLimiterFailure::unknown_last_counter,0x1000115b);
        if(speedrun::limiter_elapsed(*now,*renderer.last_counter_at_3d08550,*r.interval)){renderer.last_counter_at_3d08550=*now;break;}
        if(clock.wait_hint){const auto elapsed=static_cast<std::int64_t>(static_cast<std::uint64_t>(*now)-static_cast<std::uint64_t>(*renderer.last_counter_at_3d08550));clock.wait_hint(*r.interval-elapsed);}
    }
    r.completed=true;return r;
}
} // namespace sadx

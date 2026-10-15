#include "engine/game_frame_control.hpp"
#include "engine/speedrun.hpp"
#include <cstring>

namespace sadx {
// FUNCTION: SONIC 0x7899a0 stHWSetVSync
GameFrameControlResult set_frame_interval_multiplier(std::int32_t value,GameFrameRendererState& state){
    if(!state.counter_frequency.high)return {GameFrameControlFailure::unknown_frequency_high,0x7899a0,0,false};
    if(!state.counter_frequency.low)return {GameFrameControlFailure::unknown_frequency_low,0x7899ab,0,false};
    const auto frequency=(std::uint64_t(*state.counter_frequency.high)<<32)|*state.counter_frequency.low;
    state.frame_interval_multiplier=value;
    // __allmul deliberately keeps the low64 bits. Unsigned arithmetic and a
    // bit-preserving signed view reproduce it without C++ signed overflow.
    const auto product=std::uint64_t(std::int64_t(value))*frequency;
    std::int64_t signed_product;std::memcpy(&signed_product,&product,8);
    const auto interval=signed_product/60;std::uint64_t bits;std::memcpy(&bits,&interval,8);
    state.frame_interval.low=std::uint32_t(bits);state.frame_interval.high=std::uint32_t(bits>>32);
    return {GameFrameControlFailure::none,0,0,true};
}
// FUNCTION: SONIC 0x411e30 dsInitInt
GameFrameControlResult set_frame_rate_mode(std::int32_t render_ratio,std::int32_t logic_ratio,
    FrameRateModeState& timing,GameFrameRendererState& renderer){
    timing.frame_multiplier=render_ratio;timing.logic_increment=logic_ratio;timing.pending_logic_ticks=0;
    std::optional<std::int32_t> multiplier;std::uint32_t call=0;
    if(render_ratio==1){
        if(!renderer.configured_frame_interval)return {GameFrameControlFailure::unknown_render_factor,0x411e72,0,false};
        multiplier=*renderer.configured_frame_interval;call=0x411e79;
    }else if(render_ratio==2){
        if(!renderer.configured_frame_interval)return {GameFrameControlFailure::unknown_render_factor,0x411e53,0,false};
        if(*renderer.configured_frame_interval==1){multiplier=2;call=0x411e5e;}
    }
    if(multiplier){auto result=set_frame_interval_multiplier(*multiplier,renderer);
        if(!result){result.nested_source_boundary=result.source_boundary;result.source_boundary=call;return result;}}
    //77ff50 stores this native callable. No guest code address is ever cast.
    renderer.input_callback=&run_frame_input;
    return {GameFrameControlFailure::none,0,0,true};
}
// FUNCTION: SONIC 0x40f010 UpdateSoftResetFlags
GameFrameControlResult update_soft_reset_flags(GameFrameInputState& state){
    //40f010's initial MOV from3b0e760 contributes only an incidental EAX
    // return, unused by this void callback; it has no modeled state effect.
    if(!state.soft_reset_flags)return {GameFrameControlFailure::unknown_flags,0x40f015,0,false};
    if(*state.soft_reset_flags!=0){
        if(!state.save_phase)return {GameFrameControlFailure::unknown_save_phase,0x40f01e,0,false};
        if(*state.save_phase==0)*state.soft_reset_flags|=1;
        return {GameFrameControlFailure::none,0,0,true};
    }
    //3b0e493 is the existing strided raw byte owner:3b0e3e2+59*3.
    const auto& edge=state.reset_key_edge;
    if(!edge)return {GameFrameControlFailure::unknown_edge,0x40f037,0,false};
    if(*edge!=0){
        if(!state.reset_input_mask)return {GameFrameControlFailure::unknown_mask,0x40f040,0,false};
        if((*state.reset_input_mask&0x44)!=0){*state.soft_reset_flags|=4;return {GameFrameControlFailure::none,0,0,true};}
    }
    *state.soft_reset_flags&=0xfb;return {GameFrameControlFailure::none,0,0,true};
}
// FUNCTION: SONIC 0x411e20 RunFrameInput
GameFrameControlResult run_frame_input(GameFrameInputState& state){
    // Actual792f60 is XOR EAX,EAX; RET.411e20 then tail-jumps40f010.
    return update_soft_reset_flags(state);
}
// PATCH: SPEEDRUN +0x10f0 -> 0x7899e0
SpeedrunLimiterResult run_speedrun_frame_limiter(GameFrameRendererState& renderer,const FrameRateModeState& timing,std::int32_t cadence,const SpeedrunLimiterClock& clock){
    SpeedrunLimiterResult r;const auto stop=[&](SpeedrunLimiterFailure f,std::uint32_t at){r.failure=f;r.source_boundary=at;return r;};
    if(!clock.frequency||!clock.counter)return stop(SpeedrunLimiterFailure::missing_clock,0x1000110c);
    const auto frequency=clock.frequency();
    if(!frequency){ // 10001195: QueryPerformanceCounter(last)
        r.counter_reads=1;const auto now=clock.counter();if(!now)return stop(SpeedrunLimiterFailure::counter_unavailable,0x1000119b);
        renderer.previous_counter=*now;r.completed=true;return r;}
    r.frequency_available=true;
    renderer.counter_frequency.low=static_cast<std::uint32_t>(static_cast<std::uint64_t>(*frequency));
    renderer.counter_frequency.high=static_cast<std::uint32_t>(static_cast<std::uint64_t>(*frequency)>>32);
    if(!timing.frame_multiplier)return stop(SpeedrunLimiterFailure::unknown_frame_multiplier,0x1000111c);
    r.interval=speedrun::limiter_interval(*timing.frame_multiplier,*frequency,cadence);
    if(!r.interval)return stop(SpeedrunLimiterFailure::divide_fault,0x1000113d);
    for(;;){
        const auto now=clock.counter();++r.counter_reads;if(!now)return stop(SpeedrunLimiterFailure::counter_unavailable,0x10001155);
        if(!renderer.previous_counter)return stop(SpeedrunLimiterFailure::unknown_last_counter,0x1000115b);
        if(speedrun::limiter_elapsed(*now,*renderer.previous_counter,*r.interval)){renderer.previous_counter=*now;break;}
        if(clock.wait_hint){const auto elapsed=static_cast<std::int64_t>(static_cast<std::uint64_t>(*now)-static_cast<std::uint64_t>(*renderer.previous_counter));clock.wait_hint(*r.interval-elapsed);}
    }
    r.completed=true;return r;
}
} // namespace sadx

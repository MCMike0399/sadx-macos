#include "engine/game_frame_control.hpp"
#include <algorithm>
// Native Speedrun 2.0 limiter matching tools/verify_original_speedrun_limiter.py.
// in: 0 freq low,1 freq high,2 qpf ok,3 frame multiplier,4 cadence,5/6 last low/high,7/8 start low/high,9/10 step low/high,11 read cap
extern "C" int sadx_native_speedrun_limiter(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=16||!out||capacity<12)return 0;
    const auto join=[](std::uint32_t lo,std::uint32_t hi){return static_cast<std::int64_t>((std::uint64_t(hi)<<32)|lo);};
    sadx::GameFrameRendererState renderer{};renderer.previous_counter=join(in[5],in[6]);renderer.counter_frequency.low=0xa5a5a5a5;renderer.counter_frequency.high=0x5a5a5a5a;
    sadx::FrameRateModeState timing;timing.frame_multiplier=static_cast<std::int32_t>(in[3]);
    std::uint64_t reads=0;const auto start=static_cast<std::uint64_t>(join(in[7],in[8])),step=static_cast<std::uint64_t>(join(in[9],in[10]));
    sadx::SpeedrunLimiterClock clock{[&]()->std::optional<std::int64_t>{if(!in[2])return std::nullopt;return join(in[0],in[1]);},
        [&]()->std::optional<std::int64_t>{if(reads>=in[11])return std::nullopt;return static_cast<std::int64_t>(start+step*reads++);}};
    const auto r=sadx::run_speedrun_frame_limiter(renderer,timing,static_cast<std::int32_t>(in[4]),clock);
    std::fill(out,out+capacity,0);const auto last=static_cast<std::uint64_t>(renderer.previous_counter.value_or(0));
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=static_cast<std::uint32_t>(r.counter_reads);out[3]=static_cast<std::uint32_t>(last);out[4]=static_cast<std::uint32_t>(last>>32);
    out[5]=renderer.counter_frequency.low.value_or(0);out[6]=renderer.counter_frequency.high.value_or(0);out[7]=r.completed;
    return 1;
}

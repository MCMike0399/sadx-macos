#include "platform/mac/mac_clock.hpp"
#include <iostream>
#include <pthread/qos.h>
#include <stdexcept>
#include <vector>
// Real host pacing check: 63 source frames through the Speedrun 2.0 limiter.
int main(){try{
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE,0); // a game thread's class
    sadx::MacPerformanceCounter clock;const auto a=clock.now(),b=clock.now();if(b<a)throw std::runtime_error("counter must be monotonic");
    sadx::GameFrameRendererState renderer{};sadx::GameLogicTimingState timing;timing.word_at_3b11188=1;renderer.last_counter_at_3d08550=clock.now();
    const auto limiter=clock.limiter_clock();std::vector<std::int64_t> stamps{*renderer.last_counter_at_3d08550};
    for(int frame=0;frame<63;++frame){const auto r=sadx::run_speedrun_frame_limiter(renderer,timing,63,limiter);if(!r||r.interval!=158'730)throw std::runtime_error("limiter frame failed");stamps.push_back(*renderer.last_counter_at_3d08550);}
    std::int64_t shortest=INT64_MAX,longest=0;for(std::size_t i=1;i<stamps.size();++i){const auto d=stamps[i]-stamps[i-1];shortest=std::min(shortest,d);longest=std::max(longest,d);}
    if(shortest<158'730)throw std::runtime_error("a frame ended before the source interval");
    const double seconds=double(stamps.back()-stamps.front())/sadx::MacPerformanceCounter::frequency_hz;
    std::cout<<"Mac clock: 10 MHz QPC; 63 limiter frames in "<<seconds<<" s (min "<<shortest<<", max "<<longest<<" ticks; source interval 158730).\n";
    if(longest>158'730*3/2)std::cout<<"note: host scheduling overshoot observed (no catch-up by design, as in the DLL).\n";
    return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

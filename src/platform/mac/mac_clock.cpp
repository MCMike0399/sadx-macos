#include "platform/mac/mac_clock.hpp"
#include <mach/mach_time.h>
#include <thread>
namespace sadx {
MacPerformanceCounter::MacPerformanceCounter(){mach_timebase_info_data_t info{};mach_timebase_info(&info);numer_=info.numer;denom_=info.denom;}
std::int64_t MacPerformanceCounter::now()const noexcept{
    // ticks * numer / denom = ns; 10 MHz = ns / 100.
    const auto ns=static_cast<unsigned __int128>(mach_absolute_time())*numer_/denom_;
    return static_cast<std::int64_t>(ns/100);
}
SpeedrunLimiterClock MacPerformanceCounter::limiter_clock()const{
    return {[]{return std::optional<std::int64_t>{frequency_hz};},[this]{return std::optional<std::int64_t>{now()};},
            [](std::int64_t remaining){constexpr std::int64_t margin=frequency_hz/250;if(remaining>margin+frequency_hz/2000)std::this_thread::sleep_for(std::chrono::microseconds((remaining-margin)/10));}};
}
} // namespace sadx

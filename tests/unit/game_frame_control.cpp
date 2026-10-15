#include "engine/game_frame_control.hpp"
#include "engine/game_logic_support.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void test(){
    sadx::GameFrameRendererState renderer;renderer.frame_interval_multiplier=7;
    auto result=sadx::set_frame_interval_multiplier(2,renderer);
    require(result.failure==sadx::GameFrameControlFailure::unknown_frequency_high&&renderer.frame_interval_multiplier==7,"Frequency high read precedes multiplier store");
    renderer.counter_frequency.high=0;result=sadx::set_frame_interval_multiplier(2,renderer);
    require(result.failure==sadx::GameFrameControlFailure::unknown_frequency_low&&renderer.frame_interval_multiplier==7,"Frequency low read also precedes multiplier store");
    renderer.counter_frequency.low=10000000;result=sadx::set_frame_interval_multiplier(2,renderer);
    require(bool(result)&&renderer.frame_interval.low==333333&&renderer.frame_interval.high==0,"Source counter interval uses integer division by60");
    sadx::GameLogicTimingState timing;result=sadx::set_frame_rate_mode(1,2,timing,renderer);
    require(result.failure==sadx::GameFrameControlFailure::unknown_render_factor&&timing.frame_multiplier==1&&timing.logic_increment==2&&timing.pending_logic_ticks==0,"Mode stores survive later absent render factor");
    renderer.configured_frame_interval=2;renderer.counter_frequency.high.reset();
    result=sadx::set_frame_rate_mode(2,1,timing,renderer);
    require(bool(result)&&renderer.frame_interval_multiplier==2&&renderer.input_callback&&*renderer.input_callback==&sadx::run_frame_input,"Unselected multiplier branch avoids frequency read and installs real callable");
    renderer.configured_frame_interval=1;result=sadx::set_frame_rate_mode(2,1,timing,renderer);
    require(result.failure==sadx::GameFrameControlFailure::unknown_frequency_high&&result.source_boundary==0x411e5e&&result.nested_source_boundary==0x7899a0,"Nested arithmetic failure keeps both source boundaries");
    sadx::GameLogicEdgeState edges;std::optional<std::int32_t> save_phase;sadx::GameFrameInputState input{edges.soft_reset_edge(),{},{},save_phase};
    input.soft_reset_flags=2;input.save_phase=0;
    result=(*renderer.input_callback)(input);
    require(bool(result)&&input.soft_reset_flags==3,"Real installed callback applies source nonzero branch without absent edge/mask defaults");
    input.soft_reset_flags=0;edges.raw_at_3b0e3e2[59][0]=1;
    result=sadx::run_frame_input(input);
    require(result.failure==sadx::GameFrameControlFailure::unknown_mask&&input.soft_reset_flags==0,"Unknown mask keeps flag prefix");
    input.reset_input_mask=0x40;result=sadx::run_frame_input(input);
    require(bool(result)&&input.soft_reset_flags==4,"Input byte uses existing strided edge59 and actual44 mask");
    std::cout<<"Frame control: source frequency/read/store order, mode branches, real callback and explicit unknown facts passed.\n";
}
}
void limiter_tests(){
    sadx::GameFrameRendererState renderer{};sadx::GameLogicTimingState timing;timing.frame_multiplier=1;std::int64_t now=0;std::uint64_t reads=0;
    sadx::SpeedrunLimiterClock clock{[]{return std::optional<std::int64_t>{10'000'000};},[&]{++reads;now+=50'000;return std::optional<std::int64_t>{now};}};
    auto r=sadx::run_speedrun_frame_limiter(renderer,timing,63,clock);if(r.failure!=sadx::SpeedrunLimiterFailure::unknown_last_counter||r.source_boundary!=0x1000115b||renderer.counter_frequency.low!=10'000'000u)throw std::runtime_error("unknown last counter after QPF store");
    renderer.previous_counter=0;now=0;reads=0;r=sadx::run_speedrun_frame_limiter(renderer,timing,63,clock);
    if(!r||r.interval!=158'730||reads!=4||renderer.previous_counter!=200'000)throw std::runtime_error("63 FPS interval spins until elapsed then stores now");
    sadx::SpeedrunLimiterClock none{};if(sadx::run_speedrun_frame_limiter(renderer,timing,63,none).failure!=sadx::SpeedrunLimiterFailure::missing_clock)throw std::runtime_error("missing clock owner");
    sadx::SpeedrunLimiterClock failing{[]{return std::optional<std::int64_t>{};},[]{return std::optional<std::int64_t>{77};}};r=sadx::run_speedrun_frame_limiter(renderer,timing,63,failing);
    if(!r||r.frequency_available||renderer.previous_counter!=77)throw std::runtime_error("QPF failure stores one QPC");
}
int main(){try{test();limiter_tests();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

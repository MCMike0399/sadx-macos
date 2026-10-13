#include "game/mod_loader_frame.hpp"
#include <algorithm>
#include <array>
#include <vector>
// Native Speedrun 2.0 OnFrame with the layout of tools/verify_original_speedrun_on_frame.py.
// in: [0] 100107e9 [1] 100107e8 [2] 10010818 [3] 10010819 [4] 1001081a [5] 3b22de4 (u16) [6] 3abdc7c [7] 3b22dcc (u16) [8] fail-at nested call
// out: [0] completed [1..5] the five DLL bytes [6] nested count [7..10] nested (callee, site) x 2
extern "C" int sadx_native_speedrun_on_frame(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    SpeedrunModState m;const auto b=[&](int i){return std::optional<std::uint8_t>{static_cast<std::uint8_t>(in[i])};};
    m.quick_save_at_100107e9=b(0);m.chao_viewer_at_100107e8=b(1);m.timer_frames_at_10010818=b(2);m.timer_seconds_at_10010819=b(3);m.timer_minutes_at_1001081a=b(4);
    std::optional<std::uint16_t> game_mode=static_cast<std::uint16_t>(in[5]),stage=static_cast<std::uint16_t>(in[7]);std::optional<std::int32_t> mode=static_cast<std::int32_t>(in[6]);
    std::vector<std::array<std::uint32_t,2>> nested;
    SpeedrunFrameRuntime rt{m,game_mode,mode,stage,[&](std::uint32_t site,std::uint32_t callee){nested.push_back({callee,site});
        return GameLogicOwnerResult{in[8]&&nested.size()==in[8]?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,callee,{}};}};
    const auto r=run_mod_loader_process_codes(rt);
    std::fill(out,out+16,0);out[0]=r?1U:0U;
    const auto v=[](const std::optional<std::uint8_t>& o){return o?std::uint32_t{*o}:0xdeadbeefU;};
    out[1]=v(m.quick_save_at_100107e9);out[2]=v(m.chao_viewer_at_100107e8);out[3]=v(m.timer_frames_at_10010818);out[4]=v(m.timer_seconds_at_10010819);out[5]=v(m.timer_minutes_at_1001081a);
    out[6]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<2;++i){out[7+2*i]=nested[i][0];out[8+2*i]=nested[i][1];}
    return 1;
}

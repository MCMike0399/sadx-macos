#include "game/game_window.hpp"
#include <algorithm>
#include <vector>
// Native 789bd0 with the layout of tools/verify_original_message_pump.py.
// in: [0] 3d0fd30 [1] message count [2] dispatch that fails (0 none) [3] 3d08534 before [4..] messages x 7 words
// out: [0] completed [1] 3d08534 (0xdeadbeef unknown) [2] known [3] dispatched [4..] (hwnd, message, wparam) per dispatch, peeked hwnds after
extern "C" int sadx_native_message_pump(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    GameWindowState s;s.last_wparam_at_3d08534=in[3];std::optional<std::uint32_t> window=in[0];std::size_t next=0,dispatches=0;
    std::vector<std::uint32_t> trace;
    GameMessagePumpRuntime rt{s,window,[&](std::uint32_t hwnd)->std::optional<std::optional<GameMessage>>{
            trace.push_back(0xb0000000u|hwnd);
            if(next>=in[1])return std::optional<GameMessage>{};
            const auto* w=in+4+7*next++;return std::optional<GameMessage>{GameMessage{w[0],w[1],w[2],w[3],w[4],w[5],w[6]}};},
        [&](const GameMessage& m){trace.push_back(m.hwnd);trace.push_back(m.message);trace.push_back(m.wparam);
            return GameLogicOwnerResult{in[2]&&++dispatches==in[2]?GameLogicOwnerState::unavailable:GameLogicOwnerState::completed,0,{}};},GameMessagePumpProfile::original,{}};
    const auto r=run_message_pump_789bd0(rt);
    std::fill(out,out+256,0);out[0]=r?1U:0U;out[1]=s.last_wparam_at_3d08534.value_or(0xdeadbeef);out[2]=s.last_wparam_at_3d08534.has_value();out[3]=r.dispatched;
    out[4]=static_cast<std::uint32_t>(trace.size());for(std::size_t i=0;i<trace.size()&&i<200;++i)out[5+i]=trace[i];
    return 1;
}

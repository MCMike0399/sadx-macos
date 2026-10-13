#include "game/game_window.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
// 789bd0: no message (unknown wParam), a drained queue (last wParam), unknown window and host gaps.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    GameWindowState s;s.last_wparam_at_3d08534=7;std::optional<std::uint32_t> window=0x1234;std::vector<GameMessage> queue;std::vector<std::uint32_t> seen;
    GameMessagePumpRuntime rt{s,window,[&](std::uint32_t)->std::optional<std::optional<GameMessage>>{if(queue.empty())return std::optional<GameMessage>{};auto m=queue.front();queue.erase(queue.begin());return std::optional<GameMessage>{m};},
        [&](const GameMessage& m){seen.push_back(m.message);return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};},GameMessagePumpProfile::original,{}};
    require(bool(run_message_pump_789bd0(rt))&&!s.last_wparam_at_3d08534,"no message: uninitialised wParam is unknown");
    queue={{0x1234,0x100,5},{0x1234,0x101,9}};const auto r=run_message_pump_789bd0(rt);
    require(r&&r.dispatched==2&&s.last_wparam_at_3d08534==9u&&seen.size()==2,"drained queue keeps the last wParam");
    window.reset();require(run_message_pump_789bd0(rt).required_fact==0x3d0fd30u,"unknown window");window=1;
    rt.peek=nullptr;require(run_message_pump_789bd0(rt).failure==GameWindowFailure::host_unavailable,"no host");
    {GameWindowState ls;std::optional<std::uint32_t> lw=0x99;std::vector<GameMessage> q{{1,0x100,3},{1,0x101,4}};std::vector<std::uint32_t> peeks,sent;
        GameMessagePumpRuntime lr{ls,lw,[&](std::uint32_t hwnd)->std::optional<std::optional<GameMessage>>{peeks.push_back(hwnd);if(q.empty())return std::optional<GameMessage>{};auto m=q.front();q.erase(q.begin());return std::optional<GameMessage>{m};},
            [&](const GameMessage& m){sent.push_back(m.message);return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};},GameMessagePumpProfile::mod_loader,
            [](const GameMessage& m)->std::optional<bool>{return m.message==0x100;}};
        require(bool(run_message_pump_789bd0(lr))&&sent==std::vector<std::uint32_t>{0x101}&&peeks==std::vector<std::uint32_t>{0,0,0}&&ls.last_wparam_at_3d08534==4u,"loader pump: null-window peeks, accelerators skip dispatch");}
    std::cout<<"Message pump 789bd0: empty queue, drained queue, unknown window and host gaps passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

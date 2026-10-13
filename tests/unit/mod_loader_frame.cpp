#include "game/mod_loader_frame.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
// Owner semantics the oracle does not compare: Init's starting state, unknown
// facts and the viewer/quick-save nested owners' stops.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(){try{
    auto m=speedrun_mod_after_init(false);
    require(m.quick_save_at_100107e9==0&&m.chao_viewer_at_100107e8==0&&m.timer_frames_at_10010818==0&&m.timer_minutes_at_1001081a==0,"Init state for QuickSaveSettings.Enabled=false");
    std::optional<std::uint16_t> game_mode=4,stage=0;std::optional<std::int32_t> mode=4;std::vector<std::uint32_t> nested;
    SpeedrunFrameRuntime rt{m,game_mode,mode,stage,[&](std::uint32_t,std::uint32_t callee){nested.push_back(callee);return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};}};
    m.timer_frames_at_10010818=0x3d;m.timer_seconds_at_10010819=0x3b;m.timer_minutes_at_1001081a=0x63;
    require(bool(run_mod_loader_process_codes(rt))&&m.timer_frames_at_10010818==0x3b&&m.timer_seconds_at_10010819==0x3b&&m.timer_minutes_at_1001081a==0x63&&nested.empty(),"timer saturates at 99:59:59");
    mode=0;require(bool(run_mod_loader_process_codes(rt))&&m.timer_frames_at_10010818==0&&m.timer_minutes_at_1001081a==0,"outside modes 4/9 the timer resets");
    game_mode.reset();require(run_mod_loader_process_codes(rt).required_fact==0x3b22de4u,"unknown 3b22de4");game_mode=7;
    m.chao_viewer_at_100107e8=1;stage=0x27;require(bool(run_mod_loader_process_codes(rt))&&nested==std::vector<std::uint32_t>{0x10002e7f}&&m.chao_viewer_at_100107e8==1,"viewer stays on in the Chao race");
    stage=3;nested.clear();require(bool(run_mod_loader_process_codes(rt))&&m.chao_viewer_at_100107e8==0,"viewer clears outside stage 0x27");
    m.quick_save_at_100107e9=1;rt.invoke=[&](std::uint32_t,std::uint32_t callee){nested.push_back(callee);return GameLogicOwnerResult{GameLogicOwnerState::unavailable,callee,{}};};
    {const auto r=run_mod_loader_process_codes(rt);require(r.failure==ModFrameFailure::missing_owner&&r.source_boundary==0x10002e6du&&r.pending_callee==0x10004f80u,"quick save is a nested owner");}
    std::cout<<"Mod loader frame: Init state, Speedrun OnFrame timer roll-over/reset, viewer and quick-save owners and unknown facts passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

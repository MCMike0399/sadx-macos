#include "ninja/ninja_vibration.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
// 785190: idle entries call nothing; unknown state and duration stop at their reads.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    NinjaVibrationState s;std::array<std::optional<std::uint32_t>,4> states{0u,0u,0u,0u};std::vector<std::uint32_t> calls;
    NinjaVibrationRuntime rt{s,states,[&](const NinjaVibrationCall& c)->std::pair<GameLogicOwnerResult,std::optional<std::uint32_t>>{calls.push_back(c.callee);return {{GameLogicOwnerState::completed,0,{}},5u};}};
    require(bool(run_vibration_785190(rt))&&calls.empty(),"all idle: no calls");
    states[2]=2;require(run_vibration_785190(rt).required_fact==0x3d08428u+0x30+0xc,"unknown duration");
    s.duration_at_3d08434[2]=3;calls.clear();require(bool(run_vibration_785190(rt))&&states[2]==0u&&calls.back()==0x792000u,"expired: stop and reset");
    states[0].reset();require(run_vibration_785190(rt).required_fact==0x3d08428u,"unknown state");
    std::cout<<"Vibration 785190: idle, expiry, unknown state and duration passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

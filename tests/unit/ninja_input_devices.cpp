#include "engine/ninja_input_devices.hpp"
#include <iostream>
#include <stdexcept>
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    NinjaInputDeviceState s;s.joysticks_at_3d08814=0;s.mouse_at_3d08834=0;int bodies=0;
    const NinjaInputDeviceBody run=[&](std::uint32_t){++bodies;return GameLogicOwnerResult{GameLogicOwnerState::unavailable,0,{}};};
    require(bool(run_joystick_poll_792100(s,run))&&bool(run_mouse_poll_791df0(s,run))&&bodies==0,"no devices: both return");
    s.mouse_at_3d08834=0x1234;require(run_mouse_poll_791df0(s,run).source_boundary==0x791e0bu&&bodies==1,"a mouse device reaches the body owner");
    s.joysticks_at_3d08814.reset();require(run_joystick_poll_792100(s,run).required_fact==0x3d08814u,"unknown joystick count");
    std::cout<<"Input device polls 792100/791df0: no-device returns, body owner and unknown count passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

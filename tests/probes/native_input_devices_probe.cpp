#include "ninja/ninja_input_devices.hpp"
// in: [0] which (0 joystick 792100, 1 mouse 791df0) [1] the gate word; out: [0] completed [1] body reached
extern "C" int sadx_native_input_devices(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;NinjaInputDeviceState s;s.joysticks_at_3d08814=in[1];s.mouse_at_3d08834=in[1];std::uint32_t body=0;
    const NinjaInputDeviceBody run=[&](std::uint32_t b){body=b;return GameLogicOwnerResult{GameLogicOwnerState::unavailable,b,{}};};
    const auto r=in[0]?run_mouse_poll_791df0(s,run):run_joystick_poll_792100(s,run);
    out[0]=r?1U:0U;out[1]=body;return 1;
}

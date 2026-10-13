#include "ninja/ninja_input_devices.hpp"
namespace sadx {
namespace {
NinjaInputDeviceResult poll(const std::optional<std::uint32_t>& gate,std::uint32_t read_at,std::uint32_t fact,std::uint32_t body,const NinjaInputDeviceBody& run){
    NinjaInputDeviceResult r;
    if(!gate){r.failure=NinjaInputDeviceFailure::unknown_state;r.source_boundary=read_at;r.required_fact=fact;return r;}
    if(*gate==0)return r; // only the stack-cookie check follows
    if(!run){r.failure=NinjaInputDeviceFailure::missing_owner;r.source_boundary=body;return r;}
    const auto b=run(body);
    if(b.state!=GameLogicOwnerState::completed){r.failure=b.state==GameLogicOwnerState::failed?NinjaInputDeviceFailure::owner_failed:NinjaInputDeviceFailure::missing_owner;r.source_boundary=body;}
    return r;
}
}
NinjaInputDeviceResult run_joystick_poll_792100(const NinjaInputDeviceState& s,const NinjaInputDeviceBody& run){return poll(s.joysticks_at_3d08814,0x792112,0x3d08814,0x792122,run);}
NinjaInputDeviceResult run_mouse_poll_791df0(const NinjaInputDeviceState& s,const NinjaInputDeviceBody& run){return poll(s.mouse_at_3d08834,0x791df9,0x3d08834,0x791e0b,run);}
} // namespace sadx

#include "guest/frame_control_reference.hpp"
#include "engine/ninja_device_init.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
// Owner semantics the oracle does not compare: unknown device, a missing host,
// the 7943c0 contract (it must leave a device) and an unknown matrix fact.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    NinjaDeviceState s;for(auto& w:s.matrix_at_389d318)w=0;NinjaFrameCallState frame;NinjaRenderState render;GameFrameRendererState renderer{};
    renderer.counter_frequency.low=0;renderer.counter_frequency.high=0;X87Context fp(0x7f);std::vector<std::uint32_t> calls;
    NinjaDeviceHost host{[](std::uint32_t)->std::optional<std::uint32_t>{return 0x1000u;},
        [](const D3D8FrameCommand&)->std::optional<std::vector<std::uint8_t>>{return std::vector<std::uint8_t>(0xd4,0);},
        [](const D3D8FrameCommand&)->std::optional<std::array<std::uint32_t,6>>{return std::array<std::uint32_t,6>{0,0,640,480,0,0x3f800000};},
        [](const D3D8FrameCommand&,std::uint32_t,std::uint32_t,float,std::uint32_t)->std::optional<std::int32_t>{return 0;}};
    NinjaDeviceInitRuntime rt{s,frame,render,renderer,fp,host,[&](const NinjaDeviceCall& c)->GameLogicOwnerResult{calls.push_back(c.callee);
        if(c.callee==0x7943c0)render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});return {GameLogicOwnerState::completed,0,{}};}};
    auto r=run_device_setup_791610(rt);
    require(r&&s.half_width_at_3d12948==320.0F&&s.right_at_3d0fd28==639.0F&&s.bottom_at_3d0fd2c==479.0F&&renderer.frame_interval_multiplier==1,"640x480 viewport");
    require(calls==std::vector<std::uint32_t>{0x7943c0,0x794ec0,0x78d0d0,0x796710,0x791860,0x7943f0,0x7914b0},"nested owners in order");
    rt.invoke=[&](const NinjaDeviceCall&)->GameLogicOwnerResult{return {GameLogicOwnerState::completed,0,{}};};
    r=run_device_setup_791610(rt);require(r.failure==NinjaDeviceFailure::null_device&&r.source_boundary==0x791674u,"7943c0 left 3d128b0 null: the vtable load faults");
    rt.host.viewport=nullptr;rt.invoke=[&](const NinjaDeviceCall& c)->GameLogicOwnerResult{if(c.callee==0x7943c0)render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});return {GameLogicOwnerState::completed,0,{}};};
    require(run_device_setup_791610(rt).failure==NinjaDeviceFailure::host_unavailable,"no viewport host");
    std::cout<<"Device setup 791610: viewport math, nested order, the 7943c0 device contract and a missing host passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

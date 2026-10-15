#include "engine/ninja_frame_calls.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
// Owner semantics the oracle does not compare: the mod loader's Present
// replacement, unknown/null devices, an unknown 3d0fa34, partial state before a
// nested stop, the callback-slot bound and an unavailable backend.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int g_inputs=0;
GameFrameControlResult input(GameFrameInputState&){++g_inputs;return {GameFrameControlFailure::none,0,0,true};}
}
int main(){try{
    NinjaFrameCallState s;s.scene_flags_at_3d08588=0;s.device_flags_at_3d1289c=0;s.vertex_buffer_at_3d128ac=0;
    s.hook_at_3d0f7a4=0;s.hook_at_3d0f7a8=0;s.hook_at_3d0fa30=0;s.argument_at_3d0fa38=0;
    for(auto& h:s.hooks_at_3d08af0)h=0;for(auto& r:s.records_at_3d0fe00)for(auto& w:r)w=0;
    NinjaCallbackList callbacks;callbacks.count_at_3ce712c=1;callbacks.slots_at_3d0fa40={0x77f090};
    GameFrameRendererState renderer{};renderer.input_callback=&input;
    GameLogicEdgeState edges;std::optional<std::int32_t> phase=0;GameFrameInputState frame_input{edges.soft_reset_edge(),{},{},phase};
    std::array<std::uint8_t,88> raw{};for(std::size_t i=0;i<raw.size();i+=4)raw[i]=static_cast<std::uint8_t>(i/4+1);
    const auto tables=decode_ninja_blend_tables(raw.data(),raw.size());
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});render.blend_tables=&*tables;
    std::vector<std::uint32_t> states,nested,methods;
    render.set_render_state=[&](const NinjaRenderStateCommand& c){states.push_back(c.state);return NinjaRenderCallResult{true,{}};};
    NinjaFrameRuntime rt{s,callbacks,renderer,&frame_input,render,{},NinjaPresentPolicy::mod_loader,
        [&](const NinjaFrameCall& c){nested.push_back(c.callee);return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};}};
    std::int32_t present=0;
    rt.device.begin_scene=[&](const D3D8FrameCommand&){methods.push_back(1);return std::optional<std::int32_t>{0};};
    rt.device.end_scene=[&](const D3D8FrameCommand&){methods.push_back(2);return std::optional<std::int32_t>{0};};
    rt.device.present=[&](const D3D8FrameCommand&){methods.push_back(3);return std::optional<std::int32_t>{present};};
    rt.device.clear=[&](const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t,float,std::uint32_t){methods.push_back(10+flags);return std::optional<std::int32_t>{0};};
    rt.device.get_render_state=[&](const D3D8FrameCommand&,std::uint32_t state){return std::optional<std::uint32_t>{state};};
    // 781010: the keyboard poll callback, then the native input callback.
    require(bool(run_ninja_frame_call(0x781010,rt))&&nested==std::vector<std::uint32_t>{0x77f090}&&g_inputs==1,"callback list and 3d0fa34");
    renderer.input_callback.reset();require(run_ninja_frame_call(0x781010,rt).required_fact==0x3d0fa34u,"unknown 3d0fa34 stops");
    renderer.input_callback=nullptr;
    callbacks.count_at_3ce712c=2;nested.clear();
    {const auto r=run_ninja_frame_call(0x781010,rt);require(r.failure==NinjaFrameFailure::index_out_of_range&&r.required_fact==0x3d0fa44u&&nested.size()==1,"slot past the registered list");}
    callbacks.count_at_3ce712c=1;
    // 781050 BeginScene then 781020 EndScene, with the 3d128ac draw skipped (null).
    nested.clear();methods.clear();states.clear();
    require(bool(run_ninja_frame_call(0x781050,rt))&&s.scene_flags_at_3d08588==1u&&methods==std::vector<std::uint32_t>{1}&&states.size()==2&&nested.empty(),"BeginScene without the 3d128ac draw");
    require(bool(run_ninja_frame_call(0x781050,rt))&&methods.size()==1,"BeginScene is idempotent inside a scene");
    require(bool(run_ninja_frame_call(0x781020,rt))&&s.scene_flags_at_3d08588==0u&&methods.back()==2&&nested==std::vector<std::uint32_t>{0x7912e0,0x793cc0},"EndScene path");
    // Partial state: the flag is cleared before 7912e0 stops.
    s.scene_flags_at_3d08588=3;rt.invoke=[&](const NinjaFrameCall& c){nested.push_back(c.callee);return GameLogicOwnerResult{GameLogicOwnerState::unavailable,c.callee,{}};};
    {const auto r=run_ninja_frame_call(0x781020,rt);require(r.failure==NinjaFrameFailure::missing_owner&&r.source_boundary==0x78b977u&&s.scene_flags_at_3d08588==2u&&r.pending_call&&r.pending_call->callee==0x7912e0u,"EndScene stops at 7912e0 after clearing bit 0");}
    // Present: the loader's reset_device on DEVICELOST, 789dd0 under the original policy.
    present=kD3DErrDeviceLost;nested.clear();
    {const auto r=run_ninja_frame_call(0x781030,rt);require(r.source_boundary==0x100502acu&&r.present_hresult==kD3DErrDeviceLost&&nested==std::vector<std::uint32_t>{0x10050300},"loader reset_device owner");}
    rt.present_policy=NinjaPresentPolicy::original;nested.clear();
    require(run_ninja_frame_call(0x781030,rt).source_boundary==0x78ba4au&&nested==std::vector<std::uint32_t>{0x789dd0},"original 789dd0 tail jump");
    present=0;rt.invoke=[&](const NinjaFrameCall& c){nested.push_back(c.callee);return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};};
    // Clear: stencil flag, and 794800's record+8 store before its Lock owner.
    s.device_flags_at_3d1289c=1;s.records_at_3d0fe00[4]={0x1234u,100u,0u,30u};methods.clear();nested.clear();
    require(bool(run_ninja_frame_call(0x781040,rt))&&methods==std::vector<std::uint32_t>{17}&&s.records_at_3d0fe00[4][2]==70u&&nested==std::vector<std::uint32_t>{0x794800},"Clear with stencil and one live record");
    // Device boundaries.
    render.device_at_3d128b0.reset();require(run_ninja_frame_call(0x781030,rt).failure==NinjaFrameFailure::unknown_device,"unknown device");
    render.device_at_3d128b0=NinjaRenderDeviceHandle{};require(run_ninja_frame_call(0x781030,rt).source_boundary==0x78ba35u,"null device faults at the vtable load");
    rt.present_policy=NinjaPresentPolicy::mod_loader;require(run_ninja_frame_call(0x781030,rt).source_boundary==0x100502a0u,"loader Present faults at its vtable load");rt.present_policy=NinjaPresentPolicy::original;
    render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});rt.device.present=nullptr;
    require(run_ninja_frame_call(0x781030,rt).failure==NinjaFrameFailure::device_unavailable,"no backend Present");
    require(run_ninja_frame_call(0x781060,rt).failure==NinjaFrameFailure::not_a_frame_call,"781060 is the limiter, not a frame call");
    {NinjaFrameCallState h;for(auto& x:h.hooks_at_3d08af0)x=0;h.hooks_at_3d08af0[0]=0x1111;
        require(bool(ninja_hook_register_7959c0(h,0x2222))&&h.hooks_at_3d08af0[1]==0x2222u&&bool(ninja_hook_register_7959c0(h,0x2222))&&h.hooks_at_3d08af0[2]==0u,"7959c0 first free slot, no duplicate");
        for(auto& x:h.hooks_at_3d08af0)x=1;require(bool(ninja_hook_register_7959c0(h,0x3333)),"a full table is left unchanged");
        h.hooks_at_3d08af0[5].reset();require(ninja_hook_register_7959c0(h,0x4444).required_fact==0x3d08af0u+20,"unknown slot");}
    std::cout<<"Ninja frame calls: callback list/3d0fa34/slot bound, BeginScene/EndScene, loader and original Present, Clear records and device boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

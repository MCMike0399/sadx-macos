#include "guest/polybuff_owners.hpp"
#include "guest/frame_control_reference.hpp"
#include "engine/ninja_frame_calls.hpp"
namespace sadx {
namespace {
struct Stop{};
struct Operation {
    NinjaFrameRuntime& rt;NinjaFrameResult r;
    [[noreturn]]void fail(NinjaFrameFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class V>auto need(const V& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(NinjaFrameFailure::unknown_state,at,fact);return *v;}
    void call(std::uint32_t site,std::uint32_t callee,std::optional<std::uint32_t> ecx={},std::array<std::optional<std::uint32_t>,4> arguments={}){
        r.pending_call=NinjaFrameCall{site,callee,ecx,arguments};
        if(!rt.invoke)fail(NinjaFrameFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);
        if(receipt.state!=GameLogicOwnerState::completed)fail(receipt.state==GameLogicOwnerState::cancelled?NinjaFrameFailure::cancelled:
            receipt.state==GameLogicOwnerState::failed?NinjaFrameFailure::owner_failed:NinjaFrameFailure::missing_owner,site);
        r.pending_call.reset();}
    // mov eax,[3d128b0] (load) ; mov ecx,[eax] (dereference) ; ... call [ecx+n].
    D3D8FrameCommand device(std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        if(!rt.render.device_at_3d128b0)fail(NinjaFrameFailure::unknown_device,load,0x3d128b0);
        if(!*rt.render.device_at_3d128b0)fail(NinjaFrameFailure::null_device,dereference);
        return {*rt.render.device_at_3d128b0,site};}
    std::int32_t method(const std::function<std::optional<std::int32_t>(const D3D8FrameCommand&)>& m,const D3D8FrameCommand& c){
        if(!m)fail(NinjaFrameFailure::device_unavailable,c.source_call);
        const auto hr=m(c);if(!hr)fail(NinjaFrameFailure::device_unavailable,c.source_call);return *hr;}
    std::uint32_t get_state(std::uint32_t state,std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        const auto c=device(load,dereference,site);
        if(!rt.device.get_render_state)fail(NinjaFrameFailure::device_unavailable,site);
        const auto v=rt.device.get_render_state(c,state);if(!v)fail(NinjaFrameFailure::device_unavailable,site);return *v;}
    void set_state(std::uint32_t state,std::uint32_t value,std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        NinjaRenderResult result;
        if(!ninja_issue_render_state(result,rt.render,state,value,{load,dereference,site}))
            fail(result.failure==NinjaRenderFailure::unknown_device?NinjaFrameFailure::unknown_device:result.failure==NinjaRenderFailure::null_device?NinjaFrameFailure::null_device:NinjaFrameFailure::render_failed,result.source_boundary);}
    void blend(std::uint32_t selector,std::uint32_t value){
        const auto result=ninja_set_blend_77ec60(selector,value,rt.render);
        if(!result)fail(result.failure==NinjaRenderFailure::unknown_device?NinjaFrameFailure::unknown_device:result.failure==NinjaRenderFailure::null_device?NinjaFrameFailure::null_device:NinjaFrameFailure::render_failed,result.source_boundary);}
    // 780030
    void callback_list(){
        auto count=need(rt.callbacks.count_at_3ce712c,0x780030,0x3ce712c);std::uint32_t index=0;
        if(count!=0){
            do{
                const auto slot=0x3d0fa40+4*index;
                if(index>=rt.callbacks.slots_at_3d0fa40.size())fail(NinjaFrameFailure::index_out_of_range,0x780042,slot);
                call(0x780042,need(rt.callbacks.slots_at_3d0fa40[index],0x780042,slot));
                count=need(rt.callbacks.count_at_3ce712c,0x780044,0x3ce712c);++index; // reread: a callback may unregister (77ffe0)
            }while(index!=count);
        }
        if(const auto h=need(rt.state.hook_at_3d0f7a4,0x780051,0x3d0f7a4))call(0x78005c,h);
        if(const auto h=need(rt.state.hook_at_3d0f7a8,0x78005e,0x3d0f7a8))call(0x780067,h);
        // 3d0fa34: 77ff50's native callable (411e20) or null.
        if(!rt.renderer.input_callback)fail(NinjaFrameFailure::unknown_state,0x780069,0x3d0fa34);
        if(*rt.renderer.input_callback){
            if(!rt.input)fail(NinjaFrameFailure::missing_owner,0x780072);
            const auto f=(*rt.renderer.input_callback)(*rt.input);
            if(!f)fail(NinjaFrameFailure::input_failed,f.source_boundary);}
        if(const auto h=need(rt.state.hook_at_3d0fa30,0x780074,0x3d0fa30))call(0x780084,h,{},{need(rt.state.argument_at_3d0fa38,0x78007d,0x3d0fa38)});
    }
    // 7959a0
    void hook_table(){
        for(std::uint32_t i=0;i<16;++i)if(const auto h=need(rt.state.hooks_at_3d08af0[i],0x7959a6,0x3d08af0+4*i))call(0x7959ac,h);
    }
    // 78b960 (the mod loader's trampoline runs it unchanged; no onRenderSceneEnd subscribers)
    void end_scene(){
        auto& flags=rt.state.scene_flags_at_3d08588;
        if((need(flags,0x78b960,0x3d08588)&1)==0)return;
        *flags&=0xfffffffeu;
        call(0x78b977,0x7912e0);
        blend(0,8);blend(1,6);
        const auto depth_write=get_state(0xe,0x78b98e,0x78b993,0x78b99f),depth_function=get_state(0x17,0x78b9a5,0x78b9aa,0x78b9b4);
        set_state(0xe,0,0x78b9ba,0x78b9bf,0x78b9c6);set_state(0x17,8,0x78b9cc,0x78b9d1,0x78b9d8);
        call(0x78b9e3,0x793cc0,1);
        set_state(0xe,depth_write,0x78b9eb,0x78b9f0,0x78b9f6);set_state(0x17,depth_function,0x78ba00,0x78ba05,0x78ba0b);
        method(rt.device.end_scene,device(0x78ba11,0x78ba16,0x78ba19));
    }
    // 78ba30, or the mod loader's Direct3D_Present_r (DLL 3293cf35 +0x50290) that replaces it.
    void present(){
        if(rt.present_policy==NinjaPresentPolicy::original){
            const auto hr=method(rt.device.present,device(0x78ba30,0x78ba35,0x78ba40));r.present_hresult=hr;
            if(hr==kD3DErrDeviceLost)call(0x78ba4a,0x789dd0); // tail jump
            return;}
        // [100f167c] = &Direct3D_Device (3d128b0): load the device, its vtable, call +0x3c.
        const auto hr=method(rt.device.present,device(0x1005029b,0x100502a0,0x100502a2));r.present_hresult=hr;
        if(hr==kD3DErrDeviceLost)call(0x100502ac,0x10050300); // direct3d::reset_device
        // Then RaiseEvents(onRenderSceneStart) over 100f88f8..100f88fc: empty (no enabled mod exports it).
    }
    // 78ba50, then 794800 on each 3d0fe00 record.
    void clear(){
        const auto stencil=(need(rt.state.device_flags_at_3d1289c,0x78ba50,0x3d1289c)&1)!=0;
        const auto c=device(0x78ba5e,stencil?0x78ba67:0x78ba78,stencil?0x78ba70:0x78ba81);
        if(!rt.device.clear)fail(NinjaFrameFailure::device_unavailable,c.source_call);
        if(!rt.device.clear(c,stencil?7:3,0,1.0F,0))fail(NinjaFrameFailure::device_unavailable,c.source_call);
        for(std::uint32_t i=0;i<9;++i){
            auto& record=rt.state.polygon_facts->records[i];const auto at=0x3d0fe00+0x24*i;
            if(need(record[0],0x794804,at)==0)continue;
            const auto size=need(record[3],0x79480a,at+0xc),end=need(record[1],0x79480d,at+4);
            record[2]=end-size;
            call(0x794826,0x794800,at); // vertex buffer Lock(0, size, &p, 0x2000) then Unlock: no native vertex-buffer owner yet
        }
    }
    // 78b880
    void begin_scene(){
        auto& flags=rt.state.scene_flags_at_3d08588;
        if((need(flags,0x78b880,0x3d08588)&1)!=0)return;
        *flags|=1;
        method(rt.device.begin_scene,device(0x78b897,0x78b89c,0x78b89f));
        blend(0,8);blend(1,6);
        if(need(rt.state.polygon_facts->vertices,0x78b8b7,0x3d128ac)==0)return;
        const auto depth_write=get_state(0xe,0x78b8c7,0x78b8cc,0x78b8d5),depth_function=get_state(0x17,0x78b8db,0x78b8e0,0x78b8ea);
        set_state(0xe,0,0x78b8f0,0x78b8f5,0x78b8fc);set_state(0x17,8,0x78b902,0x78b907,0x78b90e);
        call(0x78b922,0x77dfc0,{},{0x3ce7164u,4u,0xbf800000u,0x22u});
        set_state(0xe,depth_write,0x78b92b,0x78b930,0x78b939);set_state(0x17,depth_function,0x78b943,0x78b948,0x78b94e);
    }
};
}
NinjaFrameResult run_ninja_frame_call(std::uint32_t thunk,NinjaFrameRuntime& rt){
    Operation op{rt,{}};
    try{
        switch(thunk){
        case 0x781010:op.callback_list();op.hook_table();break;
        case 0x781020:op.end_scene();break;
        case 0x781030:op.present();break;
        case 0x781040:op.clear();break;
        case 0x781050:op.begin_scene();break;
        default:op.r.failure=NinjaFrameFailure::not_a_frame_call;op.r.source_boundary=thunk;break;
        }
    }catch(Stop){}
    return op.r;
}
NinjaFrameResult ninja_hook_register_7959c0(NinjaFrameCallState& s,std::uint32_t hook){
    NinjaFrameResult r;
    for(std::uint32_t i=0;i<16;++i){
        if(!s.hooks_at_3d08af0[i]){r.failure=NinjaFrameFailure::unknown_state;r.source_boundary=0x7959d0;r.required_fact=0x3d08af0+4*i;return r;}
        if(*s.hooks_at_3d08af0[i]==hook)return r;}
    for(auto& slot:s.hooks_at_3d08af0)if(*slot==0){slot=hook;return r;}
    return r;
}
} // namespace sadx

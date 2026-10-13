#include "ninja/ninja_device_init.hpp"
#include <cstring>
namespace sadx {
namespace {
struct Stop{};
struct Operation {
    NinjaDeviceInitRuntime& rt;NinjaDeviceResult r;
    [[noreturn]]void fail(NinjaDeviceFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(NinjaDeviceFailure::unknown_state,at,fact);return *v;}
    void call(std::uint32_t site,std::uint32_t callee){
        r.pending_call=NinjaDeviceCall{site,callee};
        if(!rt.invoke)fail(NinjaDeviceFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);
        if(receipt.state!=GameLogicOwnerState::completed)fail(receipt.state==GameLogicOwnerState::cancelled?NinjaDeviceFailure::cancelled:
            receipt.state==GameLogicOwnerState::failed?NinjaDeviceFailure::owner_failed:NinjaDeviceFailure::missing_owner,site);
        r.pending_call.reset();}
    D3D8FrameCommand device(std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        if(!rt.render.device_at_3d128b0)fail(NinjaDeviceFailure::unknown_device,load,0x3d128b0);
        if(!*rt.render.device_at_3d128b0)fail(NinjaDeviceFailure::null_device,dereference);
        return {*rt.render.device_at_3d128b0,site};}
    // FILD m32 then FADD 2^32 when the integer is negative (an unsigned load).
    X87Value unsigned_load(std::uint32_t v){auto x=rt.fp.from_int64(static_cast<std::int32_t>(v));if(static_cast<std::int32_t>(v)<0)x=rt.fp.add_float_memory(x,4294967296.0F);return x;}
    void run(){
        auto& s=rt.state;
        if(!set_game_frame_multiplier(1,rt.renderer))fail(NinjaDeviceFailure::multiplier_failed,0x791616);
        if(!s.block_at_3d0fae0.fill(0x3d0fae0,0x200,0))fail(NinjaDeviceFailure::unknown_state,0x79162c,0x3d0fae0);
        rt.frame.device_flags_at_3d1289c=0;
        for(auto& record:rt.frame.records_at_3d0fe00)record[0]=0;
        if(!rt.host.create_direct3d)fail(NinjaDeviceFailure::host_unavailable,0x791651);
        const auto d3d=rt.host.create_direct3d(0xdc);if(!d3d)fail(NinjaDeviceFailure::host_unavailable,0x791651);
        s.direct3d_at_3d11f60=*d3d;
        if(*d3d==0)call(0x79165f,0x789dd0);
        rt.render.device_at_3d128b0=NinjaRenderDeviceHandle{};          // 3d128b0 = 0
        call(0x79166a,0x7943c0);                                         // device creation
        {const auto c=device(0x79166f,0x791674,0x79167c);
            if(!rt.host.caps)fail(NinjaDeviceFailure::host_unavailable,0x79167c);
            const auto bytes=rt.host.caps(c);if(!bytes)fail(NinjaDeviceFailure::host_unavailable,0x79167c);
            for(std::size_t i=0;i<bytes->size()&&i<s.caps_at_3d127c0.size;++i)s.caps_at_3d127c0.bytes[i]=(*bytes)[i];}
        call(0x79167f,0x794ec0);call(0x791684,0x78d0d0);                 // then 78c5a0 (RET)
        call(0x79168e,0x796710);call(0x791693,0x791860);call(0x791698,0x7943f0); // then 78c5a0 (RET)
        {const auto stencil=(need(rt.frame.device_flags_at_3d1289c,0x7916a2,0x3d1289c)&1)!=0;
            const auto c=device(0x7916af,stencil?0x7916b7:0x7916c6,stencil?0x7916be:0x7916cd);
            if(!rt.host.clear||!rt.host.clear(c,stencil?7:3,0,1.0F,0))fail(NinjaDeviceFailure::host_unavailable,c.source_call);}
        {const auto c=device(0x7916d3,0x7916d8,0x7916e0);
            if(!rt.host.viewport)fail(NinjaDeviceFailure::host_unavailable,0x7916e0);
            const auto vp=rt.host.viewport(c);if(!vp)fail(NinjaDeviceFailure::host_unavailable,0x7916e0);
            for(std::size_t i=0;i<6;++i)s.viewport_at_3d12780[i]=(*vp)[i];}
        const auto width=*s.viewport_at_3d12780[2],height=*s.viewport_at_3d12780[3];
        s.half_width_at_3d12948=rt.fp.to_float(rt.fp.multiply_float_memory(unsigned_load(width),0.5F));
        auto half_height=rt.fp.multiply_float_memory(unsigned_load(height),0.5F);
        for(std::size_t i=0;i<3;++i)s.words_at_3d0fd00[i]=0xffffffffu;
        s.half_height_at_3d1294c=rt.fp.to_float(half_height);
        const auto x=unsigned_load(*s.viewport_at_3d12780[0]);
        for(std::size_t i=3;i<7;++i)s.words_at_3d0fd00[i]=0xffffffffu;
        for(std::size_t i=0;i<16;++i)s.matrix_at_3d0fd80[i]=need(s.matrix_at_389d318[i],0x79176c,0x389d318+4*static_cast<std::uint32_t>(i));
        s.left_at_3d0fd20=rt.fp.to_float(x);
        s.top_at_3d0fd24=rt.fp.to_float(unsigned_load(*s.viewport_at_3d12780[1]));
        s.right_at_3d0fd28=rt.fp.to_float(rt.fp.add_float_memory(unsigned_load(width-1U),*s.left_at_3d0fd20));
        s.bottom_at_3d0fd2c=rt.fp.to_float(rt.fp.add_float_memory(unsigned_load(height-1U),*s.top_at_3d0fd24));
        call(0x7917dc,0x7914b0);                                        // tail jump
    }
};
}
NinjaDeviceResult run_device_setup_791610(NinjaDeviceInitRuntime& rt){
    Operation op{rt,{}};
    try{op.run();}catch(Stop){}catch(const X87Error&){op.r.failure=NinjaDeviceFailure::fp_failed;op.r.source_boundary=0x7916e6;}
    return op.r;
}
} // namespace sadx

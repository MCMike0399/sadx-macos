#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/ninja_render.hpp"
#include "engine/ninja_fog.hpp"
#include "engine/ninja_view.hpp"
#include <unicorn/unicorn.h>
#include <cstring>
#include <array>
#include <cstdio>
#include <map>
namespace sadx {
namespace {
std::string hex(std::uint32_t v){char text[16];std::snprintf(text,sizeof text,"%x",v);return text;}
std::uint32_t argument(X86Guest& g,unsigned i){return g.u32(g.esp()+4+4*i).value_or(0);}
struct RenderBridge {
    HleProcess& process;
    NinjaRenderState render;
    std::map<std::uint64_t,std::uint32_t> guests;
    std::function<void()> before_call;
    explicit RenderBridge(HleProcess& p):process(p){
        refresh();
        render.set_render_state=[this](const NinjaRenderStateCommand& command){
            if(before_call)before_call();
            const auto r=invoke(command.device,"SetRenderState",0xc8,command.state,command.value,command.source_call);refresh();return r;
        };
    }
    void refresh(){
        const auto address=process.guest().u32(0x3d128b0);
        if(!address){render.device_at_3d128b0.reset();return;}
        if(!*address){render.device_at_3d128b0=NinjaRenderDeviceHandle{};return;}
        std::uint64_t identity=0;
        for(const auto& [id,guest]:guests)if(guest==*address){identity=id;break;}
        if(!identity){identity=guests.size()+1;guests[identity]=*address;}
        render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{identity});
    }
    NinjaRenderCallResult invoke(const NinjaRenderDeviceHandle& native,const char* symbol,std::uint32_t offset,std::uint32_t state,std::uint32_t value,std::uint32_t site){
        auto& g=process.guest();const auto device=guests.find(native->identity);
        if(device==guests.end()){g.stop("device bridge: unknown native device at "+hex(site));return {false,{}};}
        const auto table=g.u32(device->second),target=table?g.u32(*table+offset):std::nullopt;
        const auto* method=process.hle().find("com:IDirect3DDevice8",symbol);
        const auto expected=method?process.hle().trap(g,"com:IDirect3DDevice8",symbol):std::nullopt;
        if(!method||!target||!expected||*target!=*expected||method->argument_bytes!=12){
            g.stop(std::string("device bridge: non-HLE ")+symbol+" target at "+hex(site));return {false,{}};
        }
        const auto frame=g.esp()-0x40;std::array<std::uint32_t,4> previous{};
        if(!g.read(frame,previous.data(),sizeof previous)){g.stop("device bridge: unreadable call frame");return {false,{}};}
        const std::array<std::uint32_t,4> words={site+6,device->second,state,value};
        if(!g.write(frame,words.data(),sizeof words)){g.stop("device bridge: unwritable call frame");return {false,{}};}
        HleCall call(g,frame);const auto receipt=method->body(call);
        g.write(frame,previous.data(),sizeof previous);
        process.hle().record({"com:IDirect3DDevice8",symbol,words[0],{words[1],words[2],words[3]},receipt});
        g.set_eax(receipt);
        return {!g.stop_reason().has_value(),static_cast<std::int32_t>(receipt)};
    }
};
float floating(std::uint32_t raw){float value;std::memcpy(&value,&raw,4);return value;}
template<std::size_t N>void flush_words(X86Guest& g,std::uint32_t address,const std::array<std::optional<std::uint32_t>,N>& values){
    for(unsigned k=0;k<N;++k)if(values[k])g.set_u32(address+4*k,*values[k]);
}
struct DispatchNeeded{};
struct ViewBridge {
    HleProcess& process;RenderBridge bridge;
    NinjaViewState view;NinjaProjectionRangeState range{};NinjaSpriteProjection projection;
    NinjaScreenState screen;NinjaDeviceState device;
    NinjaScreenDevice methods;NinjaMatrixStack stack;
    NinjaMath::SineTable sine{};
    bool perspective_flushed=false,projection_flushed=false,view_flushed=false,full_view;
    explicit ViewBridge(HleProcess& p,bool full):process(p),bridge(p),stack(std::vector<NinjaMatrix>(1),1,0),full_view(full){
        auto& g=p.guest();
        for(unsigned k=0;k<8;++k)view.camera_at_3aad0a0[k]=g.u32(0x3aad0a0+4*k);
        const auto near=g.u32(0x3aad0c0),far=g.u32(0x3aad0c4);
        if(near)range.cache_x_at_3aad0c0=floating(*near);if(far)range.cache_y_at_3aad0c4=floating(*far);
        for(unsigned k=0;k<6;++k)device.viewport_at_3d12780[k]=g.u32(0x3d12780+4*k);
        reload_screen();
        if(!g.read(0x88af78,sine.data(),sizeof sine))g.stop("view bridge: missing sine table at 88af78");
        methods.set_transform=[this](const D3D8FrameCommand& c,std::uint32_t state,const D3dxMatrix&)->std::optional<std::int32_t>{
            if(process.guest().u32(0x389b784)!=0x774310||process.guest().u32(0x389b7c0)!=0x7757e0){
                if(process.guest().u32(0x389b948)==0xffff)throw DispatchNeeded{};
                process.guest().stop("view bridge: non-SSE D3DX dispatch at 79126a");return std::nullopt;
            }
            flush();
            const auto address=state==3?0x3d129c0U:process.guest().u32(0x3d0f818).value_or(0);
            const auto r=bridge.invoke(c.device,"SetTransform",0x94,state,address,c.source_call);
            bridge.refresh();reload_screen();
            return r.completed?r.original_hresult:std::nullopt;
        };
        methods.multiply_transform=[this](const D3D8FrameCommand& c,std::uint32_t state,const D3dxMatrix&)->std::optional<std::int32_t>{
            flush();const auto r=bridge.invoke(c.device,"MultiplyTransform",0x9c,state,0x3d0fd80,c.source_call);
            bridge.refresh();reload_screen();
            if(!full_view){
                const auto current=process.guest().u32(0x3d0f818);
                if(!current||!process.guest().read(*current,stack.current().data(),64)){process.guest().stop("view bridge: missing current matrix at 791299");return std::nullopt;}
            }
            return r.completed?r.original_hresult:std::nullopt;
        };
    }
    void reload_screen(){for(unsigned k=0;k<16;++k)device.matrix_at_3d0fd80[k]=process.guest().u32(0x3d0fd80+4*k);}
    void flush_view(){
        auto& g=process.guest();
        if(full_view&&!view_flushed&&view.view_copy_at_3d0f980[0]){
            flush_words(g,0x3d0f760,view.view_at_3d0f760);flush_words(g,0x3d0f980,view.view_copy_at_3d0f980);
            const auto base=g.u32(0x3d0fa1c);
            if(!base||!g.write(*base,stack.current().data(),64)){g.stop("view bridge: missing base matrix at 781421");return;}
            g.set_u32(0x3d0f818,*base);g.set_u32(0x3d0fa2c,1);
            const auto write_float=[&](std::uint32_t at,const std::optional<float>& f){if(f)g.write(at,&*f,4);};
            write_float(0x3d0f9e0,projection.scale_at_3d0f9e0);write_float(0x3d0f808,range.near_at_3d0f808);write_float(0x3d0f968,range.far_at_3d0f968);
            view_flushed=true;
        }
    }
    void flush_perspective(){
        if(!perspective_flushed&&view.perspective_at_3d12960[0]){flush_words(process.guest(),0x3d12960,view.perspective_at_3d12960);perspective_flushed=true;}
    }
    void flush(){
        flush_view();flush_perspective();
        if(!projection_flushed&&screen.projection_at_3d129c0[0]){flush_words(process.guest(),0x3d129c0,screen.projection_at_3d129c0);projection_flushed=true;}
    }
    std::uint32_t run(X86Guest& g,const std::vector<std::uint32_t>& args){
        if(full_view&&args[0]!=0x3aad0a0){g.stop("view bridge: camera blocks other than 3aad0a0 are not modelled");return 0;}
        X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));SseContext sse(g.mxcsr());
        NinjaViewRuntime rt{view,range,projection,screen,device,bridge.render,methods,stack,sine,fp,sse,sse_reciprocal_by_division};
        const auto result=full_view?ninja_set_view_781250(rt):ninja_set_projection_791170(floating(args[0]),floating(args[1]),floating(args[2]),rt);
        flush();flush_words(g,0x389d398,view.view_at_389d398);flush_words(g,0x389d358,view.view_inverse_at_389d358);
        if(!result&&!g.stop_reason())g.stop("view bridge: native boundary "+hex(result.source_boundary));
        return full_view?0:12;
    }
};

}
std::vector<OverrideEntry> device_overrides(HleProcess& p){
    std::vector<OverrideEntry> out;
    for(const auto address:{0x4030d0U,0x77ec60U}){
        out.push_back({address,address==0x4030d0?"ninja_set_blend":"ninja_set_blend_leaf",
            address==0x4030d0?std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0x3ab9890,1}}:std::vector<std::pair<std::uint32_t,std::uint32_t>>{},
            [&p,address](X86Guest& g)->std::uint32_t{
                RenderBridge bridge(p);
                std::array<std::uint8_t,88> bytes{};
                const auto table=g.read(0x88ad64,bytes.data(),bytes.size())?decode_ninja_blend_tables(bytes.data(),bytes.size()):std::nullopt;
                if(table)bridge.render.blend_tables=&*table;
                std::uint8_t packed=0;if(g.read(0x3ab9890,&packed,1))bridge.render.packed_blend_at_3ab9890=packed;
                const auto selector=argument(g,0),value=argument(g,1);
                bool stored=false;
                const auto flush=[&](){if(address==0x4030d0&&selector<2&&!stored&&bridge.render.packed_blend_at_3ab9890){const auto v=*bridge.render.packed_blend_at_3ab9890;g.write(0x3ab9890,&v,1);stored=true;}};
                bridge.before_call=flush;
                const auto result=address==0x4030d0?ninja_set_blend(selector,value,bridge.render):ninja_set_blend_77ec60(selector,value,bridge.render);
                flush();
                if(!result&&!g.stop_reason())g.stop("override "+hex(address)+": native boundary "+hex(result.source_boundary));
                return 0;
            }});
    }
    out.push_back({0x7949b0,"ninja_fog_color",{{0x3d12898,4}},[&p](X86Guest& g)->std::uint32_t{
        RenderBridge bridge(p);NinjaQueueConsumerState state;NinjaFogDeviceState device;
        state.fog_supported_at_3d128a4=g.u32(0x3d128a4);device.color_at_3d12898=g.u32(0x3d12898);
        const auto old_color=device.color_at_3d12898;
        bool stored=false;const auto flush=[&](){if(!stored&&device.color_at_3d12898&&device.color_at_3d12898!=old_color){g.set_u32(0x3d12898,*device.color_at_3d12898);stored=true;}};
        bridge.before_call=flush;NinjaFogDeviceOwners owners{state,device,&bridge.render};
        const auto result=ninja_fog_color_7949b0(argument(g,0),owners);flush();
        if(!result&&!g.stop_reason())g.stop("override 7949b0: native boundary "+hex(result.source_boundary));
        return 0;
    }});
    for(const auto entry:{0x781250U,0x791170U}){
        out.push_back({entry,entry==0x781250?"ninja_set_view":"ninja_set_projection",
            {{0x3d0f760,64},{0x3d0f980,64},{0x3d0f9e0,4},{0x3d0f808,4},{0x3d0f968,4},{0x3d12960,64},{0x3d129c0,64},{0x389d398,64},{0x389d358,64}},
            [&p,entry](X86Guest& g)->std::uint32_t{
                const auto full=entry==0x781250;std::vector<std::uint32_t> args;
                for(unsigned k=0;k<(full?1U:3U);++k)args.push_back(argument(g,k));
                auto bridge=std::make_shared<ViewBridge>(p,full);
                try{return bridge->run(g,args);}
                catch(const DispatchNeeded&){
                    // The source has stored the view/perspective but not the multiplied projection
                    // when its first D3DX dispatch wrapper calls the original initializer.
                    bridge->flush_view();bridge->flush_perspective();
                    g.redirect(0x77350c,{1},full?0:12,[bridge,args](X86Guest& guest){
                        if(guest.u32(0x389b784)!=0x774310||guest.u32(0x389b7c0)!=0x7757e0){guest.stop("view bridge: non-SSE dispatch after 77350c");return;}
                        bridge->run(guest,args);
                    },full?0x8c:0x6c);
                    return full?0:12;
                }
            }});
    }
    return out;
}
std::vector<OverrideEntry> process_overrides(HleProcess& p,const ExecutableImage& image){
    auto out=all_overrides(image);
    for(auto& entry:device_overrides(p))out.push_back(std::move(entry));
    return out;
}

}

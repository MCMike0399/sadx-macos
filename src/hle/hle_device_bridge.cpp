#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/ninja_render.hpp"
#include "engine/ninja_fog.hpp"
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
            const auto r=invoke(command);refresh();return r;
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
    NinjaRenderCallResult invoke(const NinjaRenderStateCommand& command){
        auto& g=process.guest();const auto device=guests.find(command.device->identity);
        if(device==guests.end()){g.stop("device bridge: unknown native device at "+hex(command.source_call));return {false,{}};}
        const auto table=g.u32(device->second),target=table?g.u32(*table+0xc8):std::nullopt;
        const auto* method=process.hle().find("com:IDirect3DDevice8","SetRenderState");
        const auto expected=method?process.hle().trap(g,"com:IDirect3DDevice8","SetRenderState"):std::nullopt;
        if(!method||!target||!expected||*target!=*expected||method->argument_bytes!=12){
            g.stop("device bridge: non-HLE SetRenderState target at "+hex(command.source_call));return {false,{}};
        }
        const auto frame=g.esp()-0x40;std::array<std::uint32_t,4> previous{};
        if(!g.read(frame,previous.data(),sizeof previous)){g.stop("device bridge: unreadable call frame");return {false,{}};}
        const std::array<std::uint32_t,4> words={command.source_call+6,device->second,command.state,command.value};
        if(!g.write(frame,words.data(),sizeof words)){g.stop("device bridge: unwritable call frame");return {false,{}};}
        HleCall call(g,frame);const auto receipt=method->body(call);
        g.write(frame,previous.data(),sizeof previous);
        process.hle().record({"com:IDirect3DDevice8","SetRenderState",words[0],{words[1],words[2],words[3]},receipt});
        g.set_eax(receipt);
        return {!g.stop_reason().has_value(),static_cast<std::int32_t>(receipt)};
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
    return out;
}
std::vector<OverrideEntry> process_overrides(HleProcess& p,const ExecutableImage& image){
    auto out=all_overrides(image);
    for(auto& entry:device_overrides(p))out.push_back(std::move(entry));
    return out;
}

}

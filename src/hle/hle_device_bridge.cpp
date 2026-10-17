#include "guest/frame_control_reference.hpp"
#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include "hle/hle_heap_bridge.hpp"
#include "hle/hle_task_bridge.hpp"
#include "engine/ninja_render.hpp"
#include "guest/ninja_fog_reference.hpp"
#include "engine/ninja_view.hpp"
#include "engine/ninja_queue_lifecycle.hpp"
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
    NinjaRenderCallResult invoke(const NinjaRenderDeviceHandle& native,const char* symbol,std::uint32_t offset,std::uint32_t state,std::uint32_t value,std::uint32_t site,std::uint32_t reserve=0x40){
        return invoke_arguments(native,symbol,offset,{state,value},site,reserve);
    }
    NinjaRenderCallResult invoke_arguments(const NinjaRenderDeviceHandle& native,const char* symbol,std::uint32_t offset,
        const std::vector<std::uint32_t>& args,std::uint32_t site,std::uint32_t reserve,bool expose_frame=false,int table_register=0){
        auto& g=process.guest();const auto device=guests.find(native->identity);
        if(device==guests.end()){g.stop("device bridge: unknown native device at "+hex(site));return {false,{}};}
        const auto table=g.u32(device->second),target=table?g.u32(*table+offset):std::nullopt;
        const auto* method=process.hle().find("com:IDirect3DDevice8",symbol);
        const auto expected=method?process.hle().trap(g,"com:IDirect3DDevice8",symbol):std::nullopt;
        if(!method||!target||!expected||*target!=*expected||method->argument_bytes!=4*(args.size()+1)){
            g.stop(std::string("device bridge: non-HLE ")+symbol+" target at "+hex(site));return {false,{}};
        }
        const auto esp=g.esp(),frame=esp-reserve;std::vector<std::uint32_t> previous(args.size()+2);
        if(!g.read(frame,previous.data(),previous.size()*4)){g.stop("device bridge: unreadable call frame");return {false,{}};}
        std::vector<std::uint32_t> words={site+6,device->second};words.insert(words.end(),args.begin(),args.end());
        if(!g.write(frame,words.data(),words.size()*4)){g.stop("device bridge: unwritable call frame");return {false,{}};}
        if(expose_frame){g.set_reg(UC_X86_REG_ESP,frame);g.set_eax(device->second);g.set_reg(table_register,*table);}
        HleCall call(g,frame);const auto receipt=method->body(call);
        if(!g.stop_reason()){
            g.write(frame,previous.data(),previous.size()*4);
            if(expose_frame)g.set_reg(UC_X86_REG_ESP,esp);
        }else if(!expose_frame)g.write(frame,previous.data(),previous.size()*4);
        std::vector<std::uint32_t> arguments={device->second};arguments.insert(arguments.end(),args.begin(),args.end());
        process.hle().record({"com:IDirect3DDevice8",symbol,words[0],std::move(arguments),receipt});
        g.set_eax(receipt);
        return {!g.stop_reason().has_value(),static_cast<std::int32_t>(receipt)};
    }
};
float floating(std::uint32_t raw){float value;std::memcpy(&value,&raw,4);return value;}
template<std::size_t N>void flush_words(X86Guest& g,std::uint32_t address,const std::array<std::optional<std::uint32_t>,N>& values){
    for(unsigned k=0;k<N;++k)if(values[k])g.set_u32(address+4*k,*values[k]);
}
struct QueueLifecycleBridge {
    HleProcess& process;
    explicit QueueLifecycleBridge(HleProcess& p):process(p){}
    void reset(bool allocated_buffer=false){
        auto& g=process.guest();const auto base=g.u32(0x3abd960),size=g.u32(0x3abd958);
        if(base&&size)g.set_u32(0x3abd954,(*base+0x1fU)&~0x1fU);
        if(!base||!size||!*base||*size!=0x88000){g.stop("queue bridge: unobserved buffer at 407ad0");return;}
        std::vector<std::uint8_t> bytes(*size);
        if(!g.read(*base,bytes.data(),bytes.size())){g.stop("queue bridge: unreadable buffer at 407ad0");return;}
        auto arena=std::make_shared<NinjaRenderQueueArena>(std::move(bytes),0,*size,g.u32(0x3abd9c8));
        NinjaSpriteQueue queue(arena);NinjaSpriteQueueRuntime runtime;NinjaQueueConsumerState consumer;
        GameLogicTimingState timing;NinjaSpriteProjection projection{};RenderBridge bridge(process);
        runtime.render_state=&bridge.render;runtime.flag_at_3b12208=g.u32(0x3b12208);runtime.mode_at_3ab98ac=g.u32(0x3ab98ac);
        std::uint8_t mode=0;if(g.read(0x3abdbc0,&mode,1))consumer.mode_byte_at_3abdbc0=mode;
        consumer.count_at_3abd9d4=g.u32(0x3abd9d4);consumer.count_at_3abd9dc=g.u32(0x3abd9dc);timing.pending_logic_ticks=g.u32(0x3b1117c);
        const auto color=[&](std::uint32_t at)->std::optional<std::array<float,4>>{std::array<float,4> value{};if(!g.read(at,value.data(),16))return {};return value;};
        runtime.color_at_3ab9864=color(0x3ab9864);projection.color_argb_at_3d0f7f0=color(0x3d0f7f0);
        const auto initial=color(0x3b18230);const auto depth_mode=g.u32(0x8928c8);
        std::array<std::uint32_t,8> depth{};const auto depth_known=g.read(0x88ad9c,depth.data(),32);
        const auto one=g.u32(0x7dcb10),negative=g.u32(0x7dcb0c);
        if(!one||!negative){g.stop("queue bridge: unknown colour constants at 402f55");return;}
        std::array<std::optional<std::uint32_t>,3> input{},captured{},source{},destination{};
        for(unsigned k=0;k<3;++k){input[k]=g.u32(0x900840+4*k);captured[k]=g.u32(0x3ab9898+4*k);source[k]=g.u32(0x900858+4*k);destination[k]=g.u32(0x90084c+4*k);}
        std::map<std::uint32_t,std::uint32_t> flushed;
        const auto store=[&](std::uint32_t at,std::optional<std::uint32_t> value,unsigned width=4){
            if(!value)return;const auto i=flushed.find(at);
            if(i==flushed.end()){std::uint32_t previous=0;if(g.read(at,&previous,width))flushed[at]=previous;}
            if(flushed.find(at)==flushed.end()||flushed[at]!=*value){if(!g.write(at,&*value,width))g.stop("queue bridge: unwritable global "+hex(at));flushed[at]=*value;}
        };
        const auto bits=[](float value){std::uint32_t raw;std::memcpy(&raw,&value,4);return raw;};
        bool buffer_flushed=false;
        const auto flush=[&](bool copy_projection=true){
            if(!buffer_flushed){g.set_u32(0x3abd954,*base+queue.arena()->cursor());if(!g.write(*base,queue.arena()->bytes().data(),queue.arena()->bytes().size()))g.stop("queue bridge: unwritable buffer at 407ad0");g.set_u32(0x3abd9c8,queue.arena()->overflow_reported().value_or(0));buffer_flushed=true;}
            if(runtime.color_at_3ab9864)for(unsigned k=0;k<4;++k)store(0x3ab9864+4*k,bits((*runtime.color_at_3ab9864)[k]));
            if(copy_projection&&projection.color_argb_at_3d0f7f0)for(unsigned k=0;k<4;++k)store(0x3d0f7f0+4*k,bits((*projection.color_argb_at_3d0f7f0)[k]));
            store(0x3b12208,runtime.flag_at_3b12208);store(0x3abdbc0,consumer.mode_byte_at_3abdbc0,1);
        };
        // Snapshot before native stores so a live backend's unrelated writes survive.
        for(const auto address:{0x3ab9864U,0x3d0f7f0U})for(unsigned k=0;k<4;++k)if(const auto v=g.u32(address+4*k))flushed[address+4*k]=*v;
        if(runtime.flag_at_3b12208)flushed[0x3b12208]=*runtime.flag_at_3b12208;if(consumer.mode_byte_at_3abdbc0)flushed[0x3abdbc0]=*consumer.mode_byte_at_3abdbc0;
        bridge.before_call=[&]{flush();};const auto invoke=bridge.render.set_render_state;
        bridge.render.set_render_state=[&](const NinjaRenderStateCommand& command){
            const auto result=invoke(command);
            // The source reads these after COM, including the final raw colour copy.
            for(unsigned k=0;k<3;++k)input[k]=g.u32(0x900840+4*k);
            runtime.color_at_3ab9864=color(0x3ab9864);projection.color_argb_at_3d0f7f0=color(0x3d0f7f0);
            return result;
        };
        NinjaQueueLifecycleOwners owners{queue,runtime,consumer,timing,projection,initial,depth_known?&depth:nullptr,floating(*one),floating(*negative),depth_mode,input,captured,source,destination};
        X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));
        const auto result=allocated_buffer?ninja_queue_allocate_408630(owners,*base,fp):ninja_queue_reset_407ab0(owners,*base,fp);flush(false);
        if(!result){if(!g.stop_reason())g.stop("queue bridge: native boundary "+hex(result.source_boundary));return;}
        const auto buckets=g.u32(0x90083c);if(!buckets){g.stop("queue bridge: unknown buckets at 40734a");return;}
        for(unsigned k=0;k<2048;++k){const auto address=*buckets+8*k;const std::uint32_t links[2]={address,address};if(!g.write(address,links,8)){g.stop("queue bridge: unwritable buckets at 407360");return;}}
        flush_words(g,0x3ab9898,captured);
        // 407330's final 77ff90 reads colour after the render-state calls.
        if(projection.color_argb_at_3d0f7f0)g.write(0x3d0f7f0,projection.color_argb_at_3d0f7f0->data(),16);
        if(runtime.mode_at_3ab98ac)g.set_u32(0x3ab98ac,*runtime.mode_at_3ab98ac);
        if(timing.pending_logic_ticks)g.set_u32(0x3b1117c,static_cast<std::uint32_t>(*timing.pending_logic_ticks));
        if(consumer.count_at_3abd9d4)g.set_u32(0x3abd9d4,static_cast<std::uint32_t>(*consumer.count_at_3abd9d4));
        if(consumer.count_at_3abd9dc)g.set_u32(0x3abd9dc,static_cast<std::uint32_t>(*consumer.count_at_3abd9dc));
    }
    void allocated(){
        auto& g=process.guest();const auto base=g.reg(UC_X86_REG_EAX);
        const auto second=g.u32(0x90085c);if(!second){g.stop("queue bridge: unknown allocation capture at 408656");return;}
        const auto third=g.u32(0x900860);if(!third){g.stop("queue bridge: unknown allocation capture at 40865c");return;}
        g.set_u32(0x3abd960,base);g.set_u32(0x3ab9874,base+0x88000);
        const auto first=g.u32(0x900858);if(!first){g.stop("queue bridge: unknown allocation capture at 408674");return;}
        g.set_u32(0x90084c,*first);g.set_u32(0x900850,*second);g.set_u32(0x900854,*third);
        reset(true);
    }
    void allocate(const std::shared_ptr<QueueLifecycleBridge>& self,bool freed=false){
        auto& g=process.guest();const auto old=g.u32(0x3abd960);if(!old){g.stop("queue bridge: unknown old buffer at 408630");return;}
        if(!freed&&*old){g.redirect(0x780fb0,{*old},0,[self](X86Guest&){self->allocate(self,true);},4,0x40863f);return;}
        g.set_u32(0x3abd958,0x88000);g.redirect(0x780f60,{0x88000},0,[self](X86Guest&){self->allocated();},4,0x408656);
    }
};
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

std::optional<float> load_float(X86Guest& g,std::uint32_t at){const auto value=g.u32(at);if(!value)return std::nullopt;return floating(*value);}
std::uint32_t projection_range(HleProcess& p){
    auto& g=p.guest();RenderBridge bridge(p);NinjaProjectionRangeState state{};
    const auto negative_one=load_float(g,0x7dcb0c),limit=load_float(g,0x7dce50);
    if(!negative_one||!limit){g.stop("projection range bridge: missing clamp constants at 7815e4");return 0;}
    state.negative_one_at_7dcb0c=*negative_one;state.limit_at_7dce50=*limit;
    state.cache_x_at_3aad0c0=load_float(g,0x3aad0c0);state.cache_y_at_3aad0c4=load_float(g,0x3aad0c4);
    bool flushed=false;
    const auto flush=[&]{
        if(flushed)return;
        for(const auto& [at,value]:std::array<std::pair<std::uint32_t,std::optional<float>>,5>{{
            {0x3d0f808,state.near_at_3d0f808},{0x3d0f968,state.far_at_3d0f968},{0x90082c,state.x_at_90082c},
            {0x3aad0c0,state.cache_x_at_3aad0c0},{0x3aad0c4,state.cache_y_at_3aad0c4}}})if(value)g.write(at,&*value,4);
        if(state.y_bits_at_900830)g.set_u32(0x900830,*state.y_bits_at_900830);
        flushed=true;
    };
    // Source SUB ESP,40 supplies both COM calls with the same local matrix pointer.
    // Place the synchronous call frame below that matrix, rather than overwriting it.
    const auto matrix_address=g.esp()-0x40;
    NinjaTransformOwner methods;
    methods.get=[&](std::uint32_t kind)->std::optional<NinjaMatrix>{
        flush();
        if(state.near_at_3d0f808)g.write(g.esp()+4,&*state.near_at_3d0f808,4);
        if(state.far_at_3d0f968)g.write(g.esp()+8,&*state.far_at_3d0f968,4);
        if(!bridge.render.device_at_3d128b0||!*bridge.render.device_at_3d128b0){g.stop("projection range bridge: missing device at 403207");return std::nullopt;}
        const auto r=bridge.invoke(*bridge.render.device_at_3d128b0,"GetTransform",0x98,kind,matrix_address,0x403207,0x50);
        bridge.refresh();NinjaMatrix matrix{};
        if(!r.completed||!g.read(matrix_address,matrix.data(),64))return std::nullopt;
        return matrix;
    };
    methods.set=[&](std::uint32_t kind,const NinjaMatrix& matrix){
        if(!g.write(matrix_address,matrix.data(),64)){g.stop("projection range bridge: missing matrix at 403236");return false;}
        if(!bridge.render.device_at_3d128b0||!*bridge.render.device_at_3d128b0){g.stop("projection range bridge: missing device at 403236");return false;}
        const auto r=bridge.invoke(*bridge.render.device_at_3d128b0,"SetTransform",0x94,kind,matrix_address,0x403236,0x50);
        return r.completed;
    };
    X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));
    const auto result=ninja_set_projection_range_403180(floating(argument(g,0)),floating(argument(g,1)),state,methods,fp);
    if(!flushed&&state.y_bits_at_900830)g.set_u32(g.esp()+8,*state.y_bits_at_900830);
    flush();
    if(!result&&!g.stop_reason())g.stop("projection range bridge: native boundary "+hex(result.source_boundary));
    return 0;
}
struct FogBridge {
    HleProcess& process;RenderBridge bridge;
    NinjaQueueConsumerState state;NinjaFogDeviceState device;NinjaFogTable table{};std::uint32_t table_address=0x3b10f78;
    std::optional<std::uint8_t> flag,flushed_flag;
    std::map<std::uint32_t,std::uint32_t> flushed;
    explicit FogBridge(HleProcess& p):process(p),bridge(p){
        auto& g=p.guest();state.fog_enabled_at_3d12798=g.u32(0x3d12798);state.fog_supported_at_3d128a4=g.u32(0x3d128a4);
        device.software_density_at_3d128a0=g.u32(0x3d128a0);device.color_at_3d12898=g.u32(0x3d12898);
        device.end_index_at_3d0fce0=g.u32(0x3d0fce0);device.start_index_at_3d12950=g.u32(0x3d12950);
        state.fog_mode_at_3d0849c=g.u32(0x3d0849c);
        std::uint8_t byte=0;if(g.read(0x3abdc6c,&byte,1))state.fog_profile_at_3abdc6c=byte;
        if(g.read(0x909eb4,&byte,1))state.special_flags_at_909eb4=byte;
        if(g.read(0x3abdcfe,&byte,1))flag=flushed_flag=byte;
        state.cached_fog_start_at_90a098=load_float(g,0x90a098);state.cached_fog_end_at_90a09c=load_float(g,0x90a09c);
        for(unsigned k=0;k<128;++k)table[k]=load_float(g,table_address+4*k);
        device.start_value_at_3d0fac0=load_float(g,0x3d0fac0);device.end_value_at_3d12894=load_float(g,0x3d12894);
        for(unsigned k=0;k<128;++k){device.w_at_3d12a00[k]=load_float(g,0x3d12a00+4*k);device.reciprocal_at_3d12c00[k]=load_float(g,0x3d12c00+4*k);}
        // Remember serialized values, including NaN payloads, so callback effects are not
        // overwritten by repeated flushes of fields the native operation has not changed.
        snapshot();bridge.before_call=[this]{flush();};
    }
    void each(const std::function<void(std::uint32_t,const std::optional<std::uint32_t>&)>& f){
        f(0x3d0849c,state.fog_mode_at_3d0849c);
        f(0x3d12798,state.fog_enabled_at_3d12798);f(0x3d128a4,state.fog_supported_at_3d128a4);
        f(0x3d128a0,device.software_density_at_3d128a0);f(0x3d12898,device.color_at_3d12898);
        f(0x3d0fce0,device.end_index_at_3d0fce0);f(0x3d12950,device.start_index_at_3d12950);
        const auto scalar=[&](std::uint32_t at,const std::optional<float>& value){
            if(!value){f(at,std::nullopt);return;}std::uint32_t bits;std::memcpy(&bits,&*value,4);f(at,bits);
        };
        scalar(0x90a098,state.cached_fog_start_at_90a098);scalar(0x90a09c,state.cached_fog_end_at_90a09c);
        scalar(0x3d0fac0,device.start_value_at_3d0fac0);scalar(0x3d12894,device.end_value_at_3d12894);
        for(unsigned k=0;k<128;++k){scalar(0x3d12a00+4*k,device.w_at_3d12a00[k]);scalar(0x3d12c00+4*k,device.reciprocal_at_3d12c00[k]);scalar(table_address+4*k,table[k]);}
    }
    void snapshot(){each([this](std::uint32_t at,const std::optional<std::uint32_t>& value){if(value)flushed[at]=*value;});}
    void flush(){each([this](std::uint32_t at,const std::optional<std::uint32_t>& value){
        if(value&&(!flushed.count(at)||flushed[at]!=*value)){process.guest().set_u32(at,*value);flushed[at]=*value;}
    });
        if(flag&&flag!=flushed_flag){const auto value=*flag;process.guest().write(0x3abdcfe,&value,1);flushed_flag=flag;}
    }
    std::uint32_t run(X86Guest& g,std::uint32_t entry){
        X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));
        NinjaFogDeviceOwners owners{state,device,&bridge.render};NinjaFogResult result;
        if(entry==0x794ec0)result=ninja_fog_initialize_794ec0(owners,{g.u32(0x3d127e4),g.u32(0x3d08ae0)},fp);
        else if(entry==0x411af0||entry==0x411b40){
            const auto display=g.u32(0x3b1117c);std::optional<std::int32_t> typed;
            if(display)typed=static_cast<std::int32_t>(*display);
            const auto start=load_float(g,0x3abdc60),end=load_float(g,0x3abdc64);const auto color=g.u32(0x3abdc68);
            const auto sentinel=load_float(g,0x7dce50);
            if(!sentinel){g.stop("fog bridge: missing sentinel at 7dce50");return 0;}
            NinjaFogOwners switch_owners{state,flag,&typed,&bridge.render,*sentinel,&device,&table,&start,&end,&color};
            result=ninja_fog_411af0_411b40(entry==0x411af0,switch_owners,fp);
        }else{
            const auto source=argument(g,0);
            if(source!=table_address){
                for(unsigned k=0;k<128;++k)flushed.erase(table_address+4*k);
                table_address=source;for(unsigned k=0;k<128;++k){table[k]=load_float(g,source+4*k);if(const auto value=g.u32(source+4*k))flushed[source+4*k]=*value;}
            }
            if(entry==0x787260)result=ninja_fog_generate_table_787260(table,floating(argument(g,1)),floating(argument(g,2)),owners,fp);
            else result=ninja_fog_set_table_794e40(table,owners,fp);
        }
        flush();
        if(!result&&!g.stop_reason())g.stop("fog bridge: native boundary "+hex(result.source_boundary));
        return 0;
    }
};

}
std::uint32_t screen_polygon(HleProcess& p){
    auto& g=p.guest();const auto frame=g.esp(),block=g.reg(UC_X86_REG_EAX);
    const auto read=[&](std::uint32_t at,std::uint32_t site)->std::optional<std::uint32_t>{
        const auto value=g.u32(at);if(!value)g.stop("polygon bridge: missing word "+hex(at)+" at "+hex(site));return value;
    };
    const auto points=read(block,0x77dfc7);if(!points)return 0;g.set_reg(UC_X86_REG_ECX,*points);
    const auto colors=read(block+4,0x77dfc9);if(!colors)return 0;g.set_edx(*colors);g.set_u32(frame,*points);
    const auto extra=read(block+8,0x77dfcf);if(!extra)return 0;g.set_reg(UC_X86_REG_ECX,*extra);g.set_u32(frame+4,*colors);
    const auto old_count=read(block+12,0x77dfd6);if(!old_count)return 0;g.set_edx(*old_count);
    const auto count=read(frame+0x18,0x77dfd9);if(!count)return 0;
    g.set_eax(*count);g.set_u32(frame+8,*extra);g.set_u32(frame+12,*count);
    if(static_cast<std::int32_t>(*count)<=2){g.tail_redirect(0x77e04a);return 0;}
    const auto original_argument=read(frame+0x1c,0x77dff4),flags=read(frame+0x20,0x77e015);
    if(!original_argument||!flags)return 0;
    // Bounded native workspace; larger or noncanonical floating contexts retain source execution.
    if(*count>16384U||g.reg(UC_X86_REG_FPTAG)!=0xffffU){g.tail_redirect(0x77dfee);return 0;}
    if((g.reg(UC_X86_REG_FPCW)&0x3fU)!=0x3fU){g.stop("polygon bridge: unmasked x87 exceptions not supplied at 77dfee");return 0;}
    const auto filter_call=g.u32(0x77e032),filter_tail=g.u32(0x77e036);
    if(filter_call!=0x00d749e8U||!filter_tail||(*filter_tail&0xffU)!=0U){
        // An internal profile variant (forced linear filter) remains original.
        g.tail_redirect(0x77dfee);return 0;
    }
    RenderBridge bridge(p);NinjaScreenState state{};NinjaDeviceState device{};NinjaScreenDevice methods{};
    NinjaTextureSelectionState textures{};std::optional<std::uint32_t> buffer;
    std::uint32_t mirrored_id=0,mirrored_active=0,generated_buffer=0;bool generated=false;
    const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return word;};
    const auto reload_depth=[&](){
        for(unsigned k=0;k<6;++k)device.viewport_at_3d12780[k]=g.u32(0x3d12780+4*k);
        for(unsigned k=0;k<16;++k){state.projection_at_3d129c0[k]=g.u32(0x3d129c0+4*k);device.matrix_at_389d318[k]=g.u32(0x389d318+4*k);}
    };
    const auto reload_texture=[&](){
        textures.global_id_at_3d0fdf4=g.u32(0x3d0fdf4);
        const auto active=g.u32(0x3d129a0);textures.active_memory_at_3d129a0.reset();
        if(active){textures.active_memory_at_3d129a0=*active?std::make_shared<const NinjaTextureMemoryReference>(NinjaTextureMemoryReference{nullptr,0,{},{},{},{},{},{}}):std::shared_ptr<const NinjaTextureMemoryReference>{};mirrored_active=*active;}
        if(textures.global_id_at_3d0fdf4)mirrored_id=*textures.global_id_at_3d0fdf4;
    };
    const auto flush_texture=[&](){
        if(textures.global_id_at_3d0fdf4&&*textures.global_id_at_3d0fdf4!=mirrored_id){
            g.set_u32(0x3d0fdf4,*textures.global_id_at_3d0fdf4);mirrored_id=*textures.global_id_at_3d0fdf4;
        }
        if(textures.active_memory_at_3d129a0&&!*textures.active_memory_at_3d129a0&&mirrored_active){
            g.set_u32(0x3d129a0,0);mirrored_active=0;
        }
    };
    const auto after=[&](){bridge.refresh();reload_texture();reload_depth();buffer=g.u32(0x3d128ac);};
    const auto table_register=[](std::uint32_t site){
        return site==0x791964||site==0x791984||site==0x78b7a1||site==0x78b7ca||site==0x78b7f2||site==0x78b822||site==0x78ea80?UC_X86_REG_EDX:UC_X86_REG_ECX;
    };
    const auto invoke=[&](const NinjaRenderDeviceHandle& handle,const char* name,std::uint32_t offset,
        const std::vector<std::uint32_t>& args,std::uint32_t site,std::uint32_t reserve){
        flush_texture();const auto r=bridge.invoke_arguments(handle,name,offset,args,site,reserve,true,table_register(site));
        if(!g.stop_reason())after();return r;
    };
    bridge.render.set_render_state=[&](const NinjaRenderStateCommand& c){return invoke(c.device,"SetRenderState",0xc8,{c.state,c.value},c.source_call,0x14);};
    bridge.render.set_texture_stage_state=[&](const NinjaRenderStageCommand& c){return invoke(c.device,"SetTextureStageState",0xfc,{c.stage,c.state,c.value},c.source_call,0x18);};
    NinjaTextureSelectionRuntime texture_runtime{bridge.render,nullptr,nullptr,[&](const NinjaTextureBindCommand& c){
        return invoke(c.device,"SetTexture",0xf4,{c.stage,0},c.source_call,generated?0x30U:0x14U);
    }};
    methods.set_vertex_shader=[&](const D3D8FrameCommand& c,std::uint32_t shader)->std::optional<std::int32_t>{
        const auto r=invoke(c.device,"SetVertexShader",0x130,{shader},c.source_call,0x28);return r.completed?r.original_hresult:std::nullopt;
    };
    const std::array<std::uint32_t,4> saved={g.reg(UC_X86_REG_EBP),g.reg(UC_X86_REG_EBX),g.reg(UC_X86_REG_ESI),g.reg(UC_X86_REG_EDI)};
    methods.draw_primitive_up=[&](const D3D8FrameCommand& c,const NinjaScreenPolygon& polygon)->std::optional<std::int32_t>{
        const auto current=read(0x3d128ac,0x78ea86);if(!current)return std::nullopt;
        g.set_edx(polygon.primitive_count);
        const auto r=invoke(c.device,"DrawPrimitiveUP",0x120,{polygon.primitive_type,polygon.primitive_count,*current,polygon.stride},c.source_call,0x34);
        if(!g.stop_reason()){
            g.set_reg(UC_X86_REG_EBP,saved[0]);g.set_reg(UC_X86_REG_EBX,saved[1]);g.set_reg(UC_X86_REG_ESI,saved[2]);g.set_reg(UC_X86_REG_EDI,saved[3]);
        }
        return r.completed?r.original_hresult:std::nullopt;
    };
    after();X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));SseContext sse(g.mxcsr());
    NinjaPolygonRuntime runtime{[&](std::uint32_t at){return g.u32(at);},bridge.render,textures,texture_runtime,buffer,methods,
        NinjaDepthRuntime{state,device,sse,fp,sse_reciprocal_by_division}};
    runtime.argument_ready=[&](float value){
        if(static_cast<std::int32_t>(*flags)>=0)g.set_u32(frame+8,0);
        return g.set_u32(frame+0x1c,bits(value));
    };
    runtime.vertices_begin=[&](std::uint32_t address,std::uint32_t n){
        if(std::uint64_t(address)+20ULL*n>0xffffffffULL){g.stop("polygon bridge: wrapping vertex buffer at 78ea29");return false;}
        generated_buffer=address;
        g.set_u32(frame-8,0x77e045);g.set_u32(frame-4,*g.u32(frame+0x1c));g.set_u32(frame-12,n);
        for(unsigned k=0;k<4;++k)g.set_u32(frame-16-4*k,saved[k]);
        return true;
    };
    runtime.depth_begin=[&](){
        if(g.u32(0x389b784)!=0x774310U||g.u32(0x389b7a0)!=0x774150U||g.u32(0x389b7b0)!=0x772615U){
            g.stop("polygon bridge: non-SSE D3DX dispatch at 78dc3a");return false;
        }
        return true;
    };
    runtime.vertex_store=[&](std::uint32_t i,std::uint32_t component,std::uint32_t word){
        g.set_reg(UC_X86_REG_ESI,generated_buffer+20*(i+1));g.set_reg(UC_X86_REG_EDI,*points+8*(i+1));
        g.set_reg(UC_X86_REG_EBX,*colors+4*(i+1));g.set_reg(UC_X86_REG_EBP,*count-1-i);
        const auto address=generated_buffer+20*i+4*component;
        if(!g.set_u32(address,word)){g.stop("polygon bridge: missing vertex output at 78ea50/78ea63/78ea66/78ea6c, "+hex(address));return false;}
        if(component==4)g.set_reg(UC_X86_REG_ECX,word);
        if((std::uint64_t(address)<0x3d129c0ULL+64&&std::uint64_t(address)+4>0x3d129c0ULL)
            ||(std::uint64_t(address)<0x389d318ULL+64&&std::uint64_t(address)+4>0x389d318ULL)
            ||(std::uint64_t(address)<0x3d12780ULL+24&&std::uint64_t(address)+4>0x3d12780ULL))reload_depth();
        if(address==0x3d0fdf4||address==0x3d129a0)reload_texture();
        return true;
    };
    runtime.vertices_ready=[&](const NinjaScreenPolygon&){generated=true;return true;};
    const auto r=run_screen_polygon_77dfc0(frame,*count,floating(*original_argument),*flags,runtime);
    if(!g.stop_reason())flush_texture();
    if(!r){if(!g.stop_reason())g.stop("polygon bridge: native boundary "+hex(r.status.source_boundary));return 0;}
    g.set_fpu_status(static_cast<std::uint16_t>(g.fpu_status()|(fp.exception_flags()&0x3fU)));
    g.tail_redirect(0x77e04a);return 0;
}
std::uint32_t screen_initialize(HleProcess& p){
    auto& g=p.guest();const auto entry=g.esp();
    const auto flags=g.u32(entry+4),word=g.u32(entry+8),ratio=g.u32(entry+12);
    if(!word){g.stop("screen bridge: missing argument at 780c30");return 0;}
    const auto original=[&](){g.set_eax(*word);g.tail_redirect(0x780c34);return 0U;};
    const auto overlap=[&](std::uint32_t base,std::uint32_t size){return std::uint64_t(entry)+16>base&&entry>=0x40&&std::uint64_t(entry)-0x40<std::uint64_t(base)+size;};
    std::array<std::uint8_t,256> frame_bytes{};
    if(entry<256||!g.read(entry-256,frame_bytes.data(),frame_bytes.size()))return original();
    if(!flags||!ratio||overlap(0x389d000,0x1000)||overlap(0x3ce7000,0x1000)||overlap(0x3d08000,0x1000)||overlap(0x3d0f000,0x1000)||overlap(0x3d12000,0x1000)||overlap(0x88a000,0x1000))return original();
    if((g.reg(UC_X86_REG_EFLAGS)&0x400U)||g.reg(UC_X86_REG_FPTAG)!=0xffffU||(g.reg(UC_X86_REG_FPCW)&0x3fU)!=0x3fU){
        return original();
    }
    const auto saved_esi=g.reg(UC_X86_REG_ESI),saved_edi=g.reg(UC_X86_REG_EDI);
    RenderBridge bridge(p);NinjaScreenState state{};NinjaDeviceState d{};NinjaScreenDevice methods{};
    GameFrameRendererState renderer{};NinjaSpriteProjection projection{};NinjaCallbackList callbacks{};
    std::optional<float> width;std::optional<std::uint32_t> w840,w9c4,pool,size;
    std::array<std::optional<std::uint32_t>,4> colors{};
    renderer.counter_frequency.low=g.u32(0x3d08538);renderer.counter_frequency.high=g.u32(0x3d0853c);
    state.invalid_at_3d0fd1c=g.u32(0x3d0fd1c);const bool viewport_initially_invalid=state.invalid_at_3d0fd1c==1U;
    for(unsigned k=0;k<6;++k)d.viewport_at_3d12780[k]=g.u32(0x3d12780+4*k);
    for(unsigned k=0;k<16;++k){d.matrix_at_389d318[k]=g.u32(0x389d318+4*k);state.projection_at_3d129c0[k]=g.u32(0x3d129c0+4*k);}
    X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));std::optional<NinjaBlendTables> tables;
    std::map<std::uint32_t,std::uint32_t> written;bool viewport_flushed=false,prefix_flushed=false,tail_flushed=false;unsigned blends=0;
    const auto bits=[](float value){std::uint32_t raw;std::memcpy(&raw,&value,4);return raw;};
    const auto put=[&](std::uint32_t at,const std::optional<std::uint32_t>& value,std::uint32_t site){
        if(!value||g.stop_reason())return;
        const auto i=written.find(at);if(i!=written.end()&&i->second==*value)return;
        if(!g.set_u32(at,*value)){g.stop("screen bridge: missing output "+hex(at)+" at "+hex(site));return;}
        written[at]=*value;
    };
    const auto fput=[&](std::uint32_t at,const std::optional<float>& value,std::uint32_t site){if(value)put(at,bits(*value),site);};
    const auto stack_put=[&](std::uint32_t at,std::uint32_t value,std::uint32_t site){
        if(g.stop_reason())return;
        // Stack words are source writes even if an earlier phase used the same
        // value: a callback may have changed that slot in between.
        if(!g.set_u32(at,value))g.stop("screen bridge: missing stack output "+hex(at)+" at "+hex(site));
    };
    const auto flush_prefix=[&](){
        if(prefix_flushed)return;prefix_flushed=true;
        put(0x3d0f804,state.word_at_3d0f804,0x780c34);put(0x3d0f964,state.flags_at_3d0f964,0x780c43);put(0x3d0f814,state.word_at_3d0f814,0x780c49);
        if(renderer.frame_interval_multiplier)put(0x389d7dc,static_cast<std::uint32_t>(*renderer.frame_interval_multiplier),0x7899b1);
        put(0x3d084a8,renderer.frame_interval.low,0x7899ca);put(0x3d084ac,renderer.frame_interval.high,0x7899cf);
        fput(0x3d0f9e4,width,0x780c5b);put(0x3d0fa20,state.width_at_3d0fa20,0x780c65);
        fput(0x3d0f9e8,state.height_at_3d0f9e8,0x780c8a);put(0x3d0f838,state.height_at_3d0f838,0x780c94);
        fput(0x3d0fa04,state.width_at_3d0fa04,0x780cd0);fput(0x3d0fa08,state.height_at_3d0fa08,0x780ce6);
        fput(0x3d0fa0c,projection.center_x_at_3d0fa0c,0x780cff);fput(0x3d0fa10,projection.center_y_at_3d0fa10,0x780d11);
        put(0x3ce7160,state.word_at_3ce7160,0x780d4c);
        fput(0x3d0f820,state.left_at_3d0f820,0x780d5c);fput(0x3d0f824,state.top_at_3d0f824,0x780d66);
        fput(0x3d0f828,state.right_at_3d0f828,0x780d70);fput(0x3d0f82c,state.bottom_at_3d0f82c,0x780d86);
        if(state.bottom_at_3d0f82c)put(entry+4,bits(*state.bottom_at_3d0f82c),0x780d82);
    };
    const auto flush_viewport=[&](){
        if(viewport_initially_invalid)for(unsigned k=0;k<16;++k)put(0x3d0fd80+4*k,d.matrix_at_3d0fd80[k],0x79130c);
        // The initial reset flag is input only; publish its source clear, not a stale initial value.
        if(viewport_initially_invalid&&state.invalid_at_3d0fd1c==0U)put(0x3d0fd1c,0,0x79130e);
        fput(0x3d0fd20,d.left_at_3d0fd20,0x791326);fput(0x3d0fd24,d.top_at_3d0fd24,0x791342);
        fput(0x3d0fd28,d.right_at_3d0fd28,0x79136d);fput(0x3d0fd2c,d.bottom_at_3d0fd2c,0x791390);
    };
    const auto flush_attributes=[&](){
        if(tail_flushed)return;tail_flushed=true;
        put(0x3d0f840,w840,0x77ff78);put(0x3d0f9c4,w9c4,0x77ff7d);put(0x3ce712c,callbacks.count_at_3ce712c,0x780090);
    };
    const auto flush_polygon=[&](){
        put(0x3ce718c,state.points_at_3ce7184[2],0x780dfb);
        put(0x3ce71b0,pool,0x780e03);put(0x3ce71a4,size,0x780e09);
        for(unsigned k=0;k<4;++k)put(0x3ce7174+4*k,colors[k],0x780e0f+6*k);
        for(const auto& item:std::array<std::pair<unsigned,std::uint32_t>,4>{{{0,0x780e27},{1,0x780e2d},{3,0x780e33},{6,0x780e39}}})put(0x3ce7184+4*item.first,state.points_at_3ce7184[item.first],item.second);
        put(0x3ce7164,state.points_at_3ce7164,0x780e3f);put(0x3ce7168,state.colors_at_3ce7168,0x780e49);put(0x3ce7170,state.count_at_3ce7170,0x780e53);
        put(0x3ce7194,state.points_at_3ce7184[4],0x780e5d);put(0x3ce7198,state.points_at_3ce7184[5],0x780e63);put(0x3ce71a0,state.points_at_3ce7184[7],0x780e68);
    };
    const auto reload=[&](){
        bridge.refresh();
        for(unsigned k=0;k<16;++k){state.projection_at_3d129c0[k]=g.u32(0x3d129c0+4*k);d.matrix_at_3d0fd80[k]=g.u32(0x3d0fd80+4*k);}
        std::array<std::uint8_t,88> bytes{};tables=g.read(0x88ad64,bytes.data(),bytes.size())?decode_ninja_blend_tables(bytes.data(),bytes.size()):std::nullopt;
        bridge.render.blend_tables=tables?&*tables:nullptr;
    };
    std::array<std::uint8_t,88> table_bytes{};tables=g.read(0x88ad64,table_bytes.data(),table_bytes.size())?decode_ninja_blend_tables(table_bytes.data(),table_bytes.size()):std::nullopt;
    bridge.render.blend_tables=tables?&*tables:nullptr;
    const auto before_viewport=[&](){
        flush_prefix();
        if(!viewport_flushed){
            flush_viewport();
            stack_put(entry-8,0x780dcb,0x780dc6);
            stack_put(entry-4,saved_esi,0x780dc5);stack_put(entry-16,saved_esi,0x7912fb);stack_put(entry-20,saved_edi,0x7912fc);
            if(d.viewport_at_3d12780[3]){
                const auto last=*d.viewport_at_3d12780[3]-1U;stack_put(entry-12,last,0x791369);
                const auto flags=((__builtin_popcount(last&255U)&1)?0U:4U)|(last==0?64U:0U)|(last&0x80000000U?128U:0U);
                g.set_reg(UC_X86_REG_EFLAGS,(g.reg(UC_X86_REG_EFLAGS)&~0x8d5U)|flags);
            }
            g.set_reg(UC_X86_REG_ESI,0x389d358);g.set_reg(UC_X86_REG_EDI,0x3d0fdc0);
            if(d.viewport_at_3d12780[2])g.set_edx(*d.viewport_at_3d12780[2]-1U);
            g.set_fpu_status(static_cast<std::uint16_t>((g.fpu_status()&~0x4700U)|(fp.exception_flags()&0x3fU)));
            viewport_flushed=true;
        }
    };
    methods.set_viewport=[&](const D3D8FrameCommand& c,const std::array<std::uint32_t,6>&)->std::optional<std::int32_t>{
        before_viewport();if(g.stop_reason())return std::nullopt;
        const auto r=bridge.invoke_arguments(c.device,"SetViewport",0xa0,{0x3d12780},c.source_call,0x20,true,UC_X86_REG_ECX);
        if(!g.stop_reason())reload();return r.completed?r.original_hresult:std::nullopt;
    };
    methods.set_transform=[&](const D3D8FrameCommand& c,std::uint32_t which,const D3dxMatrix&)->std::optional<std::int32_t>{
        const auto r=bridge.invoke_arguments(c.device,"SetTransform",0x94,{which,0x3d129c0},c.source_call,0x24,true,UC_X86_REG_EDX);
        if(!g.stop_reason())reload();return r.completed?r.original_hresult:std::nullopt;
    };
    methods.multiply_transform=[&](const D3D8FrameCommand& c,std::uint32_t which,const D3dxMatrix&)->std::optional<std::int32_t>{
        const auto r=bridge.invoke_arguments(c.device,"MultiplyTransform",0x9c,{which,0x3d0fd80},c.source_call,0x24,true,UC_X86_REG_ECX);
        if(!g.stop_reason())reload();return r.completed?r.original_hresult:std::nullopt;
    };
    const auto before_blend=[&](){
        if(blends==0){
            if(!viewport_flushed){stack_put(entry-4,saved_esi,0x780dc5);stack_put(entry-8,0x780dcb,0x780dc6);}
            else if(const auto live=g.u32(entry-20))g.set_reg(UC_X86_REG_EDI,*live);
            g.set_reg(UC_X86_REG_ESI,0);
            stack_put(entry-16,0x780dd5,0x780dd0);stack_put(entry-12,0,0x780dcf);stack_put(entry-8,8,0x780dcb);
        }else{stack_put(entry-24,0x780dde,0x780dd9);stack_put(entry-20,1,0x780dd7);stack_put(entry-16,6,0x780dd5);}
        const auto value=blends?6U:8U,sub=value-1U;
        const auto integer_flags=((__builtin_popcount(sub&255U)&1)?0U:4U)|(sub==0?64U:0U)|(sub&0x80000000U?128U:0U)|((value^1U^sub)&0x10U);
        g.set_reg(UC_X86_REG_EFLAGS,(g.reg(UC_X86_REG_EFLAGS)&~0x8d5U)|integer_flags);
    };
    bridge.render.set_render_state=[&](const NinjaRenderStateCommand& c){
        flush_prefix();if(g.stop_reason())return NinjaRenderCallResult{false,{}};
        before_blend();if(g.stop_reason())return NinjaRenderCallResult{false,{}};
        g.set_reg(UC_X86_REG_ECX,c.state);
        const auto r=bridge.invoke_arguments(c.device,"SetRenderState",0xc8,{c.state,c.value},c.source_call,blends++?0x28:0x20,true,UC_X86_REG_EDX);
        if(!g.stop_reason())reload();return r;
    };
    NinjaScreenInitRuntime runtime{NinjaViewportRuntime{state,d,bridge.render,methods,fp},renderer,width,projection,callbacks,w840,w9c4,pool,size,colors};
    runtime.final_inputs_ready=[&](){
        flush_attributes();
        if(g.stop_reason())return false;
        state.width_at_3d0fa04=load_float(g,0x3d0fa04);state.height_at_3d0fa08=load_float(g,0x3d0fa08);
        return true;
    };
    const auto result=run_screen_init_780c30(*flags,*word,*ratio,runtime);
    if(!g.stop_reason()){flush_prefix();if(!viewport_flushed)flush_viewport();flush_attributes();flush_polygon();}
    if(!result){
        if(!g.stop_reason()){
            if(result.source_boundary==0x791385||result.source_boundary==0x791396){
                before_viewport();
                if(result.source_boundary==0x791396){stack_put(entry-24,0x3d12780,0x79138a);stack_put(entry-28,0,0x79138f);g.set_reg(UC_X86_REG_ESP,entry-28);}
                else g.set_reg(UC_X86_REG_ESP,entry-20);
            }else if(result.source_boundary==0x77ecb7||result.source_boundary==0x77ecbe||result.source_boundary==0x77ecc3){
                before_blend();g.set_reg(UC_X86_REG_ESP,entry-(blends?24U:16U));
            }
            const auto fact=result.required_fact?result.required_fact:
                result.failure==NinjaScreenFailure::multiplier_failed?std::optional<std::uint32_t>{!renderer.counter_frequency.high?0x3d0853cU:0x3d08538U}:std::nullopt;
            g.stop("screen bridge: native boundary "+hex(result.source_boundary)+(fact?", required fact "+hex(*fact):""));
        }
        return 0;
    }
    if(g.stop_reason())return 0;
    if(!state.height_at_3d0fa08||!state.width_at_3d0fa04){g.stop("screen bridge: missing final polygon extents at 780dee/780df4");return 0;}
    g.set_eax(bits(*state.height_at_3d0fa08));g.set_reg(UC_X86_REG_ECX,bits(*state.height_at_3d0fa08));g.set_edx(bits(*state.width_at_3d0fa04));
    const auto add_left=entry-28U,add_result=entry-4U;
    const auto add_flags=((__builtin_popcount(add_result&255U)&1)?0U:4U)|(add_result==0?64U:0U)|(add_result&0x80000000U?128U:0U)
        |((add_left^24U^add_result)&0x10U)|(add_result<add_left?1U:0U)|((~(add_left^24U)&(add_left^add_result)&0x80000000U)?0x800U:0U);
    g.set_reg(UC_X86_REG_EFLAGS,(g.reg(UC_X86_REG_EFLAGS)&~0x8d5U)|add_flags);
    g.set_reg(UC_X86_REG_ESP,entry-4);g.tail_redirect(0x780e6e);return 0;
}
std::uint32_t viewport_refresh(HleProcess& p){
    auto& g=p.guest();const auto entry=g.esp(),saved_ecx=g.reg(UC_X86_REG_ECX);
    const auto invalid=g.u32(0x3d0fd1c);
    if(!invalid){g.stop("viewport bridge: unknown reset flag at 7912e1, 3d0fd1c");return 0;}
    if(!g.set_u32(entry-4,saved_ecx)){g.stop("viewport bridge: missing saved ECX at 7912e0");return 0;}
    if(*invalid!=1){g.set_reg(UC_X86_REG_ESP,entry-4);g.tail_redirect(0x7912e1);return 0;}
    // REP direction / an occupied x87 stack require the original instruction state.
    if((g.reg(UC_X86_REG_EFLAGS)&0x400U)||g.reg(UC_X86_REG_FPTAG)!=0xffffU){
        g.set_reg(UC_X86_REG_ESP,entry-4);g.tail_redirect(0x7912e1);return 0;
    }
    if((g.reg(UC_X86_REG_FPCW)&0x3fU)!=0x3fU){g.stop("viewport bridge: unmasked x87 exceptions not supplied at 7912f3");return 0;}
    if(!g.set_u32(entry-8,g.reg(UC_X86_REG_ESI))||!g.set_u32(entry-12,g.reg(UC_X86_REG_EDI))){
        g.stop("viewport bridge: missing saved registers at 7912fb/7912fc");return 0;
    }
    RenderBridge bridge(p);NinjaScreenState state{};NinjaDeviceState device_state{};NinjaScreenDevice methods{};
    state.invalid_at_3d0fd1c=*invalid;
    for(unsigned k=0;k<6;++k)device_state.viewport_at_3d12780[k]=g.u32(0x3d12780+4*k);
    for(unsigned k=0;k<16;++k)device_state.matrix_at_389d318[k]=g.u32(0x389d318+4*k);
    const auto reload=[&](){
        for(unsigned k=0;k<16;++k){state.projection_at_3d129c0[k]=g.u32(0x3d129c0+4*k);device_state.matrix_at_3d0fd80[k]=g.u32(0x3d0fd80+4*k);}
        bridge.refresh();
    };
    for(unsigned k=0;k<16;++k)state.projection_at_3d129c0[k]=g.u32(0x3d129c0+4*k);
    X87Context fp(static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW)));bool flushed=false;
    const auto flush=[&](){
        if(flushed)return;
        flush_words(g,0x3d0fd80,device_state.matrix_at_3d0fd80);
        if(state.invalid_at_3d0fd1c==0U)g.set_u32(0x3d0fd1c,0);
        const std::optional<float> rectangle[]={device_state.left_at_3d0fd20,device_state.top_at_3d0fd24,device_state.right_at_3d0fd28,device_state.bottom_at_3d0fd2c};
        for(unsigned k=0;k<4;++k)if(rectangle[k]){std::uint32_t raw;std::memcpy(&raw,&*rectangle[k],4);g.set_u32(0x3d0fd20+4*k,raw);}
        if(device_state.viewport_at_3d12780[3]){
            const auto last=*device_state.viewport_at_3d12780[3]-1U;
            g.set_u32(entry-4,last); // source reused PUSH ECX's word as its FILD temporary
            const auto flags=((__builtin_popcount(last&255U)&1)?0U:4U)|(last==0?64U:0U)|(last&0x80000000U?128U:0U);
            g.set_reg(UC_X86_REG_EFLAGS,(g.reg(UC_X86_REG_EFLAGS)&~0x8d5U)|flags);
        }
        g.set_fpu_status(static_cast<std::uint16_t>((g.fpu_status()&~0x200U)|(fp.exception_flags()&0x3fU)));
        g.set_reg(UC_X86_REG_ESI,0x389d358);g.set_reg(UC_X86_REG_EDI,0x3d0fdc0);
        if(device_state.viewport_at_3d12780[2])g.set_edx(*device_state.viewport_at_3d12780[2]-1U);
        flushed=true;
    };
    methods.set_viewport=[&](const D3D8FrameCommand& c,const std::array<std::uint32_t,6>&)->std::optional<std::int32_t>{
        flush();const auto r=bridge.invoke_arguments(c.device,"SetViewport",0xa0,{0x3d12780},c.source_call,0x18,true,UC_X86_REG_ECX);
        if(!g.stop_reason())reload();return r.completed?r.original_hresult:std::nullopt;
    };
    methods.set_transform=[&](const D3D8FrameCommand& c,std::uint32_t which,const D3dxMatrix&)->std::optional<std::int32_t>{
        const auto r=bridge.invoke_arguments(c.device,"SetTransform",0x94,{which,0x3d129c0},c.source_call,0x1c,true,UC_X86_REG_EDX);
        if(!g.stop_reason())reload();return r.completed?r.original_hresult:std::nullopt;
    };
    methods.multiply_transform=[&](const D3D8FrameCommand& c,std::uint32_t which,const D3dxMatrix&)->std::optional<std::int32_t>{
        const auto r=bridge.invoke_arguments(c.device,"MultiplyTransform",0x9c,{which,0x3d0fd80},c.source_call,0x1c,true,UC_X86_REG_ECX);
        if(!g.stop_reason())reload();return r.completed?r.original_hresult:std::nullopt;
    };
    NinjaViewportRuntime runtime{state,device_state,bridge.render,methods,fp};
    const auto r=run_viewport_refresh_7912e0(runtime);
    if(!g.stop_reason())flush();
    if(!r){if(!g.stop_reason())g.stop("viewport bridge: native boundary "+hex(r.source_boundary));return 0;}
    g.set_reg(UC_X86_REG_ESP,entry-12);g.tail_redirect(0x7913c8);return 0;
}
std::vector<OverrideEntry> device_overrides(HleProcess& p){
    std::vector<OverrideEntry> out;
    out.push_back({0x780c30,"ninja_screen_initialize",{{0x3d0f804,4},{0x3d0f964,4},{0x3d0f814,4},{0x3d0f9e4,8},{0x3d0fa04,16},{0x3d0f820,16},{0x3ce7160,0x54}},
        [&p](X86Guest&)->std::uint32_t{return screen_initialize(p);},true});
    out.push_back({0x77dfc7,"ninja_screen_polygon",{{0x3d0fdf4,4},{0x3d129a0,4}},
        [&p](X86Guest&)->std::uint32_t{return screen_polygon(p);}});
    out.push_back({0x7912e0,"ninja_viewport_refresh",{{0x3d0fd1c,4},{0x3d0fd20,16},{0x3d0fd80,64}},
        [&p](X86Guest&)->std::uint32_t{return viewport_refresh(p);},true});
    out.push_back({0x403180,"ninja_set_projection_range",{{0x3d0f808,4},{0x3d0f968,4},{0x90082c,8},{0x3aad0c0,8}},
        [&p](X86Guest&)->std::uint32_t{return projection_range(p);}});
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
    for(const auto entry:{0x794ec0U,0x794e40U,0x787260U,0x411af0U,0x411b40U})
        out.push_back({entry,entry==0x794ec0?"ninja_fog_initialize":entry==0x794e40?"ninja_fog_set_table":entry==0x787260?"ninja_fog_generate":entry==0x411af0?"ninja_fog_enable":"ninja_fog_disable",
            {{0x3d12798,4},{0x3d128a4,4},{0x3d128a0,4},{0x3d12898,4},{0x3d0fce0,4},{0x3d12950,4},{0x3d0fac0,4},{0x3d12894,4},{0x3d12a00,512},{0x3d12c00,512}},
            [&p,entry](X86Guest& g)->std::uint32_t{FogBridge bridge(p);return bridge.run(g,entry);}});
    for(const auto entry:{0x407ab0U,0x408630U})out.push_back({entry,entry==0x407ab0?"ninja_queue_reset":"ninja_queue_allocate",
        {{0x3abd954,16},{0x3ab9864,16},{0x3ab9898,12},{0x3ab98ac,4}},
        [&p,entry](X86Guest&)->std::uint32_t{auto bridge=std::make_shared<QueueLifecycleBridge>(p);if(entry==0x407ab0)bridge->reset();else bridge->allocate(bridge);return 0;}});
    return out;
}
std::vector<OverrideEntry> process_overrides(HleProcess& p,const ExecutableImage& image){
    auto out=all_overrides(image);
    for(auto& entry:task_overrides(p))out.push_back(std::move(entry));
    for(auto& entry:heap_overrides(p))out.push_back(std::move(entry));
    for(auto& entry:device_overrides(p))out.push_back(std::move(entry));
    return out;
}

}

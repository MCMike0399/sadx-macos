#include "engine/ninja_screen.hpp"
#include <cstring>
namespace sadx {
namespace {
struct Stop{};
float bits_float(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
std::uint32_t float_bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
// FILD m32 then FADD 2^32 (7dcb7c) when the integer is negative: an unsigned load.
X87Value unsigned_load(X87Context& fp,std::uint32_t v){
    auto x=fp.from_int64(static_cast<std::int32_t>(v));
    if(static_cast<std::int32_t>(v)<0)x=fp.add_float_memory(x,4294967296.0F);
    return x;}
struct Base {
    NinjaScreenResult r;
    [[noreturn]]void fail(NinjaScreenFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(NinjaScreenFailure::unknown_state,at,fact);return *v;}
    // mov eax,[3d128b0] (load) ; mov ecx,[eax] (dereference) ; ... call [ecx+n].
    D3D8FrameCommand device(const NinjaRenderState& render,std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        if(!render.device_at_3d128b0)fail(NinjaScreenFailure::unknown_device,load,0x3d128b0);
        if(!*render.device_at_3d128b0)fail(NinjaScreenFailure::null_device,dereference);
        return {*render.device_at_3d128b0,site};}
    void issued(const std::optional<std::int32_t>& hr,bool present,std::uint32_t site){if(!present||!hr)fail(NinjaScreenFailure::device_unavailable,site);}
    void render_failed(const NinjaRenderFailure f,std::uint32_t at){
        fail(f==NinjaRenderFailure::unknown_device?NinjaScreenFailure::unknown_device:f==NinjaRenderFailure::null_device?NinjaScreenFailure::null_device:NinjaScreenFailure::render_failed,at);}
    void state(NinjaRenderState& render,std::uint32_t s,std::uint32_t v,NinjaRenderSourceSites sites){
        NinjaRenderResult result;if(!ninja_issue_render_state(result,render,s,v,sites))render_failed(result.failure,result.source_boundary);}
    void stage(NinjaRenderState& render,std::uint32_t s,std::uint32_t v,NinjaRenderSourceSites sites){
        NinjaRenderStageResult result;if(!ninja_issue_texture_stage_state(result,render,0,s,v,sites))render_failed(result.failure,result.source_boundary);}
};
struct Viewport:Base {
    NinjaViewportRuntime& rt;explicit Viewport(NinjaViewportRuntime& r_):rt(r_){}
    void run(){
        auto& s=rt.state;auto& d=rt.device_state;auto& fp=rt.fp;
        if(need(s.invalid_at_3d0fd1c,0x7912e1,0x3d0fd1c)!=1)return;
        const auto x=need(d.viewport_at_3d12780[0],0x7912ee,0x3d12780);
        auto left=unsigned_load(fp,x);                                  // FILD before the copy, FADD after it
        for(std::size_t i=0;i<16;++i)d.matrix_at_3d0fd80[i]=need(d.matrix_at_389d318[i],0x79130c,0x389d318+4*static_cast<std::uint32_t>(i));
        s.invalid_at_3d0fd1c=0;
        d.left_at_3d0fd20=fp.to_float(left);
        d.top_at_3d0fd24=fp.to_float(unsigned_load(fp,need(d.viewport_at_3d12780[1],0x791320,0x3d12784)));
        const auto width=need(d.viewport_at_3d12780[2],0x79133c,0x3d12788);
        d.right_at_3d0fd28=fp.to_float(fp.add_float_memory(unsigned_load(fp,width-1U),*d.left_at_3d0fd20));
        const auto height=need(d.viewport_at_3d12780[3],0x791361,0x3d1278c);
        d.bottom_at_3d0fd2c=fp.to_float(fp.add_float_memory(unsigned_load(fp,height-1U),*d.top_at_3d0fd24));
        {std::array<std::uint32_t,6> vp{};for(std::size_t i=0;i<6;++i)vp[i]=need(d.viewport_at_3d12780[i],0x791398,0x3d12780+4*static_cast<std::uint32_t>(i));
            const auto c=device(rt.render,0x791385,0x791396,0x791398);
            issued(rt.device.set_viewport?rt.device.set_viewport(c,vp):std::nullopt,bool(rt.device.set_viewport),0x791398);}
        {D3dxMatrix m{};for(std::size_t i=0;i<16;++i)m[i]=need(s.projection_at_3d129c0[i],0x7913ad,0x3d129c0+4*static_cast<std::uint32_t>(i));
            const auto c=device(rt.render,0x79139e,0x7913a3,0x7913ad);
            issued(rt.device.set_transform?rt.device.set_transform(c,3,m):std::nullopt,bool(rt.device.set_transform),0x7913ad);}
        {D3dxMatrix m{};for(std::size_t i=0;i<16;++i)m[i]=*d.matrix_at_3d0fd80[i];
            const auto c=device(rt.render,0x7913b3,0x7913b8,0x7913c2);
            issued(rt.device.multiply_transform?rt.device.multiply_transform(c,3,m):std::nullopt,bool(rt.device.multiply_transform),0x7913c2);}
    }
};
struct ScreenInit:Base {
    NinjaScreenInitRuntime& rt;explicit ScreenInit(NinjaScreenInitRuntime& r_):rt(r_){}
    void blend(std::uint32_t selector,std::uint32_t value){const auto result=ninja_set_blend_77ec60(selector,value,rt.viewport.render);if(!result)render_failed(result.failure,result.source_boundary);}
    void run(std::uint32_t flags,std::uint32_t word_3d0f804,std::uint32_t word_3d0f814){
        auto& s=rt.viewport.state;auto& fp=rt.viewport.fp;const auto& vp=rt.viewport.device_state.viewport_at_3d12780;
        s.word_at_3d0f804=word_3d0f804;s.flags_at_3d0f964=flags;s.word_at_3d0f814=word_3d0f814;
        if(!set_game_frame_multiplier(static_cast<std::int32_t>(word_3d0f814),rt.renderer))fail(NinjaScreenFailure::multiplier_failed,0x780c4e);
        if(flags&0x10){rt.width_at_3d0f9e4=640.0F;s.width_at_3d0fa20=0x280;}else{rt.width_at_3d0f9e4=320.0F;s.width_at_3d0fa20=0x140;}
        if(flags&0x20){s.height_at_3d0f9e8=480.0F;s.height_at_3d0f838=0x1e0;}else{s.height_at_3d0f9e8=240.0F;s.height_at_3d0f838=0xf0;}
        s.width_at_3d0fa04=fp.to_float(unsigned_load(fp,need(vp[2],0x780cb4,0x3d12788)));
        s.height_at_3d0fa08=fp.to_float(unsigned_load(fp,need(vp[3],0x780cca,0x3d1278c)));
        rt.projection.center_x_at_3d0fa0c=fp.to_float(fp.multiply_float_memory(fp.from_float(*s.width_at_3d0fa04),0.5F));
        rt.projection.center_y_at_3d0fa10=fp.to_float(fp.multiply_float_memory(fp.from_float(*s.height_at_3d0fa08),0.5F));
        if(flags&0x10000){
            const auto w=fp.from_float(*rt.width_at_3d0f9e4);rt.width_at_3d0f9e4=fp.to_float(fp.add(w,w));
            // FLD 240.0 ; FLD 3d0f9e8 ; FUCOMPP ; TEST AH,0x44 ; JP: 480.0 only when equal.
            if(fp.compare_quiet(fp.from_float(*s.height_at_3d0f9e8),fp.from_float(240.0F))==X87Comparison::equal)s.height_at_3d0f9e8=480.0F;
        }
        s.word_at_3ce7160=2;
        const auto right=fp.subtract_float_memory(fp.from_float(*rt.width_at_3d0f9e4),1.0F);
        s.left_at_3d0f820=0.0F;s.top_at_3d0f824=0.0F;
        s.right_at_3d0f828=fp.to_float(right);                          // FST: the register keeps `right`
        const auto bottom=fp.to_float(fp.subtract_float_memory(fp.from_float(*s.height_at_3d0f9e8),1.0F)); // FST [esp+4] then FSTP
        s.bottom_at_3d0f82c=bottom;
        // TEST AH,0x41 ; JP: the store is taken when the screen size is below or equal to its edge.
        const auto edge=[](X87Comparison c){return c==X87Comparison::less||c==X87Comparison::equal;};
        if(edge(fp.compare_float_memory(fp.from_float(*rt.width_at_3d0f9e4),*s.right_at_3d0f828)))s.right_at_3d0f828=fp.to_float(right);
        if(edge(fp.compare_float_memory(fp.from_float(*s.height_at_3d0f9e8),*s.bottom_at_3d0f82c)))s.bottom_at_3d0f82c=bottom;
        {Viewport v(rt.viewport);try{v.run();}catch(Stop){r=v.r;throw;}}
        blend(0,8);blend(1,6);
        rt.word_at_3d0f840=0xffffffffU;rt.word_at_3d0f9c4=0;          // 77ff70(-1, 0)
        rt.callbacks.count_at_3ce712c=0;                                // 780090
        const auto width=float_bits(*s.width_at_3d0fa04),height=float_bits(*s.height_at_3d0fa08);
        rt.pool_at_3ce71b0=0;rt.size_at_3ce71a4=0;
        for(auto& c:rt.clear_colors_at_3ce7174)c=0;
        s.points_at_3ce7184={0u,0u,width,0u,width,height,0u,height};
        s.points_at_3ce7164=0x3ce7184;s.colors_at_3ce7168=0x3ce7174;s.count_at_3ce7170=4;
    }
};
struct Depth:Base {
    NinjaDepthRuntime& rt;explicit Depth(NinjaDepthRuntime& r_):rt(r_){}
    X87Value run(float w){
        auto& fp=rt.fp;
        std::uint32_t z=0x477fff00U;                                    // 65535.0
        if(fp.compare_float_memory(fp.from_float(w),bits_float(0x37800080U))!=X87Comparison::less)
            z=float_bits(fp.to_float(fp.divide_float_memory(fp.from_float(1.0F),w)));
        D3dxViewport vp;std::array<std::uint32_t,6> words{};
        for(std::size_t i=0;i<6;++i)words[i]=need(rt.device_state.viewport_at_3d12780[i],0x78dc3a,0x3d12780+4*static_cast<std::uint32_t>(i));
        vp={words[0],words[1],words[2],words[3],words[4],words[5]};
        D3dxMatrix projection{},world{};
        for(std::size_t i=0;i<16;++i){
            projection[i]=need(rt.state.projection_at_3d129c0[i],0x78dc3a,0x3d129c0+4*static_cast<std::uint32_t>(i));
            world[i]=need(rt.device_state.matrix_at_389d318[i],0x78dc3a,0x389d318+4*static_cast<std::uint32_t>(i));}
        if(!rt.reciprocal)fail(NinjaScreenFailure::sse_failed,0x78dc3a);
        const auto out=d3dx_vec3_project_772615({0,0,z},vp,D3dxMatrixArgument{projection,true},D3dxMatrixArgument{world,false},D3dxMatrixArgument{world,false},rt.sse,fp,rt.reciprocal);
        return fp.from_float(bits_float(out[2]));                       // FLD out.z
    }
};
struct Polygon:Base {
    NinjaPolygonRuntime& rt;std::optional<NinjaScreenPolygon> polygon;explicit Polygon(NinjaPolygonRuntime& r_):rt(r_){}
    std::uint32_t read(std::uint32_t address,std::uint32_t at){const auto v=rt.read?rt.read(address):std::nullopt;if(!v)fail(NinjaScreenFailure::unknown_state,at,address);return *v;}
    // 78bba0
    void release_texture(){
        if(need(rt.textures.global_id_at_3d0fdf4,0x78bba0,0x3d0fdf4)==0xffffffffU)return;
        const auto c=device(rt.render,0x78bba9,0x78bbae,0x78bbb5);
        if(!rt.texture_runtime.set_texture)fail(NinjaScreenFailure::texture_failed,0x78bbb5);
        const auto receipt=rt.texture_runtime.set_texture(NinjaTextureBindCommand{c.device,0,0x78bbb5,nullptr,true,0u,{}});
        if(!receipt.completed)fail(NinjaScreenFailure::texture_failed,0x78bbb5);
        rt.textures.global_id_at_3d0fdf4=0xffffffffU;rt.textures.active_memory_at_3d129a0=std::shared_ptr<const NinjaTextureMemoryReference>{};
    }
    void run(std::uint32_t block,std::uint32_t count,float argument,std::uint32_t flags){
        auto& fp=rt.depth.fp;
        const auto points=read(block,0x77dfc7),colors=read(block+4,0x77dfc9);
        if(static_cast<std::int32_t>(count)<=2)return;                 // 77dfdd: JLE
        // FLD -1.0 ; FDIV arg ; FSTP arg ; FLD 1.0 ; FCOMP arg ; TEST AH,5 ; JP: clamp to 1.0 when 1.0 < -1/arg.
        argument=fp.to_float(fp.divide_float_memory(fp.from_float(-1.0F),argument));
        if(fp.compare_float_memory(fp.from_float(1.0F),argument)==X87Comparison::less)argument=1.0F;
        if(rt.argument_ready&&!rt.argument_ready(argument))fail(NinjaScreenFailure::unknown_state,0x77dff8);
        // 791940 (ECX = flags & 0x40): alpha blending and the stage-0 alpha operation.
        if(flags&0x40){state(rt.render,0x1b,1,{0x791940,0x791947,0x791950});stage(rt.render,4,4,{0x791956,0x79195b,0x791964});}
        else{state(rt.render,0x1b,0,{0x791940,0x791947,0x791970});stage(rt.render,4,3,{0x791976,0x79197b,0x791984});}
        release_texture();
        // 78b780
        state(rt.render,0x16,1,{0x78b780,0x78b785,0x78b78c});state(rt.render,0x89,0,{0x78b792,0x78b797,0x78b7a1});state(rt.render,0x91,1,{0x78b7a7,0x78b7ac,0x78b7b6});
        stage(rt.render,0x10,1,{0x78b7bc,0x78b7c1,0x78b7ca});stage(rt.render,0x11,1,{0x78b7d0,0x78b7d5,0x78b7de});stage(rt.render,0x12,1,{0x78b7e4,0x78b7e9,0x78b7f2});
        // 78ea10 (ECX = the local copy of the block with count as its fourth word)
        const auto buffer=need(rt.vertex_buffer_at_3d128ac,0x78ea29,0x3d128ac);(void)buffer;
        if(rt.vertices_begin&&!rt.vertices_begin(buffer,count))fail(NinjaScreenFailure::unknown_state,0x78ea29,0x3d128ac);
        NinjaScreenPolygon p;p.primitive_count=count-2U;p.vertex_bytes.resize(static_cast<std::size_t>(count)*0x14U);
        for(std::uint32_t i=0;i<count;++i){
            if(rt.depth_begin&&!rt.depth_begin())fail(NinjaScreenFailure::sse_failed,0x78dc3a);
            Depth depth(rt.depth);X87Value z=fp.from_float(0.0F);
            try{z=depth.run(argument);}catch(Stop){r=depth.r;throw;}
            const auto store=[&](unsigned component,std::uint32_t word){
                std::memcpy(p.vertex_bytes.data()+0x14U*i+4*component,&word,4);
                if(rt.vertex_store&&!rt.vertex_store(i,component,word))fail(NinjaScreenFailure::unknown_state,0x78ea69,buffer+0x14U*i+4*component);
            };
            const float x=fp.to_float(fp.add_float_memory(fp.from_float(bits_float(read(points+8*i,0x78ea3e))),0.5F));
            store(0,float_bits(x));
            const auto loaded_y=fp.from_float(bits_float(read(points+8*i+4,0x78ea53)));
            store(3,0x3f800000U);
            const float y=fp.to_float(fp.add_float_memory(loaded_y,0.5F));
            store(1,float_bits(y));store(2,float_bits(fp.to_float(z)));
            store(4,read(colors+4*i,0x78ea69));
        }
        if(rt.vertices_ready&&!rt.vertices_ready(p))fail(NinjaScreenFailure::unknown_state,0x78ea69,0x3d128ac);
        release_texture();
        {const auto c=device(rt.render,0x78ea76,0x78ea7b,0x78ea80);
            issued(rt.device.set_vertex_shader?rt.device.set_vertex_shader(c,0x44):std::nullopt,bool(rt.device.set_vertex_shader),0x78ea80);}
        {(void)need(rt.vertex_buffer_at_3d128ac,0x78ea86,0x3d128ac);const auto c=device(rt.render,0x78ea8c,0x78ea91,0x78eaa1);
            polygon=p;issued(rt.device.draw_primitive_up?rt.device.draw_primitive_up(c,p):std::nullopt,bool(rt.device.draw_primitive_up),0x78eaa1);}
        // 78b800: linear filters again.
        stage(rt.render,0x10,2,{0x78b800,0x78b805,0x78b80e});stage(rt.render,0x11,2,{0x78b814,0x78b819,0x78b822});stage(rt.render,0x12,2,{0x78b828,0x78b82d,0x78b836});
    }
};
}
NinjaScreenResult run_viewport_refresh_7912e0(NinjaViewportRuntime& rt){
    Viewport op(rt);
    try{op.run();}catch(Stop){}catch(const X87Error&){op.r.failure=NinjaScreenFailure::fp_failed;op.r.source_boundary=0x7912f3;}
    return op.r;
}
NinjaScreenResult run_screen_init_780c30(std::uint32_t flags,std::uint32_t word_3d0f804,std::uint32_t word_3d0f814,NinjaScreenInitRuntime& rt){
    ScreenInit op(rt);
    try{op.run(flags,word_3d0f804,word_3d0f814);}catch(Stop){}catch(const X87Error&){op.r.failure=NinjaScreenFailure::fp_failed;op.r.source_boundary=0x780cba;}
    return op.r;
}
NinjaDepthResult ninja_projected_depth_78dbe0(float w,NinjaDepthRuntime& rt){
    Depth op(rt);NinjaDepthResult out;
    try{out.value=op.run(w);}catch(Stop){out.source_boundary=op.r.source_boundary;out.required_fact=op.r.required_fact;}
    catch(const X87Error&){out.source_boundary=0x78dbe0;}catch(const SseError&){out.source_boundary=0x78dc3a;}
    return out;
}
NinjaPolygonResult run_screen_polygon_77dfc0(std::uint32_t block,std::uint32_t count,float argument,std::uint32_t flags,NinjaPolygonRuntime& rt){
    Polygon op(rt);
    try{op.run(block,count,argument,flags);}catch(Stop){}catch(const X87Error&){op.r.failure=NinjaScreenFailure::fp_failed;op.r.source_boundary=0x77dfee;}
    catch(const SseError&){op.r.failure=NinjaScreenFailure::sse_failed;op.r.source_boundary=0x78dc3a;}
    return {op.r,op.polygon};
}
} // namespace sadx

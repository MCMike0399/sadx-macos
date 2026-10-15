#include "guest/override_table.hpp"
#include "engine/game_frame_control.hpp"
#include "engine/ninja_projection_range.hpp"
#include "engine/ninja_queue_consumer.hpp"
#include "engine/ninja_frame_calls.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_view.hpp"
#include "engine/ninja_screen.hpp"
#include "math/d3dx_math.hpp"
#include "game/game_startup.hpp"
#include <unicorn/unicorn.h>
#include <cstring>
#include <memory>
namespace sadx {
namespace {
float as_float(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
std::uint32_t bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
std::uint16_t control_word(X86Guest& g){return static_cast<std::uint16_t>(g.reg(UC_X86_REG_FPCW));}
std::uint32_t stack_argument(X86Guest& g,unsigned i){return g.u32(g.esp()+4+4*i).value_or(0);}
template<class T>std::optional<T> load(X86Guest& g,std::uint32_t at){std::uint32_t v=0;if(!g.read(at,&v,4))return std::nullopt;T t;std::memcpy(&t,&v,4);return t;}
template<class T>void store(X86Guest& g,std::uint32_t at,const std::optional<T>& v){if(!v)return;std::uint32_t w;std::memcpy(&w,&*v,4);g.set_u32(at,w);}
}
std::vector<OverrideEntry> frame_loop_overrides(const ExecutableImage& image){
    std::vector<OverrideEntry> out;
    // 7899a0 (cdecl, multiplier): GameFrameRendererState from 3d08538/3c, 389d7dc, 3d084a8/ac.
    out.push_back({0x7899a0,"set_game_frame_multiplier",{{0x389d7dc,4},{0x3d084a8,8}},[](X86Guest& g)->std::uint32_t{
        GameFrameRendererState s{};
        s.frequency_low_at_3d08538=load<std::uint32_t>(g,0x3d08538);s.frequency_high_at_3d0853c=load<std::uint32_t>(g,0x3d0853c);
        s.frame_multiplier_at_389d7dc=load<std::int32_t>(g,0x389d7dc);s.interval_low_at_3d084a8=load<std::uint32_t>(g,0x3d084a8);s.interval_high_at_3d084ac=load<std::uint32_t>(g,0x3d084ac);
        const auto r=set_game_frame_multiplier(static_cast<std::int32_t>(stack_argument(g,0)),s);
        if(!r){g.stop("override 7899a0: the native port failed");return 0;}
        store(g,0x389d7dc,s.frame_multiplier_at_389d7dc);store(g,0x3d084a8,s.interval_low_at_3d084a8);store(g,0x3d084ac,s.interval_high_at_3d084ac);
        return 0;}});
    // 7815e0 (cdecl, near, far): writes 3d0f808/3d0f968.
    const float negative_one=as_float(image.u32(0x7dcb0c).value_or(0)),limit=as_float(image.u32(0x7dce50).value_or(0));
    out.push_back({0x7815e0,"ninja_clamp_projection_range",{{0x3d0f808,4},{0x3d0f968,4}},[negative_one,limit](X86Guest& g)->std::uint32_t{
        NinjaProjectionRangeState s{};s.negative_one_at_7dcb0c=negative_one;s.limit_at_7dce50=limit;
        s.near_at_3d0f808=load<float>(g,0x3d0f808);s.far_at_3d0f968=load<float>(g,0x3d0f968);
        X87Context fp(control_word(g));
        const auto r=ninja_clamp_projection_range_7815e0(as_float(stack_argument(g,0)),as_float(stack_argument(g,1)),s,fp);
        if(!r){g.stop("override 7815e0: the native port failed at "+std::to_string(r.source_boundary));return 0;}
        store(g,0x3d0f808,s.near_at_3d0f808);store(g,0x3d0f968,s.far_at_3d0f968);
        return 0;}});
    // 7815c0 (cdecl, angle): 3d0f9e0 = 3d0f9e4 / (2 tan(angle/2)) through 787e30 and the 88af78 table.
    auto table=std::make_shared<NinjaMath::SineTable>();
    for(std::size_t i=0;i<table->size();++i)(*table)[i]=as_float(image.u32(0x88af78+4*static_cast<std::uint32_t>(i)).value_or(0));
    out.push_back({0x7815c0,"ninja_set_fov",{{0x3d0f9e0,4}},[table](X86Guest& g)->std::uint32_t{
        NinjaSpriteProjection projection{};const auto width=load<float>(g,0x3d0f9e4);X87Context fp(control_word(g));
        const auto r=ninja_set_fov_7815c0(static_cast<std::int32_t>(stack_argument(g,0)),*table,projection,width,fp);
        if(!r){g.stop("override 7815c0: the native port failed at "+std::to_string(r.source_boundary));return 0;}
        store(g,0x3d0f9e0,projection.scale_at_3d0f9e0);return 0;}});
    // 781570 (thiscall, ECX = 3aad0a0): the camera block.
    out.push_back({0x781570,"ninja_view_init",{{0x3aad0a0,0x40}},[](X86Guest& g)->std::uint32_t{
        if(g.reg(UC_X86_REG_ECX)!=0x3aad0a0){g.stop("override 781570: called with another block than 3aad0a0");return 0;}
        NinjaViewState view;NinjaProjectionRangeState range{};ninja_view_init_781570(view,range);
        for(std::size_t i=0;i<8;++i)g.set_u32(0x3aad0a0+4*static_cast<std::uint32_t>(i),*view.camera_at_3aad0a0[i]);
        g.set_u32(0x3aad0c0,bits(*range.cache_x_at_3aad0c0));g.set_u32(0x3aad0c4,bits(*range.cache_y_at_3aad0c4));
        for(std::size_t i=0;i<6;++i)g.set_u32(0x3aad0c8+4*static_cast<std::uint32_t>(i),*view.camera_tail_at_3aad0c8[i]);
        return 0;}});
    // 402f40 (cdecl, colour): the words are copied one at a time, each read just before its store, as the source
    // does (an input overlapping 3ab9864 reads the words already stored); then the native clamp and 77ff90's copy.
    // Leftovers as the source: EAX = 3ab9864 (77ff90's argument), ECX/EDX = its last two copied words, C3/C2/C0
    // from the last FCOMP (component 3 against 1.0, else against -1.0), C1 clear, the FLD/FCOMP sticky flags.
    const float one=as_float(image.u32(0x7dcb10).value_or(0));
    out.push_back({0x402f40,"ninja_queue_color",{{0x3ab9864,16},{0x3d0f7f0,16}},[one,negative_one](X86Guest& g)->std::uint32_t{
        const auto input=stack_argument(g,0);std::array<float,4> words{};
        static constexpr std::uint32_t reads[]={0x402f44,0x402f52,0x402f61,0x402f6a};
        for(std::uint32_t i=0;i<4;++i){const auto w=g.u32(input+4*i);
            if(!w){g.stop("override 402f40: unreadable colour word at "+std::to_string(reads[i]));return 0;}
            g.set_u32(0x3ab9864+4*i,*w);words[i]=as_float(*w);}
        std::optional<std::array<float,4>> color=words,projection;
        NinjaQueueColorOwners owners{color,projection,one,negative_one};NinjaQueueColorResult partial;
        X87Context fp(control_word(g));std::uint32_t site=0;
        try{if(!ninja_queue_color_402f40(words,owners,partial,fp,site)||!color||!projection){g.stop("override 402f40: the native port failed");return 0;}}
        catch(const X87Error&){g.stop("override 402f40: unmasked x87 exception at "+std::to_string(site));return 0;}
        X87Context last(control_word(g));auto compared=last.compare_float_memory(last.from_float(words[3]),one);
        if(compared!=X87Comparison::greater)compared=last.compare_float_memory(last.from_float(words[3]),negative_one);
        for(std::uint32_t i=0;i<4;++i){g.set_u32(0x3ab9864+4*i,bits((*color)[i]));g.set_u32(0x3d0f7f0+4*i,bits((*projection)[i]));}
        g.set_eax(0x3ab9864);g.set_reg(UC_X86_REG_ECX,bits((*projection)[2]));g.set_reg(UC_X86_REG_EDX,bits((*projection)[3]));
        g.set_fpu_status(static_cast<std::uint16_t>((g.fpu_status()&~0x4700U)|x87_comparison_status(compared)|(fp.exception_flags()&0x3f)));
        return 0;},true,{},true});
    // 78dbe0 (stdcall, w): the depth of (0, 0, w below 7e9b40 or unordered ? 65535 : 1/w) through slot 14
    // (thunk 770295) = 772615 with viewport 3d12780, projection 3d129c0 and 389d318 as view and world, returned
    // in ST(0). Bound only on the SSE dispatch table (slots 3/10/14 = 774310/774150/772615), as for 772615.
    // EAX is 772615's pOut, the source's local at entry ESP - 0xc; ECX/EDX (772615's leftovers) are not modelled.
    out.push_back({0x78dbe0,"ninja_projected_depth",{},[](X86Guest& g)->std::uint32_t{
        if(g.u32(0x389b7b0)!=0x772615U||g.u32(0x389b784)!=0x774310U||g.u32(0x389b7a0)!=0x774150U){
            g.stop("override 78dbe0: D3DX dispatch slots 3/10/14 are not the SSE table (77350c not run, or another CPU path) at 78dc3a");return 0;}
        { // the source pushes at most two x87 values (772615's viewport mapping); a stack fault there is not modelled
            const auto top=(g.fpu_status()>>11)&7U,tag=g.reg(UC_X86_REG_FPTAG);
            for(std::uint32_t k=1;k<=2;++k)if(((tag>>(2*((top-k)&7U)))&3U)!=3U){g.stop("override 78dbe0: x87 stack overflow (fewer than two free registers) is not modelled");return 0;}}
        NinjaScreenState state{};NinjaDeviceState device{};
        for(std::uint32_t i=0;i<6;++i)device.viewport_at_3d12780[i]=g.u32(0x3d12780+4*i);
        for(std::uint32_t i=0;i<16;++i){state.projection_at_3d129c0[i]=g.u32(0x3d129c0+4*i);device.matrix_at_389d318[i]=g.u32(0x389d318+4*i);}
        SseContext sse(g.mxcsr());X87Context fp(control_word(g));
        NinjaDepthRuntime rt{state,device,sse,fp,sse_reciprocal_by_division};
        const auto r=ninja_projected_depth_78dbe0(as_float(stack_argument(g,0)),rt);
        if(!r){g.stop("override 78dbe0: the native port stopped at "+std::to_string(r.source_boundary));return 0;}
        g.set_fpu_status(static_cast<std::uint16_t>(g.fpu_status()|(fp.exception_flags()&0x3f)));
        g.return_x87(r.value->significand(),r.value->sign_exponent());
        g.set_eax(g.esp()-0xc);return 4;},true,{},false,true});
    return out;
}
std::vector<OverrideEntry> startup_overrides(){
    std::vector<OverrideEntry> out;
    // 40f350: fixed input-edge stores, no input reads or nested calls. The comparison
    // covers the whole page so clearing the two trailing row bytes is detected.
    out.push_back({0x40f350,"clear_input_edges",{{0x3b0e000,0x1000}},[](X86Guest& g)->std::uint32_t{
        GameLogicEdgeState edges{};std::optional<std::int32_t> save_phase;
        GameFrameInputState input{edges,{},{},save_phase};GameControllerInitGlobals globals{};
        clear_input_edges_40f350(edges,input,globals);
        std::array<std::uint8_t,0x300> cleared{};
        cleared[0]=*edges.bytes_at_3b0e3e0[0];cleared[1]=*edges.bytes_at_3b0e3e0[1];
        for(std::uint32_t i=2;i<cleared.size();++i)cleared[i]=*edges.raw_at_3b0e3e2[(i-2)/3][(i-2)%3];
        const auto byte=[&](std::uint32_t at,std::uint8_t value,std::uint32_t site){
            if(g.write(at,&value,1))return true;
            g.stop("override 40f350: unavailable output "+std::to_string(at)+" at "+std::to_string(site));return false;};
        for(std::uint32_t i=0;i<cleared.size();i+=3){
            g.set_eax(0x3b0e3e1+i);g.set_reg(UC_X86_REG_ECX,0);
            if(!byte(0x3b0e3e0+i+2,cleared[i+2],0x40f357)||!byte(0x3b0e3e0+i+1,cleared[i+1],0x40f35a)
                ||!byte(0x3b0e3e0+i,cleared[i],0x40f35c))return 0;
        }
        g.set_eax(0x3b0e6e1);
        // Final CMP is equal: CF/AF/SF/OF clear, PF/ZF set; other flags survive.
        g.set_reg(UC_X86_REG_EFLAGS,(g.reg(UC_X86_REG_EFLAGS)&~0x8d5U)|0x44U);
        if(!byte(0x3b0e3ba,*edges.special_at_3b0e3ba,0x40f369)
            ||!byte(0x3b0e3b9,*edges.byte_at_3b0e3b9,0x40f36f)
            ||!byte(0x3b0e3b8,*input.mask_at_3b0e3b8,0x40f375))return 0;
        constexpr std::uint32_t indices[]={2,3,4,0,1},sites[]={0x40f37b,0x40f381,0x40f387,0x40f38d,0x40f393};
        for(std::uint32_t i=0;i<5;++i){
            const auto at=0x3b0e990+4*indices[i],value=*globals.words_at_3b0e990[indices[i]];
            if(!g.write(at,&value,4)){g.stop("override 40f350: unavailable output "+std::to_string(at)+" at "+std::to_string(sites[i]));return 0;}
        }
        return 0;},true,{},true});
    // 789930 (cdecl, function): the first empty slot of the 32 at 3d084b0 takes it; EAX 1, or 0 when all are taken.
    out.push_back({0x789930,"register_exit_callback",{{0x3d084b0,0x80}},[](X86Guest& g)->std::uint32_t{
        ExitCallbackList list;for(std::uint32_t i=0;i<32;++i)list.slots_at_3d084b0[i]=load<std::uint32_t>(g,0x3d084b0+4*i);
        const auto r=register_exit_callback_789930(stack_argument(g,0),list);
        if(!r.returned){g.stop("override 789930: the native port failed at "+std::to_string(r.source_boundary));return 0;}
        for(std::uint32_t i=0;i<32;++i)store(g,0x3d084b0+4*i,list.slots_at_3d084b0[i]);
        g.set_eax(*r.returned);return 0;},true});
    // 40a8a0 (no arguments): the menu block reset.
    out.push_back({0x40a8a0,"reset_menu_block",{{0x900e88,48},{0x9008b4,48},{0x3abdbc1,1},{0x903e58,1}},[](X86Guest& g)->std::uint32_t{
        MenuBlockState m;std::uint8_t a[48],b[48],c=0,d=0;g.read(0x900e88,a,48);g.read(0x9008b4,b,48);g.read(0x3abdbc1,&c,1);g.read(0x903e58,&d,1);
        for(std::size_t i=0;i<48;++i){m.block_at_900e88[i]=a[i];m.block_at_9008b4[i]=b[i];}m.byte_at_3abdbc1=c;m.byte_at_903e58=d;
        if(!reset_menu_block_40a8a0(m)){g.stop("override 40a8a0: the native port failed");return 0;}
        for(std::size_t i=0;i<48;++i){a[i]=*m.block_at_900e88[i];b[i]=*m.block_at_9008b4[i];}c=*m.byte_at_3abdbc1;d=*m.byte_at_903e58;
        g.write(0x900e88,a,48);g.write(0x9008b4,b,48);g.write(0x3abdbc1,&c,1);g.write(0x903e58,&d,1);return 0;}});
    // 77ffc0 (cdecl, callback): slot [3ce712c] of 3d0fa40 = callback, then the count increments.
    out.push_back({0x77ffc0,"ninja_callback_register",{{0x3ce712c,4},{0x3d0fa40,0x100}},[](X86Guest& g)->std::uint32_t{
        NinjaCallbackList list;list.count_at_3ce712c=load<std::uint32_t>(g,0x3ce712c);
        const auto r=ninja_callback_register_77ffc0(list,stack_argument(g,0));
        if(r.failure!=NinjaKeyboardFailure::none){g.stop("override 77ffc0: the native port failed at "+std::to_string(r.source_boundary));return 0;}
        const auto index=*list.count_at_3ce712c-1;store(g,0x3d0fa40+4*index,list.slots_at_3d0fa40[index]);store(g,0x3ce712c,list.count_at_3ce712c);return 0;}});
    // 7959c0 (cdecl, hook): unless already present, the first zero of the 16 slots at 3d08af0 takes it.
    out.push_back({0x7959c0,"ninja_hook_register",{{0x3d08af0,0x40}},[](X86Guest& g)->std::uint32_t{
        NinjaFrameCallState s{};for(std::uint32_t i=0;i<16;++i)s.hooks_at_3d08af0[i]=load<std::uint32_t>(g,0x3d08af0+4*i);
        const auto r=ninja_hook_register_7959c0(s,stack_argument(g,0));
        if(r.failure!=NinjaFrameFailure::none){g.stop("override 7959c0: the native port failed at "+std::to_string(r.source_boundary));return 0;}
        for(std::uint32_t i=0;i<16;++i)store(g,0x3d08af0+4*i,s.hooks_at_3d08af0[i]);return 0;}});
    return out;
}
namespace {
D3dxMatrix matrix_at(X86Guest& g,std::uint32_t at){D3dxMatrix m{};g.read(at,m.data(),64);return m;}
}
std::vector<OverrideEntry> math_overrides(){
    std::vector<OverrideEntry> out;
    const SseReciprocalEstimate estimate=sse_reciprocal_by_division;
    // 774310 (pOut, pM1, pM2): every load before the stores, so pOut may alias.
    out.push_back({0x774310,"d3dx_matrix_multiply",{},[](X86Guest& g)->std::uint32_t{
        const auto o=stack_argument(g,0);SseContext sse(g.mxcsr());
        const auto r=d3dx_matrix_multiply_774310(matrix_at(g,stack_argument(g,1)),matrix_at(g,stack_argument(g,2)),sse);
        g.write(o,r.data(),64);g.set_eax(o);return 12;},true,{{0,64}}});
    // 774150 (pOut, pV, pM): the movaps path when pM is 16-byte aligned.
    out.push_back({0x774150,"d3dx_vec3_transform_coord",{},[estimate](X86Guest& g)->std::uint32_t{
        const auto o=stack_argument(g,0),m=stack_argument(g,2);D3dxVector3 v{};g.read(stack_argument(g,1),v.data(),12);SseContext sse(g.mxcsr());
        const auto r=d3dx_vec3_transform_coord_774150(v,matrix_at(g,m),(m&0xf)==0,sse,estimate);
        g.write(o,r.data(),12);g.set_eax(o);return 12;},true,{{0,12}}});
    // 7757e0 (pOut, pDeterminant, pM).
    out.push_back({0x7757e0,"d3dx_matrix_inverse",{},[estimate](X86Guest& g)->std::uint32_t{
        const auto o=stack_argument(g,0),det=stack_argument(g,1);SseContext sse(g.mxcsr());
        const auto r=d3dx_matrix_inverse_7757e0(matrix_at(g,stack_argument(g,2)),sse,estimate);
        if(!r.matrix){g.set_eax(0);return 12;}
        if(det&&r.determinant)g.set_u32(det,*r.determinant);
        g.write(o,r.matrix->data(),64);g.set_eax(o);return 12;},true,{{0,64},{1,4}}});
    // 7716df (pOut, fovy, aspect, zn, zf); |fovy/2| >= 2^63 is the native port's boundary (the source stores from an empty register).
    out.push_back({0x7716df,"d3dx_perspective",{},[](X86Guest& g)->std::uint32_t{
        const auto o=stack_argument(g,0);X87Context fp(control_word(g));
        const auto r=d3dx_perspective_7716df(as_float(stack_argument(g,1)),as_float(stack_argument(g,2)),as_float(stack_argument(g,3)),as_float(stack_argument(g,4)),fp);
        if(!r){g.stop("override 7716df: |fovy/2| >= 2^63 (FSINCOS C2) is not modelled");return 0;}
        g.set_fpu_status(static_cast<std::uint16_t>(g.fpu_status()|(fp.exception_flags()&0x3f))); // the x87 stores' sticky flags
        g.write(o,r->data(),64);g.set_eax(o);return 20;},true,{{0,64}}});
    // 772615 (pOut, pV, pViewport, pProjection, pView, pWorld): reached through slot 14 (thunk 770295). Its
    // product and transform calls jump through slots 3 (thunk 770804) and 10 (77020b); only the SSE table's
    // 774310/774150 are modelled, so any other slot value stops here instead of guessing that table's arithmetic.
    out.push_back({0x772615,"d3dx_vec3_project",{},[estimate](X86Guest& g)->std::uint32_t{
        if(g.u32(0x389b784)!=0x774310U||g.u32(0x389b7a0)!=0x774150U){
            g.stop("override 772615: D3DX dispatch slots 3/10 are not the SSE table's 774310/774150 (77350c not run, or another CPU path)");return 0;}
        const auto o=stack_argument(g,0);D3dxVector3 v{};
        if(!g.read(stack_argument(g,1),v.data(),12)){g.stop("override 772615: unreadable pV");return 0;}
        bool unreadable=false;
        const auto matrix=[&g,&unreadable](unsigned index)->std::optional<D3dxMatrixArgument>{
            const auto at=stack_argument(g,index);if(!at)return std::nullopt;
            D3dxMatrixArgument m{{},(at&0xf)==0};unreadable=unreadable||!g.read(at,m.matrix.data(),64);return m;};
        const auto projection=matrix(3),view=matrix(4),world=matrix(5);
        if(unreadable){g.stop("override 772615: unreadable matrix argument");return 0;}
        std::optional<D3dxViewport> viewport;
        if(const auto at=stack_argument(g,2)){std::array<std::uint32_t,6> w{};if(!g.read(at,w.data(),24)){g.stop("override 772615: unreadable pViewport");return 0;}viewport=D3dxViewport{w[0],w[1],w[2],w[3],w[4],w[5]};}
        SseContext sse(g.mxcsr());X87Context fp(control_word(g));
        const auto r=d3dx_vec3_project_772615(v,viewport,projection,view,world,sse,fp,estimate);
        g.set_fpu_status(static_cast<std::uint16_t>(g.fpu_status()|(fp.exception_flags()&0x3f))); // the viewport mapping's sticky flags
        g.write(o,r.data(),12);g.set_eax(o);return 24;},true,{{0,12}}});
    return out;
}
std::vector<OverrideEntry> all_overrides(const ExecutableImage& image){
    auto out=frame_loop_overrides(image);for(auto& e:startup_overrides())out.push_back(std::move(e));for(auto& e:math_overrides())out.push_back(std::move(e));return out;
}
bool install_overrides(X86Guest& g,const std::vector<OverrideEntry>& entries,std::string* error){
    for(const auto& e:entries)if(!g.add_override(e.address,e.bridge,error))return false;
    return true;
}
} // namespace sadx

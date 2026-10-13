#include "guest/override_table.hpp"
#include "engine/game_frame_control.hpp"
#include "engine/ninja_projection_range.hpp"
#include "engine/ninja_queue_consumer.hpp"
#include "engine/ninja_frame_calls.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_view.hpp"
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
    return out;
}
std::vector<OverrideEntry> startup_overrides(){
    std::vector<OverrideEntry> out;
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

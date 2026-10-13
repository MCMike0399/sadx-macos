#include "guest/override_table.hpp"
#include "engine/game_frame_control.hpp"
#include "engine/ninja_projection_range.hpp"
#include "engine/ninja_queue_consumer.hpp"
#include "engine/ninja_view.hpp"
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
bool install_overrides(X86Guest& g,const std::vector<OverrideEntry>& entries,std::string* error){
    for(const auto& e:entries)if(!g.add_override(e.address,e.bridge,error))return false;
    return true;
}
} // namespace sadx

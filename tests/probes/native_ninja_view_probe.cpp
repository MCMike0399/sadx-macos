#include "engine/ninja_view.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 781570 / 781250 / 791170 with the layout of tools/oracles/verify_original_ninja_view.py. RCPSS/RCPPS are the
// division model (Unicorn's). The FPTAN/FPATAN/FSINCOS helpers export the Bochs models for the oracle's seeded profile.
// in: [0] CW [1] MXCSR [2] mode (0: 781570 then 781250, 1: 781250 on the given block) [3..10] block words 3aad0a0..bc
//     [11] 3aad0c0 [12] 3aad0c4 [13..18] viewport 3d12780 [19..34] 3d0fd80 [35] matrix-stack slots [36] depth
// table: the 1025-float quarter sine table at 88af78
// out: [0] failure [1] boundary [2] inverse stored [3..18] 3d0f760 [19..34] slot 0 [35..50] 3d0f980 [51] 3d0f9e0
//      [52] 3d0f808 [53] 3d0f968 [54..69] 3d12960 [70..85] 3d129c0 [86..101] 389d398 [102..117] 389d358 [118] depth
//      [119] device calls [120..173] calls (vtable offset, state, 16 words) x3 [174] CW [175..182] block [183..188] tail
//      [189] 3aad0c0 [190] 3aad0c4. Unwritten words read 0xa5a5a5a5.
namespace {
std::uint32_t bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
float fl(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
}
extern "C" int sadx_native_ninja_view(const std::uint32_t* in,const float* table,std::uint32_t* out){
    using namespace sadx;
    std::fill(out,out+200,0xa5a5a5a5U);
    X87Context fp(static_cast<std::uint16_t>(in[0]));SseContext sse(in[1]);
    NinjaViewState view;NinjaProjectionRangeState range{};NinjaSpriteProjection projection;NinjaScreenState screen;NinjaDeviceState device_state;
    for(int i=0;i<8;++i)view.camera_at_3aad0a0[i]=in[3+i];
    range.cache_x_at_3aad0c0=fl(in[11]);range.cache_y_at_3aad0c4=fl(in[12]);
    for(int i=0;i<6;++i)device_state.viewport_at_3d12780[i]=in[13+i];
    for(int i=0;i<16;++i)device_state.matrix_at_3d0fd80[i]=in[19+i];
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
    struct Call{std::uint32_t offset,state;D3dxMatrix m;};std::vector<Call> calls;
    NinjaScreenDevice device;
    device.set_transform=[&](const D3D8FrameCommand&,std::uint32_t s,const D3dxMatrix& m)->std::optional<std::int32_t>{calls.push_back({0x94,s,m});return 0;};
    device.multiply_transform=[&](const D3D8FrameCommand&,std::uint32_t s,const D3dxMatrix& m)->std::optional<std::int32_t>{calls.push_back({0x9c,s,m});return 0;};
    NinjaMath::SineTable sine;std::copy(table,table+1025,sine.begin());
    NinjaMatrix filler;filler.fill(fl(0xa5a5a5a5U));
    NinjaMatrixStack stack(std::vector<NinjaMatrix>(in[35],filler),in[36],0);
    NinjaViewRuntime rt{view,range,projection,screen,device_state,render,device,stack,sine,fp,sse,sse_reciprocal_by_division};
    if(in[2]==0)ninja_view_init_781570(view,range);
    NinjaViewResult r;
    try{r=ninja_set_view_781250(rt);}catch(const X87Error&){return 0;}
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.inverse_stored;
    const auto put=[&](int at,const auto& a){for(std::size_t i=0;i<a.size();++i)if(a[i])out[at+i]=*a[i];};
    put(3,view.view_at_3d0f760);
    for(int i=0;i<16;++i)out[19+i]=bits(stack.slots()[0][i]);
    put(35,view.view_copy_at_3d0f980);
    if(projection.scale_at_3d0f9e0)out[51]=bits(*projection.scale_at_3d0f9e0);
    if(range.near_at_3d0f808)out[52]=bits(*range.near_at_3d0f808);
    if(range.far_at_3d0f968)out[53]=bits(*range.far_at_3d0f968);
    put(54,view.perspective_at_3d12960);put(70,screen.projection_at_3d129c0);put(86,view.view_at_389d398);put(102,view.view_inverse_at_389d358);
    out[118]=stack.depth();out[119]=static_cast<std::uint32_t>(calls.size());
    for(std::size_t k=0;k<calls.size()&&k<3;++k){out[120+18*k]=calls[k].offset;out[121+18*k]=calls[k].state;std::copy(calls[k].m.begin(),calls[k].m.end(),out+122+18*k);}
    out[174]=fp.control_word();
    put(175,view.camera_at_3aad0a0);put(183,view.camera_tail_at_3aad0c8);
    if(range.cache_x_at_3aad0c0)out[189]=bits(*range.cache_x_at_3aad0c0);
    if(range.cache_y_at_3aad0c4)out[190]=bits(*range.cache_y_at_3aad0c4);
    return 1;
}
// Bochs models for the seeded profile. op 0 FPTAN(a), 1 FPATAN(y=a, x=b), 2 FSINCOS(a). Ext80 as (sign/exponent, significand).
// out: [0] ok [1] first se [2] second se ; sig [0] first [1] second (FSINCOS: sine, cosine)
extern "C" int sadx_native_ninja_view_x87(int op,std::uint32_t cw,std::uint32_t ase,std::uint64_t asig,std::uint32_t bse,std::uint64_t bsig,std::uint32_t* out,std::uint64_t* sig){
    using namespace sadx;X87Context fp(static_cast<std::uint16_t>(cw));
    out[0]=out[1]=out[2]=0;sig[0]=sig[1]=0;
    try{
    const auto a=fp.from_canonical_ext80(static_cast<std::uint16_t>(ase),asig);
    if(op==0){const auto t=X87Transcendental::fptan_bochs(fp,a);if(t){out[0]=1;out[1]=t->sign_exponent();sig[0]=t->significand();}}
    else if(op==1){const auto t=X87Transcendental::fpatan_bochs_p5_p6(fp,a,fp.from_canonical_ext80(static_cast<std::uint16_t>(bse),bsig));out[0]=1;out[1]=t.sign_exponent();sig[0]=t.significand();}
    else{const auto t=X87Transcendental::fsincos_bochs(fp,a);if(t){out[0]=1;out[1]=t->sine.sign_exponent();sig[0]=t->sine.significand();out[2]=t->cosine.sign_exponent();sig[1]=t->cosine.significand();}}
    }catch(const X87Error&){return 0;}
    return 1;
}

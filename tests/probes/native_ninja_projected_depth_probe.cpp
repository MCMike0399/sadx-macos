#include "engine/ninja_screen.hpp"
#include <algorithm>
#include <cstring>
// Native 78dbe0 (772615/774310/774150 inside) with the layout of tools/oracles/verify_original_ninja_projected_depth.py.
// in:  [0] CW [1] MXCSR [2] w [3..8] viewport 3d12780 [9..24] projection 3d129c0 [25..40] 389d318
// out: [0] completed [1] source boundary [2..3] ST0 significand (low, high) [4] ST0 sign/exponent [5] x87 exception flags
extern "C" int sadx_native_ninja_projected_depth(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    NinjaScreenState state{};NinjaDeviceState device{};
    for(std::size_t i=0;i<6;++i)device.viewport_at_3d12780[i]=in[3+i];
    for(std::size_t i=0;i<16;++i){state.projection_at_3d129c0[i]=in[9+i];device.matrix_at_389d318[i]=in[25+i];}
    SseContext sse(in[1]);X87Context fp(static_cast<std::uint16_t>(in[0]));
    NinjaDepthRuntime rt{state,device,sse,fp,sse_reciprocal_by_division};
    float w;std::memcpy(&w,&in[2],4);
    const auto r=ninja_projected_depth_78dbe0(w,rt);
    std::fill(out,out+6,0);out[1]=r.source_boundary;
    if(r){out[0]=1;out[2]=static_cast<std::uint32_t>(r.value->significand());out[3]=static_cast<std::uint32_t>(r.value->significand()>>32);out[4]=r.value->sign_exponent();}
    out[5]=fp.exception_flags();
    return 1;
}

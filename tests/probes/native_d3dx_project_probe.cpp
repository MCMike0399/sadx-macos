#include "math/d3dx_math.hpp"
#include <algorithm>
#include <cstring>
// Native D3DX 772615 (+774310, 774150) with the layout of tools/verify_original_d3dx_project.py. RCPPS is the
// division model (Unicorn's).
// in: [0] CW [1] MXCSR [2] present (bit 0 projection, 1 view, 2 world, 3 viewport) [3] aligned (bit 0 projection,
//     1 view, 2 world) [4..6] v [7..12] viewport [13..28] projection [29..44] view [45..60] world
// out: [0..2] the projected vector [3] MXCSR
extern "C" int sadx_native_d3dx_project(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    X87Context fp(static_cast<std::uint16_t>(in[0]));SseContext sse(in[1]);
    const auto matrix=[&](int bit,int at)->std::optional<D3dxMatrixArgument>{
        if(!(in[2]>>bit&1))return std::nullopt;D3dxMatrixArgument m{{},(in[3]>>bit&1)!=0};for(int i=0;i<16;++i)m.matrix[i]=in[at+i];return m;};
    std::optional<D3dxViewport> vp;if(in[2]&8)vp=D3dxViewport{in[7],in[8],in[9],in[10],in[11],in[12]};
    const auto v=d3dx_vec3_project_772615({in[4],in[5],in[6]},vp,matrix(0,13),matrix(1,29),matrix(2,45),sse,fp,sse_reciprocal_by_division);
    std::fill(out,out+8,0);out[0]=v[0];out[1]=v[1];out[2]=v[2];out[3]=sse.mxcsr();
    return 1;
}

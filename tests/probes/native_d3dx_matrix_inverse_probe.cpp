#include "math/d3dx_math.hpp"
#include <algorithm>
// Native D3DX 7757e0 (MatrixInverse, SSE) with the layout of tools/oracles/verify_original_d3dx_matrix_inverse.py.
// RCPSS is the division model (Unicorn's), not a real CPU's estimate.
// in: [0] MXCSR [1..16] matrix (float bits)
// out: [0] 1 when an inverse is returned, 0 for NULL [1] determinant bits (when returned) [2..17] matrix [18] MXCSR
extern "C" int sadx_native_d3dx_matrix_inverse(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    SseContext sse(in[0]);D3dxMatrix m{};std::copy(in+1,in+17,m.begin());
    const auto r=d3dx_matrix_inverse_7757e0(m,sse,sse_reciprocal_by_division);
    std::fill(out,out+19,0);
    if(r.matrix){out[0]=1;out[1]=*r.determinant;std::copy(r.matrix->begin(),r.matrix->end(),out+2);}
    out[18]=sse.mxcsr();
    return 1;
}

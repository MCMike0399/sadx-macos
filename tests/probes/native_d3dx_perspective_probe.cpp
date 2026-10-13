#include "math/d3dx_math.hpp"
#include <algorithm>
#include <cstring>
// Native D3DX 7716df (perspective, x87 + FSINCOS) with the layout of tools/oracles/verify_original_d3dx_perspective.py.
// in: [0] CW [1] fovy [2] aspect [3] near [4] far (float bits)
// out: [0] 1 when a matrix is produced, 0 for the explicit unknown (|fovy/2| >= 2^63) [1..16] matrix [17] x87 exception flags
extern "C" int sadx_native_d3dx_perspective(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    X87Context fp(static_cast<std::uint16_t>(in[0]));
    float f[4];std::memcpy(f,in+1,16);
    std::fill(out,out+18,0);
    const auto m=d3dx_perspective_7716df(f[0],f[1],f[2],f[3],fp);
    if(m){out[0]=1;std::copy(m->begin(),m->end(),out+1);}
    out[17]=fp.exception_flags();
    return 1;
}
// The FSINCOS at 0x771706 on the stored half angle (float bits), for the oracle's seeded-suffix profile.
// out: [0] ok [1] sine sign/exponent [2] cosine sign/exponent; sig: [0] sine [1] cosine significands
extern "C" int sadx_native_d3dx_perspective_sincos(std::uint32_t cw,std::uint32_t half_bits,std::uint32_t* out,std::uint64_t* sig){
    using namespace sadx;
    X87Context fp(static_cast<std::uint16_t>(cw));float half;std::memcpy(&half,&half_bits,4);
    const auto sc=X87Transcendental::fsincos_bochs(fp,fp.from_float(half));
    std::fill(out,out+3,0);sig[0]=sig[1]=0;
    if(!sc)return 1;
    out[0]=1;out[1]=sc->sine.sign_exponent();out[2]=sc->cosine.sign_exponent();sig[0]=sc->sine.significand();sig[1]=sc->cosine.significand();
    return 1;
}

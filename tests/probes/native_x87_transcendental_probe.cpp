#include "cpu/x87_transcendental.hpp"
#include <algorithm>
// The Bochs FSINCOS / FPTAN wrappers on one canonical ext80 operand, for tools/oracles/verify_original_x87_transcendental.py.
// op 0 = FSINCOS, 1 = FPTAN. out: [0] 1 when computed, 0 for the C2 path (|x| >= 2^63, ST(0) unchanged)
// [1] first result sign/exponent [2] second (FSINCOS: sine, cosine; FPTAN: tangent, unused) [3] exception flags
// sig: [0] first [1] second significand. Returns 0 when the operand is not canonical.
extern "C" int sadx_native_x87_transcendental(int op,std::uint16_t cw,std::uint16_t sign_exponent,std::uint64_t significand,
                                              std::uint32_t* out,std::uint64_t* sig){
    using namespace sadx;
    std::fill(out,out+4,0);sig[0]=sig[1]=0;
    X87Context fp(cw);
    std::optional<X87Value> x;
    try{x=fp.from_canonical_ext80(sign_exponent,significand);}catch(const X87Error&){return 0;}
    if(op==0){
        const auto r=X87Transcendental::fsincos_bochs(fp,*x);
        if(r){out[0]=1;out[1]=r->sine.sign_exponent();sig[0]=r->sine.significand();out[2]=r->cosine.sign_exponent();sig[1]=r->cosine.significand();}
    }else{
        const auto r=X87Transcendental::fptan_bochs(fp,*x);
        if(r){out[0]=1;out[1]=r->sign_exponent();sig[0]=r->significand();}
    }
    out[3]=fp.exception_flags();
    return 1;
}

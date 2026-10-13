#include "cpu/crt_acos.hpp"
#include "cpu/crt_inverse_internal.hpp"
#include <cstring>

namespace sadx {
CrtAsinResult crt_acos(X87Value input,const CrtAsinRuntime& runtime,X87Context& fp) {
    const double classified=fp.to_double(input); // FST64; ST0 survives classification.
    std::uint64_t bits;std::memcpy(&bits,&classified,8);
    const auto exponent=bits&0x7ff0000000000000ULL;
    const auto fraction=bits&0x000fffffffffffffULL;
    const auto saved=fp.control_word();
    const bool special=exponent==0x7ff0000000000000ULL;
    if(!special && saved!=0x027f) fp.set_control_word((saved&0x0300U)|0x007fU);
    const CrtInverseFrame frame{CrtMathFunction::acos,runtime,fp,classified,saved,0x645138,0x64519c};
    const auto finish=[&](X87Value value,unsigned code,bool error) {
        return finish_crt_inverse(frame,value,code,error);
    };
    if(special) {
        if(fraction!=0) {
            if((bits&0x0008000000000000ULL)!=0) return finish(input,7,true);
            return finish(fp.add_double_memory(input,1.0),1,true); // helper0x648bbc, literal0x844df0
        }
        return finish(fp.from_canonical_ext80(0xffff,0xc000000000000000ULL),1,true);
    }
    if(exponent<0x3ff0000000000000ULL) {
        const auto plus=fp.add(fp.from_int64(1),input);
        const auto minus=fp.subtract(fp.from_int64(1),input);
        const auto root=fp.sqrt(fp.multiply(plus,minus));
        // The source FXCH reverses FPATAN's operands relative to asin.
        return finish(X87Transcendental::fpatan_bochs_p5_p6(fp,root,input),0,false);
    }
    if(exponent==0x3ff0000000000000ULL && fraction==0) {
        if((bits>>63)==0) return finish(fp.from_int64(0),0,false); // FLDZ
        // Source FLDPI ignores PC and uses RC. Its entry path has already
        // forced RC=nearest (or exact CW0x027f), so this is its exact encoding.
        return finish(fp.from_canonical_ext80(0x4000,0xc90fdaa22168c235ULL),0,false);
    }
    return finish(fp.from_canonical_ext80(0xffff,0xc000000000000000ULL),1,true);
}
} // namespace sadx

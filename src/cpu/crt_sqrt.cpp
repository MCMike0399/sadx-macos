#include "cpu/crt_sqrt.hpp"
#include "cpu/crt_inverse_internal.hpp"
#include "cpu/crt_argument.hpp"
#include <cstring>
namespace sadx {
namespace {
std::uint64_t bits(double d){std::uint64_t u;std::memcpy(&u,&d,8);return u;}
CrtAsinResult body(X87Value input,double classified,const CrtAsinRuntime& runtime,X87Context& fp){
    const auto u=bits(classified);const auto exponent=u&0x7ff0000000000000ULL;
    const auto fraction=u&0xfffffffffffffULL;const auto saved=fp.control_word();
    const bool special=exponent==0x7ff0000000000000ULL;
    if(!special&&saved!=0x027f)fp.set_control_word((saved&0x0300U)|0x007fU);
    const CrtInverseFrame frame{CrtMathFunction::sqrt,runtime,fp,classified,saved,0x6452ce,0x64532b};
    if(special&&fraction){
        if(fraction&0x8000000000000ULL)return finish_crt_inverse(frame,input,7,true);
        return finish_crt_inverse(frame,fp.add_double_memory(input,1.0),1,true);
    }
    if((u>>63)&&((u&0x7fffffffffffffffULL)!=0))
        return finish_crt_inverse(frame,fp.from_canonical_ext80(0xffff,0xc000000000000000ULL),1,true);
    // Positive infinity and negative zero bypass FSQRT in the source.
    if(special || u==0x8000000000000000ULL)return finish_crt_inverse(frame,input,0,false);
    return finish_crt_inverse(frame,fp.sqrt(input),0,false);
}
}
CrtAsinResult crt_sqrt(double input,const CrtAsinRuntime& runtime,X87Context& fp){
    return body(load_crt_binary64_argument(input,fp),input,runtime,fp);
}
CrtAsinResult crt_sqrt_extended(X87Value input,const CrtAsinRuntime& runtime,X87Context& fp){
    return body(input,fp.to_double(input),runtime,fp);
}
} // namespace sadx

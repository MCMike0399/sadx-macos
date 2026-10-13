#include "cpu/crt_asin.hpp"
#include "cpu/crt_inverse_internal.hpp"
#include "cpu/crt_argument.hpp"
#include <cstring>

namespace sadx {
namespace {
std::uint64_t bits(double value) {
    std::uint64_t result;
    std::memcpy(&result,&value,8);
    return result;
}
}

CrtAsinResult finish_crt_inverse(const CrtInverseFrame& frame,X87Value value,
                               unsigned error_code,bool unconditional_error) {
    const auto& runtime=frame.runtime;auto& fp=frame.fp;
    const auto saved=frame.saved_control;const auto classified=frame.classified;
    auto dispatch=[&](unsigned code,X87Value value)->CrtAsinResult {
        if (!runtime.matherr_handler) {
            // Source helper first FSTP64's the result, before dispatch.
            const double stored=fp.to_double(value);
            return {CrtAsinFailure::missing_matherr_handler,{},0x648b6b,stored};
        }
        const double stored=fp.to_double(value);
        const auto handled=runtime.matherr_handler(frame.function,code,classified,stored,saved,fp);
        if (handled.failure!=CrtAsinFailure::none || !handled.stored_result)
            return {handled.failure==CrtAsinFailure::none ? CrtAsinFailure::matherr_unavailable : handled.failure,
                    {},0x648b6b,stored};
        // FLD64 occurs before the source restores the incoming control word.
        const auto returned=fp.from_double(*handled.stored_result);
        if(saved!=0x027f) fp.set_control_word(saved);
        return {CrtAsinFailure::none,returned,0,{}};
    };

    auto finish=[&](X87Value value,unsigned error_code,bool unconditional_error)->CrtAsinResult {
        if (!runtime.matherr_control)
            return {CrtAsinFailure::unknown_matherr_policy,value,
                    unconditional_error ? frame.error_policy_boundary : frame.ordinary_policy_boundary,{}};
        if (*runtime.matherr_control!=0) {
            if(saved!=0x027f) fp.set_control_word(saved);
            return {CrtAsinFailure::none,value,0,{}};
        }
        if (unconditional_error) return dispatch(error_code,value);
        if(saved!=0x027f && (saved & 0x20U)!=0 && (fp.exception_flags() & 0x20U)!=0)
            return dispatch(8,value); // source precision-loss dispatcher path
        if(saved!=0x027f) fp.set_control_word(saved);
        return {CrtAsinFailure::none,value,0,{}};
    };

    return finish(value,error_code,unconditional_error);
}

namespace {
CrtAsinResult asin_body(X87Value input,double classified,const CrtAsinRuntime& runtime,X87Context& fp) {
    const auto classification=bits(classified);
    const auto exponent=classification & 0x7ff0000000000000ULL;
    const auto fraction=classification & 0x000fffffffffffffULL;
    const auto saved=fp.control_word();
    const bool special=exponent==0x7ff0000000000000ULL;
    // The nonfinite entry branch bypasses the internal-CW setup.
    if (!special && saved!=0x027f) fp.set_control_word((saved & 0x0300U)|0x007fU);

    const CrtInverseFrame frame{CrtMathFunction::asin,runtime,fp,classified,saved,0x645066,0x6450cc};
    const auto finish=[&](X87Value value,unsigned code,bool always) {
        return finish_crt_inverse(frame,value,code,always);
    };

    if(special) {
        if(fraction!=0) {
            // F64 quiet bit determines source helper 0x648bbc's branch.
            if((classification & 0x0008000000000000ULL)!=0) return finish(input,7,true);
            // Signaling operands raise invalid through arithmetic. The exact
            // nonfinite dispatcher, rather than a host NaN default, owns the result.
            const auto quieted=fp.add_double_memory(input,1); // source0x844df0 is double1.0
            return finish(quieted,1,true);
        }
        const auto indefinite=fp.from_canonical_ext80(0xffff,0xc000000000000000ULL);
        return finish(indefinite,1,true);
    }
    if(exponent<0x3ff0000000000000ULL) {
        // Original order: (1+x)*(1-x), sqrt, FPATAN(x,root).
        const auto plus=fp.add(fp.from_int64(1),input);
        const auto minus=fp.subtract(fp.from_int64(1),input);
        const auto root=fp.sqrt(fp.multiply(plus,minus));
        const auto result=X87Transcendental::fpatan_bochs_p5_p6(fp,input,root);
        return finish(result,0,false);
    }
    if(exponent==0x3ff0000000000000ULL && fraction==0) {
        auto result=fp.from_canonical_ext80(0x3fff,0xc90fdaa22168c235ULL);
        if((classification>>63)!=0) result=result.negated();
        return finish(result,0,false);
    }
    const auto indefinite=fp.from_canonical_ext80(0xffff,0xc000000000000000ULL);
    return finish(indefinite,1,true);
}
} // namespace
CrtAsinResult crt_asin(X87Value input,const CrtAsinRuntime& runtime,X87Context& fp){
    return asin_body(input,fp.to_double(input),runtime,fp); // source645020 FST64
}
CrtAsinResult crt_asin_binary64(double input,const CrtAsinRuntime& runtime,X87Context& fp){
    return asin_body(load_crt_binary64_argument(input,fp),input,runtime,fp); // source645034
}
} // namespace sadx

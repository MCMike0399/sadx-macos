#include "cpu/crt_atan2.hpp"
#include <cstring>

namespace sadx {
namespace {
std::uint64_t bits(double value) {
    std::uint64_t word; std::memcpy(&word,&value,sizeof(word)); return word;
}
X87Value absolute(X87Value value) { return value.negative()?value.negated():value; }

// Source64b556 is distinct from648bd5: SHLD builds the high significand
// word, while its low word copies the original low32 without shifting.
X87Value load_argument(double value,X87Context& fp) {
    const auto word=bits(value);
    if((word&0x7ff0000000000000ULL)!=0x7ff0000000000000ULL)
        return fp.from_double(value);
    const auto high=static_cast<std::uint32_t>(word>>32);
    const auto low=static_cast<std::uint32_t>(word);
    const auto significand=(std::uint64_t((high<<11)|(low>>21))<<32)|low;
    return fp.from_canonical_ext80(static_cast<std::uint16_t>((high>>16)|0x7fffU),significand);
}
unsigned argument_class(double value) {
    const auto magnitude=bits(value)&0x7fffffffffffffffULL;
    if(magnitude==0)return 1;
    if(magnitude>0x7ff0000000000000ULL)return 2;
    if(magnitude==0x7ff0000000000000ULL)return 3;
    // Every nonzero finite binary64 (including subnormal) loads as a normal
    // ext80. This is the actual FXAM/classifier table route, not host fpclassify.
    return 0;
}
bool quiet(X87Value value) { return (value.significand()&0x4000000000000000ULL)!=0; }

CrtAtan2Result finish_return(X87Value value,std::uint16_t saved,
                            std::optional<double> scratch,X87Context& fp) {
    fp.set_control_word(saved); // source64b43d FLDCW, then64b443 FWAIT
    if((fp.exception_flags()&~saved&0x3fU)!=0)
        return {CrtAtan2Failure::unmasked_return_exception,{},0x64b443,{},{},scratch};
    return {CrtAtan2Failure::none,value,0,{},{},scratch};
}
}

CrtAtan2Result crt_atan2_binary64(double argument_y,double argument_x,
        const CrtAtan2Runtime& runtime,X87Context& fp,CrtAtan2Trace* trace) {
    if(trace)trace->result_after_fpatan.reset();
    // Source first loads both arguments, then saves CW (64b3cb).
    const auto y=load_argument(argument_y,fp);
    const auto x=load_argument(argument_x,fp);
    const auto saved=fp.control_word();
    fp.set_control_word(0x133f); // descriptor operation16:6489de

    // Source16-way table at2bcc180, addressed by4*class(y)+class(x).
    constexpr std::uint32_t targets[16]={
        0x64b29c,0x64b318,0x648a9f,0x64b2f5,
        0x64b2f5,0x64b2f5,0x648a9f,0x64b2f5,
        0x648a9d,0x648a9d,0x648ac7,0x648a9d,
        0x64b318,0x64b318,0x648a9f,0x648b06};
    const auto target=targets[argument_class(argument_y)*4+argument_class(argument_x)];
    auto result=fp.from_canonical_ext80(0,0);
    int error_code=0;
    switch(target) {
    case 0x64b29c:
        error_code=-2;
        result=X87Transcendental::fpatan_bochs_p5_p6(fp,absolute(y),absolute(x));
        if(trace)trace->result_after_fpatan=result;
        if(x.negative())result=fp.subtract(fp.from_canonical_ext80(0x4000,0xc90fdaa22168c235ULL),result);
        if(y.negative())result=result.negated();
        break;
    case 0x64b318:
        result=fp.from_canonical_ext80(0x3fff,0xc90fdaa22168c235ULL);
        if(y.negative())result=result.negated();
        break;
    case 0x64b2f5:
        if(x.negative())result=fp.from_canonical_ext80(0x4000,0xc90fdaa22168c235ULL);
        if(y.negative())result=result.negated();
        break;
    case 0x648a9f:
        error_code=quiet(x)?7:1;
        result=fp.add(y,x); // FADDP after checking ST0=x
        break;
    case 0x648a9d:
        error_code=quiet(y)?7:1;
        result=fp.add(x,y); // FXCH, then FADDP
        break;
    case 0x648ac7:
        if(quiet(x)) {
            error_code=quiet(y)?7:1;
            result=fp.add(x,y); // source swaps only after first QNaN test
        } else {
            error_code=1;result=fp.add(y,x);
        }
        break;
    case 0x648b06:
        error_code=1;
        result=fp.from_canonical_ext80(0xffff,0xc000000000000000ULL);
        break;
    }

    if(!runtime.matherr_control)
        return {CrtAtan2Failure::unknown_matherr_policy,result,0x64b3e5,{},
                error_code>0?std::optional<unsigned>(error_code):std::nullopt,{}};
    if(*runtime.matherr_control!=0)return finish_return(result,saved,{},fp);

    // Source64b3ee FST64 does not pop or reload its extended input.
    const double scratch=fp.to_double(result);
    if(error_code==-2) {
        const auto exponent=bits(scratch)&0x7ff0000000000000ULL;
        if(exponent==0) {
            error_code=4;
            //64b480 FSCALE by+1536. For finite atan2 of binary64 inputs,
            // this is an exact normal ext80 power-of-two product at PC64;
            // it is not an implementation of arbitrary FSCALE operands.
            result=fp.multiply(result,fp.from_canonical_ext80(0x45ff,0x8000000000000000ULL));
            if(fp.compare_double_memory(absolute(result),0x1p-1022)==X87Comparison::less)
                result=fp.multiply_double_memory(result,0.0); //844fa8
        } else if(exponent==0x7ff0000000000000ULL) {
            error_code=3;
            result=fp.multiply(result,fp.from_canonical_ext80(0x39ff,0x8000000000000000ULL));
            if(fp.compare_double_memory(absolute(result),0x1.fffffffffffffp1023)==X87Comparison::greater)
                result=fp.multiply_double_memory(result,
                    []{std::uint64_t u=0x7ff0000000000000ULL;double d;std::memcpy(&d,&u,8);return d;}());
        } else error_code=0;
    }
    if(error_code==0 && (saved&0x20U)==0 && (fp.exception_flags()&0x20U)!=0)
        error_code=8; // actual64b418 tests an UNMASKED incoming precision bit
    if(error_code==0)return finish_return(result,saved,scratch,fp);

    // Source copies both original arguments, then FSTP64 before64b518 call.
    const double stored=fp.to_double(result);
    if(!runtime.matherr_handler)
        return {CrtAtan2Failure::missing_matherr_handler,{},0x64b518,stored,
                static_cast<unsigned>(error_code),scratch};
    const CrtAtan2MathErrorRequest request{0x2bcc170,16,static_cast<unsigned>(error_code),
                                         argument_y,argument_x,stored,saved,scratch};
    const auto handled=runtime.matherr_handler(request,fp);
    if(handled.failure!=CrtAtan2Failure::none || !handled.stored_result)
        return {handled.failure==CrtAtan2Failure::none?CrtAtan2Failure::matherr_unavailable:handled.failure,
                {},0x64b518,stored,static_cast<unsigned>(error_code),scratch};
    //64b522 reloads the returned slot under the callback's actual current CW.
    return finish_return(fp.from_double(*handled.stored_result),saved,scratch,fp);
}
} // namespace sadx

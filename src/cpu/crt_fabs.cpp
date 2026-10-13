#include "cpu/crt_fabs.hpp"
#include <cstring>
namespace sadx {
namespace {
std::uint64_t bits(double d){std::uint64_t u;std::memcpy(&u,&d,8);return u;}
double number(std::uint64_t u){double d;std::memcpy(&d,&u,8);return d;}
}
CrtFabsResult crt_fabs(double input,CrtFabsRuntime& runtime,X87Context& fp){
    const auto u=bits(input);const auto saved=fp.control_word();fp.set_control_word(0x133f);
    if((u&0x7ff0000000000000ULL)!=0x7ff0000000000000ULL){
        fp.set_control_word(saved);return {CrtFabsFailure::none,fp.from_double(number(u&0x7fffffffffffffffULL)),0,{}};
    }
    const double stored=fp.to_double(fp.from_double(input)); // source FLD64/FST64 classification
    if((bits(stored)&0xfffffffffffffULL)==0){
        fp.set_control_word(saved);auto value=fp.from_double(input);
        if(u>>63)value=value.negated();return {CrtFabsFailure::none,value,0,{}};
    }
    // Source qNaN path stores its original argument again before64af93.
    const double argument=fp.to_double(fp.from_double(input));
    if(!runtime.nan_policy)return {CrtFabsFailure::unknown_nan_policy,{},0x64af96,argument};
    if(*runtime.nan_policy!=0){
        runtime.errno_value=0x21;fp.set_control_word(saved);
        return {CrtFabsFailure::none,fp.from_double(argument),0,{}};
    }
    // 64af93 performs FLD/FST for result, FLDZ/FST, FLD/FST argument.
    (void)fp.to_double(fp.from_double(argument));
    (void)fp.to_double(fp.from_int64(0));
    (void)fp.to_double(fp.from_double(argument));
    if(!runtime.nan_handler)return {CrtFabsFailure::missing_nan_handler,{},0x64afbd,argument};
    auto reply=runtime.nan_handler(argument,saved,fp);
    if(reply.failure!=CrtFabsFailure::none)return reply;
    if(!reply.value)return {CrtFabsFailure::unavailable_result,{},0x64afbd,argument};
    return reply;
}
} // namespace sadx

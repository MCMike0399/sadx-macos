#include "cpu/crt_floor.hpp"
#include <cstring>
namespace sadx {
namespace {
std::uint64_t bits(double x) { std::uint64_t u;std::memcpy(&u,&x,8);return u; }
double number(std::uint64_t u) { double x;std::memcpy(&x,&u,8);return x; }
CrtFloorResult failed(CrtFloorFailure f,std::uint32_t site,bool sse,
    std::optional<double> pending={}) { return {f,{},site,pending,sse}; }
CrtFloorResult dispatch(CrtFloorErrorRoute route,double input,double pending,
    std::uint16_t saved,const CrtFloorRuntime& runtime,X87Context& fp,
    std::uint32_t site,bool sse) {
    if(!runtime.error_handler) return failed(CrtFloorFailure::missing_error_handler,site,sse,pending);
    const auto reply=runtime.error_handler(route,input,pending,saved,fp,runtime.sse);
    if(reply.failure!=CrtFloorFailure::none) return failed(reply.failure,site,sse,pending);
    if(!reply.value) return failed(CrtFloorFailure::unavailable_error_result,site,sse,pending);
    return {CrtFloorFailure::none,reply.value,0,pending,sse};
}
}
std::optional<CrtFloorConstants> decode_crt_floor_constants(const std::uint8_t* data,std::size_t size) {
    if(!data||size!=80) return {};
    std::uint64_t values[10];std::memcpy(values,data,80);
    return CrtFloorConstants{values[0],values[2],values[4],values[6],values[8]};
}
CrtFloorResult crt_floor(double input,const CrtFloorConstants& c,
    const CrtFloorRuntime& runtime,X87Context& fp) {
    if(!runtime.cpu_sse2) return failed(CrtFloorFailure::unknown_cpu_policy,0x6449f0,false);
    bool fast=false;
    if(*runtime.cpu_sse2!=0) {
        if(!runtime.sse) return failed(CrtFloorFailure::missing_sse_context,0x644a00,false);
        fast=(runtime.sse->mxcsr()&0x1f80U)==0x1f80U && (fp.control_word()&0x7fU)==0x7fU;
    }
    const auto u=bits(input);
    if(fast) {
        auto& sse=*runtime.sse;
        const auto exponent=u>>52;
        // PSUBD then PSRLQ/PSLLQ: an unsigned count>=64 zeros the lane.
        const std::uint32_t count=static_cast<std::uint32_t>(c.shift_bias)-
            static_cast<std::uint32_t>(exponent&c.exponent_mask);
        const auto truncated=count>=64?0ULL:(u>>count)<<count;
        if((exponent&0x800U)==0) {
            if(exponent<0x3ffU) return {CrtFloorFailure::none,fp.from_int64(0),0,{},true};
            if(exponent<=0x432U) return {CrtFloorFailure::none,fp.from_double(number(truncated)),0,{},true};
        } else {
            const bool below=sse.less_than(Sse64{u},Sse64{truncated}); // source executes before large-value branch
            if(exponent<0xbffU) {
                const bool negative=sse.less_than(Sse64{u},Sse64{c.negative_zero});
                const auto out=((negative?~std::uint64_t{0}:0)|c.negative_zero)&c.negative_one_mask;
                return {CrtFloorFailure::none,fp.from_double(number(out)),0,{},true};
            }
            if(exponent<=0xc32U) {
                const auto out=sse.subtract_double(Sse64{truncated},Sse64{below?c.one:0});
                return {CrtFloorFailure::none,fp.from_double(number(out.bits)),0,{},true};
            }
        }
        if(sse.ucomisd(Sse64{u},Sse64{u})==SseComparison::unordered)
            return dispatch(CrtFloorErrorRoute::sse_nan,input,input,fp.control_word(),runtime,fp,0x644aa5,true);
        return {CrtFloorFailure::none,fp.from_double(input),0,{},true};
    }
    if(!runtime.fallback_control_word) return failed(CrtFloorFailure::unknown_fallback_control_word,0x649187,false);
    const auto saved=fp.control_word();
    fp.set_control_word(*runtime.fallback_control_word); // source64b1fd mask0xffff rawCW
    const double loaded=fp.to_double(fp.from_double(input)); // FLD/FST64 before classification/helper
    if((u&0x7ff0000000000000ULL)==0x7ff0000000000000ULL) {
        if((bits(loaded)&0xfffffffffffffULL)!=0) {
            const double pending=fp.to_double(fp.from_double(input)); // source call argument store
            return dispatch(CrtFloorErrorRoute::x87_nan,input,pending,saved,runtime,fp,0x6491cb,false);
        }
        fp.set_control_word(saved);
        return {CrtFloorFailure::none,fp.from_double(input),0,{},false};
    }
    // 64dbc9 loads the stored binary64 argument, FRNDINT, FST64, FLD64.
    const double rounded=fp.to_double(fp.round_to_integer(fp.from_double(loaded)));
    const double stored=fp.to_double(fp.from_double(rounded)); // caller FST64
    const auto comparison=fp.compare_double_memory(fp.from_double(stored),input);
    if(comparison!=X87Comparison::equal && !(saved&0x20U)) {
        const double pending=fp.to_double(fp.from_double(stored));
        (void)fp.to_double(fp.from_double(input)); // source input argument store before dispatcher
        return dispatch(CrtFloorErrorRoute::x87_precision,input,pending,saved,runtime,fp,0x64923f,false);
    }
    fp.set_control_word(saved); // source restores before final FLD64
    return {CrtFloorFailure::none,fp.from_double(stored),0,{},false};
}
} // namespace sadx

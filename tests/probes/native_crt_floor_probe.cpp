#include "cpu/crt_floor.hpp"
#include <cstring>
extern "C" int sadx_native_crt_floor(const std::uint8_t* bytes,std::size_t size,
    std::uint64_t input,std::uint16_t cw,std::uint32_t mxcsr,std::uint32_t cpu,
    std::uint16_t fallback,std::uint32_t* out,std::size_t count) {
    if(!out||count!=12)return 1;
    const auto c=sadx::decode_crt_floor_constants(bytes,size);if(!c)return 2;
    try {
        sadx::X87Context fp(cw);sadx::SseContext sse(mxcsr);double x;std::memcpy(&x,&input,8);
        const sadx::CrtFloorRuntime runtime{cpu,&sse,fallback,{}};
        const auto r=sadx::crt_floor(x,*c,runtime,fp);
        std::memset(out,0,48);out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;
        out[2]=fp.control_word();out[3]=fp.exception_flags();out[4]=sse.mxcsr();
        if(r.value){out[5]=r.value->significand();out[6]=r.value->significand()>>32;out[7]=r.value->sign_exponent();}
        out[8]=r.used_sse;out[9]=r.pending_stored_result.has_value();
        if(r.pending_stored_result){std::uint64_t b;std::memcpy(&b,&*r.pending_stored_result,8);out[10]=b;out[11]=b>>32;}
        return 0;
    }catch(const sadx::X87Error&){return 3;}catch(const sadx::SseError&){return 4;}
}

extern "C" int sadx_native_round_to_integer(std::uint64_t input,std::uint16_t cw,std::uint32_t* out) {
    if(!out)return 1;
    try {sadx::X87Context fp(cw);double x;std::memcpy(&x,&input,8);
        auto v=fp.round_to_integer(fp.from_double(x));out[0]=v.significand();out[1]=v.significand()>>32;
        out[2]=v.sign_exponent();out[3]=fp.exception_flags();return 0;
    }catch(const sadx::X87Error&){return 2;}
}

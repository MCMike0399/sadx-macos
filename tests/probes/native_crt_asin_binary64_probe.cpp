#include "cpu/crt_asin.hpp"
#include <cstring>

extern "C" int sadx_native_crt_asin_binary64(std::uint64_t word,std::uint16_t cw,
        std::uint32_t policy,std::uint32_t* out) {
    if (!out) return 1;
    std::memset(out,0,48);
    try {
        sadx::X87Context fp(cw);double input;std::memcpy(&input,&word,8);
        const sadx::CrtAsinRuntime runtime{policy,{}};
        const auto result=sadx::crt_asin_binary64(input,runtime,fp);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;
        out[2]=fp.control_word();out[3]=fp.exception_flags();
        if (result.value) {
            out[4]=result.value->significand();out[5]=result.value->significand()>>32;
            out[6]=result.value->sign_exponent();out[7]=1;
        }
        if (result.pending_stored_result) {
            std::uint64_t raw;std::memcpy(&raw,&*result.pending_stored_result,8);
            out[8]=raw;out[9]=raw>>32;out[10]=1;
        }
        return 0;
    } catch (const sadx::X87Error&) { return 2; }
}

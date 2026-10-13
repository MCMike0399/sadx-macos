#include "cpu/crt_atan2.hpp"
#include <cstring>

extern "C" int sadx_native_crt_atan2(std::uint64_t yword,std::uint64_t xword,
        std::uint16_t cw,std::uint32_t policy,std::uint32_t* out) {
    if(!out)return 1;
    std::memset(out,0,96);
    try {
        double y,x;std::memcpy(&y,&yword,8);std::memcpy(&x,&xword,8);
        sadx::X87Context fp(cw);
        sadx::CrtAtan2Trace trace;
        const auto result=sadx::crt_atan2_binary64(y,x,{policy,{}},fp,&trace);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;
        out[2]=fp.control_word();out[3]=fp.exception_flags();
        if(result.value){out[4]=result.value->significand();out[5]=result.value->significand()>>32;
            out[6]=result.value->sign_exponent();out[7]=1;}
        const auto store=[&](std::optional<double> value,unsigned index){
            if(value){std::uint64_t word;std::memcpy(&word,&*value,8);out[index]=word;
                out[index+1]=word>>32;out[index+2]=1;}
        };
        store(result.pending_stored_result,8);store(result.scratch_result_at_3c83960,12);
        if(result.pending_error_code){out[11]=*result.pending_error_code;out[15]=1;}
        out[16]=result.argument_copy_flag_at_3c83968;
        if(trace.result_after_fpatan){out[20]=trace.result_after_fpatan->significand();
            out[21]=trace.result_after_fpatan->significand()>>32;
            out[22]=trace.result_after_fpatan->sign_exponent();out[23]=1;}
        return 0;
    } catch(const sadx::X87Error& error) {out[17]=static_cast<std::uint32_t>(error.failure());
        out[18]=error.exceptions();return 2;}
}

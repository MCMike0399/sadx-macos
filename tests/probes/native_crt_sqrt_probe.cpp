#include "cpu/crt_sqrt.hpp"
#include <cstring>
extern "C" int sadx_native_crt_sqrt(std::uint64_t word,std::uint16_t cw,
    std::uint32_t policy,std::uint32_t* out){
    if(!out)return 1;
    try{sadx::X87Context fp(cw);sadx::CrtAsinRuntime runtime{policy,{}};double d;std::memcpy(&d,&word,8);
        const auto result=sadx::crt_sqrt(d,runtime,fp);std::memset(out,0,40);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=fp.control_word();out[3]=fp.exception_flags();
        if(result.value){out[4]=result.value->significand();out[5]=result.value->significand()>>32;out[6]=result.value->sign_exponent();out[7]=1;}
        if(result.pending_stored_result){std::uint64_t u;std::memcpy(&u,&*result.pending_stored_result,8);out[8]=u;out[9]=u>>32;}return 0;
    }catch(const sadx::X87Error&){return 2;}
}

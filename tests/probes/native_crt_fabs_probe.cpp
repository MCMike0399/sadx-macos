#include "cpu/crt_fabs.hpp"
#include <cstring>
extern "C" int sadx_native_crt_fabs(std::uint64_t u,std::uint16_t cw,std::uint32_t* out){
    if(!out)return 1;
    try{double d;std::memcpy(&d,&u,8);sadx::X87Context fp(cw);sadx::CrtFabsRuntime runtime{1,0,{}};
        const auto r=sadx::crt_fabs(d,runtime,fp);std::memset(out,0,40);out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=fp.control_word();out[3]=fp.exception_flags();
        if(r.value){out[4]=r.value->significand();out[5]=r.value->significand()>>32;out[6]=r.value->sign_exponent();out[7]=1;}
        out[8]=runtime.errno_value.value_or(0);return 0;
    }catch(const sadx::X87Error&){return 2;}
}

#include "cpu/crt_atan2.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

namespace {
double word(std::uint64_t u){double value;std::memcpy(&value,&u,8);return value;}
std::uint64_t bits(double value){std::uint64_t u;std::memcpy(&u,&value,8);return u;}
void value(const sadx::CrtAtan2Result& result,std::uint16_t se,std::uint64_t sig){
    assert(result.failure==sadx::CrtAtan2Failure::none && result.value);
    assert(result.value->sign_exponent()==se && result.value->significand()==sig);
}
}
int main(){
    using namespace sadx;
    for(unsigned pc:{0U,0x200U,0x300U})for(unsigned rc:{0U,0x400U,0x800U,0xc00U}){
        const auto cw=static_cast<std::uint16_t>(pc|rc|0x7fU);
        X87Context fp(cw);const CrtAtan2Runtime bypass{1,{}};
        value(crt_atan2_binary64(0,1,bypass,fp),0,0);
        value(crt_atan2_binary64(word(0x8000000000000000ULL),1,bypass,fp),0x8000,0);
        value(crt_atan2_binary64(0,-1,bypass,fp),0x4000,0xc90fdaa22168c235ULL);
        value(crt_atan2_binary64(word(0x8000000000000000ULL),-1,bypass,fp),0xc000,0xc90fdaa22168c235ULL);
        value(crt_atan2_binary64(1,0,bypass,fp),0x3fff,0xc90fdaa22168c235ULL);
        value(crt_atan2_binary64(-1,0,bypass,fp),0xbfff,0xc90fdaa22168c235ULL);
        value(crt_atan2_binary64(1,1,bypass,fp),0x3ffe,0xc90fdaa22168c235ULL);
        value(crt_atan2_binary64(1,-1,bypass,fp),0x4000,0x96cbe3f9990e91a8ULL);
        value(crt_atan2_binary64(word(0x7ff0000000000000ULL),word(0x7ff0000000000000ULL),bypass,fp),0xffff,0xc000000000000000ULL);
        assert(fp.control_word()==cw);

        fp.clear_exceptions();
        const auto unknown=crt_atan2_binary64(1,0,{{},{}},fp);
        assert(unknown.failure==CrtAtan2Failure::unknown_matherr_policy && unknown.value);
        assert(unknown.source_boundary==0x64b3e5 && fp.control_word()==0x133f);
        assert(!unknown.scratch_result_at_3c83960 && !unknown.pending_stored_result);
        assert(unknown.argument_copy_flag_at_3c83968==0);
        fp.set_control_word(cw);fp.clear_exceptions();

        const auto nan=crt_atan2_binary64(word(0x7ff8000000000123ULL),1,bypass,fp);
        value(nan,0x7fff,0xc000000000000123ULL); // actual64b556 low-word copy
        const auto pending=crt_atan2_binary64(word(0x7ff8000000000123ULL),-2,{0,{}},fp);
        assert(pending.failure==CrtAtan2Failure::missing_matherr_handler);
        assert(pending.source_boundary==0x64b518 && pending.pending_error_code==7);
        assert(pending.pending_stored_result && pending.scratch_result_at_3c83960);
        assert(fp.control_word()==0x133f);
        fp.set_control_word(cw);fp.clear_exceptions();
        const auto underflow=crt_atan2_binary64(word(1),0.125,{0,{}},fp);
        assert(underflow.failure==CrtAtan2Failure::missing_matherr_handler && underflow.pending_error_code==4);
        assert(bits(*underflow.scratch_result_at_3c83960)==8);
        assert(bits(*underflow.pending_stored_result)==0x5d00000000000000ULL);
        assert(fp.control_word()==0x133f);
        fp.set_control_word(cw);fp.clear_exceptions();

        unsigned calls=0;
        CrtAtan2Runtime handled{0,[&](const CrtAtan2MathErrorRequest& request,X87Context& active){
            ++calls;
            assert(request.descriptor==0x2bcc170 && request.operation==16 && request.error_code==7);
            assert(bits(request.argument_y)==0x7ff8000000000123ULL);
            assert(bits(request.argument_x)==0xc000000000000000ULL && request.saved_control_word==cw);
            assert(bits(request.scratch_result_at_3c83960)==bits(request.stored_result));
            assert(request.argument_copy_flag_at_3c83968==0);
            assert(active.control_word()==0x133f);
            // Test fixture is an explicitly supplied callback, not a claim
            // that the real Windows dispatcher replaces errors with0.125.
            return CrtAtan2MathErrorResult{CrtAtan2Failure::none,0.125};
        }};
        value(crt_atan2_binary64(word(0x7ff8000000000123ULL),-2,handled,fp),0x3ffc,0x8000000000000000ULL);
        assert(calls==1 && fp.control_word()==cw);
        handled.matherr_handler=[](const CrtAtan2MathErrorRequest&,X87Context&){return CrtAtan2MathErrorResult{CrtAtan2Failure::none,{}};};
        const auto unavailable=crt_atan2_binary64(word(0x7ff8000000000123ULL),-2,handled,fp);
        assert(unavailable.failure==CrtAtan2Failure::matherr_unavailable && !unavailable.value);
        assert(unavailable.source_boundary==0x64b518 && unavailable.pending_error_code==7);
    }
    // Source64b443 is an explicit waiting boundary after restoring an
    // incoming unmasked precision exception; no hidden masking/rollback.
    X87Context unmasked(0x035f);
    const auto trapped=crt_atan2_binary64(1,1,{1,{}},unmasked);
    assert(trapped.failure==CrtAtan2Failure::unmasked_return_exception);
    assert(trapped.source_boundary==0x64b443 && !trapped.value);
    assert(unmasked.control_word()==0x035f && (unmasked.exception_flags()&0x20));
    X87Context dispatched(0x035f);
    const auto precision=crt_atan2_binary64(1,1,{0,{}},dispatched);
    assert(precision.failure==CrtAtan2Failure::missing_matherr_handler && precision.pending_error_code==8);
    assert(dispatched.control_word()==0x133f);
    std::cout<<"CRT binary64 atan2 passed12 CW profiles, two-argument boundaries and precision policy\n";
}

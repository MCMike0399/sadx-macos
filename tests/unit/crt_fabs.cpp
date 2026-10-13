#include "cpu/crt_fabs.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace sadx;
namespace{double number(std::uint64_t u){double d;std::memcpy(&d,&u,8);return d;}}
int main(){
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        const auto cw=static_cast<std::uint16_t>(pc|rc|0x7f);X87Context fp(cw);CrtFabsRuntime runtime{};
        for(double input:{-4.,-0.,4.}){auto r=crt_fabs(input,runtime,fp);assert(r&&r.value&&!r.value->negative()&&fp.control_word()==cw);}
        auto r=crt_fabs(number(0xfff0000000000000ULL),runtime,fp);assert(r&&r.value->sign_exponent()==0x7fff);
        r=crt_fabs(number(0xfff8000000000123ULL),runtime,fp);assert(!r&&r.failure==CrtFabsFailure::unknown_nan_policy&&fp.control_word()==0x133f&&r.pending_stored_argument);
        fp.set_control_word(cw);runtime.nan_policy=1;r=crt_fabs(number(0xfff8000000000123ULL),runtime,fp);assert(r&&r.value->negative()&&runtime.errno_value==0x21&&fp.control_word()==cw);
        runtime.nan_policy=0;r=crt_fabs(number(0x7ff8000000000123ULL),runtime,fp);assert(r.failure==CrtFabsFailure::missing_nan_handler&&r.source_boundary==0x64afbd&&fp.control_word()==0x133f);
    }
    std::cout<<"CRT fabs: source sign-bit path, NaN sign/policy/errno and retained control boundaries passed.\n";
}

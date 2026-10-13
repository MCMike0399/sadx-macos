#include "cpu/crt_sqrt.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace sadx;
namespace { double number(std::uint64_t u){double d;std::memcpy(&d,&u,8);return d;} }
int main(){
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        const auto cw=static_cast<std::uint16_t>(pc|rc|0x7f);X87Context fp(cw);CrtAsinRuntime runtime{1,{}};
        auto result=crt_sqrt(4,runtime,fp);assert(result.value&&fp.to_double(*result.value)==2&&fp.control_word()==cw);
        result=crt_sqrt(-0.0,runtime,fp);assert(result.value&&result.value->negative()&&result.value->significand()==0);
        result=crt_sqrt(-4,runtime,fp);assert(result.value&&result.value->sign_exponent()==0xffff&&result.value->significand()==0xc000000000000000ULL);
        result=crt_sqrt(number(0x7ff8000000000123ULL),runtime,fp);assert(result.value&&result.value->significand()==(0xc000000000000000ULL|(0x123ULL<<11)));
        fp.clear_exceptions();result=crt_sqrt(number(0x7ff0000000000123ULL),runtime,fp);assert(result.value&&(fp.exception_flags()&1));
    }
    X87Context fp(0x0b7f);CrtAsinRuntime unknown{};auto result=crt_sqrt(4,unknown,fp);
    assert(result.failure==CrtAsinFailure::unknown_matherr_policy&&result.source_boundary==0x6452ce&&result.value&&fp.control_word()==0x037f);
    fp.set_control_word(0x037f);CrtAsinRuntime dispatch{0,{}};result=crt_sqrt(-4,dispatch,fp);
    assert(result.failure==CrtAsinFailure::missing_matherr_handler&&result.pending_stored_result&&result.source_boundary==0x648b6b);
    bool called=false;dispatch.matherr_handler=[&](CrtMathFunction function,unsigned code,double argument,double stored,std::uint16_t saved,X87Context&){
        called=true;assert(function==CrtMathFunction::sqrt&&code==1&&argument==-4&&saved==0x037f);return CrtMathErrorResult{CrtAsinFailure::none,stored};};
    fp.set_control_word(0x037f);result=crt_sqrt(-4,dispatch,fp);assert(called&&result.failure==CrtAsinFailure::none&&fp.control_word()==0x037f);
    std::cout<<"CRT square root: source classification, control words, NaN payloads, negative zero and error policy passed.\n";
}

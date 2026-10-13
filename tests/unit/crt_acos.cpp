#include "cpu/crt_acos.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    const sadx::CrtAsinRuntime bypass{1,{}};
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        const auto cw=static_cast<std::uint16_t>(pc|rc|0x7f);sadx::X87Context fp(cw);
        const auto positive=sadx::crt_acos(fp.from_float(1),bypass,fp);
        require(positive.value&&positive.value->sign_exponent()==0&&positive.value->significand()==0&&fp.control_word()==cw,"positive endpoint/source control restoration");
        const auto negative=sadx::crt_acos(fp.from_float(-1),bypass,fp);
        require(negative.value&&negative.value->sign_exponent()==0x4000&&negative.value->significand()==0xc90fdaa22168c235ULL&&fp.control_word()==cw,"negative endpoint is exact nearest FLDPI after internal RC setup");
        const auto interior=sadx::crt_acos(fp.from_float(.5F),bypass,fp);
        require(interior.value&&fp.to_double(*interior.value)>1&&fp.to_double(*interior.value)<1.1&&fp.control_word()==cw,"actual FPATAN operand order and control restore");
        const auto unknown=sadx::crt_acos(fp.from_float(.5F),{{},{}},fp);
        require(unknown.failure==sadx::CrtAsinFailure::unknown_matherr_policy&&unknown.source_boundary==0x645138&&fp.control_word()==static_cast<std::uint16_t>((cw&0x300)|0x7f),"ordinary policy boundary retains computed result/internal control");
        fp.set_control_word(cw);
        const auto domain=sadx::crt_acos(fp.from_float(2),{{},{}},fp);
        require(domain.failure==sadx::CrtAsinFailure::unknown_matherr_policy&&domain.source_boundary==0x64519c,"acos error-policy site is distinct");
    }
    sadx::X87Context fp(0x0b7f);unsigned calls=0;
    const sadx::CrtAsinRuntime handled{0,[&](sadx::CrtMathFunction function,unsigned code,double argument,double stored,std::uint16_t saved,sadx::X87Context& actual){
        ++calls;require(function==sadx::CrtMathFunction::acos&&code==1&&argument==2&&std::isnan(stored)&&saved==0x0b7f&&actual.control_word()==0x037f,"dispatcher keeps acos identity and source fields/control");
        return sadx::CrtMathErrorResult{sadx::CrtAsinFailure::none,.25};
    }};
    const auto result=sadx::crt_acos(fp.from_float(2),handled,fp);
    require(calls==1&&result.value&&fp.to_double(*result.value)==.25&&fp.control_word()==0x0b7f,"handler result loads before caller control restoration");
    std::cout<<"CRT acos: classified endpoints, source FPATAN order, explicit policy and math-function identity passed.\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

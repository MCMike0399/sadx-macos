#include "cpu/crt_asin.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
bool same(sadx::X87Value a,sadx::X87Value b) {
    return a.sign_exponent()==b.sign_exponent() && a.significand()==b.significand();
}

void test_source_constants_and_control_restore() {
    const sadx::CrtAsinRuntime bypass{1,{}}; // explicit test-selected runtime word
    for(std::uint16_t pc:{0x0000,0x0200,0x0300}) for(std::uint16_t rc:{0,0x400,0x800,0xc00}) {
        const auto word=static_cast<std::uint16_t>(pc|rc|0x7f);sadx::X87Context fp(word);
        for(float value:{0.0F,-0.0F,1.0F,-1.0F}) {
            fp.clear_exceptions();const auto input=fp.from_float(value);
            const auto result=sadx::crt_asin(input,bypass,fp);
            require(result.failure==sadx::CrtAsinFailure::none && result.value,"supported local asin result");
            require(fp.control_word()==word,"asin restores every incoming precision/rounding word");
            if(value==0) require(same(*result.value,input),"source FPATAN zero sign retained");
            else {
                auto expected=fp.from_canonical_ext80(0x3fff,0xc90fdaa22168c235ULL);
                if(value<0) expected=expected.negated();
                require(same(*result.value,expected),"source signedPI/2 literal, no host asin oracle");
            }
        }
        const auto result=sadx::crt_asin(fp.from_float(0.5F),bypass,fp);
        require(result.value && result.failure==sadx::CrtAsinFailure::none && fp.control_word()==word,
                "general Bochs-model arithmetic returns and restores control");
    }
    sadx::X87Context fp(0x037f);
    // FST64 classification chooses the literal endpoint even though ST0's
    // canonical source value is below/above one. This is an instruction-order
    // test, not a mathematical asin-domain assertion.
    for(const auto input:{fp.from_canonical_ext80(0x3ffe,0xffffffffffffffffULL),
                          fp.from_canonical_ext80(0x3fff,0x8000000000000001ULL)}) {
        const auto result=sadx::crt_asin(input,bypass,fp);
        require(result.value && same(*result.value,fp.from_canonical_ext80(0x3fff,0xc90fdaa22168c235ULL)),
                "classification uses the binary64 spill rather than unrounded ST0");
    }
    const auto quiet=fp.from_canonical_ext80(0xffff,0xc123456789abcdefULL);
    const auto result=sadx::crt_asin(quiet,bypass,fp);
    require(result.value && same(*result.value,quiet),"source quietNaN bypass preserves extended payload/sign");
    for(float value:{1.5F,std::numeric_limits<float>::infinity()}) {
        const auto failed_domain=sadx::crt_asin(fp.from_float(value),bypass,fp);
        require(failed_domain.value && same(*failed_domain.value,fp.from_canonical_ext80(0xffff,0xc000000000000000ULL)),
                "source domain/infinity bypass returns negative indefinite literal");
    }
}

void test_unknown_and_missing_dispatch() {
    sadx::X87Context fp(0x0b7f);
    const auto unknown=sadx::crt_asin(fp.from_float(0.5F),{{},{}},fp);
    require(unknown.failure==sadx::CrtAsinFailure::unknown_matherr_policy && unknown.value &&
            unknown.source_boundary==0x645066 && fp.control_word()==0x037f,
            "unknown valid-path policy preserves result and temporary source control state");
    fp.set_control_word(0x0b7f);
    const auto unknown_domain=sadx::crt_asin(fp.from_float(1.5F),{{},{}},fp);
    require(unknown_domain.failure==sadx::CrtAsinFailure::unknown_matherr_policy &&
            unknown_domain.source_boundary==0x6450cc && fp.control_word()==0x037f,
            "domain policy has its distinct actual source read boundary");
    fp.set_control_word(0x0b7f);
    const auto missing=sadx::crt_asin(fp.from_float(1.5F),{0,{}},fp);
    require(missing.failure==sadx::CrtAsinFailure::missing_matherr_handler &&
            missing.source_boundary==0x648b6b && missing.pending_stored_result &&
            std::isnan(*missing.pending_stored_result) && fp.control_word()==0x037f,
            "missing dispatcher retains binary64 store and source call-site control");

    // Source defaultCW skips precision dispatch even when PE was already set.
    sadx::X87Context default_fp(0x027f);
    (void)default_fp.to_float(default_fp.divide(default_fp.from_float(1),default_fp.from_float(3)));
    require((default_fp.exception_flags()&0x20)!=0,"fixture accrued precision flag");
    const auto ordinary=sadx::crt_asin(default_fp.from_float(1),{0,{}},default_fp);
    require(ordinary.failure==sadx::CrtAsinFailure::none && ordinary.value,
            "exactdefaultCW does not invent a required precision dispatcher");
    sadx::X87Context extended_fp(0x037f);
    (void)extended_fp.to_float(extended_fp.divide(extended_fp.from_float(1),extended_fp.from_float(3)));
    const auto precision=sadx::crt_asin(extended_fp.from_float(1),{0,{}},extended_fp);
    require(precision.failure==sadx::CrtAsinFailure::missing_matherr_handler,
            "sticky precision flag is not silently cleared before source return dispatch");
}

void test_dispatch_arguments_and_load_restore_order() {
    for(const std::uint16_t saved:{0x027f,0x0a7f,0x0b7f}) {
        sadx::X87Context fp(saved);unsigned calls=0;
        const sadx::CrtAsinRuntime runtime{0,[&](sadx::CrtMathFunction function,unsigned code,double argument,double stored,
                                                   std::uint16_t incoming,sadx::X87Context& context) {
            ++calls;require(function==sadx::CrtMathFunction::asin && code==1 && argument==1.5 && std::isnan(stored) && incoming==saved,
                            "source domain matherr arguments");
            require(context.control_word()==(saved==0x027f ? saved : static_cast<std::uint16_t>((saved&0x300)|0x7f)),
                    "callback sees source temporaryCW, not prematurely restored callerCW");
            return sadx::CrtMathErrorResult{sadx::CrtAsinFailure::none,0.25};
        }};
        const auto result=sadx::crt_asin(fp.from_float(1.5F),runtime,fp);
        require(result.failure==sadx::CrtAsinFailure::none && result.value && calls==1 &&
                fp.control_word()==saved && same(*result.value,fp.from_double(0.25)),
                "explicit handler result loads then restores incoming control word");
    }
    sadx::X87Context fp(0x0b7f);
    const sadx::CrtAsinRuntime unavailable{0,[](sadx::CrtMathFunction,unsigned,double,double,std::uint16_t,sadx::X87Context&) {
        return sadx::CrtMathErrorResult{sadx::CrtAsinFailure::none,{}};
    }};
    const auto failed=sadx::crt_asin(fp.from_float(1.5F),unavailable,fp);
    require(failed.failure==sadx::CrtAsinFailure::matherr_unavailable && fp.control_word()==0x037f,
            "unknown handler result cannot become fake matherr success or rollback");

    // Nonfinite entry bypasses temporary control setup, including callback CW.
    fp.set_control_word(0x0b7f);unsigned code_seen=0;
    const sadx::CrtAsinRuntime special{0,[&](sadx::CrtMathFunction function,unsigned code,double,double,std::uint16_t,sadx::X87Context& context) {
        code_seen=code;require(function==sadx::CrtMathFunction::asin && context.control_word()==0x0b7f,"nonfinite callback uses untouched incomingCW");
        return sadx::CrtMathErrorResult{sadx::CrtAsinFailure::none,2.0};
    }};
    const auto result=sadx::crt_asin(fp.from_canonical_ext80(0x7fff,0xc123456789abcdefULL),special,fp);
    require(result.value && code_seen==7 && fp.to_double(*result.value)==2,
            "quietNaN source errorcode7, explicit handler owns its result");

    // Explicit callback seed demonstrates why FLD64 must precede restoration:
    // its signaling result traps under the callback's changed control word.
    fp.set_control_word(0x0b7f);fp.clear_exceptions();
    const sadx::CrtAsinRuntime load_order{0,[](sadx::CrtMathFunction,unsigned,double,double,std::uint16_t,sadx::X87Context& context) {
        context.set_control_word(0x037e);
        const std::uint64_t signaling_bits=0x7ff0000000000001ULL;
        double signaling;std::memcpy(&signaling,&signaling_bits,8);
        return sadx::CrtMathErrorResult{sadx::CrtAsinFailure::none,signaling};
    }};
    bool stopped=false;
    try { (void)sadx::crt_asin(fp.from_float(1.5F),load_order,fp); }
    catch(const sadx::X87Error& error) { stopped=error.failure()==sadx::X87Failure::unmasked_exception && error.exceptions()==1; }
    require(stopped && fp.control_word()==0x037e,
            "handler result load occurs before source restore, with no invented exception rollback");
}

void test_binary64_entry_classifier() {
    // 0x645034 uses the original binary64 bits for classification. Unlike the
    // retained-ST0 entry, it constructs an ext80 nonfinite value without an
    // intervening FLD64/FST64 that would quiet a signaling NaN.
    for (const std::uint64_t raw : {0x7ff0000000000123ULL,0xfff0000000000123ULL,
                                    0x7ff8000000000123ULL,0xfff8000000000123ULL}) {
        double input;std::memcpy(&input,&raw,8);
        const bool quiet=(raw&0x0008000000000000ULL)!=0;
        sadx::X87Context fp(0x0b7f);unsigned calls=0;
        const sadx::CrtAsinRuntime runtime{0,[&](sadx::CrtMathFunction function,unsigned code,
                double argument,double stored,std::uint16_t saved,sadx::X87Context& context) {
            ++calls;std::uint64_t argument_bits;std::memcpy(&argument_bits,&argument,8);
            require(function==sadx::CrtMathFunction::asin && code==(quiet?7U:1U) &&
                    argument_bits==raw && std::isnan(stored) && saved==0x0b7f &&
                    context.control_word()==saved,"binary64 classifier preserves raw argument and error branch");
            require((context.exception_flags()&1U)==(quiet?0U:1U),
                    "signaling NaN raises invalid in source arithmetic, quiet NaN does not");
            return sadx::CrtMathErrorResult{sadx::CrtAsinFailure::none,0.25};
        }};
        const auto result=sadx::crt_asin_binary64(input,runtime,fp);
        require(result.value && calls==1 && fp.to_double(*result.value)==0.25 && fp.control_word()==0x0b7f,
                "binary64 entry restores caller state after explicit handler");
    }
    for (double input : {0.,-0.,0.5,-0.5,1.,-1.,1.5}) {
        sadx::X87Context binary(0x0b7f),extended(0x0b7f);
        const sadx::CrtAsinRuntime bypass{1,{}};
        const auto a=sadx::crt_asin_binary64(input,bypass,binary);
        const auto b=sadx::crt_asin(extended.from_double(input),bypass,extended);
        require(a.value && b.value && same(*a.value,*b.value) && binary.control_word()==extended.control_word(),
                "finite exactly represented inputs share the recovered asin body");
    }
}
}

int main() {
    try {
        test_source_constants_and_control_restore();test_unknown_and_missing_dispatch();test_dispatch_arguments_and_load_restore_order();test_binary64_entry_classifier();
        std::cout<<"Source CRT asin: literal/classifier branches, 12controlwords, explicit policy/dispatcher and source control-state effects passed.\n"
                     "FPATAN is the selected Bochs model; original hardware and real matherr runtime remain unverified.\n";return 0;
    }catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}

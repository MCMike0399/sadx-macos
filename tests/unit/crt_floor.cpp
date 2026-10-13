#include "cpu/crt_floor.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
double number(std::uint64_t b){double x;std::memcpy(&x,&b,8);return x;}
std::uint64_t bits(double x){std::uint64_t b;std::memcpy(&b,&x,8);return b;}
}
int main(int argc,char**argv) {
    if(argc!=2)throw std::runtime_error("requires source CRTfloor constants");
    std::uint8_t raw[80];std::ifstream f(argv[1],std::ios::binary);f.read(reinterpret_cast<char*>(raw),80);
    assert(f.gcount()==80&&f.peek()==std::char_traits<char>::eof());
    const auto constants=decode_crt_floor_constants(raw,80);assert(constants);
    X87Context fp(0x037f);SseContext sse(0x1f80);
    CrtFloorRuntime runtime{1,&sse,0x173f,{}};
    for(const auto pair: {std::pair<double,double>{1.25,1},{-1.25,-2},{-0.25,-1},{0.25,0},{-2,-2}}) {
        auto out=crt_floor(pair.first,*constants,runtime,fp);
        assert(out&&out.used_sse&&out.value&&fp.to_double(*out.value)==pair.second&&fp.control_word()==0x037f);
    }
    auto out=crt_floor(-0.0,*constants,runtime,fp);assert(out&&bits(fp.to_double(*out.value))==0x8000000000000000ULL);
    sse.set_mxcsr(0x1fc0);out=crt_floor(number(0x8000000000000001ULL),*constants,runtime,fp);
    assert(out&&bits(fp.to_double(*out.value))==0x8000000000000000ULL);
    sse.set_mxcsr(0x1f80);out=crt_floor(number(0x8000000000000001ULL),*constants,runtime,fp);
    assert(out&&fp.to_double(*out.value)==-1&& (sse.mxcsr()&2));
    out=crt_floor(number(0x7ff8000000000123ULL),*constants,runtime,fp);
    assert(out.failure==CrtFloorFailure::missing_error_handler&&out.source_boundary==0x644aa5&&out.pending_stored_result);
    // Nonzero feature does not force SSE when either source mask gate fails.
    sse.set_mxcsr(0x1f00);out=crt_floor(-1.25,*constants,runtime,fp);
    assert(out&&!out.used_sse&&fp.to_double(*out.value)==-2);
    sse.set_mxcsr(0x1f80);fp.set_control_word(0x033f);
    out=crt_floor(-1.25,*constants,runtime,fp);assert(out&&!out.used_sse&&fp.control_word()==0x033f);
    fp.set_control_word(0x037f);
    runtime.cpu_sse2=0;
    for(auto cw:{std::uint16_t{0x007f},std::uint16_t{0x0e7f},std::uint16_t{0x037f}}) {
        fp.set_control_word(cw);out=crt_floor(-1.25,*constants,runtime,fp);
        assert(out&&!out.used_sse&&fp.to_double(*out.value)==-2&&fp.control_word()==cw);
    }
    fp.set_control_word(0x037f);out=crt_floor(number(0x7ff8000000000123ULL),*constants,runtime,fp);
    assert(out.failure==CrtFloorFailure::missing_error_handler&&out.source_boundary==0x6491cb&&fp.control_word()==0x173f);
    fp.set_control_word(0x035f);fp.clear_exceptions();out=crt_floor(1.25,*constants,runtime,fp);
    assert(out.failure==CrtFloorFailure::missing_error_handler&&out.source_boundary==0x64923f&&fp.control_word()==0x173f);
    fp.set_control_word(0x037f);fp.clear_exceptions();runtime.fallback_control_word={};
    out=crt_floor(2,*constants,runtime,fp);assert(out.failure==CrtFloorFailure::unknown_fallback_control_word&&fp.control_word()==0x037f);
    runtime.cpu_sse2={};out=crt_floor(2,*constants,runtime,fp);assert(out.failure==CrtFloorFailure::unknown_cpu_policy);
    // FRNDINT retains full integer precision regardless of PC; integer casts
    // and arithmetic under24-bit PC would lose this exact source value.
    fp.set_control_word(0x007f);auto v=fp.round_to_integer(fp.from_double(16777217.25));
    assert(fp.to_double(v)==16777217.0);
    std::cout<<"Source CRTfloor: SSE/fallback routing, DAZ, signed zero, FRNDINT PC independence and partial error boundaries passed.\n";
}

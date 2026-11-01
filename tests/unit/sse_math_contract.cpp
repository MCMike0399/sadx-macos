// Maintained-model characterization frozen before the SSE readability rewrite.
// No Intel/Windows claim: the unchanged opcode judge supplies separate evidence.
#include "cpu/sse_math.hpp"
#include "cpu/x87_math.hpp"
extern "C" {
#include "third_party/softfloat/platform.h"
#include "third_party/softfloat/source/include/softfloat.h"
}
#include <array>
#include <cassert>
#include <cfenv>
#include <cstring>
#include <iostream>
#include <thread>
using namespace sadx;
namespace {
bool capture = false;
std::uint64_t digest = 14695981039346656037ULL;
std::uint64_t count = 0;
void word(std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8) {
        const auto byte = static_cast<unsigned char>(value >> shift);
        digest = (digest ^ byte) * 1099511628211ULL;
        ++count;
        if (capture) std::cout.put(static_cast<char>(byte));
    }
}
std::uint64_t invoke(SseContext &fp, unsigned operation, std::uint64_t a, std::uint64_t b) {
    const Sse32 x{static_cast<std::uint32_t>(a)}, y{static_cast<std::uint32_t>(b)};
    switch (operation) {
    case 0: return fp.add(x,y).bits;
    case 1: return fp.subtract(x,y).bits;
    case 2: return fp.multiply(x,y).bits;
    case 3: return fp.divide(x,y).bits;
    case 4: return fp.minimum(x,y).bits;
    case 5: return fp.to_double(x).bits;
    case 6: return fp.to_single({a}).bits;
    case 7: return fp.sqrt({a}).bits;
    case 8: return static_cast<std::uint32_t>(fp.truncate_int32(x));
    case 9: return fp.from_int32(static_cast<std::int32_t>(a)).bits;
    case 10: return fp.sqrt_single(x).bits;
    case 11: return fp.subtract_double({a},{b}).bits;
    case 12: return fp.less_than({a},{b});
    case 13: return static_cast<unsigned>(fp.comiss(x,y));
    case 14: return static_cast<unsigned>(fp.ucomisd({a},{b}));
    case 15: fp.clear_exceptions(); return 0;
    case 16: fp.set_mxcsr(static_cast<std::uint32_t>(a)); return 0;
    default: return fp.mxcsr();
    }
}
void observe(SseContext &fp, unsigned operation, std::uint64_t a, std::uint64_t b) {
    std::uint64_t result=0x0123456789abcdefULL;
    unsigned failure=0, exceptions=0;
    std::array<char,64> message{};
    try {
        result=invoke(fp,operation,a,b);
    } catch (const SseError &error) {
        failure=1+static_cast<unsigned>(error.failure());
        exceptions=error.exceptions();
        std::strncpy(message.data(),error.what(),message.size()-1);
    }
    word(operation);word(a);word(b);word(result);word(failure);word(exceptions);
    for (unsigned offset=0;offset<message.size();offset+=8) {
        std::uint64_t bytes;
        std::memcpy(&bytes,message.data()+offset,8);
        word(bytes);
    }
}
}
int main(int argc, char **argv) {
    capture=argc>1 && std::strcmp(argv[1],"--capture")==0;
    const std::array<std::uint32_t,18> single{{0,0x80000000,1,0x80000001,0x7fffff,
        0x800000,0x80800000,0x33800000,0x3f000000,0x3f800000,0xbf800000,
        0x7f7fffff,0x7f800000,0xff800000,0x7f800001,0x7fc00001,0x7fc12345,0xff800001}};
    const std::array<std::uint64_t,18> wide{{0,0x8000000000000000ULL,1,0x8000000000000001ULL,
        0xfffffffffffffULL,0x10000000000000ULL,0x8010000000000000ULL,0x3e60000000000000ULL,
        0x3fe0000000000000ULL,0x3ff0000000000000ULL,0xbff0000000000000ULL,
        0x7fefffffffffffffULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,
        0x7ff0000000000001ULL,0x7ff8000000000001ULL,0x7ff8001234500000ULL,0xfff0000000000001ULL}};
    const std::array<unsigned,8> masks{{0,1,2,4,8,16,32,63}};
    const auto saved_round=std::fegetround();
    std::fenv_t environment;
    std::fegetenv(&environment);
    std::fesetround(FE_DOWNWARD);
    std::feraiseexcept(FE_INVALID|FE_INEXACT);
    const auto host_flags=std::fetestexcept(FE_ALL_EXCEPT);
    std::uint64_t cases=0;
    for (unsigned control=0;control<16;++control) {
        for (const auto unmasked : masks) {
            for (const auto sticky : {0U,63U}) {
                const auto mxcsr=(0x1f80U & ~(unmasked<<7)) | ((control&3)<<13) |
                    ((control&4)<<4) | ((control&8)<<12) | sticky;
                for (unsigned operation=0;operation<15;++operation) {
                    for (unsigned i=0;i<single.size();++i) {
                        for (unsigned j=0;j<single.size();++j) {
                            const bool is_wide=operation==6 || operation==7 || operation==11 ||
                                operation==12 || operation==14;
                            SseContext fp(mxcsr);
                            softfloat_roundingMode=softfloat_round_max;
                            softfloat_detectTininess=softfloat_tininess_beforeRounding;
                            softfloat_exceptionFlags=softfloat_flag_overflow;
                            extF80_roundingPrecision=64;
                            word(mxcsr);
                            observe(fp,operation,is_wide?wide[i]:single[i],is_wide?wide[j]:single[j]);
                            word(fp.mxcsr());
                            word(softfloat_roundingMode);word(softfloat_detectTininess);
                            word(softfloat_exceptionFlags);word(extF80_roundingPrecision);
                            assert(softfloat_roundingMode==softfloat_round_max);
                            assert(softfloat_detectTininess==softfloat_tininess_beforeRounding);
                            assert(softfloat_exceptionFlags==softfloat_flag_overflow);
                            assert(extF80_roundingPrecision==64);
                            assert(std::fegetround()==FE_DOWNWARD);
                            assert(std::fetestexcept(FE_ALL_EXCEPT)==host_flags);
                            ++cases;
                        }
                    }
                }
            }
        }
    }
    // Every public entry rejects use from a different thread, before touching TLS.
    for (unsigned operation=0;operation<18;++operation) {
        SseContext fp(0x1fa0);
        std::thread other([&] { observe(fp,operation,0x10000,1); });
        other.join();
        word(fp.mxcsr());
    }
    SseContext fp(0x1fbf);
    observe(fp,16,0x10000,0);word(fp.mxcsr());
    observe(fp,15,0,0);word(fp.mxcsr());
    observe(fp,16,0x7fc0,0);word(fp.mxcsr());
    try { SseContext bad(0x10000);assert(false); }
    catch (const SseError &error) {
        assert(error.failure()==SseFailure::reserved_mxcsr && error.exceptions()==0);
        assert(std::strcmp(error.what(),"reserved MXCSR bits")==0);
    }
    X87Context x87(0x037f);
    const auto retained=x87.from_double(1.0+0x1p-24);
    (void)fp.add({0x3f800000},{0x33800000});
    assert(x87.to_float(retained)==1.0f);
    std::fesetenv(&environment);
    assert(std::fegetround()==saved_round);
    if (!capture) std::cout << cases << " cases " << count << " bytes " << digest << '\n';
    assert(count==199068120 && digest==4458530012893139995ULL);
}

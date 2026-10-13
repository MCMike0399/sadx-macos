#include "cpu/sse_math.hpp"
extern "C" {
#include "../../third_party/softfloat/platform.h"
#include "../../third_party/softfloat/source/include/softfloat.h"
}
namespace sadx {
namespace {
bool nan32(std::uint32_t a) { return (a & 0x7fffffffU) > 0x7f800000U; }
bool nan64(std::uint64_t a) { return (a & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL; }
bool snan64(std::uint64_t a) { return nan64(a) && !(a & 0x8000000000000ULL); }
bool den32(std::uint32_t a) { return !(a & 0x7f800000U) && (a & 0x7fffffU); }
bool den64(std::uint64_t a) { return !(a & 0x7ff0000000000000ULL) && (a & 0xfffffffffffffULL); }
std::uint32_t daz32(std::uint32_t a, std::uint32_t m) { return (m & 64) && den32(a) ? a & 0x80000000U : a; }
std::uint64_t daz64(std::uint64_t a, std::uint32_t m) { return (m & 64) && den64(a) ? a & 0x8000000000000000ULL : a; }
struct Operation {
    std::uint_fast8_t rounding=softfloat_roundingMode, tiny=softfloat_detectTininess, flags=softfloat_exceptionFlags;
    explicit Operation(std::uint32_t m) {
        constexpr std::uint_fast8_t modes[] = {softfloat_round_near_even, softfloat_round_min, softfloat_round_max, softfloat_round_minMag};
        softfloat_roundingMode=modes[(m>>13)&3];
        softfloat_detectTininess=softfloat_tininess_afterRounding;
        softfloat_exceptionFlags=0;
    }
    ~Operation() { softfloat_roundingMode=rounding; softfloat_detectTininess=tiny; softfloat_exceptionFlags=flags; }
    std::uint8_t exceptions() const {
        auto f=softfloat_exceptionFlags;
        return ((f & softfloat_flag_invalid)?1:0) | ((f & softfloat_flag_infinite)?4:0)
          | ((f & softfloat_flag_overflow)?8:0) | ((f & softfloat_flag_underflow)?16:0) | ((f & softfloat_flag_inexact)?32:0);
    }
};
template<typename U> SseComparison ordered(U a, U b, U sign) {
    if (a==b || !((a|b)&~sign)) return SseComparison::equal;
    bool less=(a&sign)!=(b&sign) ? bool(a&sign) : ((a&sign) ? a>b : a<b);
    return less ? SseComparison::less : SseComparison::greater;
}
// Intel's unmasked underflow detects even exact tiny working results. FTZ
// applies when UM is masked and flushes tiny results, including exact ones.
std::uint32_t finish32(std::uint32_t z, std::uint8_t& f, std::uint32_t m) {
    bool tiny=den32(z) || (f&16);
    if (tiny && !(m & 0x800)) f=static_cast<std::uint8_t>((f|16)&~32U);
    else if (tiny && (m & 0x8000)) { z &= 0x80000000U; f|=48; }
    if ((f&8) && !(m&0x400)) f&=static_cast<std::uint8_t>(~32U);
    return z;
}
}
SseContext::SseContext(std::uint32_t m) : mxcsr_(0), owner_(std::this_thread::get_id()) { set_mxcsr(m); }
void SseContext::check_thread() const { if(owner_!=std::this_thread::get_id()) throw SseError(SseFailure::cross_thread_context,0,"SSE context belongs to another thread"); }
std::uint32_t SseContext::mxcsr() const { check_thread(); return mxcsr_; }
void SseContext::set_mxcsr(std::uint32_t m) { check_thread(); if(m&0xffff0000U) throw SseError(SseFailure::reserved_mxcsr,0,"reserved MXCSR bits"); mxcsr_=m; }
void SseContext::clear_exceptions() { check_thread(); mxcsr_&=~63U; }
void SseContext::commit(std::uint8_t f) {
    mxcsr_|=f;
    auto unmasked=static_cast<std::uint8_t>(f & ~(mxcsr_>>7) & 63);
    if(unmasked) throw SseError(SseFailure::unmasked_exception,unmasked,"unmasked scalar SSE exception");
}
Sse32 SseContext::from_int32(std::int32_t v) { check_thread(); Operation op(mxcsr_); auto z=i32_to_f32(v); commit(op.exceptions()); return {z.v}; }
Sse32 SseContext::binary(Sse32 a,Sse32 b,unsigned kind) {
    check_thread(); a.bits=daz32(a.bits,mxcsr_); b.bits=daz32(b.bits,mxcsr_);
    Operation op(mxcsr_); float32_t x{a.bits},y{b.bits},z{};
    switch(kind) { case 0:z=f32_add(x,y);break;case 1:z=f32_sub(x,y);break;case 2:z=f32_mul(x,y);break;default:z=f32_div(x,y); }
    auto f=op.exceptions();
    // NaN, other invalid and divide-by-zero precede denormal operand faults.
    if(!(nan32(a.bits)||nan32(b.bits)||(f&5)) && (den32(a.bits)||den32(b.bits))) {
        if(!(mxcsr_&0x100)) { commit(2); } f|=2;
    }
    z.v=finish32(z.v,f,mxcsr_); commit(f); return {z.v};
}
Sse32 SseContext::add(Sse32 a,Sse32 b) { return binary(a,b,0); }
Sse32 SseContext::subtract(Sse32 a,Sse32 b) { return binary(a,b,1); }
Sse32 SseContext::multiply(Sse32 a,Sse32 b) { return binary(a,b,2); }
Sse32 SseContext::divide(Sse32 a,Sse32 b) { return binary(a,b,3); }
Sse64 SseContext::to_double(Sse32 a) {
    check_thread(); a.bits=daz32(a.bits,mxcsr_); if(den32(a.bits)) commit(2);
    Operation op(mxcsr_); auto z=f32_to_f64(float32_t{a.bits}); commit(op.exceptions()); return {z.v};
}
Sse32 SseContext::to_single(Sse64 a) {
    check_thread(); a.bits=daz64(a.bits,mxcsr_); if(den64(a.bits)) commit(2);
    Operation op(mxcsr_); auto z=f64_to_f32(float64_t{a.bits}); auto f=op.exceptions(); z.v=finish32(z.v,f,mxcsr_); commit(f); return {z.v};
}
Sse64 SseContext::sqrt(Sse64 a) {
    check_thread(); a.bits=daz64(a.bits,mxcsr_); Operation op(mxcsr_); auto z=f64_sqrt(float64_t{a.bits}); auto f=op.exceptions();
    if(!(f&1) && !nan64(a.bits) && den64(a.bits)) { if(!(mxcsr_&0x100)) commit(2); f|=2; }
    commit(f); return {z.v};
}
Sse64 SseContext::subtract_double(Sse64 a,Sse64 b) {
    check_thread(); a.bits=daz64(a.bits,mxcsr_); b.bits=daz64(b.bits,mxcsr_);
    Operation op(mxcsr_); auto z=f64_sub(float64_t{a.bits},float64_t{b.bits}); auto f=op.exceptions();
    if(!(nan64(a.bits)||nan64(b.bits)||(f&5)) && (den64(a.bits)||den64(b.bits))) {
        if(!(mxcsr_&0x100)) commit(2);
        f|=2;
    }
    const bool tiny=den64(z.v)||(f&16);
    if(tiny && !(mxcsr_&0x800)) f=static_cast<std::uint8_t>((f|16)&~32U);
    else if(tiny && (mxcsr_&0x8000)) { z.v&=0x8000000000000000ULL; f|=48; }
    if((f&8) && !(mxcsr_&0x400)) f&=static_cast<std::uint8_t>(~32U);
    commit(f); return {z.v};
}
bool SseContext::less_than(Sse64 a,Sse64 b) {
    check_thread(); a.bits=daz64(a.bits,mxcsr_); b.bits=daz64(b.bits,mxcsr_);
    if(nan64(a.bits)||nan64(b.bits)) { commit(1); return false; }
    if(den64(a.bits)||den64(b.bits)) commit(2);
    return ordered(a.bits,b.bits,std::uint64_t{0x8000000000000000ULL})==SseComparison::less;
}
std::int32_t SseContext::truncate_int32(Sse32 a) {
    check_thread(); a.bits=daz32(a.bits,mxcsr_); Operation op(mxcsr_); auto z=f32_to_i32_r_minMag(float32_t{a.bits},true); commit(op.exceptions()); return static_cast<std::int32_t>(z);
}
SseComparison SseContext::comiss(Sse32 a,Sse32 b) {
    check_thread(); a.bits=daz32(a.bits,mxcsr_); b.bits=daz32(b.bits,mxcsr_);
    if(nan32(a.bits)||nan32(b.bits)) { commit(1); return SseComparison::unordered; }
    if(den32(a.bits)||den32(b.bits)) commit(2);
    return ordered(a.bits,b.bits,0x80000000U);
}
SseComparison SseContext::ucomisd(Sse64 a,Sse64 b) {
    check_thread(); a.bits=daz64(a.bits,mxcsr_); b.bits=daz64(b.bits,mxcsr_);
    if(nan64(a.bits)||nan64(b.bits)) { if(snan64(a.bits)||snan64(b.bits)) commit(1); return SseComparison::unordered; }
    if(den64(a.bits)||den64(b.bits)) commit(2);
    return ordered(a.bits,b.bits,std::uint64_t{0x8000000000000000ULL});
}
Sse32 SseContext::minimum(Sse32 a,Sse32 b) {
    check_thread(); a.bits=daz32(a.bits,mxcsr_); b.bits=daz32(b.bits,mxcsr_);
    if(nan32(a.bits)||nan32(b.bits)) { commit(1); return b; }
    if(den32(a.bits)||den32(b.bits)) commit(2);
    return ordered(a.bits,b.bits,0x80000000U)==SseComparison::less ? a : b;
}
} // namespace sadx

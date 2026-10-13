#include "cpu/sse_math.hpp"
#include "cpu/x87_math.hpp"
#include <cassert>
#include <thread>
using namespace sadx;
int main() {
    SseContext fp(0x1f80);
    assert(fp.add({0x3f800000},{0x33800000}).bits==0x3f800000);
    assert((fp.mxcsr()&63)==32); fp.clear_exceptions();
    SseContext up(0x5f80),down(0x3f80),zero(0x7f80);
    assert(up.add({0x3f800000},{0x33800000}).bits==0x3f800001);
    assert(down.from_int32(-16777217).bits==0xcb800001);
    assert(zero.from_int32(-16777217).bits==0xcb800000);
    assert(fp.multiply({0x7fc00001},{0x7f800123}).bits==0x7fc00001);
    assert((fp.mxcsr()&63)==1);fp.clear_exceptions();
    assert(fp.add({0x7f800123},{0x7fc12345}).bits==0x7fc00123);
    assert(fp.minimum({0x3f800000},{0x7f800123}).bits==0x7f800123);
    assert(fp.minimum({0},{0x80000000}).bits==0x80000000);
    fp.clear_exceptions();assert(fp.add({1},{0x7fc00001}).bits==0x7fc00001); assert((fp.mxcsr()&63)==0);
    assert(fp.truncate_int32({1})==0);assert((fp.mxcsr()&63)==32);fp.clear_exceptions();
    assert(fp.to_double({1}).bits==0x36a0000000000000ULL);assert((fp.mxcsr()&63)==2);fp.clear_exceptions();
    SseContext daz(0x1fc0);assert(daz.to_double({0x80000001}).bits==0x8000000000000000ULL);assert((daz.mxcsr()&63)==0);
    assert(fp.multiply({0x00800000},{0x3f000000}).bits==0x00400000);assert((fp.mxcsr()&63)==0);
    SseContext ftz(0x9f80);assert(ftz.multiply({0x00800000},{0x3f000000}).bits==0);assert((ftz.mxcsr()&63)==48);
    SseContext tiny_trap(0x1780); bool trapped=false;
    try { (void)tiny_trap.multiply({0x00800000},{0x3f000000}); } catch(const SseError& e) { trapped=e.exceptions()==16; }
    assert(trapped && (tiny_trap.mxcsr()&63)==16);
    SseContext invalid(0x1f00);trapped=false;
    try { (void)invalid.comiss({0x7fc00000},{0}); } catch(const SseError& e) { trapped=e.exceptions()==1; }
    assert(trapped);
    // Pre-existing unmasked sticky IE does not make a later exact ADD trap.
    SseContext sticky(0x1f01); assert(sticky.add({0x3f800000},{0x3f800000}).bits==0x40000000);
    assert(fp.ucomisd({0x7ff8000000000001ULL},{0})==SseComparison::unordered);
    assert(fp.sqrt({0x4010000000000000ULL}).bits==0x4000000000000000ULL);
    assert(fp.subtract_double({0xc000000000000000ULL},{0x3ff0000000000000ULL}).bits==0xc008000000000000ULL);
    fp.clear_exceptions();assert(!fp.less_than({0x7ff8000000000123ULL},{0}));assert((fp.mxcsr()&63)==1);
    fp.clear_exceptions();assert(fp.less_than({0x8000000000000001ULL},{0}));assert((fp.mxcsr()&63)==2);
    assert(!daz.less_than({0x8000000000000001ULL},{0}));assert((daz.mxcsr()&63)==0);
    assert(fp.to_single({0x3ff0000010000000ULL}).bits==0x3f800000); // exact halfway
    assert(up.to_single({0x3ff0000010000000ULL}).bits==0x3f800001);
    assert(zero.truncate_int32({0x3fffffff})==1);assert(down.truncate_int32({0xbfffffff})==-1);
    assert(fp.truncate_int32({0x7fc00001})==INT32_MIN);
    bool reserved=false;try { SseContext bad(0x10000); }catch(const SseError& e){reserved=e.failure()==SseFailure::reserved_mxcsr;}assert(reserved);
    SseContext denormal_trap(0x1e80);trapped=false;
    try {(void)denormal_trap.divide({1},{0x40400000});}catch(const SseError& e){trapped=e.exceptions()==2;}
    assert(trapped&&(denormal_trap.mxcsr()&63)==2); // stop before PE/UE
    bool cross=false;std::thread thread([&] {try {(void)fp.mxcsr();}catch(const SseError& e){cross=e.failure()==SseFailure::cross_thread_context;}});thread.join();assert(cross);
    // Explicit SSE up-rounding cannot alter the existing x87 owner.
    X87Context x87(0x037f);auto value=x87.from_double(1.0+0x1p-24);
    (void)up.add({0x3f800000},{0x33800000});assert(x87.to_float(value)==1.0f);
}

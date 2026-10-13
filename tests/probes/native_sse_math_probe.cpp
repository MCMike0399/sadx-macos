#include "cpu/sse_math.hpp"
extern "C" int sadx_native_sse_math(unsigned operation,std::uint64_t a,std::uint64_t b,std::uint32_t mxcsr,std::uint64_t* result,std::uint32_t* status) {
    sadx::SseContext fp(mxcsr);int failed=0;
    try {
        switch(operation) {
        case 0:*result=fp.add({std::uint32_t(a)},{std::uint32_t(b)}).bits;break;
        case 1:*result=fp.subtract({std::uint32_t(a)},{std::uint32_t(b)}).bits;break;
        case 2:*result=fp.multiply({std::uint32_t(a)},{std::uint32_t(b)}).bits;break;
        case 3:*result=fp.divide({std::uint32_t(a)},{std::uint32_t(b)}).bits;break;
        case 4:*result=fp.minimum({std::uint32_t(a)},{std::uint32_t(b)}).bits;break;
        case 5:*result=fp.to_double({std::uint32_t(a)}).bits;break;
        case 6:*result=fp.to_single({a}).bits;break;
        case 7:*result=fp.sqrt({a}).bits;break;
        case 8:*result=std::uint32_t(fp.truncate_int32({std::uint32_t(a)}));break;
        default:return -1;
        }
    }catch(const sadx::SseError& e){failed=e.exceptions();}
    *status=fp.mxcsr();return failed;
}

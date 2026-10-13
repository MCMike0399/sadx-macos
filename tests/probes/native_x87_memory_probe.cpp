// Test-only arithmetic boundary. All inputs/outputs are integer bit patterns;
// crossing ctypes must not quiet a signaling NaN before the native operation.
#include "cpu/x87_math.hpp"
#include <cstdint>
#include <cstring>

extern "C" int sadx_native_x87_memory(std::uint64_t first_bits,
    std::uint64_t memory_bits, unsigned width, unsigned operation,
    std::uint16_t control_word, std::uint32_t* output) {
    if (!output || (width!=32 && width!=64) || operation>6) return 1;
    try {
        sadx::X87Context fp(control_word);
        float a32,b32;double a64,b64;
        const auto low_a=static_cast<std::uint32_t>(first_bits);
        const auto low_b=static_cast<std::uint32_t>(memory_bits);
        std::memcpy(&a32,&low_a,4);std::memcpy(&b32,&low_b,4);
        std::memcpy(&a64,&first_bits,8);std::memcpy(&b64,&memory_bits,8);
        auto result=width==32?fp.from_float(a32):fp.from_double(a64);
        output[3]=0;
        if (width==32) {
            switch(operation) {
            case 0: result=fp.add_float_memory(result,b32);break;
            case 1: result=fp.multiply_float_memory(result,b32);break;
            case 2: result=fp.subtract_float_memory(result,b32);break;
            case 3: result=fp.reverse_subtract_float_memory(result,b32);break;
            case 4: result=fp.divide_float_memory(result,b32);break;
            case 5: result=fp.reverse_divide_float_memory(result,b32);break;
            case 6: output[3]=sadx::x87_comparison_status(fp.compare_float_memory(result,b32));break;
            }
        } else {
            switch(operation) {
            case 0: result=fp.add_double_memory(result,b64);break;
            case 1: result=fp.multiply_double_memory(result,b64);break;
            case 2: result=fp.subtract_double_memory(result,b64);break;
            case 3: result=fp.reverse_subtract_double_memory(result,b64);break;
            case 4: result=fp.divide_double_memory(result,b64);break;
            case 5: result=fp.reverse_divide_double_memory(result,b64);break;
            case 6: output[3]=sadx::x87_comparison_status(fp.compare_double_memory(result,b64));break;
            }
        }
        output[0]=static_cast<std::uint32_t>(result.significand());
        output[1]=static_cast<std::uint32_t>(result.significand()>>32U);
        output[2]=result.sign_exponent();output[4]=fp.exception_flags();
        return 0;
    } catch (const sadx::X87Error&) { return 2; }
}

#include "cpu/crt_integer.hpp"
#include <cstdint>
#include <cstring>

extern "C" int sadx_native_crt_integer(std::uint64_t input_bits,
                                        std::uint16_t control_word,
                                        std::uint64_t* output,
                                        std::uint16_t* exception_flags) {
    if (!output || !exception_flags) return 1;
    try {
        double value;
        std::memcpy(&value,&input_bits,8);
        sadx::X87Context context(control_word);
        *output=sadx::crt_ftol(context.from_double(value),context);
        *exception_flags=context.exception_flags();
        return 0;
    } catch (const sadx::X87Error&) { return 2; }
}

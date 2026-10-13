#include "cpu/crt_integer.hpp"
#include <cstring>

namespace sadx {
namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result,&value,4);
    return result;
}
}

std::uint64_t crt_ftol(X87Value value, X87Context& context) {
    const auto classification = bits(context.to_float(value)); // FST, retain ST0
    const auto rounded = context.to_int64_current_rounding(value); // FISTP64
    const auto reloaded = context.from_int64(rounded); // exact FILD64
    std::uint64_t result;
    std::memcpy(&result,&rounded,8);
    const auto low = static_cast<std::uint32_t>(result);
    const auto high = static_cast<std::uint32_t>(result >> 32);
    if (low != 0 || (high & 0x7fffffffU) != 0) {
        auto difference = bits(context.to_float(context.subtract(value,reloaded)));
        const bool negative = (classification & 0x80000000U) != 0;
        if (negative) difference ^= 0x80000000U;
        const bool carry = static_cast<std::uint64_t>(difference) + 0x7fffffffU > 0xffffffffULL;
        if (carry) result = negative ? result + 1 : result - 1;
    } else {
        // The source's zero/indefinite path discards these two registers via
        // binary32 stores; those stores can still affect exception flags.
        (void)context.to_float(reloaded);
        (void)context.to_float(value);
    }
    return result;
}

std::uint32_t crt_ftol_low32(X87Value value, X87Context& context) {
    return static_cast<std::uint32_t>(crt_ftol(value,context));
}

} // namespace sadx

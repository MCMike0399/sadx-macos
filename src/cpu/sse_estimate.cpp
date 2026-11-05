#include "cpu/sse_estimate.hpp"

namespace sadx {
Sse32 reciprocal_sqrt_by_division(Sse32 source, const SseContext &arithmetic) {
    constexpr std::uint32_t kRoundingAndDaz = (3U << 13) | (1U << 6);
    constexpr std::uint32_t kMaskedExceptions = 63U << 7;
    SseContext scratch((arithmetic.mxcsr() & kRoundingAndDaz) | kMaskedExceptions);
    const auto one = scratch.from_int32(1);
    const auto root = scratch.sqrt_single(source);
    return scratch.divide(one, root);
}
} // namespace sadx

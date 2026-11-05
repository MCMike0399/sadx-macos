#include "engine/vector_normalize.hpp"
#include "cpu/sse_estimate.hpp"
#include <cstring>

extern "C" int sadx_native_normalize(const std::uint32_t *input, std::uint32_t mxcsr,
                                      unsigned alias, std::uint32_t *output) {
    std::array<float, 3> vector{}, result{};
    std::memcpy(vector.data(), input, 12);
    std::memcpy(result.data(), output, 12);
    sadx::SseContext fp(mxcsr);
    int fault = 0;
    try {
        if (alias) sadx::normalize_vector(vector, vector, fp);
        else sadx::normalize_vector(result, vector, fp);
    } catch (const sadx::SseError &error) {
        fault = error.exceptions();
    }
    std::memcpy(output, alias ? vector.data() : result.data(), 12);
    output[3] = fp.mxcsr();
    return fault;
}

extern "C" std::uint32_t sadx_native_normalize_rsqrt(std::uint32_t input, std::uint32_t mxcsr,
                                                    std::uint32_t *status) {
    sadx::SseContext fp(mxcsr);
    const auto result = sadx::reciprocal_sqrt_by_division({input}, fp);
    *status = fp.mxcsr();
    return result.bits;
}

#include "engine/vector_normalize.hpp"
#include "cpu/sse_estimate.hpp"
#include <cstring>

namespace sadx {
namespace {
// NOTSA: preserve NaN payloads when moving between game vectors and SSE words.
Sse32 word(float value) {
    Sse32 result{};
    std::memcpy(&result.bits, &value, sizeof(value));
    return result;
}
// NOTSA: store raw SSE results without host arithmetic or NaN conversion.
void store(float &output, Sse32 value) {
    std::memcpy(&output, &value.bits, sizeof(output));
}
} // namespace
void normalize_vector(std::array<float, 3> &output, const std::array<float, 3> &input,
                      SseContext &arithmetic) {
    const auto x = word(input[0]);
    const auto y = word(input[1]);
    const auto z = word(input[2]);
    auto length_squared = arithmetic.multiply(x, x);
    const auto y_squared = arithmetic.multiply(y, y);
    length_squared = arithmetic.add(length_squared, y_squared);
    const auto z_squared = arithmetic.multiply(z, z);
    length_squared = arithmetic.add(length_squared, z_squared);
    // COMISS/JBE sends unordered inputs through the refinement, too.
    if (arithmetic.comiss(word(1.4210855e-14f), length_squared) == SseComparison::greater) {
        output = {0, 0, 0};
        return;
    }
    const auto reciprocal_length = reciprocal_sqrt_by_division(length_squared, arithmetic);
    auto correction = arithmetic.multiply(length_squared, reciprocal_length);
    const auto half_reciprocal = arithmetic.multiply(word(0.5f), reciprocal_length);
    correction = arithmetic.multiply(correction, reciprocal_length);
    correction = arithmetic.subtract(word(3.0f), correction);
    const auto scale = arithmetic.multiply(half_reciprocal, correction);
    store(output[0], arithmetic.multiply(scale, x));
    store(output[1], arithmetic.multiply(scale, y));
    store(output[2], arithmetic.multiply(scale, z));
}
} // namespace sadx

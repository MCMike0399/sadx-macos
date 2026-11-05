#pragma once
#include "cpu/sse_math.hpp"
#include <array>

namespace sadx {
// Loads all inputs before writing; output may alias input. Each component store
// follows its multiply, preserving the prefix if an unmasked exception occurs.
// FUNCTION: SONIC 0x775630 D3DXVec3Normalize_SSE
void normalize_vector(std::array<float, 3> &output, const std::array<float, 3> &input,
                      SseContext &arithmetic);
} // namespace sadx

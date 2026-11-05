#pragma once
#include "cpu/sse_math.hpp"

namespace sadx {
// NOTSA: Unicorn/QEMU RSQRTSS model: rounded single sqrt then division,
// selected rounding/DAZ, discarded flags. Not a measured Intel CPU estimate.
Sse32 reciprocal_sqrt_by_division(Sse32 source, const SseContext &arithmetic);
} // namespace sadx

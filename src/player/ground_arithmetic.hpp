#pragma once
#include "cpu/fidelity_float.hpp"

namespace sadx::physics {
// NOTSA: semantic comparison predicates preserve ordered and unordered source branches.
inline bool less(X87Comparison value) { return value == X87Comparison::less; }
inline bool greater(X87Comparison value) { return value == X87Comparison::greater; }
inline bool equal(X87Comparison value) { return value == X87Comparison::equal; }
// NOTSA: the source's parity branch includes unordered along with greater.
inline bool greater_or_unordered(X87Comparison value) {
    return value == X87Comparison::greater || value == X87Comparison::unordered;
}
// NOTSA: recovered flag roles; unnamed source attributes retain behavioral names.
constexpr std::uint16_t kEventMove = 16384;
constexpr std::uint16_t kResetDirection = 8192;
constexpr std::uint32_t kAttributeAcceleration = 131072;
constexpr std::uint32_t kAlternateAcceleration = 16384;
constexpr std::uint32_t kAttributeRotation = 8388608;
constexpr std::uint32_t kGroundTraction = 4;
} // namespace sadx::physics

#pragma once
#include "cpu/fidelity_float.hpp"
#include "math/ninja_math.hpp"

namespace sadx {
// FUNCTION: SONIC 0x787cc0 njScalor
fidelity::ExactFloat vector_length(const NinjaVector &, fidelity::Context &);
} // namespace sadx

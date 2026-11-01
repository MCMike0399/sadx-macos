#pragma once
#include "cpu/fidelity_float.hpp"
#include "math/ninja_math.hpp"

namespace sadx {
// NOTSA: typed vector wrapper; njScalor provenance belongs to the CPU return operation.
fidelity::ExactFloat vector_length(const NinjaVector &, fidelity::Context &);
} // namespace sadx

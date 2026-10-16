#pragma once
#include <array>

namespace sadx {
using NinjaQuarterSineTable = std::array<float, 1025>;
// NOTSA: each native math owner receives its own maintained quarter-wave coefficients.
NinjaQuarterSineTable ninja_sine_table();
} // namespace sadx

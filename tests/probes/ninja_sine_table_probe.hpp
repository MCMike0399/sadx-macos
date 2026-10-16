#pragma once
#include "math/ninja_sine_table.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
std::array<std::uint32_t, 1025> sine_words(const NinjaQuarterSineTable &);
NinjaQuarterSineTable varied_sine_table();
} // namespace sadx::test

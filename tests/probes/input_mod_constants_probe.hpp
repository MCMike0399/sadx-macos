#pragma once
#include "input/input_mod_constants.hpp"
#include <array>
#include <cstdint>
namespace sadx::test {
std::array<std::uint32_t, 7> input_constant_words(const InputModPadConstants &);
InputModPadConstants varied_input_constants();
} // namespace sadx::test

#pragma once
#include "input/input_initial_state.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
std::array<std::uint8_t, 1869> input_initial_bytes(const input::InputInitialState &);
input::InputInitialState varied_input_initial_state();
} // namespace sadx::test

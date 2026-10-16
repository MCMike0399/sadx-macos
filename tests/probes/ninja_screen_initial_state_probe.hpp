#pragma once
#include "engine/ninja_screen_initial_state.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
std::array<std::uint32_t, 90> screen_initial_words(const NinjaScreenInitialState &);
NinjaScreenInitialState varied_screen_initial_state();
} // namespace sadx::test

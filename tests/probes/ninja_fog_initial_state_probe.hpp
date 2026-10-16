#pragma once
#include "engine/ninja_fog_initial_state.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
std::array<std::uint32_t, 398> fog_initial_words(const NinjaFogInitialState &);
NinjaFogInitialState varied_fog_initial_state();
} // namespace sadx::test

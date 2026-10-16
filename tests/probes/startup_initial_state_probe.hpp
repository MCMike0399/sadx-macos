#pragma once
#include "game/startup_initial_state.hpp"
namespace sadx::test {
std::array<std::uint32_t, 41> startup_initial_words(const StartupInitialState &);
StartupInitialState varied_startup_initial_state();
} // namespace sadx::test

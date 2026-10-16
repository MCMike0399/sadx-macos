#pragma once
#include "game/game_view_initial_state.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
std::array<std::uint32_t, 30> view_initial_words(const GameViewInitialState &);
GameViewInitialState varied_view_initial_state();
} // namespace sadx::test

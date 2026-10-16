#pragma once
#include "game/sound_initial_state.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
std::array<std::uint32_t, 329> sound_initial_words(const SoundInitialState &);
SoundInitialState varied_sound_initial_state();
} // namespace sadx::test

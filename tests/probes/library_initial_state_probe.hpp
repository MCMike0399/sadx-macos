#pragma once
#include "game/library_initial_state.hpp"
namespace sadx::test {
std::array<std::uint32_t, 28> library_initial_words(const LibraryInitialState &);
LibraryInitialState varied_library_initial_state();
} // namespace sadx::test

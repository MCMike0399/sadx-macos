#pragma once
#include "engine/ninja_queue_initial_state.hpp"
#include <array>
#include <cstdint>
namespace sadx::test {
std::array<std::uint32_t, 21> queue_initial_words(const NinjaQueueInitialState &);
NinjaQueueInitialState varied_queue_initial_state();
} // namespace sadx::test

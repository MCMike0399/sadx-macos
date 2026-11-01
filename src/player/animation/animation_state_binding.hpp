#pragma once
#include "player/animation/animation.hpp"

namespace sadx::animation::detail {
enum class Multiplier { Speed, Charge };
// NOTSA: the existing player allocation owns both live aliases. This binding
// borrows its identity rather than caching either scalar in animation state.
float read_live_multiplier(const View &, TaskWorkArena *, Multiplier, Phase);
} // namespace sadx::animation::detail

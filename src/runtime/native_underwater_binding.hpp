#pragma once

#include "engine/task.hpp"
#include "player/player_initialization.hpp"

namespace sadx {
struct PlayerPhysicsState;
}

namespace sadx::native {
class Display;
struct PlayerSystem;

// NOTSA: the shared RunUnderwaterStuff product owner for the Gamma and Big
// callbacks. Both cores publish the same primary, player, motion and display
// shapes, so the product services and player view assembly live once here.
// The caller keeps its own source identity check and physics boundary, so each
// binding reports the same stops under its own name.
void run_underwater(PlayerSystem &, Display &, const TaskHandle &,
                    const player_initialization::PrimaryView &, PlayerPhysicsState &,
                    const player_initialization::PlayerHandle &);
} // namespace sadx::native

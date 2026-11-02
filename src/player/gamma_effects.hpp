#pragma once

#include "engine/particle_pool.hpp"

namespace sadx::gamma_effects {
struct Asset {
    texture_lifetime::List *textures = nullptr;
    // DATA: SONIC 0x91d168 GammaHitFrames
    std::array<particles::Frame, 17> hit;
    // DATA: SONIC 0x91d458 GammaBurstFrames
    std::array<particles::Frame, 5> burst;
};
struct State {
    // GLOBAL: SONIC 0x3c56df0 GammaBurstTask size=4
    TaskHandle burst_task;
};
// NOTSA: decode pointer-free imported declarations using the existing E102HIT list.
Asset decode(const std::vector<std::uint8_t> &, texture_lifetime::List &);
// FUNCTION: SONIC 0x49f9f0 InitializeGammaBurst
void initialize_burst(State &, Asset &, TaskScheduler &, TaskWorkArena &, fidelity::Context &);
// NOTSA: complete typed GammaInitializeEffects body; its entry annotation stays
// on gamma_selection::initialize_effects, the existing caller seam.
void initialize(State &, Asset &, TaskScheduler &, TaskWorkArena &, fidelity::Context &);
} // namespace sadx::gamma_effects

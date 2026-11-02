#pragma once

#include "engine/sprite_definition.hpp"
#include "engine/sprite_registry.hpp"

namespace sadx::gamma_setup {
// The particle callback consumes growth, animation speed, drag and acceleration.
// Remaining source parameter slots have no established use in this closure.
struct ParticleParameters {
    float leading_parameter = 0;
    float scale_growth = 0;
    float animation_speed = 0;
    float drag = 0;
    float vertical_acceleration = 0;
    std::array<float, 10> remaining{};
};
struct Asset {
    // DATA: SONIC 0x9820f8 GammaSetupDefinition
    sprite_registry::Definition definition;
    // DATA: SONIC 0x98210c GammaSetupParticleParameters
    ParticleParameters parameters;
};
using DefinitionPointer = const sprite_registry::Definition *;
struct State {
    // GLOBAL: SONIC 0x982148 GammaSetupDefinitionPointer size=4
    DefinitionPointer definition = nullptr;
    // GLOBAL: SONIC 0x982154 GammaSetupSprite size=4
    sprite_registry::NodeHandle sprite;
};
// NOTSA: pointer-free imported declaration, bound to the canonical E102EFFECT list.
// Parsing fails before publication; no source image or address is needed at runtime.
Asset decode(const std::vector<std::uint8_t> &, const texture_lifetime::List &);
// NOTSA: typed body of GammaLoadSetupResource, whose source entry annotation
// remains on gamma_selection::load_setup_resource until Root binds that seam.
void load_resource(State &, sprite_registry::State &, TaskWorkArena &);
} // namespace sadx::gamma_setup

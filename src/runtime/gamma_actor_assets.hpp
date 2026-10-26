#pragma once
#include "engine/character_pack.hpp"
#include "player/player_initialization.hpp"
#include "player/player_animation.hpp"
#include <string>

namespace sadx::native {
// NOTSA: imported content keeps genuine NULL distinct from an unavailable payload.
enum class ActorResourceStatus { Loaded, KnownNull, Unavailable };
struct GammaActionDeclaration {
    ActorResourceStatus status = ActorResourceStatus::Unavailable;
    ActionHandle action;
    std::string boundary;
};
struct GammaAnimationRecipe {
    GammaActionDeclaration resource;
    std::uint8_t object_count = 0;
    std::int8_t mode = 0;
    std::uint16_t next = 0;
    float transition = 0, speed = 0;
};
// NOTSA: one imported table, shared by all native actors using its recipes.
struct GammaActorAssets {
    player_initialization::ParameterTable parameters{};
    std::array<GammaActionDeclaration, 78> declarations{};
    std::array<GammaAnimationRecipe, 91> recipes{};
    std::shared_ptr<const PlayerAnimationTable> animation_table;
    ActionHandle stand;
    std::array<std::uint32_t, 36> collision_words{};
    std::array<std::uint32_t, 2> line_colors{};
};
} // namespace sadx::native

#pragma once
#include "engine/character_pack.hpp"
#include "player/gamma_cursors.hpp"
#include "player/player_join_vertices.hpp"
#include "engine/collision_component.hpp"
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
// NOTSA: imported pointer declarations retain ResourceIds until bound to the shared scene.
struct GammaJoinDeclaration {
    character_pack::ResourceId object = 0, source_object = 0, destination_object = 0;
    std::uint8_t vertex_count = 0;
    std::int8_t input_mode = 0;
    std::uint8_t source_depth = 0, destination_depth = 0;
};
struct GammaCursorDeclaration {
    character_pack::ResourceId points = 0, colors = 0, indices = 0;
    std::int16_t line_count = 0;
    std::uint16_t padding = 0;
};
// NOTSA: one imported table, shared by all native actors using its recipes.
struct GammaActorAssets {
    player_initialization::ParameterTable parameters{};
    std::array<GammaActionDeclaration, 78> declarations{};
    std::array<GammaAnimationRecipe, 91> recipes{};
    std::shared_ptr<const PlayerAnimationTable> animation_table;
    ActionHandle stand;
    std::vector<collision_component::Info> collision_info;
    std::array<std::uint32_t, 2> line_colors{};
    std::array<GammaJoinDeclaration, 5> joins{};
    std::array<GammaCursorDeclaration, 2> cursors{};
};
} // namespace sadx::native

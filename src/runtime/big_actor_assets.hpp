#pragma once
#include "runtime/gamma_actor_assets.hpp"

#include <array>
#include <vector>

namespace sadx::native {
// NOTSA: Big shares the imported declaration and recipe shapes with Gamma;
// only the weld rows carry an additional vertex-pair array identity.
using BigActionDeclaration = GammaActionDeclaration;
using BigAnimationRecipe = GammaAnimationRecipe;
struct BigJoinDeclaration {
    character_pack::ResourceId object = 0, source_object = 0, destination_object = 0;
    std::uint8_t vertex_count = 0;
    std::int8_t input_mode = 0;
    std::uint8_t source_depth = 0, destination_depth = 0;
    character_pack::ResourceId vertex_pairs = 0;
};
// NOTSA: one imported table, shared by the Big preview actor.
struct BigActorAssets {
    std::array<BigActionDeclaration, 90> declarations{};
    std::array<BigAnimationRecipe, 105> recipes{};
    std::shared_ptr<const animation::Table> animation_table;
    ActionHandle stand;
    std::array<BigJoinDeclaration, 17> joins{};
    // Decoded pair arrays, parallel to joins; empty where the row has none.
    std::array<std::vector<std::uint16_t>, 17> join_pairs{};
    std::vector<collision_component::Info> collision_info;
};
} // namespace sadx::native

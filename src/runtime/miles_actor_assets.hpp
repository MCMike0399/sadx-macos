#pragma once
#include "runtime/gamma_actor_assets.hpp"

#include <array>
#include <vector>

namespace sadx::native {
// NOTSA: Miles shares the imported declaration and recipe shapes with Gamma;
// only the weld rows carry an additional vertex-pair array identity.
using MilesActionDeclaration = GammaActionDeclaration;
using MilesAnimationRecipe = GammaAnimationRecipe;
struct MilesJoinDeclaration {
    character_pack::ResourceId object = 0, source_object = 0, destination_object = 0;
    std::uint8_t vertex_count = 0;
    std::int8_t input_mode = 0;
    std::uint8_t source_depth = 0, destination_depth = 0;
    character_pack::ResourceId vertex_pairs = 0;
};
// NOTSA: one imported table, shared by the Miles preview actor.
struct MilesActorAssets {
    std::array<MilesActionDeclaration, 114> declarations{};
    std::array<MilesAnimationRecipe, 136> recipes{};
    std::shared_ptr<const animation::Table> animation_table;
    ActionHandle stand;
    std::array<MilesJoinDeclaration, 30> joins{};
    // Decoded pair arrays, parallel to joins; empty where the row has none.
    std::array<std::vector<std::uint16_t>, 30> join_pairs{};
    // MILES_OBJECTS rows 28/31/48/51 receive sibling identities; rows 62/63
    // receive a model identity. The tail rewrites weld rows 8/9/20/21.
    std::array<character_pack::ResourceId, 6> tail_objects{};
    std::array<character_pack::ResourceId, 8> sibling_objects{};
    std::array<character_pack::ResourceId, 3> tail_models{};
    std::vector<collision_component::Info> collision_info;
};
} // namespace sadx::native

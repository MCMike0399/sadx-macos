#pragma once
#include "runtime/gamma_actor_assets.hpp"

#include <array>
#include <vector>

namespace sadx::native {
// NOTSA: Amy shares the imported declaration and recipe shapes with Gamma;
// only the weld rows carry an additional vertex-pair array identity.
using AmyActionDeclaration = GammaActionDeclaration;
using AmyAnimationRecipe = GammaAnimationRecipe;
struct AmyJoinDeclaration {
    character_pack::ResourceId object = 0, source_object = 0, destination_object = 0;
    std::uint8_t vertex_count = 0;
    std::int8_t input_mode = 0;
    std::uint8_t source_depth = 0, destination_depth = 0;
    character_pack::ResourceId vertex_pairs = 0;
};
// NOTSA: one imported table, shared by the Amy preview actor.
struct AmyActorAssets {
    std::array<AmyActionDeclaration, 80> declarations{};
    std::array<AmyAnimationRecipe, 102> recipes{};
    std::shared_ptr<const animation::Table> animation_table;
    ActionHandle stand;
    std::array<AmyJoinDeclaration, 18> joins{};
    // Decoded pair arrays, parallel to joins; empty where the row has none.
    std::array<std::vector<std::uint16_t>, 18> join_pairs{};
    // AMY_OBJECTS rows 36..38: row 36 receives one of rows 37/38 as its sibling.
    std::array<character_pack::ResourceId, 3> tail_objects{};
    std::vector<collision_component::Info> collision_info;
};
} // namespace sadx::native

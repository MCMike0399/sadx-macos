#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/character_pack.hpp"
#include "engine/scene_resource_reference.hpp"
#include "player/animation/animation_content.hpp"
#include "player/animation/animation_outcome.hpp"
#include "player/player_scene_resources.hpp"

namespace sadx {
struct PlayerPhysicsState;
struct PlayerAnimationTail;
} // namespace sadx
namespace sadx::animation {
struct ResourceReference {
    std::optional<std::size_t> id;
};
struct Action {
    SceneResourceReference object;
    ResourceReference motion;
};
struct Motion {
    std::uint32_t frame_count = 0;
    std::uint16_t type = 0, input_function = 0;
    ResourceReference data;
};
struct Node {
    std::array<ResourceReference, 3> keys{};
    std::array<std::uint32_t, 3> counts{};
    // Original MDATA3 sampling consumes the scale pointer word as position count.
    // Resource identity and raw source bits must remain separate.
    std::uint32_t raw_scale_pointer_bits = 0;
};
struct KeyBytes {
    std::shared_ptr<const std::vector<std::uint8_t>> bytes;
    std::size_t offset = 0, byte_extent = 0;
};
struct Resources {
    std::unordered_map<std::size_t, Action> actions;
    std::unordered_map<std::size_t, Motion> motions;
    std::unordered_map<std::size_t, std::vector<Node>> node_arrays;
    std::unordered_map<std::size_t, KeyBytes> key_arrays;
    const std::unordered_map<std::size_t, SceneGeometryObject> *objects = nullptr;
};
// NOTSA: borrow the existing physics/tail owner without a copied animation snapshot.
struct View {
    PlayerPhysicsState &player;
    TaskWorkHandle player_allocation;
};
struct Runtime {
    TaskWorkArena *arena;
    const Resources &resources;
    const character_pack::ObjectState *scene_objects = nullptr;
};
Outcome update(View &, Runtime &, fidelity::Context &);
Outcome reset_frame(View &, const Runtime &, fidelity::Context &);
Outcome next(View &, const Runtime &, std::uint16_t from, fidelity::Context &);
Outcome release_keys(View &, Runtime &);
Outcome blend(View &, Runtime &, fidelity::Context &);
Outcome build_transition(View &, Runtime &, fidelity::Context &);
} // namespace sadx::animation

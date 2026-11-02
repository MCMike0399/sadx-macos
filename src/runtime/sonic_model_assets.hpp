#pragma once
#include "engine/collision_component.hpp"
#include "player/sonic_model.hpp"

namespace sadx::native {
// NOTSA: pointer-free declarations from the existing sonic-actor metadata.
struct SonicJoinDeclaration {
    character_pack::ResourceId object = 0, source = 0, destination = 0;
    std::uint8_t count = 0;
    std::int8_t mode = 0;
    std::uint8_t source_depth = 0, destination_depth = 0;
    std::size_t pairs = 0;
};
struct SonicModelDeclarations {
    character_pack::ResourceId attach = 0, root = 0, join_target = 0;
    std::array<character_pack::ResourceId, 3> direct_ahead{};
    std::array<SonicJoinDeclaration, 8> metal_joins{};
    std::array<SonicJoinDeclaration, 37> joins{};
    std::vector<std::vector<std::uint16_t>> pairs;
    std::vector<collision_component::Info> collision;
};
// NOTSA: owns only source Sonic join/cache state. Objects, geometry, materials,
// parameters, animations and collision allocations retain their existing owners.
class SonicModelAssets {
  public:
    SonicModelAssets(const character_pack::CharacterPack &, SonicModelDeclarations);
    SonicModelAssets(const SonicModelAssets &) = delete;
    SonicModelAssets &operator=(const SonicModelAssets &) = delete;
    void bind_scene(character_pack::ObjectState &);
    character_pack::ObjectState &objects() const;
    sonic_model::State &state() { return state_; }
    character_pack::ResourceId attach() const { return declarations_.attach; }
    character_pack::ResourceId root() const { return declarations_.root; }
    character_pack::ResourceId join_target() const { return declarations_.join_target; }
    const std::vector<collision_component::Info> &collision() const {
        return declarations_.collision;
    }

  private:
    friend struct SonicModelProbeAccess; // NOTSA: read canonical pair storage in isolated judges.
    SonicModelDeclarations declarations_;
    sonic_model::State state_;
    character_pack::ObjectState *objects_ = nullptr;
};
} // namespace sadx::native

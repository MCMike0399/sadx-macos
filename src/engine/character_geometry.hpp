#pragma once
#include "engine/character_pack.hpp"

namespace sadx::character_pack {
// NOTSA: one typed array per imported resource identity, shared by all model users.
// Runtime deformation writes these vector words; immutable pack payloads stay unchanged.
class GeometryState {
  public:
    explicit GeometryState(const CharacterPack &pack);
    GeometryState(const GeometryState &) = delete;
    GeometryState &operator=(const GeometryState &) = delete;
    std::vector<std::uint32_t> &vector_words(ResourceId resource);
    const std::vector<std::uint32_t> &vector_words(ResourceId resource) const;
    std::vector<std::uint32_t> &color_words(ResourceId resource);
    const std::vector<std::uint32_t> &color_words(ResourceId resource) const;
    const std::vector<std::int16_t> &indices(ResourceId resource) const;
    const std::vector<std::int16_t> &texture_coordinates(ResourceId resource) const;

  private:
    const CharacterPack &pack_;
    std::vector<std::vector<std::uint32_t>> vectors_;
    std::vector<std::vector<std::int16_t>> shorts_;
};
// NOTSA: mutable imported character resources share one scene lifetime across actor callbacks.
struct SceneState {
    explicit SceneState(const CharacterPack &pack) : objects(pack), geometry(pack) {}
    ObjectState objects;
    GeometryState geometry;
};
} // namespace sadx::character_pack

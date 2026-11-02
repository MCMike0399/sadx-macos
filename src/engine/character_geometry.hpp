#pragma once
#include "engine/character_pack.hpp"
#include "engine/character_material.hpp"

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
    // NOTSA: immutable model metadata selects this owner's mutable vector arrays.
    ModelResource model(ResourceId resource) const { return pack_.model(resource); }

  private:
    const CharacterPack &pack_;
    std::vector<std::vector<std::uint32_t>> vectors_;
    std::vector<std::vector<std::int16_t>> shorts_;
};
// NOTSA: a single fixed-size material array for each imported ResourceId. No
// array can be resized after publication, so headlight and model aliases stay valid.
class MaterialState {
  public:
    explicit MaterialState(const CharacterPack &pack);
    MaterialState(const MaterialState &) = delete;
    MaterialState &operator=(const MaterialState &) = delete;
    SceneMaterial &material(ResourceId resource, std::size_t index);
    const SceneMaterial &material(ResourceId resource, std::size_t index) const;

  private:
    const CharacterPack &pack_;
    std::vector<std::vector<SceneMaterial>> arrays_;
};
// NOTSA: mutable imported character resources share one scene lifetime across actor callbacks.
struct SceneState {
    explicit SceneState(const CharacterPack &pack) : objects(pack), geometry(pack), materials(pack) {}
    ObjectState objects;
    GeometryState geometry;
    MaterialState materials;
};
} // namespace sadx::character_pack

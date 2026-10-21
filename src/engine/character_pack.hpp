#pragma once
#include "engine/character_resources.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace sadx::character_pack {
// NOTSA: typed, pointer-free imported Ninja resources; zero is a null resource reference.
using ResourceId = std::uint32_t;
enum class StartupAction : std::uint32_t { ClearPose = 100, ClearPoseTransition = 101 };
enum class TitleAction : std::uint32_t {
    SonicClearPose, SonicClearPoseTransition, KnucklesClearPose, KnucklesClearPoseTransition,
    BigSittingToFish, BigPreparingToCast, BigCast
};
// NOTSA: semantic bindings for the original selection action sequences.
enum class SelectionAction : std::uint32_t {
    SonicIdle, SonicSelected, SonicSettled,
    MilesIdle, MilesSelected, MilesSettled,
    KnucklesIdle, KnucklesSelected, KnucklesSettled,
    AmyIdle, AmySelected, AmySettled,
    BigIdle, BigPrepare, BigCast, BigSettled,
    GammaIdle, GammaSelected, GammaSettled
};
enum class ResourceKind : std::uint32_t {
    Action = 1,
    Object,
    Model,
    Meshsets,
    Materials,
    Vectors,
    Colors,
    Uv,
    Indices,
    Attributes,
    Motion,
    Tracks,
    PositionKeys,
    RotationKeys
};
struct ActionResource {
    ResourceId object;
    ResourceId motion;
};
struct ObjectResource {
    std::uint32_t evaluation_flags;
    ResourceId model;
    std::array<float, 3> position;
    std::array<std::int32_t, 3> rotation;
    std::array<float, 3> scale;
    ResourceId child;
    ResourceId sibling;
};
struct ModelResource {
    ResourceId points;
    ResourceId normals;
    std::uint32_t vertex_count;
    ResourceId meshsets;
    ResourceId materials;
    std::uint16_t mesh_count;
    std::uint16_t material_count;
    std::array<float, 3> center;
    float radius;
};
struct MotionResource {
    ResourceId tracks;
    std::uint32_t frame_count;
    std::uint16_t channels;
    std::uint16_t interpolation;
    std::uint32_t part_count;
};
struct ResourceRecord {
    ResourceKind kind;
    std::vector<std::uint8_t> payload;
};

// NOTSA: owns imported data independently of executable images and process pointer widths.
// Loading validates every record, reference, traversal and contextual array length before publication.
class CharacterPack {
  public:
    static CharacterPack load(const std::filesystem::path& path);
    static CharacterPack decode(const std::vector<std::uint8_t>& bytes);
    ActionHandle action(StartupAction action) const;
    ActionHandle action(TitleAction action) const;
    ActionHandle action(SelectionAction action) const;
    ActionResource resolve(ActionHandle handle) const;
    ObjectResource object(ResourceId id) const;
    ModelResource model(ResourceId id) const;
    MotionResource motion(ResourceId id) const;
    const ResourceRecord& record(ResourceId id) const;
    std::size_t record_count() const { return records_.size(); }

  private:
    void validate() const;
    std::vector<ResourceRecord> records_;
    enum class Schema { Startup, Title, Selection };
    Schema schema_ = Schema::Startup;
    std::vector<std::array<std::uint32_t, 4>> actions_;
};
} // namespace sadx::character_pack

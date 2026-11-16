#pragma once

#include "cpu/fidelity_float.hpp"
#include "math/ninja_math.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sadx::character_shadow {
// The task half of CharacterShadow: the player's pose and the effect number the
// ripple call takes from taskwk +0x8.
struct Body {
    float x = 0;
    float y = 0;
    float z = 0;
    std::int32_t yaw = 0;           // taskwk +0x18
    std::uint8_t player_number = 0; // taskwk +0x8, PSetRippleEffect's pno
};

// The player's shadowwk. Attributes are the current frame's contacts; the
// prior attributes are last frame's, rotated at the start of the update.
struct ShadowWork {
    std::int32_t &angle_x;
    std::int32_t &angle_z;
    std::uint32_t &top_attributes;
    std::uint32_t &bottom_attributes;
    float &top_y;
    float &bottom_y;
    std::uint32_t &prior_top_attributes;
    std::uint32_t &prior_bottom_attributes;
    // shadowwk scl is not written by CharacterShadow; it is borrowed so the
    // caller and oracle can observe that the whole record is untouched.
    float &scale;
};

// One loaded land candidate. The object identity is opaque here; the culling
// reads the recorded world position, scale and model radius, and the polygon
// seams receive the identity unchanged.
struct LandCandidate {
    std::uint32_t attributes = 0;
    std::uintptr_t object = 0;
    float object_x = 0;
    float object_z = 0;
    float scale_x = 0;
    float scale_y = 0;
    float scale_z = 0;
    float radius = 0;
    // The land entry's task pointer is non-null: it enables the distance cull.
    bool has_task = false;
    // Present when that task has force work; its vertical position speed shifts
    // both polygon hits.
    std::optional<float> vertical_offset = {};
};

// A borrowed candidate table, owned by the caller for the duration of the call.
struct LandCandidates {
    const LandCandidate *data = nullptr;
    std::size_t count = 0;
    std::size_t size() const { return count; }
    const LandCandidate *begin() const { return data; }
    const LandCandidate *end() const { return data + count; }
};

// What a polygon query reports about the ray through the shadow point. lower is
// the bottom intersection and upper the top one, matching the source carry.
struct ShadowHit {
    bool found = false;
    bool lower_found = false;
    bool upper_found = false;
    float lower_position = 0;
    float upper_position = 0;
    std::int32_t lower_angle_x = 0;
    std::int32_t lower_angle_z = 0;
};

// NOTSA: callees CharacterShadow consumes. The land walk and the two polygon
// queries are unrecovered; the effect calls are product boundaries.
struct Services {
    virtual ~Services() = default;
    // STUB: SONIC 0x43acd0 ListGroundForCollision
    virtual LandCandidates list_land(const NinjaVector &point, float radius) = 0;
    // njScalor: the source returns an x87 register, so the seam retains it.
    virtual fidelity::ExactFloat horizontal_distance(const NinjaVector &offset) = 0;
    // STUB: SONIC 0x456510 GetZxShadowOnFDPolygon
    virtual ShadowHit get_zx_shadow(const NinjaVector &point, const LandCandidate &candidate) = 0;
    // STUB: SONIC 0x452b30 TestShadowPolygon
    virtual ShadowHit polygon_test(const NinjaVector &point, const LandCandidate &candidate) = 0;
    // STUB: SONIC 0x440890 PSetSplashEffect
    virtual void splash() = 0;
    // STUB: SONIC 0x4407c0 PSetRippleEffect
    virtual void ripple(std::uint8_t player_number, const NinjaVector &position,
                        const std::array<std::int32_t, 3> &angles) = 0;
    // STUB: SONIC 0x49f0b0 DrawCharacterShadow
    virtual void draw_shadow() = 0;
};

// FUNCTION: SONIC 0x49f1a0 CharacterShadow
void update(const Body &body, ShadowWork &work, Services &services,
            fidelity::Context &arithmetic);
} // namespace sadx::character_shadow

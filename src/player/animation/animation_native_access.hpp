#pragma once

#include "player/animation/animation.hpp"
#include "player/player_physics.hpp"

namespace sadx::animation::detail {
// NOTSA: native binding borrows the existing state, scene, arena and partial malloc headers.
// A successful player calloc produces the state fields; no generic fixture default is supplied.
struct NativeAccess {
    using PlayerAnimationView = View;
    using PlayerAnimationRuntime = Runtime;
    using PlayerAnimationRecord = Record;
    using PlayerAnimationAction = Action;
    using PlayerAnimationMotion = Motion;
    using PlayerAnimationTable = Table;
    using PlayerAnimationNode = Node;
    using PlayerAnimationKeyBytes = KeyBytes;
    using PlayerAnimationAllocatedAction = sadx::PlayerAnimationAllocatedAction;
    using PlayerAnimationAllocatedMotion = sadx::PlayerAnimationAllocatedMotion;
    using AnimationResourceReference = ResourceReference;

    // NOTSA: non-owning alternatives preserve existing development fixtures while
    // native callers borrow the authoritative mutable ObjectState directly.
    struct SceneObject {
        const character_pack::ObjectResource *native = nullptr;
        const SceneGeometryObject *reference = nullptr;
    };
    SceneObject object(const SceneResourceReference &, const Runtime &, Phase);
    SceneResourceReference child(const SceneObject &, Phase);
    SceneResourceReference sibling(const SceneObject &, Phase);
    std::array<std::uint32_t, 3> pose(const SceneObject &, unsigned channel, Phase);

    // NOTSA: native arithmetic uses the pinned compiler literals; no constant-pool state.
    float zero(const Runtime &) const { return 0.0f; }
    // NOTSA: the reference binding separately preserves its supplied-value contract.
    float one(const Runtime &) const { return 1.0f; }
    // NOTSA: unsigned32 conversion correction for the original signed-load sequence.
    float unsigned_correction(const Runtime &) const { return 4294967296.0f; }

    struct StateFields {
        std::optional<std::uint16_t> &current, &requested, &previous;
        std::optional<std::int16_t> &mode;
        std::optional<float> &start, &frame;
        std::optional<std::shared_ptr<sadx::PlayerAnimationAllocatedAction>> &buffer;
        std::optional<std::uint16_t> &flags, &timer, &next;
    };
    StateFields state(const View &);
    template <class T> const T &read(const T &value, Phase) { return value; }
    template <class T> const T &read(const std::optional<T> &value, Phase phase) {
        if (!value)
            fail(Failure::UnavailableField, phase);
        return *value;
    }
    std::int8_t read(Mode mode, Phase) { return static_cast<std::int8_t>(mode); }
    ResourceReference read(const ActionResource &resource, Phase phase) {
        if (resource.status == ResourceStatus::Unavailable)
            fail(Failure::UnresolvedResource, phase, resource.unavailable_reason);
        if (resource.status == ResourceStatus::KnownNull)
            return {};
        return {resource.identity};
    }
    float speed_multiplier(View &, const Runtime &, Phase);
    float charge_multiplier(View &, const Runtime &, Phase);
    std::uint32_t raw_position_count(const Node &node, Phase) {
        return node.raw_scale_pointer_bits;
    }
    const Table &table(const View &, Phase);
    [[noreturn]] void fail(Failure kind, Phase phase, const std::string &reason = {}) {
        Outcome result;
        result.failure = kind;
        result.phase = phase;
        result.unavailable_resource_reason = reason;
        throw Stop{result};
    }
};
} // namespace sadx::animation::detail

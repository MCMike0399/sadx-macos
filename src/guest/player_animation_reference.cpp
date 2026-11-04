#include "guest/animation_phase_reference.hpp"
#include "player/animation/animation_engine.hpp"
#include "player/animation/animation_state_binding.hpp"
#include "player/player_animation.hpp"
namespace sadx {
namespace {
struct ReferenceAccess {
    using PlayerAnimationView = sadx::PlayerAnimationView;
    using PlayerAnimationRuntime = sadx::PlayerAnimationRuntime;
    using PlayerAnimationRecord = sadx::PlayerAnimationRecord;
    using PlayerAnimationAction = sadx::PlayerAnimationAction;
    using PlayerAnimationMotion = sadx::PlayerAnimationMotion;
    using PlayerAnimationTable = sadx::PlayerAnimationTable;
    using PlayerAnimationNode = sadx::PlayerAnimationNode;
    using PlayerAnimationKeyBytes = sadx::PlayerAnimationKeyBytes;
    using PlayerAnimationAllocatedAction = sadx::PlayerAnimationAllocatedAction;
    using PlayerAnimationAllocatedMotion = sadx::PlayerAnimationAllocatedMotion;
    using AnimationResourceReference = sadx::AnimationResourceReference;
    using SceneObject = SceneGeometryObject;
    // NOTSA: retain lazy reference-fixture reads at their original reached phase.
    const SceneObject &object(const SceneResourceReference &ref,
                              const PlayerAnimationRuntime &rt, animation::Phase at) {
        auto stop = [&](animation::Failure kind) -> void {
            animation::Outcome result;
            result.failure = kind;
            result.phase = at;
            throw animation::Stop{result};
        };
        if (!ref.id) stop(animation::Failure::NullResource);
        if (!rt.resources.objects) stop(animation::Failure::UnresolvedResource);
        const auto found = rt.resources.objects->find(*ref.id);
        if (found == rt.resources.objects->end()) stop(animation::Failure::UnresolvedResource);
        return found->second;
    }
    SceneResourceReference child(const SceneObject &o, animation::Phase at) {
        return read(o.child, at);
    }
    SceneResourceReference sibling(const SceneObject &o, animation::Phase at) {
        return read(o.sibling, at);
    }
    std::array<std::uint32_t, 3> pose(const SceneObject &o, unsigned channel, animation::Phase at) {
        auto word = [](auto value) {
            std::uint32_t bits;
            std::memcpy(&bits, &value, 4);
            return bits;
        };
        if (channel == 1) {
            const auto a = read(o.angles, at);
            return {word(a[0]), word(a[1]), word(a[2])};
        }
        const auto p = channel == 0 ? read(o.position, at) : read(o.scale, at);
        return {word(p.x), word(p.y), word(p.z)};
    }
    // NOTSA: preserve supplied legacy reference values at each reached arithmetic use.
    float zero(const PlayerAnimationRuntime &runtime) const { return runtime.constants.zero; }
    // NOTSA: this reference contract may substitute values without adding native pool state.
    float one(const PlayerAnimationRuntime &runtime) const { return runtime.constants.one; }
    // NOTSA: retain the legacy unsigned-conversion correction, including substitutions.
    float unsigned_correction(const PlayerAnimationRuntime &runtime) const {
        return runtime.constants.unsigned_correction;
    }
    auto state(const PlayerAnimationView &v) {
        struct Fields {
            decltype((v.player.current_animation_at_126)) current;
            decltype((v.player.requested_animation_at_124)) requested;
            decltype((v.player.previous_animation_at_128)) previous;
            decltype((v.player.animation_mode)) mode;
            decltype((v.tail.start_at_134)) start;
            decltype((v.tail.frame_at_130)) frame;
            decltype((v.tail.buffer_at_148)) buffer;
            decltype((v.tail.flags_at_12e)) flags;
            decltype((v.tail.timer_at_12c)) timer;
            decltype((v.tail.next_at_12a)) next;
        };
        return Fields{v.player.current_animation_at_126,
                      v.player.requested_animation_at_124,
                      v.player.previous_animation_at_128,
                      v.player.animation_mode,
                      v.tail.start_at_134,
                      v.tail.frame_at_130,
                      v.tail.buffer_at_148,
                      v.tail.flags_at_12e,
                      v.tail.timer_at_12c,
                      v.tail.next_at_12a};
    }
    template <class T> const T &read(const std::optional<T> &value, animation::Phase phase) {
        if (!value) {
            animation::Outcome failure;
            failure.failure = animation::Failure::UnavailableField;
            failure.phase = phase;
            throw animation::Stop{failure};
        }
        return *value;
    }
    float speed_multiplier(PlayerAnimationView &v, const PlayerAnimationRuntime &rt,
                           animation::Phase phase) {
        return animation::detail::read_live_multiplier({v.player, v.player_allocation}, rt.arena,
                                                       animation::detail::Multiplier::Speed, phase);
    }
    float charge_multiplier(PlayerAnimationView &v, const PlayerAnimationRuntime &rt,
                            animation::Phase phase) {
        return animation::detail::read_live_multiplier(
            {v.player, v.player_allocation}, rt.arena,
            animation::detail::Multiplier::Charge, phase);
    }
    std::uint32_t raw_position_count(const PlayerAnimationNode &node, animation::Phase phase) {
        return read(node.scale_pointer_bits_at_8, phase);
    }
    const PlayerAnimationTable &table(const PlayerAnimationView &view, animation::Phase phase) {
        const auto &pointer = read(view.tail.table_at_140, phase);
        if (!pointer) {
            animation::Outcome failure;
            failure.failure = animation::Failure::NullResource;
            failure.phase = phase;
            throw animation::Stop{failure};
        }
        const auto *legacy = dynamic_cast<const PlayerAnimationTable *>(pointer.get());
        if (!legacy) {
            animation::Outcome failure;
            failure.failure = animation::Failure::UnresolvedResource;
            failure.phase = phase;
            throw animation::Stop{failure};
        }
        return *legacy;
    }
};
PlayerAnimationResult diagnostics(const animation::Outcome &outcome) {
    PlayerAnimationResult result;
    result.failure = static_cast<PlayerAnimationFailure>(outcome.failure);
    result.source_boundary = animation_source_boundary(outcome.phase);
    result.nested_source_boundary = animation_source_boundary(outcome.nested_phase);
    result.callee_address = outcome.nested_transition ? 0x43f2a0 : 0;
    result.work_failure = outcome.work_failure;
    result.completed = outcome.completed;
    result.wrap_iterations = outcome.wrap_iterations;
    return result;
}
} // namespace
PlayerAnimationResult update_player_animation(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                                              X87Context &fp) {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return diagnostics(engine.update_player_animation(v, rt, fp));
}
PlayerAnimationResult update_player_animation(PlayerAnimationView &view,
                                              animation::Runtime &runtime, X87Context &fp) {
    if (&view.tail != &view.player.animation_tail) {
        PlayerAnimationResult failure;
        failure.failure = PlayerAnimationFailure::unresolved_resource;
        failure.source_boundary = 0x44a800;
        return failure;
    }
    animation::View native_view{view.player, view.player_allocation};
    return diagnostics(animation::update(native_view, runtime, fp));
}
PlayerAnimationResult reset_player_animation_frame(PlayerAnimationView &v,
                                                   const PlayerAnimationRuntime &rt,
                                                   X87Context &fp) {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return diagnostics(engine.reset_player_animation_frame(v, rt, fp));
}
PlayerAnimationResult next_player_animation(PlayerAnimationView &v,
                                            const PlayerAnimationRuntime &rt, std::uint16_t from,
                                            X87Context &fp) {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return diagnostics(engine.next_player_animation(v, rt, from, fp));
}
PlayerAnimationResult release_animation_keys(PlayerAnimationView &v, PlayerAnimationRuntime &rt) {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return diagnostics(engine.release_animation_keys(v, rt));
}
PlayerAnimationResult blend_player_animation(PlayerAnimationView &v, PlayerAnimationRuntime &rt,
                                             X87Context &fp) {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return diagnostics(engine.blend_player_animation(v, rt, fp));
}
PlayerAnimationResult build_player_animation_transition(PlayerAnimationView &v,
                                                        PlayerAnimationRuntime &rt,
                                                        X87Context &fp) {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return diagnostics(engine.build_player_animation_transition(v, rt, fp));
}
std::int32_t animation_angle_delta(std::int32_t first, std::int32_t second) noexcept {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return engine.animation_angle_delta(first, second);
}
std::int32_t animation_unwrap_angle(std::int32_t first, std::int32_t second) noexcept {
    ReferenceAccess access;
    animation::detail::Engine<ReferenceAccess> engine(access);
    return engine.animation_unwrap_angle(first, second);
}
} // namespace sadx

#include "player/animation/animation_engine.hpp"
#include "player/animation/animation_native_access.hpp"
#include "player/animation/animation_state_binding.hpp"
#include <limits>
#include <stdexcept>

namespace sadx::animation::detail {
NativeAccess::SceneObject NativeAccess::object(const SceneResourceReference &ref, const Runtime &rt,
                                             Phase at) {
    if (!ref.id)
        fail(Failure::NullResource, at);
    if (rt.scene_objects) {
        if (*ref.id > std::numeric_limits<character_pack::ResourceId>::max())
            fail(Failure::UnresolvedResource, at);
        try {
            return {&rt.scene_objects->object(static_cast<character_pack::ResourceId>(*ref.id)),
                    nullptr};
        } catch (const std::runtime_error &) {
            fail(Failure::UnresolvedResource, at);
        }
    }
    if (!rt.resources.objects)
        fail(Failure::UnresolvedResource, at);
    const auto found = rt.resources.objects->find(*ref.id);
    if (found == rt.resources.objects->end())
        fail(Failure::UnresolvedResource, at);
    return {nullptr, &found->second};
}
SceneResourceReference NativeAccess::child(const SceneObject &o, Phase at) {
    if (!o.native)
        return read(o.reference->child, at);
    return {o.native->child ? std::optional<std::size_t>(o.native->child) : std::nullopt};
}
SceneResourceReference NativeAccess::sibling(const SceneObject &o, Phase at) {
    if (!o.native)
        return read(o.reference->sibling, at);
    return {o.native->sibling ? std::optional<std::size_t>(o.native->sibling) : std::nullopt};
}
std::array<std::uint32_t, 3> NativeAccess::pose(const SceneObject &o, unsigned channel, Phase at) {
    auto word = [](auto value) {
        std::uint32_t bits;
        std::memcpy(&bits, &value, 4);
        return bits;
    };
    if (o.native) {
        if (channel == 1) {
            const auto &a = o.native->rotation;
            return {word(a[0]), word(a[1]), word(a[2])};
        }
        const auto &p = channel == 0 ? o.native->position : o.native->scale;
        return {word(p[0]), word(p[1]), word(p[2])};
    }
    if (channel == 1) {
        const auto a = read(o.reference->angles, at);
        return {word(a[0]), word(a[1]), word(a[2])};
    }
    const auto p = channel == 0 ? read(o.reference->position, at) : read(o.reference->scale, at);
    return {word(p.x), word(p.y), word(p.z)};
}
float read_live_multiplier(const View &view, TaskWorkArena *arena, Multiplier kind, Phase phase) {
    NativeAccess access;
    if (!arena)
        access.fail(Failure::MissingArena, phase);
    if (!view.player_allocation)
        access.fail(Failure::UnresolvedResource, phase);
    constexpr std::uint32_t kSpeedReference = 312, kChargeReference = 316;
    constexpr std::uint32_t kSpeedMember = 56;
    const auto reference = arena->reference_view(
        view.player_allocation, kind == Multiplier::Speed ? kSpeedReference : kChargeReference);
    if (!reference) {
        Outcome failure;
        failure.failure = Failure::WorkFailed;
        failure.phase = phase;
        failure.work_failure = reference.failure;
        throw Stop{failure};
    }
    if (!reference.work)
        access.fail(Failure::NullResource, phase);
    if (reference.work != view.player_allocation)
        access.fail(Failure::UnresolvedResource, phase);
    if (reference.byte_offset == kSpeedMember)
        return view.player.motion.speed.x;
    if (reference.byte_offset == 0)
        return access.read(view.player.source_charge_at_0, phase);
    access.fail(Failure::UnresolvedResource, phase);
}
NativeAccess::StateFields NativeAccess::state(const View &v) {
    return StateFields{v.player.current_animation_at_126,     v.player.requested_animation_at_124,
                       v.player.previous_animation_at_128,    v.player.animation_mode_at_120,
                       v.player.animation_tail.start_at_134,  v.player.animation_tail.frame_at_130,
                       v.player.animation_tail.buffer_at_148, v.player.animation_tail.flags_at_12e,
                       v.player.animation_tail.timer_at_12c,  v.player.animation_tail.next_at_12a};
}
const Table &NativeAccess::table(const View &view, Phase phase) {
    const auto &pointer = read(view.player.animation_tail.table_at_140, phase);
    if (!pointer)
        fail(Failure::NullResource, phase);
    const auto *content = dynamic_cast<const Table *>(pointer.get());
    if (!content)
        fail(Failure::UnresolvedResource, phase);
    return *content;
}

float NativeAccess::speed_multiplier(View &view, const Runtime &runtime, Phase phase) {
    return read_live_multiplier(view, runtime.arena, Multiplier::Speed, phase);
}
float NativeAccess::charge_multiplier(View &view, const Runtime &runtime, Phase phase) {
    return read_live_multiplier(view, runtime.arena, Multiplier::Charge, phase);
}
} // namespace sadx::animation::detail

namespace sadx::animation {
Outcome update(View &view, Runtime &runtime, X87Context &fp) {
    detail::NativeAccess access;
    return detail::Engine<detail::NativeAccess>(access).update_player_animation(view, runtime, fp);
}
Outcome reset_frame(View &view, const Runtime &runtime, X87Context &fp) {
    detail::NativeAccess access;
    return detail::Engine<detail::NativeAccess>(access).reset_player_animation_frame(view, runtime,
                                                                                     fp);
}
Outcome next(View &view, const Runtime &runtime, std::uint16_t from, X87Context &fp) {
    detail::NativeAccess access;
    return detail::Engine<detail::NativeAccess>(access).next_player_animation(view, runtime, from,
                                                                              fp);
}
Outcome release_keys(View &view, Runtime &runtime) {
    detail::NativeAccess access;
    return detail::Engine<detail::NativeAccess>(access).release_animation_keys(view, runtime);
}
Outcome blend(View &view, Runtime &runtime, X87Context &fp) {
    detail::NativeAccess access;
    return detail::Engine<detail::NativeAccess>(access).blend_player_animation(view, runtime, fp);
}
} // namespace sadx::animation

#include "player/animation/animation_engine.hpp"
#include "player/animation/animation_native_access.hpp"
#include "player/animation/animation_state_binding.hpp"

namespace sadx::animation::detail {
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
    return StateFields{v.player.current_animation_at_126,
                       v.player.requested_animation_at_124,
                       v.player.previous_animation_at_128,
                       v.player.animation_mode_at_120,
                       v.tail.start_at_134,
                       v.tail.frame_at_130,
                       v.tail.buffer_at_148,
                       v.tail.flags_at_12e,
                       v.tail.timer_at_12c,
                       v.tail.next_at_12a};
}
const Table &NativeAccess::table(const View &view, Phase phase) {
    const auto &pointer = read(view.tail.table_at_140, phase);
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

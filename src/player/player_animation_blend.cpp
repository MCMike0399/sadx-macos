#include "player/animation/animation_engine.hpp"
#include "player/animation/animation_native_access.hpp"

namespace sadx::animation {
Outcome build_transition(View &view, Runtime &runtime, X87Context &fp) {
    detail::NativeAccess access;
    return detail::Engine<detail::NativeAccess>(access).build_player_animation_transition(
        view, runtime, fp);
}
} // namespace sadx::animation

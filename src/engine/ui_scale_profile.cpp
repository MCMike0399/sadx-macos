#include "engine/ui_scale_profile.hpp"
#include <stdexcept>
namespace sadx::ui_scale {
bool scaling_enabled(const State &state) noexcept {
    if (!state.enabled)
        return false;
    const bool background = !state.entries.empty() && state.entries.back().background;
    return !background || state.background_fill != FillMode::Stretch;
}
void require_stretch_owners(const State &state, const float &horizontal, const float &vertical) {
    if (&state.horizontal != &horizontal || &state.vertical != &vertical)
        throw Error(Boundary::StretchOwnerMismatch,
                    "UI scale profile does not borrow the caller's canonical stretch owners");
}
void run_background_scope(State &state, const std::function<void()> &body) {
    if (state.background_fill == FillMode::Stretch)
        throw Error(Boundary::StretchBackground,
                    "unrecovered loader stretch-background disable/enable branch");
    state.entries.push_back({54, state.horizontal, state.vertical, true});
    state.horizontal = 1;
    state.vertical = 1;
    state.enabled = true;
    body();
    // The source returns if the actual stack became empty during the call.
    if (state.entries.empty())
        return;
    const auto entry = state.entries.back();
    state.horizontal = entry.horizontal;
    state.vertical = entry.vertical;
    state.entries.pop_back();
    state.enabled = !state.entries.empty();
}
void run_movie_scope(State &state, const std::function<void()> &body) {
    const auto original_fill = state.background_fill;
    state.background_fill = state.movie_fill;
    run_background_scope(state, body);
    state.background_fill = original_fill;
}
void run_identity_background(State &state, std::uint32_t width, std::uint32_t height,
                             const std::function<void()> &body) {
    if (width != 640 || height != 480 || state.horizontal != 1 || state.vertical != 1)
        throw std::out_of_range("unrecovered background geometry outside native identity view");
    run_background_scope(state, body);
}
void require_message_forwarding(const State &state) {
    if (scaling_enabled(state))
        throw Error(Boundary::ActiveSpriteScaling,
                    "active loader MSG sprite scaling is unrecovered");
}
} // namespace sadx::ui_scale

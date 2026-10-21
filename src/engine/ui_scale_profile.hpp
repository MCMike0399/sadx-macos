#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>
namespace sadx::ui_scale {
enum class FillMode : std::uint8_t { Stretch, Fit, Fill };
enum class Boundary {
    // Source boundary: LOADER 0x10038f15
    StretchBackground,
    // Source boundary: LOADER 0x100ae325
    ActiveSpriteScaling,
    // Source resource boundary: LOADER 0x10038fc1
    StretchOwnerMismatch
};
class Error : public std::runtime_error {
  public:
    Error(Boundary kind, const char *message) : std::runtime_error(message), boundary(kind) {}
    Boundary boundary;
};
struct Entry {
    std::uint8_t alignment;
    float horizontal, vertical;
    bool background;
};
// Pinned loader-owned state. Stretch values borrow the canonical screen owners.
// Scope records retain their actual previous values; there is no frame-end reset.
// The owner lives for the renderer lifetime, never reconstructed per frame/draw.
struct State {
    State(float &h, float &v) : horizontal(h), vertical(v) {}
    float &horizontal, &vertical;
    // GLOBAL: LOADER 0x100f15a1 UiScaleBackgroundFill size=1
    FillMode background_fill = FillMode::Fill;
    // GLOBAL: LOADER 0x100f15a0 UiScaleMovieFill size=1
    FillMode movie_fill = FillMode::Fit;
    // GLOBAL: LOADER 0x100f8b68 UiScaleStack size=12
    std::vector<Entry> entries;
    // GLOBAL: LOADER 0x100fa2a1 UiScaleEnabled size=1
    bool enabled = false;
};
// NOTSA: source is_scale_enabled predicate, inlined in the compiled wrapper.
bool scaling_enabled(const State &) noexcept;
// NOTSA: require both callers to borrow the SAME native screen owners.
void require_stretch_owners(const State &, const float &horizontal, const float &vertical);
// Only the selected non-Stretch background branch is recovered here. An
// exception from the body leaves its source prefix active, as the loader does.
// FUNCTION: LOADER 0x10038ef0 scale_trampoline
void run_background_scope(State &, const std::function<void()> &);
// FUNCTION: LOADER 0x100aea50 DisplayVideoFrame_r
void run_movie_scope(State &, const std::function<void()> &);
// NOTSA: existing native background drawing implements only this identity view.
void run_identity_background(State &, std::uint32_t width, std::uint32_t height,
                             const std::function<void()> &);
// NOTSA: actual MSG sprite has source global-scratch identity, never lens flare.
// Call before entering the proven original404660 body; active scaling is explicit.
void require_message_forwarding(const State &);
} // namespace sadx::ui_scale

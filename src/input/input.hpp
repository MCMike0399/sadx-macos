#pragma once

#include "input/input_mod_axes.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace sadx::input {

// Semantic actions have no numeric relationship to physical keys/buttons.
enum class Action {
    move_left, move_right, move_up, move_down,
    look_left, look_right, look_up, look_down,
    jump, action, pickup, whistle, pause, camera_left, camera_right, count
};

// Original SADX SDK masks, independently verified in SADXEnums.h.
std::uint32_t action_mask(Action action) noexcept;

// SDL-compatible physical scancode namespace, independent of text layout.
// A native platform adapter must translate its key codes into this namespace.
inline constexpr int scancode_count = 512;
struct KeyboardBinding { int scancode; Action action; };
std::vector<KeyboardBinding> default_keyboard_bindings();

enum class PhysicalButton { south, east, west, north, start, left_trigger,
                            right_trigger, count };
struct ControllerBinding { PhysicalButton button; Action action; };
std::vector<ControllerBinding> default_controller_bindings();
enum class ControllerFamily { xbox, switch2 };
// Printed legends describe physical locations; they do not select bindings.
const char* face_label(ControllerFamily family, PhysicalButton position) noexcept;

struct Stick { float x = 0; float y = 0; };
// Preserve the Input Mod's separate signed direction and analog magnitude.
struct AnalogState {
    std::int16_t x = 0;
    std::int16_t y = 0;
    float magnitude = 0;
};
// Compatibility is the default: 8192 steps to/from +/-32767 every sample.
// Immediate cancellation/movement is an explicit optional native policy.
enum class KeyboardProfile { input_mod_261, immediate };
// Platform snapshots have already been normalized/deadzoned by their backend.
// This representation is not original SDL signed16 raw axes. Its subsequent
// native direction/magnitude policy has no original-controller parity claim.
struct NormalizedControllerSnapshot {
    Stick left;
    Stick right;
    // Trigger booleans are supplied by the adapter's chosen threshold policy.
    std::array<bool, static_cast<unsigned>(PhysicalButton::count)> buttons{};
};
enum class AnalogSource { input_mod_261_keyboard, immediate_keyboard,
                          native_normalized_controller_snapshot };
struct KeyboardRawSticks { InputModRawAxes left, right; };
struct TickInput {
    AnalogState left;
    AnalogState right;
    AnalogSource left_source = AnalogSource::native_normalized_controller_snapshot;
    AnalogSource right_source = AnalogSource::native_normalized_controller_snapshot;
    // Snapshot of explicit software SSE status, not a captured hardware value.
    std::uint32_t mxcsr_after_sampling = 0;
    std::uint32_t held = 0;
    std::uint32_t pressed = 0;
    std::uint32_t released = 0;
};

// One player's state, sampled once per simulation tick. Platform discovery,
// event delivery, deadzones, rumble and the game loop are outside this model.
class InputState {
public:
    explicit InputState(KeyboardProfile profile = KeyboardProfile::input_mod_261);
    // Replace atomically. Duplicate physical bindings, invalid action values,
    // scancodes <= 0 or >= 512 are rejected; aliases for one action are valid.
    bool bind_keyboard(const std::vector<KeyboardBinding>& bindings);
    // Controller buttons bind SDK button actions; stick-direction actions have
    // no button mask and are rejected rather than altering keyboard raw state.
    bool bind_controller(const std::vector<ControllerBinding>& bindings);
    bool keyboard_event(int scancode, bool down, bool autorepeat = false) noexcept;
    // Reject nonfinite/out-of-range axes without changing current state.
    bool normalized_controller_snapshot(const NormalizedControllerSnapshot& snapshot) noexcept;
    // Compatibility profile clears keyboard buttons/directions while retaining
    // the original raw ramp and independent controller sample. Immediate
    // profile clears all sources/ramp. The app owns pause-on-inactive cadence.
    void focus_lost() noexcept;
    // Advances raw keyboard ramp once, then runs the actual DLL scalar phase
    // using explicitly selected arithmetic/constants before native arbitration.
    // Unmasked SSE faults propagate; the consumed ramp remains advanced and
    // button edges are committed only when the sample completes. No auto retry.
    TickInput sample_tick(SseContext& arithmetic, const InputModAxesConstants& constants);
    KeyboardRawSticks keyboard_raw_sticks() const noexcept;

private:
    std::array<std::optional<Action>, scancode_count> keyboard_bindings_{};
    std::array<bool, scancode_count> keys_{};
    std::array<std::optional<Action>, static_cast<unsigned>(PhysicalButton::count)>
        controller_bindings_{};
    NormalizedControllerSnapshot controller_{};
    KeyboardProfile keyboard_profile_;
    std::array<std::int32_t, 4> keyboard_axes_{};
    std::uint32_t previous_held_ = 0;
};

} // namespace sadx::input

#pragma once

#include "input/input.hpp"
#include "input/input_mod_pad.hpp"

#import <AppKit/AppKit.h>
#import <GameController/GameController.h>

namespace sadx::input::mac {

// Unknown/unimplemented physical keys have no mapping. Returned codes are
// SDL scancodes, not macOS keyCodes and not characters from a keyboard layout.
std::optional<int> physical_scancode(unsigned short key_code) noexcept;

// The app owns event delivery and calls this adapter on its input thread.
// This adapter creates no windows, monitors, device discovery or input hooks.
class InputAdapter {
public:
    explicit InputAdapter(InputState& state) noexcept : state_(state) {}
    bool keyboard_event(NSEvent* event) noexcept;
    bool focus_notification(NSNotification* notification) noexcept;
    void focus_lost() noexcept;
    // Captures a stable extended profile, retaining physical button positions.
    // Axes already include Apple's deadzone; Y is flipped to SADX's down+.
    // Trigger threshold compares trunc(value * 255) > threshold (Input Mod).
    // nil means disconnected and clears controller state. Unsupported profiles
    // also clear stale controller state and return false; keyboard is retained.
    bool controller_sample(GCController* controller,
                           unsigned trigger_threshold = 30) noexcept;
private:
    InputState& state_;
};

// SDL2-domain gamepad state for the Input 2.6.1 owner (input_mod_poll).
// Buttons use SDL_GameControllerButton order. The reference SDL2.dll defaults
// SDL_GAMECONTROLLER_USE_BUTTON_LABELS on and the Input mod sets no hint or
// mapping DB, so SDL A/B/X/Y follow the printed label (Switch "A" = east,
// Xbox "A" = south). Labels come from each element's sfSymbolsName; if any
// face label is unavailable the Xbox positional layout is used and
// labels_resolved is false. Axes: trunc(value*32767), Y negated (SDL down+),
// triggers 0..32767. Apple's values already include its deadzone, so this is
// not raw XInput/HIDAPI axis parity.
struct GamepadConversion {
    InputModGamepadState state;
    bool labels_resolved=false;
};
std::optional<GamepadConversion> input_mod_gamepad_state(GCController* controller) noexcept;

} // namespace sadx::input::mac

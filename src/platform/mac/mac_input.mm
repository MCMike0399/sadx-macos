#include "platform/mac/mac_input.hpp"

#import <Carbon/Carbon.h>

#include <cmath>

namespace sadx::input::mac {

std::optional<int> physical_scancode(unsigned short key_code) noexcept {
    switch (key_code) {
    case kVK_ANSI_A: return 4;
    case kVK_ANSI_B: return 5;
    case kVK_ANSI_C: return 6;
    case kVK_ANSI_D: return 7;
    case kVK_ANSI_E: return 8;
    case kVK_ANSI_F: return 9;
    case kVK_ANSI_G: return 10;
    case kVK_ANSI_H: return 11;
    case kVK_ANSI_I: return 12;
    case kVK_ANSI_J: return 13;
    case kVK_ANSI_K: return 14;
    case kVK_ANSI_L: return 15;
    case kVK_ANSI_M: return 16;
    case kVK_ANSI_N: return 17;
    case kVK_ANSI_O: return 18;
    case kVK_ANSI_P: return 19;
    case kVK_ANSI_Q: return 20;
    case kVK_ANSI_R: return 21;
    case kVK_ANSI_S: return 22;
    case kVK_ANSI_T: return 23;
    case kVK_ANSI_U: return 24;
    case kVK_ANSI_V: return 25;
    case kVK_ANSI_W: return 26;
    case kVK_ANSI_X: return 27;
    case kVK_ANSI_Y: return 28;
    case kVK_ANSI_Z: return 29;
    case kVK_ANSI_1: return 30;
    case kVK_ANSI_2: return 31;
    case kVK_ANSI_3: return 32;
    case kVK_ANSI_4: return 33;
    case kVK_ANSI_5: return 34;
    case kVK_ANSI_6: return 35;
    case kVK_ANSI_7: return 36;
    case kVK_ANSI_8: return 37;
    case kVK_ANSI_9: return 38;
    case kVK_ANSI_0: return 39;
    case kVK_Return: return 40;
    case kVK_Escape: return 41;
    case kVK_Delete: return 42;
    case kVK_Tab: return 43;
    case kVK_Space: return 44;
    case kVK_ANSI_Minus: return 45;
    case kVK_ANSI_Equal: return 46;
    case kVK_ANSI_LeftBracket: return 47;
    case kVK_ANSI_RightBracket: return 48;
    case kVK_ANSI_Backslash: return 49;
    case kVK_ANSI_Semicolon: return 51;
    case kVK_ANSI_Quote: return 52;
    case kVK_ANSI_Grave: return 53;
    case kVK_ANSI_Comma: return 54;
    case kVK_ANSI_Period: return 55;
    case kVK_ANSI_Slash: return 56;
    case kVK_F1: return 58;
    case kVK_F2: return 59;
    case kVK_F3: return 60;
    case kVK_F4: return 61;
    case kVK_F5: return 62;
    case kVK_F6: return 63;
    case kVK_F7: return 64;
    case kVK_F8: return 65;
    case kVK_F9: return 66;
    case kVK_F10: return 67;
    case kVK_F11: return 68;
    case kVK_F12: return 69;
    case kVK_Help: return 73;
    case kVK_Home: return 74;
    case kVK_PageUp: return 75;
    case kVK_ForwardDelete: return 76;
    case kVK_End: return 77;
    case kVK_PageDown: return 78;
    case kVK_RightArrow: return 79;
    case kVK_LeftArrow: return 80;
    case kVK_DownArrow: return 81;
    case kVK_UpArrow: return 82;
    case kVK_ANSI_KeypadClear: return 83;
    case kVK_ANSI_KeypadDivide: return 84;
    case kVK_ANSI_KeypadMultiply: return 85;
    case kVK_ANSI_KeypadMinus: return 86;
    case kVK_ANSI_KeypadPlus: return 87;
    case kVK_ANSI_KeypadEnter: return 88;
    case kVK_ANSI_Keypad1: return 89;
    case kVK_ANSI_Keypad2: return 90;
    case kVK_ANSI_Keypad3: return 91;
    case kVK_ANSI_Keypad4: return 92;
    case kVK_ANSI_Keypad5: return 93;
    case kVK_ANSI_Keypad6: return 94;
    case kVK_ANSI_Keypad7: return 95;
    case kVK_ANSI_Keypad8: return 96;
    case kVK_ANSI_Keypad9: return 97;
    case kVK_ANSI_Keypad0: return 98;
    case kVK_ANSI_KeypadDecimal: return 99;
    case kVK_ISO_Section: return 100;
    case kVK_ANSI_KeypadEquals: return 103;
    case kVK_F13: return 104;
    case kVK_F14: return 105;
    case kVK_F15: return 106;
    case kVK_F16: return 107;
    case kVK_F17: return 108;
    case kVK_F18: return 109;
    case kVK_F19: return 110;
    case kVK_F20: return 111;
    default: return std::nullopt;
    }
}

bool InputAdapter::keyboard_event(NSEvent* event) noexcept {
    if (!event || (event.type != NSEventTypeKeyDown && event.type != NSEventTypeKeyUp))
        return false;
    const auto scancode = physical_scancode(event.keyCode);
    if (!scancode) return false;
    // Repeat applies only to keyDown. A keyUp always clears physical state.
    return state_.keyboard_event(*scancode, event.type == NSEventTypeKeyDown,
                                 event.type == NSEventTypeKeyDown && event.isARepeat);
}

bool InputAdapter::focus_notification(NSNotification* notification) noexcept {
    if (!notification ||
        (![notification.name isEqualToString:NSWindowDidResignKeyNotification] &&
         ![notification.name isEqualToString:NSApplicationDidResignActiveNotification]))
        return false;
    focus_lost();
    return true;
}

void InputAdapter::focus_lost() noexcept { state_.focus_lost(); }

bool InputAdapter::controller_sample(GCController* controller,
                                     unsigned trigger_threshold) noexcept {
    if (trigger_threshold > 255) return false;
    if (!controller) return state_.normalized_controller_snapshot({});
    GCController* snapshot = [controller capture];
    GCExtendedGamepad* profile = snapshot.extendedGamepad;
    if (!profile) {
        state_.normalized_controller_snapshot({});
        return false;
    }
    // Apple's float snapshot is already normalized/deadzoned. Preserve that
    // boundary; it is not an original SDL signed16 source-axis observation.
    NormalizedControllerSnapshot sample;
    sample.left = {profile.leftThumbstick.xAxis.value, -profile.leftThumbstick.yAxis.value};
    sample.right = {profile.rightThumbstick.xAxis.value, -profile.rightThumbstick.yAxis.value};
    const auto set = [&](PhysicalButton button, bool down) {
        sample.buttons[static_cast<unsigned>(button)] = down;
    };
    set(PhysicalButton::south, profile.buttonA.isPressed);
    set(PhysicalButton::east, profile.buttonB.isPressed);
    set(PhysicalButton::west, profile.buttonX.isPressed);
    set(PhysicalButton::north, profile.buttonY.isPressed);
    set(PhysicalButton::start, profile.buttonMenu.isPressed);
    const float left_trigger = profile.leftTrigger.value;
    const float right_trigger = profile.rightTrigger.value;
    if (!std::isfinite(left_trigger) || !std::isfinite(right_trigger) ||
        left_trigger < 0 || left_trigger > 1 || right_trigger < 0 || right_trigger > 1)
        return false;
    set(PhysicalButton::left_trigger,
        static_cast<unsigned>(left_trigger * 255) > trigger_threshold);
    set(PhysicalButton::right_trigger,
        static_cast<unsigned>(right_trigger * 255) > trigger_threshold);
    return state_.normalized_controller_snapshot(sample);
}

namespace {
std::optional<std::int16_t> sdl_axis(float value) noexcept {
    if (!std::isfinite(value) || value < -1 || value > 1) return std::nullopt;
    const float scaled = value * 32767.0f;
    return static_cast<std::int16_t>(scaled < -32768.0f ? -32768.0f : scaled);
}
}

std::optional<GamepadConversion> input_mod_gamepad_state(GCController* controller) noexcept {
    if (!controller) return std::nullopt;
    GCExtendedGamepad* pad = [controller capture].extendedGamepad;
    if (!pad) return std::nullopt;
    GamepadConversion out;
    const auto lx = sdl_axis(pad.leftThumbstick.xAxis.value), ly = sdl_axis(-pad.leftThumbstick.yAxis.value);
    const auto rx = sdl_axis(pad.rightThumbstick.xAxis.value), ry = sdl_axis(-pad.rightThumbstick.yAxis.value);
    const auto lt = sdl_axis(pad.leftTrigger.value), rt = sdl_axis(pad.rightTrigger.value);
    if (!lx || !ly || !rx || !ry || !lt || !rt || *lt < 0 || *rt < 0) return std::nullopt;
    out.state.axes = {{*lx, *ly, *rx, *ry, *lt, *rt}};
    // Resolve SDL A/B/X/Y by printed label.
    GCControllerButtonInput* faces[] = {pad.buttonA, pad.buttonB, pad.buttonX, pad.buttonY};
    NSString* labels[] = {@"a.circle", @"b.circle", @"x.circle", @"y.circle"};
    GCControllerButtonInput* by_label[4] = {nil, nil, nil, nil};
    for (GCControllerButtonInput* face : faces)
        for (unsigned i = 0; i < 4; ++i)
            if (face.sfSymbolsName && [face.sfSymbolsName isEqualToString:labels[i]]) by_label[i] = face;
    out.labels_resolved = by_label[0] && by_label[1] && by_label[2] && by_label[3];
    for (unsigned i = 0; i < 4; ++i) {
        GCControllerButtonInput* face = out.labels_resolved ? by_label[i] : faces[i];
        out.state.buttons[i] = face.isPressed;
    }
    auto pressed = [](GCControllerButtonInput* b) { return b != nil && b.isPressed; };
    out.state.buttons[4] = pressed(pad.buttonOptions);              // BACK
    out.state.buttons[5] = pressed(pad.buttonHome);                 // GUIDE
    out.state.buttons[6] = pressed(pad.buttonMenu);                 // START
    out.state.buttons[7] = pressed(pad.leftThumbstickButton);       // LEFTSTICK
    out.state.buttons[8] = pressed(pad.rightThumbstickButton);      // RIGHTSTICK
    out.state.buttons[9] = pressed(pad.leftShoulder);
    out.state.buttons[10] = pressed(pad.rightShoulder);
    out.state.buttons[11] = pressed(pad.dpad.up);
    out.state.buttons[12] = pressed(pad.dpad.down);
    out.state.buttons[13] = pressed(pad.dpad.left);
    out.state.buttons[14] = pressed(pad.dpad.right);
    return out;
}

} // namespace sadx::input::mac

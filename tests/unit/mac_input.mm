#include "platform/mac/mac_input.hpp"
#include "game/speedrun.hpp"
#include "input_keyboard_goldens.hpp"

#import <Carbon/Carbon.h>

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
}
std::uint32_t word(float value) {std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
bool near(float left, float right) { return std::abs(left - right) < 0.000001f; }
NSEvent* key(unsigned short code, bool down, NSString* text = @"", bool repeat = false) {
    return [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
                          location:NSZeroPoint modifierFlags:0 timestamp:0
                      windowNumber:0 context:nil characters:text
       charactersIgnoringModifiers:text isARepeat:repeat keyCode:code];
}
}

int main() {
    sadx::SseContext sse(0x1f80); // Test-selected MXCSR, not observed PC state.
    const sadx::InputModAxesConstants axes_constants{{0x34000000}, {0x46fffe00},
        {0x42fe0000}, {0x43000000}, {0x3f800000}}; // Pinned DLL .rdata.

    @autoreleasepool {
        using namespace sadx::input;
        for (unsigned profile = 0; profile < 4; ++profile) {
            InputState keyboard;
            mac::InputAdapter events(keyboard);
            sadx::SseContext selected(keyboard_goldens[profile * 13].mxcsr);
            for (unsigned step = 0; step < 13; ++step) {
                const auto& golden = keyboard_goldens[profile * 13 + step];
                if (golden.focus) events.focus_lost();
                if (golden.scancode) {
                    unsigned short physical = kVK_Return;
                    switch (golden.scancode) {
                    case 79:physical=kVK_RightArrow;break;case 80:physical=kVK_LeftArrow;break;
                    case 81:physical=kVK_DownArrow;break;case 82:physical=kVK_UpArrow;break;
                    }
                    require(events.keyboard_event(key(physical,golden.down,@"layout-independent",golden.repeat)),
                            "production Cocoa adapter rejected golden physical event");
                }
                const auto sampled = keyboard.sample_tick(selected, axes_constants);
                const auto raw = keyboard.keyboard_raw_sticks();
                require(raw.left.x == golden.raw_x && raw.left.y == golden.raw_y &&
                        std::uint32_t(std::int32_t(sampled.left.x)) == golden.x &&
                        std::uint32_t(std::int32_t(sampled.left.y)) == golden.y &&
                        word(sampled.left.magnitude) == golden.magnitude,
                        "Cocoa physical event to sampled words must match actual DLL goldens");
                require(sampled.held == (step == 8 || step == 9 ? 8U : 0U) &&
                        sampled.pressed == (step == 8 ? 8U : 0U) && sampled.released == (step == 10 ? 8U : 0U),
                        "Cocoa keyboard normalization changed Start/repeat/focus edges");
            }
        }
        InputState state;
        mac::InputAdapter adapter(state);
        require(adapter.keyboard_event(key(kVK_RightArrow, true)), "native arrow not accepted");
        for (float magnitude : {8192.0f / 32767, 16384.0f / 32767,
                                24576.0f / 32767, 1.0f}) {
            const auto tick = state.sample_tick(sse, axes_constants);
            require(tick.left.x == 127 && near(tick.left.magnitude, magnitude),
                    "Cocoa arrow must use original ramp at simulation samples");
        }
        adapter.keyboard_event(key(kVK_LeftArrow, true));
        require(near(state.sample_tick(sse, axes_constants).left.magnitude, 24575.0f / 32767),
                "opposing native keys must preserve compatibility ease to zero");
        adapter.focus_lost(); state.sample_tick(sse, axes_constants);
        require(adapter.keyboard_event(key(kVK_ANSI_X, true, @"é")), "native X key rejected");
        auto tick = state.sample_tick(sse, axes_constants);
        require(tick.held == 4 && tick.pressed == 4,
                "physical X must jump regardless of text/layout");
        adapter.keyboard_event(key(kVK_ANSI_X, true, @"X", true));
        tick = state.sample_tick(sse, axes_constants);
        require(tick.held == 4 && tick.pressed == 0, "Cocoa repeat fabricated a press edge");
        adapter.keyboard_event(key(kVK_ANSI_X, false, @"unrelated"));
        require(state.sample_tick(sse, axes_constants).released == 4, "physical keyUp must release independent of text");

        const struct { unsigned short code; std::uint32_t mask; } actions[] = {
            {kVK_ANSI_Z, 2}, {kVK_ANSI_A, 0x400}, {kVK_ANSI_S, 0x200},
            {kVK_ANSI_Q, 0x20000}, {kVK_ANSI_W, 0x10000}, {kVK_Return, 8}};
        for (const auto action : actions) {
            adapter.keyboard_event(key(action.code, true));
            require(state.sample_tick(sse, axes_constants).pressed == action.mask, "native action mapping incorrect");
            adapter.keyboard_event(key(action.code, false));
            require(state.sample_tick(sse, axes_constants).released == action.mask, "native action release incorrect");
        }
        adapter.keyboard_event(key(kVK_ANSI_J, true));
        adapter.keyboard_event(key(kVK_ANSI_M, true));
        tick = state.sample_tick(sse, axes_constants);
        require(tick.right.x == -89 && tick.right.y == 89 && tick.right.magnitude > 0,
                "native J/M camera must produce normalized diagonal ramp");
        adapter.keyboard_event(key(kVK_Return, true)); state.sample_tick(sse, axes_constants);
        tick = state.sample_tick(sse, axes_constants);
        require(sadx::speedrun::in_game_skip_check(tick.held, tick.pressed).requested,
                "held native Return must remain available for Skipma");
        require(adapter.focus_notification([NSNotification notificationWithName:
                    NSWindowDidResignKeyNotification object:nil]), "native focus notification ignored");
        adapter.keyboard_event(key(kVK_Return, true, @"", true));
        tick = state.sample_tick(sse, axes_constants);
        require(tick.held == 0 && tick.released == 8 && tick.right.magnitude > 0,
                "native focus loss must preserve reference ramp and ignore repeat reassertion");
        require(!adapter.keyboard_event(nil) && !adapter.keyboard_event(key(0xffff, true, @"x")) &&
                    !adapter.keyboard_event(key(kVK_Command, true)),
                "unknown or unsupported modifier keys must not invent bindings");
        NSEvent* unrelated = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                             location:NSZeroPoint modifierFlags:0 timestamp:0
                                         windowNumber:0 context:nil subtype:0 data1:0 data2:0];
        require(!adapter.keyboard_event(unrelated), "non-key Cocoa event accepted as a key");
        require(!adapter.focus_notification([NSNotification notificationWithName:@"unrelated" object:nil]),
                "unrelated notification must not act as focus loss");
        adapter.keyboard_event(key(kVK_ANSI_K, true));
        require(state.sample_tick(sse, axes_constants).held == 0, "known but unbound key must not activate actions");
        adapter.focus_lost();
        require(state.bind_keyboard({{44, Action::jump}}), "custom keyboard binding rejected");
        adapter.keyboard_event(key(kVK_Space, true, @"z"));
        require(state.sample_tick(sse, axes_constants).pressed == 4, "native adapter bypassed custom binding");
        adapter.keyboard_event(key(kVK_Space, false)); state.sample_tick(sse, axes_constants);
        require(state.bind_keyboard(default_keyboard_bindings()), "restoring keyboard defaults failed");

        GCController* controller = [GCController controllerWithExtendedGamepad];
        require(controller != nil && controller.isSnapshot, "native profile snapshot unavailable");
        [controller.extendedGamepad.leftThumbstick setValueForXAxis:0 yAxis:0.25f];
        [controller.extendedGamepad.rightThumbstick setValueForXAxis:0.5f yAxis:-0.5f];
        [controller.extendedGamepad.buttonA setValue:1];
        [controller.extendedGamepad.leftTrigger setValue:30.0f / 255];
        require(adapter.controller_sample(controller), "production controller adapter rejected snapshot");
        tick = state.sample_tick(sse, axes_constants);
        require(tick.held == 4 && tick.left.y == -127 && near(tick.left.magnitude, 0.25f),
                "physical south/Y-up conversion or deadzone preservation incorrect");
        require(tick.left_source == AnalogSource::native_normalized_controller_snapshot &&
                tick.right_source == AnalogSource::native_normalized_controller_snapshot,
                "Apple snapshots must retain native policy provenance instead of SDL raw identity");
        require(tick.right.x == 89 && tick.right.y == 89,
                "right stick Y orientation incorrect");
        [controller.extendedGamepad.leftTrigger setValue:31.0f / 255];
        [controller.extendedGamepad.buttonMenu setValue:1];
        adapter.controller_sample(controller);
        tick = state.sample_tick(sse, axes_constants);
        require(tick.held == (4 | 8 | 0x20000), "native trigger/menu boundaries incorrect");
        require(state.bind_controller({{PhysicalButton::east, Action::jump}}), "controller rebind failed");
        require(state.sample_tick(sse, axes_constants).held == 0, "Apple south must not silently become Nintendo east");
        [controller.extendedGamepad.buttonB setValue:1];
        adapter.controller_sample(controller);
        require(state.sample_tick(sse, axes_constants).held == 4, "physical east remap ignored");
        require(!adapter.controller_sample(controller, 256), "invalid trigger threshold accepted");
        {   // SDL-domain conversion feeding input_mod_poll.
            GCController* sdl = [GCController controllerWithExtendedGamepad];
            [sdl.extendedGamepad.leftThumbstick setValueForXAxis:1 yAxis:0.5f];
            [sdl.extendedGamepad.rightTrigger setValue:1];
            [sdl.extendedGamepad.buttonA setValue:1];
            [sdl.extendedGamepad.buttonMenu setValue:1];
            [sdl.extendedGamepad.dpad setValueForXAxis:-1 yAxis:0];
            const auto converted = sadx::input::mac::input_mod_gamepad_state(sdl);
            require(converted.has_value(), "SDL conversion rejected a snapshot");
            const auto& g = converted->state;
            require(g.axes[0] == 32767 && g.axes[1] == -16383 && g.axes[5] == 32767 && g.axes[4] == 0,
                    "SDL axis scale, Y negation or trigger range incorrect");
            require(g.buttons[6] && g.buttons[13] && !g.buttons[12], "SDL start/dpad order incorrect");
            require(converted->labels_resolved && g.buttons[0] && !g.buttons[1], "SDL A from the labelled A button");
            [sdl.extendedGamepad.leftThumbstick setValueForXAxis:-1 yAxis:-1];
            const auto edge = sadx::input::mac::input_mod_gamepad_state(sdl);
            require(edge && edge->state.axes[0] == -32767 && edge->state.axes[1] == 32767, "full negative deflection");
            require(!sadx::input::mac::input_mod_gamepad_state(nil) && !sadx::input::mac::input_mod_gamepad_state([GCController controllerWithMicroGamepad]),
                    "missing extended profile must be rejected");
        }
        adapter.controller_sample(nil);
        require(state.sample_tick(sse, axes_constants).released == 4, "disconnect did not release controller");
        adapter.keyboard_event(key(kVK_ANSI_X, true)); state.sample_tick(sse, axes_constants);
        require(!adapter.controller_sample([GCController controllerWithMicroGamepad]),
                "unsupported controller profile must report rejection");
        require(state.sample_tick(sse, axes_constants).held == 4, "unsupported profile cleared keyboard ownership");
        require(adapter.focus_notification([NSNotification notificationWithName:
                    NSApplicationDidResignActiveNotification object:nil]), "app focus notification ignored");
        require(state.sample_tick(sse, axes_constants).held == 0, "app focus loss left a held key");
        std::cout << "mac input adapter passed\n";
    }
}

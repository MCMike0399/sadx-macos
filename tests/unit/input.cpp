#include "input/input.hpp"
#include "game/speedrun.hpp"
#include "input_keyboard_goldens.hpp"

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
}
std::uint32_t word(float value) {std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
bool near(float left, float right) { return std::abs(left - right) < 0.000001f; }
}

int main() {
    sadx::SseContext sse(0x1f80); // Test-selected MXCSR, not observed PC state.
    const sadx::InputModAxesConstants axes_constants{{0x34000000}, {0x46fffe00},
        {0x42fe0000}, {0x43000000}, {0x3f800000}}; // Pinned DLL .rdata.

    using namespace sadx::input;
    for (unsigned profile = 0; profile < 4; ++profile) {
        InputState keyboard;
        sadx::SseContext selected(keyboard_goldens[profile * 13].mxcsr);
        for (unsigned step = 0; step < 13; ++step) {
            const auto& golden = keyboard_goldens[profile * 13 + step];
            if (golden.focus) keyboard.focus_lost();
            if (golden.scancode) keyboard.keyboard_event(golden.scancode, golden.down, golden.repeat);
            const auto sampled = keyboard.sample_tick(selected, axes_constants);
            const auto raw = keyboard.keyboard_raw_sticks();
            require(raw.left.x == golden.raw_x && raw.left.y == golden.raw_y,
                    "raw ramp must advance exactly once through repeat/opposing/focus transitions");
            require(std::uint32_t(std::int32_t(sampled.left.x)) == golden.x &&
                    std::uint32_t(std::int32_t(sampled.left.y)) == golden.y &&
                    word(sampled.left.magnitude) == golden.magnitude,
                    "sampled keyboard words differ from actual DLL instruction goldens");
            require(sampled.mxcsr_after_sampling == selected.mxcsr(), "selected SSE status metadata lost");
            if (golden.raw_x || golden.raw_y)
                require(sampled.left_source == AnalogSource::input_mod_261_keyboard, "keyboard source metadata lost");
            require(sampled.held == (step == 8 || step == 9 ? 8U : 0U) &&
                    sampled.pressed == (step == 8 ? 8U : 0U) && sampled.released == (step == 10 ? 8U : 0U),
                    "normalization must preserve sampled Start/repeat/focus edges");
        }
    }
    // Arithmetic failure consumes one raw ramp tick but commits no button
    // edge. A later explicit sample advances once more, never hidden retry.
    InputState failed;
    failed.keyboard_event(79, true); failed.keyboard_event(81, true); failed.keyboard_event(40, true);
    sadx::SseContext precision_trap(0x0f80);
    bool trapped = false;
    try {(void)failed.sample_tick(precision_trap, axes_constants);}catch(const sadx::SseError& e){trapped=e.exceptions()==32;}
    require(trapped && failed.keyboard_raw_sticks().left.x == 8192 && failed.keyboard_raw_sticks().left.y == 8192,
            "failed sample must retain exactly one consumed ramp update");
    const auto recovered = failed.sample_tick(sse, axes_constants);
    require(failed.keyboard_raw_sticks().left.x == 16384 && recovered.pressed == 8,
            "explicit next sample must not duplicate ramp advance or lose uncommitted button edge");
    InputState input(KeyboardProfile::immediate);
    input.keyboard_event(80, true);
    require(input.sample_tick(sse, axes_constants).left.x == -127, "left arrow must move left");
    input.keyboard_event(79, true);
    require(input.sample_tick(sse, axes_constants).left.x == 0, "opposing arrows must cancel immediately");
    input.keyboard_event(80, false);
    input.keyboard_event(82, true);
    auto tick = input.sample_tick(sse, axes_constants);
    require(tick.left.x == 89 && tick.left.y == -89 && tick.left.magnitude == 1,
            "diagonal must preserve normalized magnitude and signed SDK direction");
    require(tick.left_source == AnalogSource::immediate_keyboard,
            "explicit immediate profile must retain distinct provenance");
    input.keyboard_event(13, true);
    require(input.sample_tick(sse, axes_constants).right.x == -127, "right stick keyboard direction lost");
    input.keyboard_event(16, true);
    tick = input.sample_tick(sse, axes_constants);
    require(tick.right.x == -89 && tick.right.y == 89,
            "Input Mod default J/M camera keys must retain their physical scancodes");

    input.keyboard_event(40, true);
    tick = input.sample_tick(sse, axes_constants);
    require(tick.held == 0x8 && tick.pressed == 0x8, "Start press edge incorrect");
    input.keyboard_event(40, true, true);
    tick = input.sample_tick(sse, axes_constants);
    require(tick.held == 0x8 && tick.pressed == 0,
            "autorepeat must not generate a new pressed edge");
    require(sadx::speedrun::in_game_skip_check(tick.held, tick.pressed).requested,
            "held keyboard Start must remain available for Skipma");
    input.focus_lost();
    input.keyboard_event(40, true, true);
    tick = input.sample_tick(sse, axes_constants);
    require(tick.held == 0 && tick.released == 0x8 && tick.left.x == 0 && tick.right.x == 0,
            "focus loss must clear keys and ignore repeat reassertion");
    require(input.sample_tick(sse, axes_constants).released == 0, "release edge must last one tick");

    // Rejected rebinding is atomic; previous defaults survive.
    require(!input.bind_keyboard({{27, Action::jump}, {27, Action::pause}}),
            "duplicate physical binding accepted");
    require(!input.bind_keyboard({{0, Action::jump}}) &&
                !input.bind_keyboard({{512, Action::jump}}) &&
                !input.bind_keyboard({{-1, Action::jump}}) &&
                !input.bind_keyboard({{4, static_cast<Action>(-1)}}),
            "invalid binding accepted");
    require(!input.keyboard_event(-1, true) && !input.keyboard_event(512, true),
            "out-of-range event accepted");
    input.keyboard_event(27, true);
    require(input.sample_tick(sse, axes_constants).held == 0x4, "rejected binding modified previous map");
    require(input.bind_keyboard({{4, Action::pause}, {5, Action::pause}}),
            "multiple physical aliases for one action rejected");
    input.keyboard_event(4, true);
    input.keyboard_event(5, true);
    tick = input.sample_tick(sse, axes_constants);
    require(tick.pressed == 0x8 && tick.released == 0x4,
            "remapping held keys must transition semantic actions next tick");
    input.keyboard_event(4, false);
    require(input.sample_tick(sse, axes_constants).held == 0x8, "releasing one alias cleared another held alias");

    NormalizedControllerSnapshot controller;
    controller.buttons[static_cast<unsigned>(PhysicalButton::start)] = true;
    input.normalized_controller_snapshot(controller);
    input.keyboard_event(5, false);
    require(input.sample_tick(sse, axes_constants).held == 0x8,
            "keyboard release must preserve same action held on controller");
    input.focus_lost();
    require(input.sample_tick(sse, axes_constants).released == 0x8, "focus loss must release controller actions");
    controller = {};
    controller.left = {0.3f, 0.4f};
    controller.buttons[static_cast<unsigned>(PhysicalButton::south)] = true;
    require(input.normalized_controller_snapshot(controller), "valid controller axes rejected");
    tick = input.sample_tick(sse, axes_constants);
    require(tick.held == 0x4 && tick.left.x == 76 && tick.left.y == 101 &&
                near(tick.left.magnitude, 0.5f),
            "physical south must jump without losing analog magnitude");
    require(tick.left_source == AnalogSource::native_normalized_controller_snapshot,
            "native normalized controller must not claim original SDL raw parity");
    require(std::string(face_label(ControllerFamily::xbox, PhysicalButton::south)) == "A" &&
                std::string(face_label(ControllerFamily::switch2, PhysicalButton::south)) == "B" &&
                std::string(face_label(ControllerFamily::switch2, PhysicalButton::east)) == "A",
            "Nintendo legends must describe different physical positions");
    require(input.bind_controller({{PhysicalButton::east, Action::jump}}),
            "explicit letter-based remap to east rejected");
    require(input.sample_tick(sse, axes_constants).held == 0, "remap must retain physical south state, not relabel it east");
    controller.buttons[static_cast<unsigned>(PhysicalButton::east)] = true;
    input.normalized_controller_snapshot(controller);
    require(input.sample_tick(sse, axes_constants).pressed == 0x4, "remapped east must jump");
    require(!input.bind_controller({{PhysicalButton::east, Action::jump},
                                   {PhysicalButton::east, Action::pause}}) &&
                !input.bind_controller({{PhysicalButton::count, Action::jump}}),
            "invalid controller map accepted");
    require(!input.bind_controller({{PhysicalButton::south, Action::move_left}}),
            "controller button directions must not contaminate keyboard raw state");
    controller.left.x = std::numeric_limits<float>::quiet_NaN();
    require(!input.normalized_controller_snapshot(controller), "NaN axis accepted");
    controller.left.x = 2;
    require(!input.normalized_controller_snapshot(controller), "out-of-range axis accepted");
    require(input.sample_tick(sse, axes_constants).held == 0x4,
            "rejected controller sample modified accepted state");

    input.focus_lost(); input.sample_tick(sse, axes_constants);
    input.keyboard_event(5, true); input.keyboard_event(5, false);
    require(input.sample_tick(sse, axes_constants).pressed == 0,
            "sampled policy must not invent a buffered between-tick tap");

    InputState arbitration;
    NormalizedControllerSnapshot snapshot;
    snapshot.left = {0.5f,0}; arbitration.normalized_controller_snapshot(snapshot);
    require(arbitration.sample_tick(sse,axes_constants).left_source == AnalogSource::native_normalized_controller_snapshot,
            "native snapshot source must be reported before keyboard input");
    arbitration.keyboard_event(79,true);
    const auto keyboard_priority = arbitration.sample_tick(sse,axes_constants);
    require(keyboard_priority.left_source == AnalogSource::input_mod_261_keyboard &&
            word(keyboard_priority.left.magnitude) == 0x3e800100,
            "keyboard normalization must precede per-stick source arbitration");
    arbitration.keyboard_event(79,false);
    require(arbitration.sample_tick(sse,axes_constants).left_source == AnalogSource::native_normalized_controller_snapshot,
            "zero keyboard raw ramp must return stick ownership to native snapshot");

    // Default compatibility profile preserves the v2.6.1 8192/32767 ramp.
    InputState compatible;
    compatible.keyboard_event(79, true);
    const float ramp_magnitudes[] = {8192.0f / 32767, 16384.0f / 32767,
                                     24576.0f / 32767, 1};
    for (const float magnitude : ramp_magnitudes) {
        tick = compatible.sample_tick(sse, axes_constants);
        require(tick.left.x == 127 && tick.left.y == 0 &&
                    near(tick.left.magnitude, magnitude),
                "default keyboard ramp differs from Input Mod 2.6.1");
    }
    compatible.keyboard_event(80, true);
    for (const float magnitude : {24575.0f / 32767, 16383.0f / 32767,
                                  8191.0f / 32767, 0.0f}) {
        tick = compatible.sample_tick(sse, axes_constants);
        require(near(tick.left.magnitude, magnitude),
                "opposing directions must ramp prior axis to zero in compatibility mode");
    }
    compatible.keyboard_event(80, false);
    compatible.keyboard_event(82, true);
    tick = compatible.sample_tick(sse, axes_constants);
    require(tick.left.x == 89 && tick.left.y == -89 &&
                near(tick.left.magnitude, std::sqrt(2.0f) * 8192 / 32767),
            "compatibility diagonal direction/magnitude differs from source");
    compatible.sample_tick(sse, axes_constants); compatible.sample_tick(sse, axes_constants); compatible.sample_tick(sse, axes_constants);
    compatible.keyboard_event(79, false); compatible.keyboard_event(82, false);
    for (const float magnitude : {1.0f, std::sqrt(2.0f) * 16383 / 32767,
                                  std::sqrt(2.0f) * 8191 / 32767, 0.0f}) {
        tick = compatible.sample_tick(sse, axes_constants);
        require(near(tick.left.magnitude, magnitude),
                "diagonal release ramp/clamped magnitude differs from source");
    }
    compatible.keyboard_event(79, true); compatible.sample_tick(sse, axes_constants);
    compatible.keyboard_event(40, true);
    tick = compatible.sample_tick(sse, axes_constants);
    require(tick.pressed == 8, "compatibility button press edge lost");
    compatible.focus_lost();
    tick = compatible.sample_tick(sse, axes_constants);
    require(near(tick.left.magnitude, 8192.0f / 32767) && tick.released == 8,
            "compatibility focus loss must retain raw ramp while releasing keyboard buttons");
    NormalizedControllerSnapshot independent;
    independent.buttons[static_cast<unsigned>(PhysicalButton::start)] = true;
    compatible.normalized_controller_snapshot(independent);
    compatible.focus_lost();
    require(compatible.sample_tick(sse, axes_constants).held == 8,
            "keyboard focus event must not invent a controller release");
    std::cout << "input state passed\n";
}

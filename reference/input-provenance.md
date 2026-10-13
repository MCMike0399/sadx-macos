# Native shared keyboard/controller input findings

Date: 2026-10-11. Implemented `sadx-native/src/input.hpp`, `input.cpp`, `tests/input.cpp`, and `licenses/input-mod-MIT.txt`. These are platform-independent source components, with no real keyboard/controller backend or game-loop connection. No game files changed and no external packages installed.

## Source identity and observed behavior

Pinned upstream Input Mod **v2.6.1**, resolved commit/tree **68e2a91bec4a55967674c1420ecd9abf89bcd0ce**; loader submodule **c222b961ab1cdb7ac3ddc69bc07fe4d9c812ffe5**. Source listing and commit API records are retained as input-tree.json and input-commit.json. Source files:

* https://github.com/michael-fadely/sadx-input-mod/blob/68e2a91bec4a55967674c1420ecd9abf89bcd0ce/sadx-input-mod/KeyboardMouse.cpp — SHA-256 **232339dd102794cbd9321f961d1fee335e848bcce56feba516baa3e5ac99b053** (retained input-keyboard.cpp)
* https://github.com/michael-fadely/sadx-input-mod/blob/68e2a91bec4a55967674c1420ecd9abf89bcd0ce/sadx-input-mod/KeyboardMouse.h — SHA-256 **10038170b2c77f9869a8ad2020c0761e303087f980b034b33387b13cfada6e15** (retained input-keyboard.h)
* https://github.com/michael-fadely/sadx-input-mod/blob/68e2a91bec4a55967674c1420ecd9abf89bcd0ce/sadx-input-mod/DreamPad.cpp — SHA-256 **8df4c9adafee7d1fa774f0ec1764e40d708ab0675711b98820ff0ef1c2d175af** (retained input-dreampad.cpp)
* https://github.com/michael-fadely/sadx-input-mod/blob/68e2a91bec4a55967674c1420ecd9abf89bcd0ce/sadx-input-mod/mod.cpp — retained input-mod.cpp, establishes configurable Windows VK defaults.

`KeyboardStick::amount` is **8192**. `KeyboardStick::update()` uses interpolation: one direction increments/decrements the raw axis by 8192, clamped to **[-32767,32767]**. Simultaneous opposing directions choose neither; previous axis approaches zero by 8192 each sample. It does not cancel prior analog momentum instantly. Release has the same ramp toward zero. Up is negative Y.

`normalize()` computes float Euclidean length of raw axes, produces signed direction components truncated from **127 * axis / length**, and stores separate magnitude **min(1, length/32767)**. A diagonal full direction thus reports **89,89** with magnitude **1**, while a cardinal first keyboard sample reports **127,0** with magnitude **8192/32767**. Normalized direction and magnitude must remain separate because interpreting 127 as full input strength would destroy ramp parity. Upstream FPU/compile flags were not established; formulas/order are reproduced, but native float rounding has not been compared with x86 runtime.

`DreamPad::poll()` gives keyboard stick priority when keyboard is allowed and its signed stick direction is nonzero; it does not add keyboard and gamepad axes. Left/right stick arbitration is independent. Keyboard/gamepad digital buttons merge. `DreamPad::update_buttons()` derives pressed = new & ~old and released = old & ~new at sampling time.

Pinned `mod.cpp` VK defaults translate to physical SDL-compatible scancodes: arrows for movement; **J/L/I/M** for camera stick; **X/Z/A/S** for SADX A/B/X/Y; **Enter** for Start; **Q/W** for L/R camera triggers. The shared model includes these verified subset defaults. SDL_NUM_SCANCODES=512 was checked in the local SDL header; 0 is UNKNOWN. Scancode values preserve physical positions independent of typed text. A native adapter must translate macOS hardware keycodes; this component does not reinterpret those hardware codes directly.

Original SDK masks observed in local SADXEnums.h: A=0x4, B=0x2, X=0x400, Y=0x200, Start=0x8, L=0x20000, R=0x10000. Semantic movement/look actions have no invented button bit; they generate axes.

## Implemented policies

`InputState` defaults to **KeyboardProfile::input_mod_261**. Ramp state is updated once per `sample_tick()`, matching the pinned arithmetic and step parameters for supported keyboard actions. Immediate direction/cancellation is available only by explicit **KeyboardProfile::immediate** construction; it is a native option, not the default or a claimed original behavior.

Physical keyboard events and controller button positions are stored independently from semantic action bindings. Rebinding replaces a complete table atomically; invalid action enums, duplicate physical entries, scancode UNKNOWN/out-of-range, and invalid controller enums are rejected. Multiple physical aliases for one action are allowed; releasing one alias preserves other held aliases. Rebinding a held key changes semantic held state at the next tick.

Keyboard autorepeat does not change held state. Edges are derived from semantic aggregate state at each simulation sample, preventing repeats or keyboard/controller overlap from fabricating fresh presses. A down/up tap entirely between ticks is not buffered, consistent with the sampled button policy. Held Start persists without fresh pressed edges and was tested with the Skipma semantic API.

Controller inputs are preprocessed normalized float axes in [-1,1]; NaN, infinity, or out-of-range values reject the entire sample without overwriting accepted state. Output retains signed direction/magnitude separately. This is a boundary model, not a full port of Input Mod's signed SDL axis conversion/deadzones/radial configuration. Those remain adapter work. Trigger digital booleans likewise require an explicitly selected platform threshold.

Defaults bind **physical south/east/west/north** to jump/action/pickup/whistle, not button letters. Xbox south label is A and Switch 2 south label is B; Nintendo A is east. `face_label()` only supplies display legends and does not alter bindings. A user who wants Nintendo A to jump must explicitly bind east to jump. Tests ensure rebinding does not silently relabel stored south as east.

Compatibility focus loss clears physical keyboard keys, buttons and direction flags, while retaining the raw keyboard ramp and independent controller sample, matching the inspected WM_KILLFOCUS owner. The next tick derives edges from the resulting aggregate state; the application must own any pause-on-inactive cadence. The explicit immediate profile clears ramp and controller state as its selected native policy. Autorepeat after clear cannot reactivate old keys. The AppKit adapter accepts actual focus notifications in tests; delivery from a gameplay window and real app timing remain unverified.

## Verification and remaining scope

Strict C++17 compilation with **-Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math** and **UBSan** passed; test executable reports `input state passed`. Tests cover exact four-sample ramp, opposing-direction and diagonal-release sequences, separate magnitude, explicit immediate option, action masks, right-stick keys, Start edges/Skipma, repeat handling, focus release, aliases, remap failure atomicity, simultaneous keyboard/controller ownership, physical face labels, invalid axes and sampled between-tick tap policy.

This validates the shared input model only. Native keyboard event delivery, physical controller availability, game-loop sample cadence, mouse control, analog half-press, camera recenter, extended C/D/Z keys, D-pad menu input, rumble, multiplayer, deadzones/trigger calibration, demo playback and soft-reset integration remain unimplemented/unverified here. It therefore does not prove keyboard or controller playability of the full native game. Loader API compatibility and speedrun acceptance remain separate requirements.

## License

Pinned Input Mod is MIT, **Copyright (c) 2015 Michael Fadely**. The original notice was read from v2.6.1 LICENSE.md and retained both in input-LICENSE.md and `sadx-native/licenses/input-mod-MIT.txt`. Native code is rewritten around a platform-neutral interface, with compatible ramp/normalization behavior credited and the MIT notice preserved. This notice does not grant rights to game assets or executable reconstruction.

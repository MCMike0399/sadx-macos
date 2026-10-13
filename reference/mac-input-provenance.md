# Native macOS input adapter provenance

Implemented `sadx-native/src/mac_input.hpp`, `mac_input.mm`, and `tests/mac_input.mm` on 2026-10-11. The adapter compiles as native arm64 Objective-C++ against AppKit and GameController, with no windows, global input injection, monitors, discovery service or external installation.

## Production boundary and observed tests

`InputAdapter::keyboard_event` receives real **NSEvent** objects and dispatches keyDown/keyUp through the existing portable `InputState`. Physical `NSEvent.keyCode` is translated via Apple Carbon `kVK_*` constants to explicitly SDL-compatible scancodes. The adapter never reads `characters` or `charactersIgnoringModifiers`. Tests constructed real Cocoa key events with conflicting text (`é` on physical X, unrelated text on release) and verified the mapped SADX action remains tied to keyCode. Unknown hardware codes, currently unimplemented modifier keys and non-key events return false without changing held input.

Mappings cover standard physical letters, digits, punctuation, arrows, navigation, keypad, ISO section and F1–F20. Binding choice stays in `InputState`; a tested Space-to-jump remap reaches the model without changing the adapter. The namespace is physical and layout independent, so a letter printed differently on another layout can select a different physical key; no character-based default substitution occurs.

Autorepeat keyDown is ignored as a state transition; keyUp always clears the physical key, independent of the repeat flag. The model samples pressed/released edges once per simulation tick. Native event tests verified arrow ramp sequences, opposing arrows easing to zero, J/M camera diagonals, X/Z/A/S actions, Q/W triggers, Return/Start, and held Return feeding the Skipma check after the initial pressed edge is gone.

`focus_notification` recognizes **NSWindowDidResignKeyNotification** and **NSApplicationDidResignActiveNotification**, calls the shared focus owner, clearing keyboard keys/directions while preserving the compatibility profile’s raw ramp and independent controller state. Button edges are derived at the next simulation tick. The explicit immediate profile clears all input/ramp state. It was tested with real NSNotification objects for both names. `focus_lost()` is also callable directly by the application's focus owner. The app must route notifications from the relevant gameplay window/application; the adapter does not install an observer or pick a window. Unrelated notifications are ignored.

The adapter also captures stable **GCController** extended-profile snapshots through `[controller capture]`. SDK `GCExtendedGamepad.h` explicitly arranges buttonA south, buttonB east, buttonX west and buttonY north. These are mapped to **physical** positions in `ControllerSample`; bindings retain their own actions. Nintendo printed letters never select an Apple profile element. Tests used Apple's real developer-created `controllerWithExtendedGamepad` snapshot objects (no physical device) to verify south-to-jump, a user remap from south to east, menu/Start, and independent left/right sticks.

Apple axes have positive Y **up**; the adapter inverts Y once to match SDL/SADX positive Y **down**. Apple's GCControllerAxisInput values are already deadzoned and saturated. No second deadzone is applied; a 0.25 snapshot magnitude survived production sampling in tests. This platform policy cannot promise the same analog deadzone as original SDL Input Mod. Thumbstick output normalization/quantization still belongs to shared InputState.

Trigger digital state follows the observed Input Mod pressure rule: **trunc(value * 255) > configuredThreshold**, default **30**. Boundary tests verified 30 does not press, 31 presses; threshold 256 is rejected. Native `isPressed` is used for face/menu buttons but not substituted for the selected trigger threshold. Trigger pressure itself, rumble and calibration are not exposed by this adapter.

`controller_sample(nil)` represents disconnect and clears controller state. Unsupported profiles also clear stale controller state and report rejection, while preserving keyboard ownership. Tests exercised nil disconnect and a native micro-gamepad snapshot rejection. Discovery, controller selection, disconnect notification delivery and physical hardware validation remain application responsibilities.

## Source authorities

Apple installed SDK: `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`. Carbon `HIToolbox/Events.h` documents that kVK constants identify physical keys and ANSI names refer to US physical positions, not characters from all layouts. SDL-compatible scancode identities were checked against the local SDL header. The conversion table uses published key identities and no copied SDL backend implementation.

Inspected SDK header SHA-256 identities:

| Header | SHA-256 |
| --- | --- |
| Events.h | b80b0c6e10ea4a06494d02399424cb806cc04a2971724688cb2063e04f2149d6 |
| GCExtendedGamepad.h | 18273fdeaff7e2223e3d01ba6ecd9d65f1ae07bae2e0c752f7f18299c32673cf |
| GCControllerAxisInput.h | 001e880396a5ab85b5d29128a2ef41e1d315566c7a60555275187f2d904e3bcf |

Primary documentation:

* [NSEvent keyCode](https://developer.apple.com/documentation/appkit/nsevent/keycode)
* [GCControllerDirectionPad](https://developer.apple.com/documentation/gamecontroller/gccontrollerdirectionpad) specifies right as positive X, up as positive Y.
* [GCExtendedGamepad](https://developer.apple.com/documentation/gamecontroller/gcextendedgamepad)
* [GCControllerAxisInput](https://developer.apple.com/documentation/gamecontroller/gccontrolleraxisinput) documents deadzoned/saturated normalized values; installed header additionally inspected.
* [GCController capture](https://developer.apple.com/documentation/gamecontroller/gccontroller/capture()) describes stable profile sampling.

SDL 3.4.18 Apple producer source corroborates the Y inversion and Apple face order: [SDL_mfijoystick.m](https://github.com/libsdl-org/SDL/blob/release-3.4.18/src/joystick/apple/SDL_mfijoystick.m), retained `/private/tmp/sadx-mod-findings/sdl-mfi.m`, SHA-256 **8508e0888131c9d99a04db720cc969cf289f1b95e67c033c94810471e6a04dd8**. It was read as corroborating evidence; no source bodies copied.

Input Mod v2.6.1 commit **68e2a91bec4a55967674c1420ecd9abf89bcd0ce** corroborates raw pressure conversion to 0–255 and strict greater-than comparison in [DreamPad.cpp](https://github.com/michael-fadely/sadx-input-mod/blob/68e2a91bec4a55967674c1420ecd9abf89bcd0ce/sadx-input-mod/DreamPad.cpp). Default threshold 30 is in [DreamPad.h](https://github.com/michael-fadely/sadx-input-mod/blob/68e2a91bec4a55967674c1420ecd9abf89bcd0ce/sadx-input-mod/DreamPad.h), retained SHA-256 **8521658c2886476318763093dac60fcb95910c41a4a3eece7227d21f9738ff4c**. The Input Mod MIT notice is preserved in `sadx-native/licenses/input-mod-MIT.txt`.

## Verification scope and limits

Test executable built and ran successfully as arm64 on macOS **27.0.1 (26A434)** with **Apple clang 21.0.0 (clang-2100.3.34.2)**. Optimized strict build and UBSan build both printed **mac input adapter passed**:

```sh
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -ffp-contract=off -fno-fast-math -arch arm64 -fobjc-arc \
  -mmacosx-version-min=14.0 -Isadx-native/src \
  sadx-native/tests/mac_input.mm sadx-native/src/mac_input.mm \
  sadx-native/src/input.cpp sadx-native/src/speedrun.cpp \
  -framework AppKit -framework GameController -o /private/tmp/sadx-mod-findings/mac-input-tests
/private/tmp/sadx-mod-findings/mac-input-tests
```

The second build added `-fsanitize=undefined` and ran independently. These tests exercise the actual production adapter against native Cocoa/framework objects; they do not exercise physical keyboard event delivery, attached controllers, gameplay-window focus routing, a complete game loop, controller discovery or mod-loader integration. No full Xbox/Switch 2 support or physical keyboard/gameplay compatibility claim is established. Modifier `flagsChanged`, JIS-specific keys, function/media special events and unavailable physical scancodes remain unimplemented here. Full Input Mod analog parity still requires raw-axis/deadzone decisions and original-runtime comparison.


## Explicit SSE sampling integration

InputState::sample_tick now requires the sample owner's selected SseContext and raw Input Mod normalizer constants. The AppKit adapter only supplies physical events/snapshots; it constructs no FP defaults. Raw keyboard axes advance once per sampled tick and pass through the recovered Input2.6.1 two-stick scalar normalization before native arbitration. Tests send actual NSEvent objects through the production adapter and match52 actual-DLL arithmetic goldens across4 explicit RC profiles, including layout-independent arrows, diagonal/opposing transitions, release, repeat, Start and focus loss. Sticky MXCSR output is explicit software status, not observed Windows hardware state. Native Cocoa optimized/UBSan tests passed without windows or injected physical events.

Controller captures supply NormalizedControllerSnapshot float axes: Apple has already normalized/deadzoned them. InputState uses explicit SSE operations for its named native normalized-controller policy, with no host sqrt, added deadzone, integer reinterpretation or original SDL raw identity claim. Per-stick AnalogSource preserves this difference in every sampled TickInput. Captured physical devices, original raw-controller production, half-press, recenter, D-pad, priority/demo semantics and OS/hardware timing remain unverified or missing. Existing trigger float multiplication is part of the native Apple adapter policy; it is not proof of exact original raw SDL trigger preprocessing.

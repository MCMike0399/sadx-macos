# GetAnalog / native player input query provenance

Implemented `sadx-native/src/player_input.hpp/.cpp`, `tests/player_input.cpp`, reusable native probe `tests/native_player_input_probe.cpp`, and `tools/verify_original_player_input.py` on 2026-10-11. No original game file, shared InputState/mac_input implementation or Makefile changed.

## Source identity

The supplied `Sonic Adventure DX/sonic.exe` is SHA-256 **6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74**. Procedure **0x43bdc0..0x43bf33**, **372 bytes**, SHA-256 **1605a9592f99613b75f7c81c753209e6e4d89954007e2d6868cb367eb40b2ae3**. LLVM disassembly is `_reference/sadx/player-input-query.asm`; REA dossier is `_reference/sadx/coordinates/input-query.json`, Evidence **ev_5c55442bc23bf130098ae6aa9bb8e7009d1f292e54466e0ad4e8252a2a86b1c6**.

SDK calls this function **GetAnalog(EntityData1*, Angle*, float*)**. **PGetSpeed** itself is **0x443f50** and invokes GetAnalog with both output pointers null. That distinction prevents confusing input-query behavior with complete movement physics. The query code uses **x87 FCOMP** and, only on its forced angle-output path, x87 FPATAN/multiplication/CRT conversion. No SSE arithmetic appears in the inspected query instructions.

The current intended fidelity reference remains the isolated updated PC profile: loader **676**, external Input **2.6.1**, Speedrun **2.0**, explicit 63 FPS config, manifest **795918d0e40d30437a0fde905d40917cac861ef46b3e86dff0cffdac901f8cd8**. None of those DLLs or a native Windows process was executed by this function-local comparison. Static source inspection found Input Mod replaces upstream normalized magnitude production, not the GetAnalog body; live updated-profile bytes/behavior remain a separate proof requirement.

## Recovered branches and owning fields

Native `PlayerInputTask` owns decoded task flags, player index, character byte, yaw and optional decoded event move mode. Native `PlayerInputRuntime` retains independently unavailable stage, act, current-character global, per-player normalized input, no-control timer and camera yaw. These are native values, not casts of guest memory or implicit zero/identity globals.

* **Flag 0x4000** reads taskwk.ewp at **+0x3c**, then eventwk.move.mode byte at **+0x24**. It returns whether that byte is nonzero and leaves angle/magnitude outputs untouched. It does not read stage, pad slot or x87. Missing event owner/state is an explicit failure.
* Otherwise source reads signed 16-bit stage **0x3b22dcc**. **Flag 0x2000** considers stage **2/act2** or **4/act0**; current character comes from real callee **0x4144e0**, observed `movzx word [0x3b22dc0]`. When that current-character word is **0**, the query returns active with optional magnitude **1.0**. A null angle output skips all forced-angle floating arithmetic and camera lookup. This fully recovered path supports the real PGetSpeed null-output gameplay gate.
* On the ordinary path, task counter byte **+0x08** indexes normalized input at **0x3b0e7a0 + 8*index** (int32 direction, binary32 magnitude). The native model supports the SDK's **8** known slots; an out-of-range counter reports a boundary failure rather than reading an invented slot.
* `FLD magnitude; FCOMP +0; FNSTSW AX; TEST AH,0x44; JNP` rejects **only ordered equality to zero**. Both signed zeros are inactive; negative, infinite and unordered magnitudes enter the active branch. Quiet/signaling NaNs raise architectural invalid through signaling FCOMP when the selected x87 context supports it; no host `magnitude>0` simplification is used.
* On that active branch, without **flag 0x2000**, the selected `playerpwp[index]` at **0x3b3cdf0 + 4*index** must have word **+0x0a equal0**. The SDK identifies this as **playerwk.nocontimer**, not playerwk.flag (which is +0x10). Missing no-control timer is distinct from a known zero/enabled input state.
* Successful ordinary input copies requested int32 direction first, then requested binary32 magnitude via raw word stores. No clamp, normalization or NaN payload conversion occurs in these stores. A known magnitude may have an independently unknown angle; a null-angle query remains evaluable, whereas a requested unknown direction fails at **0x43beb5** with known active=true and leaves magnitude output untouched.
* Zero magnitude has a separate fallback: task character byte **+0x09 equal0**, task **ball flag0x100**, and **signed** stage/act `MOVSX; SHL8; OR` result **0x902 or0x903**. This returns task yaw and **positive zero** magnitude. Negative stage/act words are preserved/sign-extended, never normalized to unsigned display values.

Inactive/event paths do not overwrite outputs. Dependencies are checked in source-consumption order, retaining earlier arithmetic exceptions/known activity. Existing `X87Context` is passed explicitly; its actual native-PC control word remains unobserved. Unmasked exception behavior uses the library's existing X87Error contract; deferred hardware delivery/timing and full status/tag/stack are not claimed.

## Recovered forced angle output

Only when the forced stage/current-character path is taken **and an angle pointer is requested**, the source executes:

* FLD binary64 **-127** at **0x7e0050** and binary64 **+0** at **0x7e0048**;
* **FPATAN** at **0x43be36**;
* binary64 multiply by **65536** at **0x7dccd0**, then source binary64 value **-0.15915497620314795** at **0x7dcce8** (bits **bfc45f30b4e4e30a**);
* real CRT integer conversion **0x644340**, then **NEG camera yaw; SUB converted integer** modulo2^32.

`ForcedInputAngleConversion` is an explicitly required provider for that floating/integer conversion closure. It receives the selected x87 owner and returns a tagged optional int32 result. No host atan2, approximate π, hard-coded angle, hidden camera yaw or reset FPU default is substituted. The source camera lookup occurs after conversion, and native code preserves that order. Camera NEG/SUB wraps under int32 instructions, including INT_MIN. The independent resolver-boundary test retains an explicit test double to verify call order and camera wrapping. Production `convert_forced_input_angle` now binds the actual Bochs P5/P6 FPATAN model, two source-bound binary64 constants and reconstructed CRT conversion. The expanded instruction probe reads all four constants directly from the original executable and uses this arithmetic owner. No Intel Ultra hardware equivalence is claimed.

## Tests and instruction comparison

Strict optimized arm64 C++17 and UBSan semantic tests both printed **player input query passed**. They verify branch priority, output omission/untouched bytes, negative/zero/subnormal/NaN behavior, signaling compare exceptions, timer gating, stage/character conditions, independent missing angle/magnitude ownership, forced-null-angle completion, explicit forced resolver, camera subtraction wrap, and required missing-state failures.

The reusable verifier executes the **unchanged original GetAnalog instructions**, including real0x4144e0 current-character loads, under **Unicorn2.1.4** with explicit external task/global seeds. It compares native bool return and output words across **17,848 cases for each of12 selected control words** (24/53/64-bit significand modes ×4 rounding directions). It exercises all four independent output-pointer combinations, including 625 forced-angle cases per control profile (7,500 evaluations total). Sparse BSS pages/objects are test scaffolding, not assertions about startup or real game state. The original code is not replaced by input test doubles.

Result artifact: `_reference/sadx/original-player-input-comparison.json` plus hashed `.cases.bin`. **Boolean/angle/magnitude outputs have0 mismatches across214,176 evaluations. Overall passed remains false** because the emulator's x87 exception state disagrees with native architectural flags in **6,009 cases/profile**. The report separates value output pass from exception mismatch rather than hiding or masking those differences. This result proves a scoped emulator comparison, not real hardware/modded-game parity.

## Exception-oracle limitation established from its producer

The discrepancy was reproduced using an actual **FNCLEX** instruction between cases and by capturing **the source FNSTSW AX instruction's result**, not only UC_X86_REG_FPSW. Those two views agree: tested denormal/NaN cases report no DE/IE in Unicorn's architectural status while native retains them. So resetting or reading only the accessor is not the cause.

Pinned primary source [Unicorn2.1.4 fpu_helper.c](https://github.com/unicorn-engine/unicorn/blob/2.1.4/qemu/target/i386/fpu_helper.c), retained `_reference/sadx/player-input-unicorn-fpu-helper.c`, SHA-256 **809e3d8e0affb71cdf76e52da57d519ab227c223a5443039f2d41a8342979e33**:

* `helper_flds_ST0` converts through `float32_to_floatx80(...,&env->fp_status)`.
* `helper_fcom_ST0_FT0` calls `floatx80_compare(...,&env->fp_status)`, then merges only comparison condition codes into **env->fpus**.
* `helper_fnstsw` returns **env->fpus**/TOP; those paths do not transfer the SoftFloat exception flags from fp_status into the architectural status word.

This producing representation explains the observed oracle limitation. Native denormal detection/signaling compare semantics were **not weakened to obtain a green comparison**. Actual updated-PC flags and control state must be checked on the user-reported Intel Ultra/NVIDIA Windows11+AtlasOS reference; no such hardware result exists yet.

## Input producer and remaining native gameplay scope

Shared `InputState` supplies signed SDK axis direction and a separate magnitude; it does **not yet supply source-owned camera-relative direction**. Original **WriteAnalogs0x40f170** establishes direction/deadzone/global control behavior. Input2.6.1's after-WriteAnalogs hook at **0x40f343** overrides magnitude only when global ControlEnabled, selected controller-enabled state, connection/keyboard eligibility and an axis component exceeding **12 of127** permit it. Those conditions and direction arithmetic cannot be silently replaced by assigning raw Mac axis magnitude directly to every normalized slot.

This query permits a correctly produced known magnitude to bind PGetSpeed's null-angle gate without fabricating unknown direction. It does not itself close original WriteAnalogs, physical keyboard/controller delivery into gameplay, task/player lifecycle, level/collision/character state machine or complete movement. Keyboard half-press, camera re-center and D-pad/menu buttons remain missing from the current shared input layer and are explicitly **not claimed as compatible** here. Latest input DLL's floating normalization/compiler behavior versus AppKit/GameController preprocessing also needs an updated-PC runtime comparison.

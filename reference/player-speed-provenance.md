# PGetSpeed: local native control/math body with explicit callee boundaries

2026-10-11. Original supplied `sonic.exe` SHA-256:
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Reconstructed source: `sadx-native/src/player_speed.hpp/.cpp`; tests:
`sadx-native/tests/player_speed.cpp`, sole C-ABI probe
`sadx-native/tests/native_player_speed_probe.cpp`, reusable instruction runner
`sadx-native/tools/verify_original_player_speed.py`.

This implements the full local PGetSpeed control/math body and its observed
vector-add/limiter/small-speed-cleanup tail semantics. It still requires actual
input/context and rotation callees. Function-local correctness with test doubles
does not establish a functioning character/game loop or real mod gameplay.

## Static provenance and source-bound fields

Genuine REA dossier `movement/get-speed.json`, procedure `0x443f50`, has Evidence
ID `ev_cf65f09daeadac3322ee8d88a2168dbbd8521dad577e137edd685c0dc30d1dbb`.
Its structured content binds the executable digest, `analyze_function` operation,
Hopper provider/launcher digest
`dad6f886261fad0d4cdd01aee6fcf57439c60d7b5c54e62fc97f0180116690e0`,
and profile digest
`8c226db6a8bd36b662dc1cb83ab0ad0af698230fc322b58c6c4870878bf6ae70`.
Instruction order was independently corroborated with Xcode llvm-objdump.
The provider's pseudocode/assembly are reconstructed representations, not
original source; complete body-range/reference/classification coverage remains
limited by Hopper's public Python API.

PGetSpeed exact bytes `0x443f50..0x4441ab` (exclusive end), 603 bytes:
`e60e611ea247b5f1aedb3d91c3889242dd956f9ab3668981015211b4a9cf45ea`.
The reusable report also hashes actual original tail instructions at
`0x785170` (vector add), `0x43c380` (limiter), and `0x43c4c0` (cleanup).

Candidate names/types come from pinned [X-Hax SDK
eb4cdcca9637d611a98b910fcfed6ce18580451c](https://github.com/X-Hax/sadx-mod-loader/tree/eb4cdcca9637d611a98b910fcfed6ce18580451c),
then were checked against actual offsets. Reused domain owners are
`PlayerMotion`, `MotionWork`, `PlayerOrientation`, and `PlayerParameters`.
Additional `PlayerSpeedState` fields are dotp `+0x1c`, effect `+0x20`,
floor_normal `+0x50`, attributes `+0x5c`, and shadow_attributes_top `+0x1e4`.
The latter is `shadowwk.Attr_top`, not an animation flag. No guest addresses
become native pointers, no gravity/input/parameter defaults are inferred, and
all values retain caller-owned source bits.

## Reconstructed order

The function first adds player acceleration X to old player speed X, stores
binary32 speed X without dropping the extended result, computes/stores
`jog_speed * source_float(0.87)`, and performs the source FCOM/status-mask
branches. Positive/negative X crossing cases can clear speed directly; otherwise
some branches reach input gate `0x43bdc0`, then stored-speed and dotp stopping
checks. Zero/unordered old X follows its source path rather than a guessed
sign/absolute-value rule.

It then integrates/stores Y. Z stopping compares the ORIGINAL Z register value
against the threshold and newly stored Z, preserving the source comparisons,
signed threshold flip and zero stores. Terrain/shadow attributes select
`jog_speed * source_float(0.8)` versus `(run_speed+jog_speed)*0.5` thresholds and
conditional `run_break` addition, including the `0x400000` branch priority.

The local velocity vector passes through task-to-world `0x43ec90` or
floor-normal-to-world `0x43edd0`, selected by attribute `0x4000`; floor path
receives source normal and raw task yaw. Only then is world speed written.
`njAddVector` adds player effect using its observed X destination+source and Y/Z
source+destination order. The limiter performs source FCOM upper/lower checks
for X/Z then Y. Cleanup calls the input gate AGAIN and, when inactive, clears
world components strictly inside `(-0.02,0.02)`. It leaves the player-relative
speed and both acceleration owners alone.

The native software ext80 owner is explicit `X87Context`. Source intermediates
remain extended until actual binary32 stores; comparisons use signaling FCOM
and its C0/C2/C3 status-mask parity, not host bool comparisons. The internal
precise limiter currently duplicates the earlier finite host-comparison helper's
numeric rule because that helper does not record FCOM invalid for quiet NaNs;
the owning helper can be unified once its callers adopt the explicit context.

## Missing callees and partial facts

`PlayerSpeedCallees` owns typed callbacks for input/context check,
task-to-world transform, and floor-to-world transform. Empty/unknown callbacks
do not supply identity rotation or false input. They return a tagged failure
only when their source call site is actually reached. Input results preserve an
explicit unknown with `optional<bool>`; empty successful-looking results are
rejected as unavailable.

Earlier source stores remain in caller-owned state on boundary failure. Results
include the original boundary VA and a temporary vector when a transform
boundary is unresolved, including partial local values returned by a failing
transform. They do not claim rollback or safe restart from partially updated
state. The C-ABI probe guards malformed buffers and catches native arithmetic
errors so exceptions never unwind across ctypes; it is solely a test boundary.

## Native tests and original-instruction comparisons

Optimized native arm64 tests and wrapper/source AddressSanitizer plus
UndefinedBehaviorSanitizer runs passed. They cover source call order, X/Z
stopping, shadow-mask priority, strict cleanup bounds, untouched acceleration,
raw floor/yaw arguments, NaN FCOM invalid, missing/unknown callees and partial
stores at each reached boundary. Those native tests use explicitly labeled
test transforms/input providers.

The reusable runner builds 58-word source/state fixtures, decodes real 132-byte
Sonic parameters from source VA `0x9154e8`, and applies explicit perturbations
for branch coverage. Source parameter SHA-256:
`e7b11eb0865091f2baf1ea63052dcc7e8d9f0381005983137eee759152b9eb48`.
Deterministic seed is `0x443f50`. Native compared library SHA-256:
`2d84420f0cd3a320479831a545b4fb9b99f077456a1850f592d42fc2c9de2b7c`.

Authoritative retained artifacts:

- `original-player-speed-comparison.json`: twelve control-word records,
  source/native digests, overlays, exact instruction hashes, case counts,
  output/call/flag comparison scopes and examples.
- `original-player-speed-comparison.cases.bin`: 735,140 bytes, each record is
  58 little-endian uint32 source/state words plus one gate-pattern uint32;
  SHA-256 `fb82240c307cc4b973f6a24136a8b1ecfc27d79396dc332daa005bd1dc34ca66`.

Per control word, 3,115 cases comprise 675 systematic finite cases, 2,000 seeded
random cases and 440 raw signed-zero/subnormal/infinity/quiet/signaling-NaN
cases. Across all twelve combinations of 24/53/64-bit significand precision and
four rounding modes, 37,380 cases have ZERO stored-velocity-bit mismatches,
untouched-acceleration mismatches or input-call-count mismatches.

Only required external boundaries are memory-overlaid: input check is a RET
with a hook selecting EAX from the per-call gate pattern; BOTH transform
callees are explicit RET identity test doubles. PGetSpeed, vector-add, limiter
and cleanup instructions actually execute from the supplied artifact. No
Windows process/startup or installed game runtime is executed, and no source
file is patched.

Exception flags differ in 2,686 cases PER control word. Early examples have
emulator low exception bits zero and native precision bit `0x20` set while all
stored values and calls match. The report separates
`tested_values_and_calls_match: true` from
`emulator_exception_flags_match: false`. This observation suggests an emulator
flag-model limitation but is NOT proof of its cause or of hardware/native flag
equivalence. Flags and unmasked exception-delivery timing remain unverified
against original x86 hardware.

```sh
_reference/sadx/python/bin/python sadx-native/tools/verify_original_player_speed.py \
  "Sonic Adventure DX/sonic.exe" \
  /private/tmp/sadx-port-sources/native-player-speed-probe.dylib \
  --output _reference/sadx/original-player-speed-comparison.json
```

## Remaining integration

Actual global/controller/stage input semantics, both original rotation
providers, character ground/air acceleration, task/world collision/startup
integration and full gameplay still need recovery and verification. Actual
process/mod-modified FPU control state remains unknown. Updated required mods,
native GPU performance, real Xbox/Switch 2 Pro controllers and speedrun fidelity
are not established by these function-local results. The full user goal remains
active and unfinished.

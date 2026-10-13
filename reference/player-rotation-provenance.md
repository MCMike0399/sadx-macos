# Player rotation reconstruction ledger

Source executable SHA-256:
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This is an independent source slice in the native arm64 reconstruction. No
Hopper session was operated by this worker; retained root-owned REA dossiers
and direct Xcode LLVM disassembly were used. This does not establish startup,
updated mod execution, hardware equivalence or a playable game.

## Source and Evidence coverage

| Source body | Bytes / SHA-256 | Genuine REA Evidence ID |
| --- | --- | --- |
| Regular yaw `0x443c50..0x443de9` | 410 / `7f0aae2f159f38100768623cb99a8d3c6c2c578123700865ad7bc45d23942916` | `ev_083b72fc50046470a902782828124d559633223593cab82dca18951d5572b66b` |
| Blended yaw `0x443e60..0x443f46` | 231 / `f0d8e1380aa89174d29dbf93a0c89eb40605b2f6ec8a56c3ab9184cc4b69e52b` | `ev_c9566df941f8bf0399a6635a4c69306ebc55e2ea1d6c57d302f3383f151ce9d0` |
| Passive rotation `0x4491e0..0x449372` | 403 / `848fcf01201ed53a5b4c6763b8278ec182023c3650a8f4e0eb05b69006f10d7b` | `ev_0fd60920c0f5294392c68c46133e4991f8aaf7ef86704134ba6230199d85bcd7` |
| Attribute rotation `0x449380..0x4494ee` | 367 / `6f951a2ef4eaac30590a7a9416a084e2471a286b76b975d18ab4c4b0afe03bfb` | `ev_a47b6ba269cfff9d21074835670250a34cb983855993c921e5f205af35508f87` |
| Vector length `0x787cc0..0x787ce5` | 38 / `8394622b2f31ae87570f34c5cd791dfa7338a9a82f7cfec0533069c7bd7a74e0` | `ev_edf0a308ec3b52d08492161c1b127046fb2b6be38c9b34b562d775e6db30c6ca` |
| Normalize `0x418ac0..0x418b18` | 89 / `9a213f76f24e32084e219b0b1e895596a21627b79b0589fe76b02f9405a5b205` | `ev_07a452e623c466543e9bf8e3ae9c3d7c53f0d5898ae4b3b501b9e419c6ded7ea` |

Evidence IDs above were read from each dossier's `structuredContent`; unavailable
pseudocode does not become an invented decompilation. The native algorithms
are derivations from actual instructions. Retained local assembly is in
`ground-dependencies/yaw-owners.asm`, `rotation-owners.asm` and
`direction-reset.asm`. All code/float references are bound to this executable.
Earlier candidate labels `vector-squared-length` and `reset-collision-query`
are lexical guesses: `0x787cc0` returns length after a binary32 squared spill;
`0x49c330` walks and interpolates a path resource.

## Native ownership and recovered behavior

`src/player_rotation.hpp/.cpp` directly use common `PlayerTaskState`,
`PlayerPhysicsState`, `PlayerMotionState` and `PlayerParameters`. No parallel
task snapshot, packed guest32 cast or replacement movement loop was created.
The actual parameter block starts at playerwk `+0x9c`: `+0xd0` is
`dash_speed`, and `+0xc0` is `jog_speed`. SDK lexical guesses were corrected
against the source offsets before implementation.

Regular yaw first copies local speed, executes the actual task-to-world owner,
selects the AdjustAngle step from absolute wrapped distance (`>0x2000` →
`0x800`, `>0x1000` → distance/4, otherwise distance/8), stores yaw and executes
world-to-player. Flag-low-two zero selects literal `0.9f/0.1f` mixing.
Other flags require the actual GetAnalog owner; inactive input or dotp chooses
distinct retained weights, and source friction strictly below one scales the
weight. The old/new weights remain ext80 until each source store. Arithmetic
NaN, signed-zero, comparison and memory-operand behavior comes from X87Context.

Blended yaw compares source speed.x to `dash_speed`, spills
`(speed.x-dash_speed)*0.0625f` to binary32, calls the real signed-square-root
owner, multiplies by source 256, converts with actual CRT `_ftol`, and subtracts
the resulting word from `0x100`. Negative or wrapping steps are preserved.
Dotp chooses two retained source weights. Distinct regular/blended FLD versus
FMUL memory sites and X/Y/Z summation/store order are preserved.

Passive and attribute rotation implement actual timer/attribute gates, vector
length, normalization, gravity dot test, real world-to-player transform,
FPATAN(gravity.z, abs(gravity.x)), two source binary64 multiplies, CRT conversion,
target yaw and source step scaling, then real regular/fast yaw helpers.
The step strength uses abs(gravity.z), not gravity.y or velocity.y. Exact stack
tracking establishes those axes. Passive checks local gravity.y between its
source cutoffs; attribute checks its own greater-than cutoff. Their distinct
early returns and flag-0x100 policies are retained.

`rotation_vector_length` loads/multiplies X, Y, Z, sums `(z²+y²)+x²`, spills to
binary32 and calls signed sqrt. `rotation_normalize_vector` instead uses the
cached-Z/Y/X load and `(x²+y²)+z²` register sum, direct FSQRT and reciprocal.
Only equality with source zero clears Z/Y/X to positive zero. Unordered FCOM
continues the reciprocal path; it is not a zero-vector fallback.

The source passed-player `+0x0a` timer is an independently optional runtime fact,
distinct from GetAnalog's registered-player lookup. Gravity and input globals
are supplied by their native owners. Missing timer/gravity/input/matrix facts
return typed failures at actual source call/read boundaries; earlier yaw,
partial vectors and real matrix-stack effects survive. Outer and nested
coordinate/GetAnalog boundaries are retained. Unmasked software FP exceptions
remain typed X87Error; architectural deferred delivery is not modeled.

## Verification

`tests/player_rotation.cpp` takes source quarter-table, source unit-matrix and
source rotation-constant files. Constants contain 16 binary32 words followed
by three binary64 words, in the header's exact address order (88 bytes).
SHA-256 is `e57e7537f25e6b0628370057deb2f9034ddf01f9b5aa8a337cf7d77c1a597f7e`;
the test additionally pins FNV64 `e32a778b8a4695e2`.
Tests passed for twelve PC/RC profiles, distance endpoints, half-turn ties,
negative blended steps, untouched fields, missing input/timer/gravity facts,
timer/attribute gate order, normalization signed zero/NaNs and partial matrix
ownership. Optimized and ASan/UBSan runs passed; the linked prebuilt numeric
archive was not sanitizer-instrumented.

The reusable producer is `tests/native_player_rotation_probe.cpp`; the runner
is `tools/verify_original_player_rotation.py`. Fixtures are 154 raw little-endian
uint32 words, native outputs 107 words. Constants/table/unit and selected CW
cross explicitly. The original harness runs all four actual bodies, real
GetAnalog, matrix/Ninja routines and CRT conversion without instruction overlays.
Original caller register/stack conventions were verified separately per entry.

The final report has 2,538 fixtures in each of twelve explicit control profiles
(30,456 comparisons): no angle/local-speed/target-yaw/depth/all-six-matrix-frame
mismatches. It includes systematic thresholds, seeded finite state, both input
gate states, timer/attribute gates, capacity/depth/normalization modes, zero
normalization via negative jog threshold, QNaNs, infinities, subnormals and
signed zero in local speed.x. Initial signaling-NaN FLD hardware/emulator
differences remain a separately retained arithmetic limitation, not hidden
inside this passing fixture domain.

Exception flags differ in 2,503 cases per CW and are reported separately with
examples. The selected Bochs P5/P6 FPATAN model is not established equivalent
to the user's Intel hardware. Caller-selected input globals/matrix state and
source constants are experiment fixtures, not actual running-game captures.

Final fixture SHA-256:
`151f6dc1bfdddb5d60812c8a56da9457028f4a242b688a746e16a4bdeca6dcca`.
Temporary native-probe SHA-256:
`730ba9018d3a29f13e012011f747e4dd59ee86f9a02e92a9a5b7fefef452cec3`.
Report SHA-256:
`82ad7fa263f2643b9a7838231abdb47658fc137bdc899dba4bcaf3d7c6a44207`.
Root will wire the new sources/tests/probe/constants into the common build and
bind them into ground/air call composition; this worker did not edit Makefile
or shared build artifacts.

## Direction reset remains open

Source `0x43c130..0x43c2be` is 399 bytes, SHA-256
`81b1081b96cd6b25e1e5ac77c624ede15f24982fef6c176b81f8737c3ec5b991`;
Evidence `ev_2c33e4c705d861f8e152a1189832d5604571f8ac6e99c8ec898a58ca11f85582`.
It may clamp speed.x after GetAnalog, then adds speed.x to playerwk `+0x18`
and stores that field before querying taskwk `+0x10` through `0x49c330`.
That task field selects a path resource (type, count, point array), not an
already-owned collision lookup. The query's return status and output position
control later plane projection/normalization, angle correction and flag clearing.
The native shared state currently lacks this path resource/distance owner.

Root recovered the actual `0x49c330` dossier, Evidence
`ev_a5f2db334ffef6d3f380e351c192b766f145e4eea5a5cdcec4e0901a2f5d133f`.
Remaining concrete callees include path interpolation `0x49c210`, scalar policy
`0x4393c0` and signed angle difference `0x4383a0`. The vector-angle helper
`0x438410..0x4384b7` (168 bytes, SHA-256
`f895b77609e50ecd88bbbe47bbd417a6838b74b893ebf19806283e5ce057f37b`;
Evidence `ev_d32c3f5d0dc1ffd5d0e97eead86ec00880bf7ccbac9b650064fe444745e9d293`)
needs actual CRT acos `0x6450f0` and cross product `0x7889f0`.
The next implementation must recover those owners and preserve reset's earlier
distance/speed stores on failure. No successful reset stub was added. Full
ground/air/collision/resource lifecycle, mod execution, Metal rendering and
playable native-Mac fidelity remain unfinished. The subsequent axis/projection
stage closed CRT acos and vector-angle locally (see its ledger); reset's path
resource/distance/plane-projection ownership is still open.

# PGetAccelerationGround native reconstruction

Source: supplied `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
REA Hopper dossier `movement/ground-acceleration.json`, Evidence
`ev_da5de0a8dc74472633c027a47aed84ad3ca9fc10c38504d416a14ea04f3e5d24`,
plus the source-bound dossiers/exported bundle under `ground-dependencies/`.
The dossier's expanded pseudocode repeats many branches; implementation follows
actual instruction bytes in `movement/ground-acceleration.asm` instead.
The complete local routine is `0x44c270..0x44cde5`, 2,933 bytes. Pseudocode,
assembly and the native lift are reconstructed representations, not original
source or observations of a running game.

The new owner is `src/player_acceleration_ground.{hpp,cpp}`. It reuses the
single `PlayerTaskState` owner, native `PlayerMotion`/`MotionWork`, typed input
query, source matrix transforms, explicit x87 context, and caller-selected
132-byte physics values. Twenty-four binary32 constants are supplied as raw
source words, at the exact addresses in `GroundAccelerationConstants::addresses`;
the extractor now emits their 96-byte source asset. Neither gravity nor
collision-vector availability becomes an invented direction.

The local body selects alternate acceleration callees in source priority:
task flag `0x4000` → `0x447510`, attribute `0x20000` → `0x44b0e0`, attribute
`0x4000` → `0x447010`. The ordinary path combines world acceleration and
weighted gravity, transforms it into player coordinates, queries analog
direction/magnitude, evaluates the source reverse cross product and its
length, then applies the recovered slope adjustment, drag, input acceleration,
yaw selection, longitudinal/lateral friction and final acceleration stores.
The reverse cross-product helper at `0x7889f0` retains ext80 components for
its Z²+Y²+X² length calculation, spills the squared sum to binary32 and calls
the source signed-square-root helper. Angle distance `0x4383b0` uses wrapped
low-16 subtraction and NEG, including the half-turn tie.

Native motion fields `target_yaw` (+0x20), `source_field_34` (+0x34),
`source_field_38` (+0x38) and `friction_scale` (+0x3c) have independent ownership.
The unknown SDK meaning of the +0x34/+0x38 fields is explicit. In particular,
the source's friction spill to +0x38 is not a write to motion.speed.x (+0x04).
The final Y addition occurs before the player acceleration-X store; Y's
binary32 spill occurs afterward, followed by acceleration Y and Z stores.
Earlier mutations remain visible if an owning callee or arithmetic operation
fails. Missing input/coordinate dependencies preserve their nested reason and
source address. A temporary acceleration is exposed only after it has actually
been formed; failures before that point do not manufacture zero evidence.

Float memory arithmetic and FCOM use the instruction-specific x87 APIs,
preserving signaling-NaN operand classification until the operation. FABS uses
raw sign-bit clearing, including negative NaNs. Branches whose TEST AH masks
have special unordered behavior preserve those masks instead of rewriting
them as ordinary host comparisons. No host floating-point fallback, input
normalization or hidden parameter defaults were introduced.

The actual fast-heading owner `turn_player_heading` (`0x443df0`) is bound in
the tests/probe using the same task and player state plus the real matrix
owner. Other native mutation callees remain explicit typed integration
boundaries: passive rotation `0x4491e0`, direction reset `0x43c130`, attribute
rotation `0x449380`, blended yaw `0x443e60`, regular yaw `0x443c50`, and the
three alternate acceleration routines. Their retained REA dossiers guide the
next reconstruction; a supplied callback's existence is not proof its complete
original behavior has been implemented.

## Verification

`tests/player_acceleration_ground.cpp` passes with source assets in this order:
player parameter bytes, 96-byte ground constants, quarter sine table, unit
matrix. It exercises lazy dependency selection, variant priority, unavailable
gravity/input/collision owners, nested failure provenance, partial callee
mutations, the prior yaw-target store before a missing heading boundary, actual
shared-task heading integration, a complete inactive path with unused callees
still absent, cross-product aliasing and low-16 angle wrapping. A private
UndefinedBehaviorSanitizer build of these tests and the C++ closure also passes.

`tools/verify_original_ground_acceleration.py` and its native probe retain
`original-ground-acceleration-comparison.json` and the exact case blob. All
29,952 local comparisons (2,496 cases × 12 masked PC/RC profiles) match stored
player/motion values, task angles, selected callee traces and restored CW.
Cases include randomized state/vectors/angles, 176 systematic slope/speed
cases, 220 signed-zero/subnormal/infinity/NaN operand cases, and 300 independently
selected parameter cases, including unusual signs and threshold ordering.
Actual input, matrix, cross/length, wrapped distance and fast-heading source
instructions execute. There are 156 actual fast-heading calls and 2,304 calls
without any controlled mutation callee.

Five rotation/reset callees are explicitly controlled no-mutation boundaries
in both executions. These comparisons establish their caller's selection and
local arithmetic under that declared condition, not those callees' real
semantics. The native production owner supplies no no-op defaults: an absent
required callee returns an explicit failure. The report records the controlled
addresses and the count of cases that do not reach them.

Unicorn exception flags disagree on this profile and remain separately
recorded; no native precision/flag behavior was weakened to make the emulator
pass. The report preserves source/library/compiler identities, source-header
hashes, case digest, source constants and every discrepancy as hex bits.
Original Intel Ultra hardware, actual startup FPU/CRT state, latest loaded-mod
input traces, full character state updates and collision response remain
unverified. This completes the recovered local ground body with declared
dependencies; it does not establish a playable or speedrun-ready Mac game.

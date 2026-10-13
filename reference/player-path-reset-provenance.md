# Native path query and direction-reset source closure

Executable SHA-256:
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This independently recovered native arm64 slice uses retained root-owned REA
Evidence and direct Xcode LLVM assembly. No worker Hopper session, source
binary mutation, callback substitution, host floor or replacement game loop
was introduced. Local function agreement is not full startup/mod/hardware or
playable-game equivalence.

| Function | Source bytes / SHA-256 | Genuine Evidence ID |
| --- | --- | --- |
| `0x49c330` path query | 679 / `b375141a8d3eb5c5cf63aa0b793336f963fad845a6d464fdde898cb5ec090fc5` | `ev_a5f2db334ffef6d3f380e351c192b766f145e4eea5a5cdcec4e0901a2f5d133f` |
| `0x49c210` segment endpoints | 281 / `9c6a4b0cb9edb01329764ecb14d59ef7063c862641c52abf7a1d30546625db89` | `ev_11b97464ad75773ee3f17c6af84522dee2e60336c89978467ea83bdde4447599` |
| `0x4393c0` distance wrapping | 55 / `cb9b2bd4f8103ad3bdfee0e6f106537f34bb49269abbc00fd48f13c74e59b218` | `ev_2ee75f8e54acd1c0c7d281a7a1ddeaa26d5db1e644f04383602e22052278200f` |
| `0x4383a0` signed angle difference | 12 / `e57e93e20110e2620aa31b341a614a607ed41e77a39eab761fe6ab5b79427c09` | `ev_890ae318dd0bb1e3b2f3bd76a82f75b7cadbc596d25ce7c3db9fa39b06a8f8b8` |
| `0x43c130` direction reset | 399 / `81b1081b96cd6b25e1e5ac77c624ede15f24982fef6c176b81f8737c3ec5b991` | `ev_2c33e4c705d861f8e152a1189832d5604571f8ac6e99c8ec898a58ca11f85582` |
| `0x788d40` plane projection/distance | 166 / `695730f7d9068ac28ffb1dc85bfbba26fc7a0b525f10ac3c335a7bd2297a0cb1` | `ev_913ce0bfcbf1ce1399fb5f9581ff1be4df86268338e55d7ab52d6da1488099d2` |

IDs were read from retained dossiers' structuredContent. Initially named
reset-collision-query/normalize-cross are lexical candidate labels: actual
instructions establish path sampling and plane projection respectively.
Full path assembly is retained in physics-dependencies/path-query.asm;
reset assembly is ground-dependencies/direction-reset.asm. Root's real
`crt_floor` dependency is Evidence
`ev_33d785cb67223dda1009129a18faef198b5a8a3eb627b2e4ba2e63b3fe0aa057`.

## Ownership and source behavior

`player_path.hpp/.cpp` defines a native owned resource: type byte, logical
uint16 count, total length and physical point storage. Each source point has
two signed16 angle words, binary32 segment length and XYZ position. Logical
count is independent from physical storage; source endpoint/selection reads
can reach a sentinel beyond the count. No invented clamp or zero-point default
fills absent storage. Type0/1 neighbor branches are exact, including the first
index branch which skips next-index correction. Missing physical words are
tagged at actual read boundaries. Countzero's unassigned local denominator
is explicitly `uninitialized_segment_length` at `0x49c3ff`, after the source
distance load/subtract, not an invented successful interpolation.

Task `+0x10` is now an optional shared native path resource. Absent means
unobserved; present null means the actual null-pointer boundary. Physics
`+0x18` is optional path distance. These facts are appended to the existing
shared task/physics owners. Resource identity/lifetime never comes from casting
a guest32 address. Contact direction remains at its single PlayerContactWork
owner; task position remains at PlayerTaskState.

Path query reproduces open-path range comparisons, loop wrapping through real
CRT floor, cumulative binary32 segment spills, zero-length substitution,
neighbor-angle midpoints using source signed16 difference/arithmetic half,
unwrapped interpolated int32 angles via actual FILD/ftol, source position
duplicates/store order, table-derived direction vectors and source tangent
normalization. Integer/signaling-memory/extended precision operations reuse
existing native owners. Source false retains caller output fields. Optional
per-component output facts preserve omission and partial stores. Loop wrapping
changes query.distance, not the separate player's persisted distance.

The scalar wrapper captures distance/period argument values before its CRT
call, preserving their by-value source stack identity through any error
handler. A missing floor dependency retains its actual binary64 argument,
full CrtFloorResult and prior query distance. CPU feature word, MXCSR/context,
fallback control word and CRT error policy remain explicit owner inputs.

Direction reset reproduces the optional GetAnalog/dash-speed clamp, commits
player path distance before reading the task path resource, stores the query
scratch argument, calls the real path query and clears flag0x2000 only on the
source false/zero-projection branches. Unknown and observed-null paths have
different failures/store prefixes. Query success consumes actual contact normal
and task position, runs plane projection for displacement and world velocity,
tests source lengths, normalizes both vectors and invokes the actual signed
angle owner. It preserves query/floor/angle diagnostics and partial vectors on
failure. Source candidate yaw is masked16, compared with target yaw and may
use the observed `0x8000-candidate` correction; no modernized turn policy is
substituted. Matrix stack state is untouched by reset's pure trig query.

Plane projection owns the six-float `(point,normal)` input and its source
signed-distance/squared-normal spills, optional XYZ stores and returned metric.
Non-unit/zero normals are not normalized or given identity behavior. NaN/
subnormal semantics come from the x87 memory-operand owner.

The actual `0x418ac0` normalization implementation was moved once into Ninja as
`normalize_vector_extended(v,CoordinateAxisConstants,fp)`. The old rotation
alias and synthesized unrelated constant entries were removed. Rotation,
position, contact and collision callers now provide only actual zero/one.
Source load/square/FSQRT/FCOM/store order and PC behavior were unchanged.
`PlayerCoordinates::source_math(fp)` provides a transient numeric object from
the existing source table; no second persistent table/stack owner was created.

`bind_ground_direction_reset` borrows live runtime/coordinate/constant owners
with documented lifetime and rejects direct temporary runtime/constants.
It uses each ground call's PlayerParameters and FP context. Full
PlayerDirectionResetResult is appended to common PlayerAccelerationResult;
the existing child-failure propagation keeps path/floor/angle facts. A test
through the actual ground owner proves runtime changes after binding and a
new dash-speed parameter reach reset, which stores speed/distance before an
unavailable path failure. No successful missing-boundary no-op is used.

## Artifacts and verification

`player-path-constants.bin`: 92 bytes = binary32 zero/one/range factor at
`0x7dcb2c/0x7dcb10/0x7dd57c`, then root's five16-byte CRT floor vectors at
`0x840d90`. SHA-256
`fc2062445cbccb1e216c6d7745e74a41480c8e3aeece3b2e9d0b7bb6741b5e97`.
`player-direction-reset-constants.bin`: 120 bytes = those92 + negative-one
float `0x7dcb0c`, doubles `0x7dccd0/0x7dccc8`, plane scale double `0x7e0b40`;
SHA-256 `562ea6cc756390b202843b31458ff9d94c8e5edc606799a112cad87eee234505`.
Both decoders reject missing/wrong-length bytes and preserve source bits.

Test arguments:

```text
player-path-test TABLE PATH_CONSTANTS92
player-direction-reset-test TABLE UNIT RESET_CONSTANTS120 GROUND_CONSTANTS96
```

Optimized and ASan/UBSan owner tests passed source interpolation, real negative
loop wrap, retained false-path outputs, unknown floor arguments, sentinel
ownership, actual GetAnalog clamp/failure ordering, resource unknown/null
distinction, source distance/flag/yaw stores and live ground binding. The
linked prebuilt numeric archive was not sanitizer-instrumented.

`native_player_path_probe.cpp` has table/constant byte inputs and
`29+5*physical_count` raw words, explicit CW, 34 output words.
`native_player_direction_reset_probe.cpp` has table/unit/constants,
`36+5*physical_count` raw words, explicit CW, 50 output words. Shared test-only
path_probe_data.hpp decodes owned numeric resources/status facts once.

Reusable original runners execute the complete actual functions and real
callees without instruction overlays or substituted path callbacks:

- `verify_original_player_path.py`: 608 fixtures per12 CW profiles =7,296
  comparisons. Return, all80 caller work bytes and control word match. Flags
  differ495/CW PC24 and450/CW PC53/64. Both real CRT floor routes are covered.
- `verify_original_player_direction_reset.py`: 670 fixtures per12 profiles
  =8,040 comparisons. Player/task/target state, observed query/vector stores
  and control word match. Flags differ596/CW PC24 and545/CW PC53/64. Both real
  floor routes, actual input and explicit acos dispatch-bypass policy1 execute.

Path fixture SHA-256:
`8be8fb4e0840aacbe37dd2850c1a82a1051e31ed2427cf1134873035e4685197`;
private probe `4e5fedd10901cc40441ed4652158f5a20e662fc8c31333d1a87c749c875988e9`;
report `1f050edcdadafad082b199d3049ac3ea6688112849592df298b2c30a7b9360de`.
Reset fixture SHA-256:
`2ecaf4d2e00959349ee6fdb3cf0bb6951491b010978fb14d389c1dfdaa5c80c0`;
private probe `efb8a8e8171333a1ea0d87da8988e6419edd3081cbb49640661651bdbfcd31ca`;
report `d4ac8cb4fe14fc1a6a576f3532894b23ecb30c101d95efb4a887c25840938e00`.

These are finite fixture path/task/contact/runtime inputs and selected control
profiles, not captured game ticks or general nonfinite/failure-route hardware
conformance. Root's separate floor report retains signaling-NaN and fallback
FTZ emulator discrepancies rather than changing native producer semantics.
Actual mod/startup FP state, deferred exceptions/full status, path asset
registration and complete scene/task/game lifecycle remain unverified. Root
owns the final common build/probe gate. A fully playable native Mac game with
the required updated mods and speedrun fidelity remains unfinished.

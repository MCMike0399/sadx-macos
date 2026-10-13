# Native alternate acceleration: source evidence and verification scope

Source: user-supplied `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This is a reconstruction of retained x86 instructions into native ARM64 C++.
The native runtime uses integer-backed software x87 and the explicitly named
Bochs P5/P6 FPATAN model. Original code executes only in the separate comparison
oracle. Neither these comparisons nor the native tests establish real Intel
hardware, loaded-mod, physical-input or complete-game equivalence.

## Instruction evidence

The complete source ranges, byte hashes, jump-table bytes and available REA
Evidence IDs are retained in
[`ground-dependencies/alternate-source-inventory.json`](ground-dependencies/alternate-source-inventory.json).
Recovered assembly is retained beside that inventory. REA dossiers and exported
Evidence bundles in `ground-dependencies/` and `physics-dependencies/` preserve
the source identity and Hopper analysis profile. Bytes and instruction operands
take precedence over SDK labels or decompiler expressions.

| Native entry | Source range (exclusive end) | Bytes |
|---|---|---:|
| `get_player_acceleration_attribute_4000` | `0x447010..0x44750c` | 1276 |
| `get_player_acceleration_event_ground` | `0x447510..0x447b7e` | 1646 |
| `get_player_acceleration_event_air` | `0x447b90..0x447e54` | 708 |
| `get_player_acceleration_attribute_20000` | `0x44b0e0..0x44b9b2` | 2258 |

The event jump tables at `0x447b80` and `0x447e54` select modes 1–4.
Out-of-range modes clear task flag `0x4000`. Mode 2 subtracts the actual task
position before setting the temporary vector's Y to zero. Modes 2–4 can clear
speed Z, Y, X and store event mode 1 before a later callee fails. Mode 4 adds
the horizontal event vector before transforming gravity. Ground mode 2 reads
event target speed and acceleration step with source fallbacks; air mode 2
uses player parameters and does not read those ground-only fields.

The attribute `0x4000` routine retains its distinct gravity coordinates,
`rush_speed` cap and friction sequences. The attribute `0x20000` routine
retains floor attraction, signed raw-stick conversion, camera projection,
signed vector angle, yaw mutation and its source friction tail. These bodies
are separate owners; similarity alone does not substitute the ordinary ground
formula.

## State, arithmetic and callee ownership

All entrypoints mutate shared `PlayerTaskState`, `PlayerMotionState` and
`PlayerPhysicsState`. Event mode/vector/target speed/step, task position, floor
normal, raw axes and camera orientation remain individually optional. Missing
required facts produce tagged `PlayerAccelerationResult` failures at source
boundaries. No zero vector, origin, identity transform, camera or successful
callee is manufactured. Child failures retain their temporary values and full
rotation/projection diagnostics, with the outer call site and nested source
boundary recorded separately.

GetAnalog's actual output-store mask protects active event cases which leave
angle/magnitude scratch untouched. `from_float` represents FLD; source memory
FADD/FSUB/FMUL/FDIV/FCOM use distinct memory-operand APIs to preserve signaling
NaN classification until the original arithmetic operation. Binary32 spills,
retained ext80 intermediates, source constants, operand order and final store
order are explicit. Event vector add `0x785170` loads destination X but source
Y/Z; this asymmetry is retained. Horizontal heading follows FPATAN, two m64
multiplies and the original CRT integer conversion's low-16 result.

The common friction owner in `player_acceleration_friction.cpp` corresponds to
ordinary ground `0x44c97c..0x44cde5` and attribute `0x20000`
`0x44b549..0x44b9b2`. It retains the ordinary-ground jump which skips X friction,
the final Y addition before X store, and the binary32 Y spill after X store.
The extraction was compared against the original ordinary-ground routine for
29,952 executions before integration; the patch and source-site correspondence
are retained in `ground-friction-extraction.patch`.

Coordinate projection owns camera Y/X transforms, floor-axis rotation,
projected direction and task forward direction. A trace at the actual
`0x438410` entry proves the `0x44b518` call's argument order: **forward first,
projected second, floor normal third**. The earlier hypothesized reverse order
was corrected at the caller; the signed-angle owner was not changed to fit
results. The retained ST0 below raw-stick FPATAN/ftol is passed to the actual
source FST/FSTP stores. CRT policy 1 is an explicit function-local comparison
seed which bypasses runtime dispatch; initialized game policy is unobserved.

The comparison executes actual input, yaw, vector and matrix callees without
overlays. Attribute `0x20000` fixtures use task flags 0 so they do not select
direction reset `0x43c130`; that dependency remains explicit in production and
requires separate recovery/verification. Tests of an available native callee
do not establish every caller-selected path.

## Verification results

`tests/player_acceleration_alternate.cpp` passes under ARM64 Clang O2 and
UBSan. It checks unavailable facts, lazy event reads, retained partial stores,
actual heading/coordinate execution, untouched-input scratch provenance,
pending CRT results, and complete nested diagnostic propagation. Its seven
asset arguments are parameter rows, ground constants (96 bytes), event
constants (20), axis constants (8), projection-angle constants (20), quarter
sine table (4100), and unit matrix (64), in that order. Only the first 132-byte
parameter row is consumed from the multi-character parameter artifact.

The reusable probe and verifier are
`tests/native_alternate_acceleration_probe.cpp` and
`tools/verify_original_alternate_acceleration.py`. Each report retains source
function/artifact hashes, native source/contract hashes, compiler identity,
library hash, probe/verifier hashes, binary fixtures, exact stored-field words,
direct calls, control word and low exception flags. Twelve masked profiles
cover x87 precision 24/53/64 and all four rounding modes.

| Profile | Fixtures × CW | Stored values / direct calls / CW |
|---|---:|---|
| Attribute `0x4000` | 2496 × 12 = 29,952 | All match |
| Attribute `0x20000`, task flags 0, varied finite floor normals | 2496 × 12 = 29,952 | All match |
| Event ground | 1844 × 12 = 22,128 | One value-disagreement fixture per CW; calls/CW match |
| Event air | 1844 × 12 = 22,128 | One value-disagreement fixture per CW; calls/CW match |

Reports are respectively
`original-attribute-4000-acceleration-comparison.json`,
`original-attribute-20000-acceleration-comparison.json`,
`original-event-ground-acceleration-comparison.json` and
`original-event-air-acceleration-comparison.json`. The events intentionally
retain `values_calls_cw_match: false` and verifier exit status 1. The attribute
reports pass this limited values/calls/CW gate; all low-flag disagreements
remain recorded separately. Unicorn 2.1.4 has weaker x87/transcendental and flag
behavior than the software model and is not an Intel hardware oracle.

### Unresolved raw/nonfinite event fixture

Fixture 1843 selects mode 4 with positive SNaN `0x7f812345` in the horizontal
event vector and a negative QNaN gravity result `0xffc12345`. The values are
valid raw float encodings. Original-instruction tracing and a native replay
of the exact same `0x785170` entry operands narrow the first observed
disagreement to its Z add:

| Stage | Unicorn source instructions | Native source lift |
|---|---|---|
| Destination XYZ before add | `ffc12345 ffc12345 ffc12345` | Same replay inputs |
| Other XYZ before add | `7f812345 00000000 7f812345` | Same replay inputs |
| XYZ after add | `ffc12345 ffc12345 ffc12345` | `ffc12345 ffc12345 7fc12345` |
| XYZ after coordinate transform | `ffc12345 ffc12345 ffc12345` | `7fc12345 7fc12345 7fc12345` |

The Z instructions are FLD source.z, FADD destination.z, FSTP destination.z.
Native FLD first quiets the positive SNaN; the subsequent equal-significand
opposite-sign QNaN tie selects the positive payload in the current modeled
Intel rule. Unicorn's signaling classification across FLD is a candidate
explanation, not an established observation of hardware. The report keeps
both signs, the input fixture, source entry/output traces and native leaf
replay. No downstream normalization or host-double fallback was added.

## Remaining claim boundaries

These source bodies and explicit callee bindings advance native physics
reconstruction. They do not establish startup state, full source dispatch,
collision/position integration, the complete game, updated-mod semantics,
physical keyboard/controller behavior, playback timing or speedrun parity.
The raw/nonfinite event disagreement and exception-flag oracle limitations
remain unresolved. Original Intel Ultra/Windows 11 observations are still
required for claims of hardware equivalence.

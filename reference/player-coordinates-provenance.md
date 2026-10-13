# Native player coordinates and owned Ninja matrix state

2026-10-11. Original PC artifact is `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Source: `sadx-native/src/player_coordinates.hpp/.cpp`; native tests,
`tests/player_coordinates.cpp`; sole comparison probe,
`tests/native_player_coordinates_probe.cpp`; reusable runner,
`tools/verify_original_player_coordinates.py`.

World→player and task→world now execute reconstructed matrix semantics rather
than identity test doubles. The floor→world matrix sequence is reconstructed,
but its general normal→angles provider remains a required separate dependency;
no zero-angle/host-math fallback is supplied. This is not full gameplay.

## Correctly bound static evidence

Current REA `coordinates/` structured records independently inspected:

| Original function | Evidence ID |
| --- | --- |
| World→player `0x43ec00` | `ev_6f334535f572db5b1981c7cd9c93df41779c891de69c79ba6e1aa0329517c14b` |
| Task→world `0x43ec90` | `ev_17da574a577f342ee8f777bf1f529571f6f9d9f7fe08fc930701445ca783393c` |
| Floor→world `0x43edd0` | `ev_d9d6afae74959a3715a6bfa60fe05416a78e2de9ea0986f854a11ac371fedc2b` |
| Rotate X `0x784ee0` | `ev_f7f8ed576b41f1961b5fbccc353bf234ee1f04f745b20a868a3bf9d84170a489` |
| Rotate Y `0x784e20` | `ev_1cae3940afbabc2975ddba2faf5d8c475a74a8012254a2fda929ba02efae2913` |
| Rotate Z `0x784fb0` | `ev_ac40ef6ae93cc154d2a96c1936673d7522b4a8864577d5dd9c6603d336371c92` |
| Matrix vector `0x784d60` | `ev_df52f4441d5c9b013926d61e1debd51e9853ef6ff776f8f637e6a6440d65d797` |
| Pop `0x785140` | `ev_e606f6a6bd999702e68cc2e3c32bf41e597a733cfedc88c7d3f018661dd0c986` |

They bind the supplied executable and Hopper analysis-profile digest
`8c226db6a8bd36b662dc1cb83ab0ad0af698230fc322b58c6c4870878bf6ae70`.
Provider pseudocode/assembly are reconstructed representations, not original
source; complete body-range/reference/classification coverage is limited by
Hopper's public Python API. Exact LLVM disassembly corroborated the actual
callees and instruction order.

The initial named batch contained incorrect candidates: `matrix-push.json`
requests interior address `0x785120`, `matrix-init.json` actually requests
push `0x7850f0`, and `matrix-copy.json` requests `0x785160`, inside the final
address operand of Pop's instruction starting `0x78515c` (RET is `0x785161`).
Those filenames/dossiers do NOT establish the actual PushEx/Unit/Init semantics
used here. The actual functions are PushEx `0x781c80`, Unit `0x7875f0`, and Init
`0x780180`, independently inspected from exact original bytes. Their correctly
bound future dossiers can supplement this record, not retroactively rename the
initial candidate evidence.

Source byte digests and inclusive/exclusive ranges for every exercised function
are retained in `original-player-coordinates-comparison.json`. Original quarter
table SHA-256 is
`3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed`.
Original unit matrix at `0x389d650` is 64 file-backed bytes, SHA-256
`5998e9d1dd9bf48af6af87a1ef8c7783b818ac8329637ada08cc5fb329042376`.
It encodes the 16-float identity matrix, including source zero bits, and is
passed explicitly to the native owner rather than inferred from a name.

## Owning state and exact operations

`NinjaMatrixStack` owns a fixed native vector of sixteen-float slots. Source Init
sets count/depth to ONE at the base pointer, not zero. Current slot is therefore
depth−1. Capacity is the actual caller-supplied physical slot count. PushEx
copies all 64 current bytes to the next slot when depth<capacity, then advances
count/pointer. At capacity it is a source no-op. Pop-one does not alter state
when target depth is zero; otherwise it backs up one slot. Popped slot bytes are
retained, as in the source fixed storage. Invalid native ownership/depth is
tagged; guest pointer casting and arbitrary stack growth are absent.

Coordinates intentionally do not use an invented RAII restore. Successful
ordinary source paths push/unit/rotate/calculate/pop, preserving the prior
active frame. At full capacity the original push is a no-op, the current slot
is still overwritten, then Pop can reduce depth; that counterintuitive source
behavior is retained and tested. Missing normalization state or arithmetic
exceptions retain preceding stores and the still-owned stack state rather
than pretending cleanup occurred.

Rotate Y updates rows0/2; Rotate X rows1/2; Rotate Z rows0/1. Actual assembly
establishes these axes. Each routine stores SIN as binary32 and keeps COS in an
extended register, snapshots source row data before overwrite, updates the
first paired row before the second, and preserves observed product/add/subtract
order and binary32 stores. Angles wrap at the trig primitive's owning boundary;
the task helper still tests full int32 angle values before deciding to call a
rotation, so nonzero multiples of 65536 do not silently become skipped calls.

World→player uses Y(yaw), X(−pitch), Z(−roll). Task→world uses Z(roll), X(pitch),
Y(−yaw). A null task is an actual no-op in Task→world, leaving vector/matrix
state untouched. Floor→world first invokes normal→angles, then uses Z(derived
roll), X(derived inclination), Y(−yaw). Normal data is passed unchanged; the
floor normal is not normalized or reinterpreted.

Calculate-vector preloads all input components before any output store, allowing
in-place aliasing, then computes each component as `(z*m_row2+y*m_row1)+x*m_row0`
with software ext80 arithmetic and binary32 stores. It reads the explicit raw
normalization flag corresponding to `0x3d0f7ac`. Any nonzero flag enables the
real source normalization branch; unknown flag yields an explicit failure at
that boundary after known vector stores. Normalization sums z²+y²+x², stores
binary32 magnitude squared, calls source signed-square-root semantics, computes
extended reciprocal, and multiplies source stored components in source order.
There is no zero-vector fallback: masked zero division yields infinity and
subsequent zero×infinity yields NaNs with architectural exception flags.

## Supporting arithmetic extension

The parent authorized extending the existing arithmetic owner, avoiding a
second TLS/rounding representation. `X87Context` now also offers `divide`,
`from_double`, exact `from_int64` (FILD64), and `to_int64_current_rounding`
(FISTP64). Loads do not round to PC's arithmetic precision; FISTP uses current
RC and sets inexact/invalid, with pinned8086 integer-indefinite behavior when
masked. The unchanged pinned SoftFloat closure is now 49 C files. This extension
is required for source normalization and upcoming floor-angle/CRT semantics.
Original hardware exception timing and flag fidelity remain unverified.

## Verification scope and artifacts

Native optimized tests and AddressSanitizer/UndefinedBehaviorSanitizer source
runs pass for matrix ownership, directions, angle wrapping, null task, capacity
exhaustion, prior frame preservation, normalization/zero exception behavior and
explicit missing floor angles. The floor test's seeded angles validate only its
matrix sequence, not normal-angle recovery. Expanded arithmetic tests pass for
division, zero-denominator flags, exact int64 bound loads independent of PC,
binary64 loads, all four integer rounding directions and integer indefinite.

The reusable golden profile executes actual original World→player, Task→world,
all three rotations, vector calculation, PushEx, Unit, Pop and normalization
instructions with NO instruction/callee overlays. Only matrix storage/depth,
normalization flags and input angles/vectors are explicit test seeds. Source
table/unit bytes come directly from the supplied PE. Cases include arbitrary
int32 angles, zero/nonzero wrapped angles, nullable task, stack depth/capacity,
normalization flags0/1/high bit, random matrices/vectors and raw subnormal,
infinity and quiet/signaling-NaN vector inputs.

Authoritative results:

- `original-player-coordinates-comparison.json`: 1,977 cases for EACH of twelve
  experimental precision/rounding words, totaling 23,724 comparisons. ZERO
  vector-bit, count/depth or complete six-frame-byte mismatches.
- Native probe library SHA-256:
  `45acb28e89e99548fa751219acf6122548b28aff3331a71476b1ed6074b4b627`.
- `original-player-coordinates-comparison.cases.bin`: 838,248 bytes, 106
  little-endian words per case, SHA-256
  `0b44d7910af5750b73508cff0f73d70edc5025838757937ddb9813af33e14b3b`.
- Seed `0x43ec00`; source function, table, unit, probe and case digests are
  independently retained by the report.

Exception flags differ in 1,770 cases per control word; they are reported
separately rather than folded into a value-match claim. The cause of the
emulator/native flag difference and actual x86 hardware behavior remain
unverified. General Floor→world's normal-angle helper is not exercised by this
current golden profile.

```sh
_reference/sadx/python/bin/python sadx-native/tools/verify_original_player_coordinates.py \
  "Sonic Adventure DX/sonic.exe" \
  /private/tmp/sadx-port-sources/native-player-coordinates-probe.dylib \
  --output _reference/sadx/original-player-coordinates-comparison.json
```

## Remaining floor and gameplay work

Normal→angles helper `0x4b8e80` uses source fast-path comparisons plus CRT asin
`0x645020`, FPATAN, exact doubles65536/±0.15915497620314795 and CRT integer
conversion `0x644340`. Its cutoff source floats are ±0.9999600052833557 and
epsilon0.0020000000949949026. No host asin/atan or guessed normalization replaces
them. A rigorous native transcendental owner and source CRT conversion are
being prepared independently; only then can the general floor boundary close.

Actual running-process/control/mod state, complete acceleration/collision/task
gameplay, native GPU performance, Xbox/Switch2 Pro hardware and speedrun
fidelity remain unfinished under the original full goal.

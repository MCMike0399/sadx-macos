# Collision component core

This packet recovers the component initialization reached from Gamma's mode 0 constructor.
It is a component, not a collision solver or a physics rewrite. Root owns fresh STYLE review,
Make/judge/native-source registration, the actual Gamma binding and the full gate.

## Public API and ownership

`engine/collision_component.hpp` exposes `initialize`, `component`, `clear_info`,
`calculate_range`, `free_work` and `free_collision`. `math/ninja_length.hpp` exposes the exact
original scalar length return. All interfaces compile as C++17. `initialize` receives the
actual `TaskHandle`, its SAME `TaskWorkArena`, a reference to canonical primary status,
a vector of complete typed `Info` records, the original signed count, byte id and caller's
arithmetic context. It returns `InitResult::Initialized`, `NoPrimary` or `AllocationFailed`;
callers must compare enumerators rather than assume ordinal values. Null task is an explicit
`Error::Task` boundary, reflecting the original dereference, not a success or no-primary result.

Bind status through the existing `player_initialization::bind_primary_view(task).status`.
The unit uses the actual flags 7 TaskScheduler task and existing primary/motion binders.
There is no copied primary, player registry, collision registry, Gamma-only state or fake pointer.
The primary's real arena reference at offset 56 points to the actual 168-byte allocation.
That allocation owns ONE `TaskWork::collision_state` containing all numeric `Work` fields and
its copied, mutable `Info` vector. The component borrows its owning task through a weak pointer.
No Task/TaskGraph/Session ABI or runtime source was changed. TaskWork gains the typed payload
pointer; Root must rebuild affected consumers normally before binding.

The SAME arena retains ONLY collision cleanup metadata: the ownership byte at offset 5 and
info allocation reference at offset 12. The scheduler already reads exactly those facts.
Other component numerics and info numerics are typed values, not a second guest-memory mirror.
Within this component API the ownership bit is established once and remains set until release.
A future recovered callee that changes ownership must update that cleanup metadata at its
source store; changing public flags alone is not an ownership-transfer API. Borrow Work only
while its allocation remains live. Retained handles after release are not live collision state.

Root's existing importer/reader should decode the three complete original 48-byte Gamma
records into `Info`, preserving every scalar encoding and all integer fields. No PE/Guest
or importer dependency exists in the native closure. This API is character-independent and
accepts Sonic's later radius mutation directly. Initialization calculates the range once;
it never invents a later recalculation or registers EntryColliList.

## Recovered behavior and explicit limits

Full original entries are CCL_Init 41caf0–41cbbe, FreeColliWork 41c4e0–41c520,
Collision_Free 41caa0–41cae7, CCL_ClearInfo 418b60–418bd9 and CCL_CalcRange
41bac0–41bbb9, including its shape jump table through 41bbd7. `source-evidence.json`
contains fresh Root batch 27654 source ids, complete dossier origin/hash/profile, pinned
EXE identity, full range hashes and constant bytes. Bounded pseudocode and full assembly
are retained here. Root's provider was closed normally; this worker launched none.
The one permitted static salvage attempt returned incomplete, zero files and 53 unsupported
operations; no historical packet is used as proof.

Old work is freed before either new allocation. Failure of the first allocation returns zero.
Failure of the second calls Collision_Free against the still-null primary component, returns
zero and deliberately leaves the new work allocation orphaned. No RAII cleanup changes it.
A successful init copies the full allocation count, sets ownership and special-list flags for
ids 2, 3 and 7, narrows info_count to 16 bits, publishes the primary component, stores the task
backlink, clears collision info/status/hits, calculates range and returns one. Publication
occurs before backlink/clear; a synchronous metadata observer in the unit checks that prefix.

`Info` covers all 48 bytes. Shapes outside sphere/cylinder pair/capsule use the original
default extent expression. The scalar computes x², y², z² in that order, then (z²+y²)+x²,
spills binary32 and calls signed sqrt. Length is spilled again before the range expression.
Sphere uses length+a; capsule uses (length+b)+a; forms 1/2 exclude c; the other shapes use
sqrt((c²+b²)+a²)+length with the original intermediate spill. Maximum is spilled only when
strictly greater. Range is maximum*1.01; maximum below 0.1 overrides the result with 25.
This is not a minimum-25 clamp. The original ordered/unordered comparisons are preserved.

The existing CPU `ExactFloat::signed_square_root` was present at base 54e159a. This packet
adds no square-root emulator operation. `scalar-allocation.assembly.txt` proves the m64 zero
comparison, reload, negative multiply, FSQRT/sign restore and stdcall pop. Independent original
scalar/sqrt cases and native tests exercise it. Context spelling is a type alias only: helper
arithmetic is unchanged. Existing fidelity_float baseline allowance stays EXACTLY 37; alias
spelling is NOT claimed as debt removal. All new game files measure zero debt.
Exact arithmetic is retained because complete call-site census premises for plain float are
not established, sqrt compares m64 zero, and precision/rounding are explicit judge inputs.
Six control words cover 24/53/64-bit precision plus directed 24-bit rounding. Existing CPU
flag differences are neither masked nor called a pass.

Allocation multiplication follows original unsigned wrapping. A wrapped allocation that
contains a partial 48-byte record stops at `PartialInfoLayout` after the real allocations,
before copy/publication. Unrepresentable allocator wrap/corruption stops at `AllocationWrap`.
Insufficient complete input stops at `InfoCopy` after both allocations; it fabricates nothing.
Count beyond supplied live storage stops at `InfoCount`. Arena failures carry TaskWorkFailure.
These native memory/dependency boundaries do not claim to reproduce original memory corruption.

For reference diagnostic mapping: Task→41caf5; ComponentReference→41cafe/41c4ea;
CleanupFlags→41c4f1; InfoReference→41c4f7/41cb6b; InfoCopy/PartialInfoLayout→41cb5a;
AllocationWrap→40b2e1; InfoRelease→41c4ff; WorkRelease→41c50f;
PrimaryReference→41cb98/41c517; InfoCount→418bb0/41baf4. Runtime/ABI/address diagnostic
mapping belongs to Root's adapter, never a new address dispatcher in this component.

## Qualification

The isolated compiler recipe is executable in `tools/oracles/check_collision_component.py`:

```sh
/opt/homebrew/bin/python3 tools/oracles/check_collision_component.py \
  --exe '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe' \
  --reference-python /Users/burbujamc/Developer/sadx-macos/reference/python/bin/python
```

It builds maintained sources and its own SoftFloat archive under `build/collision-component-core`.
It launches no apps/Ghidra/shared build and uses at most one compiler process at a time.
The native unit links TaskScheduler, actual primary/motion binders and maintained CPU code,
with no PE/Guest dependency. It proves cleanup, reentry, canonical status/pose/motion borrowing,
publication prefix, genuine exhaustion, orphan allocation, zero count, wrapped partial input,
null/no-primary distinctions, source range quirks, negative signed sqrt and vector length.

Original judge development: 7,964 cases, zero value mismatches, 6,462 flag partials, exit 2.
Untouched holdout: 8,864 cases, zero value mismatches, 6,834 flag partials, exit 2.
Each contains 888 direct scalar/sqrt cases with zero values and 834 flag partials.
The other cases execute complete original clear/range/init/free and real original
CAlloc/FreeMemory/arena allocation. Native lifecycle uses the real scheduler/arena.
Ownership-bit unset and null-info free branches, reentry, exhaustion and zero count are
included. Original/native free-list heads and physical block counts match, including orphans.
The original task linkage is a declared fixture; it is not an actor or journey qualification.
Standard verdict aliases were added to reports from their existing counters without changing
cases/comparisons/results. `verdict.py` correctly reports both as PARTIAL. `proof.json`,
`dev.json` and `holdout.json` retain counts and hashes. Full ignored reports remain in build.

Seven source mutants are caught: sphere extent 444 mismatches, capsule extent 366, cylinder c
678, wrong fallback threshold 3,232, scalar sum order 45, missing primary clear 1,074 and
accidental orphan cleanup 14. Each mutant compiles from a private source copy; no production
or existing judge file was mutated. Existing legacy collision judges/probes/expectations
are byte-unchanged. No existing tolerance, fixture, harness or mutant was edited.

Profile: ten full original ranges, zero active or undetermined overlaps, zero UNHANDLED.
Readability/provenance/global-owner/layer checks pass. The original baseline file is unchanged.
Native linker closure has only system C++/libc symbols, no Guest/PE/Unicorn dependency.
Make/nativeSources/judge registration and fresh independent STYLE review are PENDING Root.
No full gate, app, GPU/actor, N3b, story, gameplay, physical-controller or 63 FPS claim. No push.

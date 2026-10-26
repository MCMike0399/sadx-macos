# Full polybuffer initialization and canonical ownership

Branch `packet/polybuff-initialization`, worktree
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-14735049`.
The incomplete candidate is preserved at `52ce302`; preparation is `219aefa`.
**Implementation steps 1–4 are complete. Fresh independent STYLE review is now pending Root.**
No runtime attachment, actual rendering, N3b, full gate, integration to Main or push is claimed.
No extra agents, apps, Ghidra or shared build writes were used.

Follow-up task ID:
`node:delegated-task:command%3Amcp%3A1867b6b7-4d7f-4ffe-be5b-55ebb7752376%3Athread-send%3Apolybuff-owner-migration-continue-20261026-1`.

## Canonical owners

`engine/polybuff_initialization.hpp` now declares the sole annotated owners:

| Source span | Native owner | Legacy session stores that same owner in |
| --- | --- | --- |
| `3d0fe00..3d0ff43` (324 bytes) | `Buffers.records`, nine complete `BufferState`s | `NinjaFrameCallState.polygon_buffers` |
| `3d128ac..3d128af` (4 bytes) | `Buffers.vertices`, native `BufferMemory *` | the same frame owner |
| `3d127a0..3d127a3` (4 bytes) | `RetainedBase.value`, native `void *` | `NinjaSystemInitState.polygon_base` |
| `3d0fae0..3d0fcdf` (512 bytes) | `Scratch.words`, 128 words | `NinjaDeviceState.polygon_scratch` |

All source BSS initial values are zero. The source receipt proves these are mapped
virtual-tail zero bytes, not file-backed assumptions. Explicit annotation sizes describe
the 32-bit source extents, not native `sizeof`. Pointer aliases retain native pointer types.
There is one persistent record 8, inside the full array; Root must borrow `buffers.records[8]`.
Do not create another normal/UV owner or another giant-scratch/clear-block owner.

The old field spellings are references to views, with no independent values. Definitions
of availability, guest pointer conversion and byte access live only in
`guest/polybuff_owners.hpp/.cpp`. `FrameFacts` borrows all nine words of every record;
its compatibility iteration still exposes the old four-word prefix. `SourceBytes`' scratch
specialization borrows the device's typed words; only standalone isolated fixtures create
an isolated `Scratch`. Its per-byte availability never duplicates byte values. Opaque
specialization declarations prevent an accidental primary-template value owner in game TUs.
Legacy states are noncopyable so borrowed views cannot silently follow another session.

`native_game_reference` loads all nine record words, scratch and retained base into those
same owners. Frame Clear and device setup mutate them through reference views. Polygon
consumers borrow a live identity accessor supporting both canonical views and existing
optional guest words, so callback mutations remain visible without a cached pointer value.
The changed reference game, HLE bridge, screen and view consumers compile privately. The unchanged HLE device-bridge unit also passes its corpus, callback, alias and wrapper cases, including 3006 polygon and 3021 viewport cases and its existing mutants.

## Shared initializer and native API

`Operation::buffers_78e720` now calls `initialize_system_polygon_buffers`, whose reference
adapter invokes the same `polybuff::initialize_buffers` engine body as the native binding.
The former legacy allocation/loop/clear algorithm has been removed. Reference description
and count reads report missing facts at their original instructions, in name, format,
stride, count order. The old `794540` receipt boundary and pending-call diagnostics remain
for the frozen system judge. Native linkage calls the actual recovered `initialize` helper.
Five corruptions of the new engine core are detected by that untouched legacy judge,
proving the shared path is exercised rather than merely retesting old code.

The shared header-template body retains base, allocates `vertex_count * 36` with source 32-bit wrapping,
stores even a null allocation result, initializes all nine records, then clears all 128
scratch words. The original caller supplies ignored first argument 32, vertex count
24576 and guest base `38fcf08`. Native callers retain their actual borrowed base pointer;
the body never interprets or dereferences it. Other legacy system/input algorithms remain
unchanged and excluded from the native product.

The body is a small template over Context, with ADL binding ordinary free functions.
Native code keeps Root's actual `polybuff::Context { Display& }` unchanged. The reference
adapter defines only `polybuff_reference::Context` and functions in that separate namespace.
It defines no native Context, DeviceBuffer or BufferMemory type, and no native device seam.
Guest resource identities remain opaque pointers, never dereferenced in that adapter.
There are no dynamic fixture fields or backend flags in game state.

Root uses these ordinary functions and its existing Context:

```cpp
// Root defines this against its actual configured library owner, after the copy.
polybuff::ConfiguredCounts polybuff::configured_counts(Context &context);
// Return nullptr for the original malloc failure; retain actual CPU storage/lifetime.
polybuff::BufferMemory *polybuff::allocate_vertex_scratch(Context &, std::uint32_t bytes);
void polybuff::observe_initialization_store(Context &, InitializationField, std::size_t);

polybuff::initialize_buffers(context, buffers, retained_base, scratch,
                            32, 24576, actual_borrowed_base);
```

`ConfiguredCounts` contains only `const std::array<uint32_t,9>&`, compatible with
`library::State::polygon_buffer_counts`. The count is now read through Context, not a
separate initializer argument. There is no dummy array, inferred draw capacity, second
count owner, registry or service hierarchy. The native binding reads the live array for
each record. The reference binding reads the already-existing optional library facts
lazily at their boundary. PE record 8 count 12000 and configured count 32000 remain distinct.

Native source linkage adds `polybuff_initialization.cpp` and
`polybuff_initialization_binding.cpp`, plus the reviewed `normal_uv_strip.cpp` dependency.
The binding supplies maintained descriptions and the real per-record helper; Root supplies
count borrowing, actual allocation/storage and observation. Native observation may be empty.
Do not link `polybuff_initialization_reference.cpp`, legacy state adapters or guest/HLE
archives into the product. Root retains ownership of real DeviceBuffer/BufferMemory,
Display/Session/runtime/app composition and their physical resource lifetimes.

The combined-link checker compiles against Root's actual `runtime/model_device.hpp`,
asserts its one-reference Context shape, instantiates the native initializer specialization,
and links the native binding, reference initializer and Root's actual device unit into one
executable. It runs the nine-call reference path and Root's real resource/device unit;
`nm` confirms only reference-domain initializer symbols are defined by the adapter. No
undefined-symbol dynamic lookup is used. The native full initializer is linked but not
executed there: test-only unbound counts/storage hooks throw rather than fabricate startup.
The independent full instruction oracle remains the full native-initializer behavior judge.
Standalone oracle backends are never linked into that combined executable or the product.

## Preserved judges and evidence

Current proof: `reference/production-integration-20261026/polybuff-owner-migration-proof.json`.
The companion `polybuff-owner-contract.json` records source extents, API and content hashes.
Raw `78e720` / `794540` dossier IDs, ranges and SHA-256 hashes remain in
`polybuff-initialization-source.json`. Source EXE SHA-256 is
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The older `polybuff-initialization-proof.json` is the preserved candidate checkpoint receipt.

* Full original `78e720` and nine original `794540` bodies: 280 development / 1048 holdout
  cases pass, with 427317 / 547793 ordered receipts. Input hashes and complete ordered/final
  receipt hashes are identical to `52ce302`. Five semantic mutants are killed; 141 native
  checks pass. Capacity arithmetic, all API/allocation stops, null allocations, ignored
  HRESULTs, distinct counts, full configured counts and untouched argument words are covered.
* Migrated legacy system init 600, device setup 2000, frame calls 3969 and screen polygon
  3000 cases pass with zero mismatches. Their original probes, units, oracle code and expected
  fixtures are byte-unchanged. Four legacy units pass. The new owner unit passes 71 checks:
  bidirectional live aliases, complete records, byte lanes, pointer rereads, live counts,
  callback mutation, real CPU backing lifetime, missing-fact prefixes and device clears.
* 23 regression mutations are caught, including the five extracted-core changes through
  the legacy system judge and all seven documented historical frame-call categories. The
  no-count-reread mutation aborts the old oracle when its native-derived callback fixture
  reaches an unmapped target, matching existing mutate.py's detected-abort rule. The frozen
  device probe does not observe SDK operands; the new owner unit catches that mutation.
  Original historical system/device mutation spec files are absent from tracked history;
  the new frozen 9/7 regressions are explicitly described as reconstructed coverage.
* The eight reviewed strip/helper algorithms are byte-identical to `1960fed`. Fresh private
  reruns retain 1014 / 2662 exact-value cases and **154 / 198 FP partials**, actual exit 2.
  Their 12 checks pass and all seven original mutation specifications are caught. No partial
  has been upgraded to pass. Those are component proofs, not runtime or GPU qualification.
* Combined profile: eight reports, 56 recognized source ranges, no opaque/unparsed or
  undetermined ranges, **0 UNHANDLED overlaps**. Provenance, one-owner, layering and
  readability checks pass. Readability allowances only decrease: legacy initializer loses
  16 code hex literals, seven address identifiers, one address call and three long lines.
  New initialization game files have zero debt. Source ownership extents remain frozen;
  only obsolete spelling permissions shrink/move to reference fact accessors.

Private builds compile every changed algorithm and consumer here. Unchanged dependencies
come from Root's read-only core/guest archives; their hashes are checked before and after.
This is not a full source-only product build. All binaries, mutated variants and full reports
stay beneath this worktree's ignored `build/`; small hashes and summaries are committed.

## Root integration and fresh review

Implementation ownership and shared-core binding are complete. Root still owns:

1. Fresh independent STYLE §12 review of this frozen commit and its evidence.
2. Build registration: link the reference owner/initializer adapters for legacy targets;
   `system-init` and `ninja-view` tests/probes that were core-only now need reference linkage
   when they construct migrated legacy states. Existing reference targets retain it.
   Frozen system/frame/device probes and units, screen polygon probe and screen unit need
   `-include guest/polybuff_owners.hpp` **only on the fixture TU compilation command**.
   Do not put this in inherited target-specific/global CXXFLAGS, which would affect game TUs.
   Every real consumer already includes the adapter explicitly where needed.
3. Register the two new unit rows and explicit recipes. Native initialization unit links its
   probe with `POLYBUFF_INITIALIZATION_NATIVE_BINDING`, the binding and both engine sources.
   Owner unit links the reference adapters, shared core and the current system/device modules.
   `check_polybuff_owner_migration.py` contains exact private build commands. No Root Make,
   unit manifest, native source list or judge manifest was edited here.
4. Bind the real canonical objects and actual library counts in the product, perform its
   source-only closure check, actual Question/actor journeys and full gate. No render claim
   follows from this packet alone. No push.

Private reproduction tools (explicit paths keep every output private):
`check_polybuff_initialization_packet.py`, `check_polybuff_owner_migration.py`, and
`check_polybuff_strip_dependency.py`, plus `check_polybuff_context_coexistence.py`, under `tools/oracles/`. Their command arguments are
recorded in the proof and available via `--help`. No game TUs receive compatibility includes.

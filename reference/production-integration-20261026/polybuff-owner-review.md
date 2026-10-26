# Polybuffer owner migration review

**Bounded approval: no blocking findings in `bdf110a9469b051fa1adedad27ca1ca704beb67e`.**
This approves the recovered initializer, canonical owner migration and legacy shared-core
binding for Root integration. It does not approve product startup or rendering.

Reviewed independently on `packet/review-polybuff-owners`, using frozen git objects and
read-only producer evidence. The implementation is on `packet/polybuff-initialization`
in `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-14735049`. `52ce302` is only the
incomplete historical checkpoint. Comparison base: `219aefa`; reviewed strip dependency:
`1960fed6b8a982138cce6c54b3bd7a53d3f25f6b`. Read AGENTS, GOAL, STYLE including section 12,
STATUS, final HANDOFF, relevant canonical memory, PACKET and source/proof/contract records.

## Source and ownership

- `src/engine/polybuff_initialization.hpp:11` owns all nine complete BufferStates
  (324 source bytes at `3d0fe00`), the giant scratch pointer (`3d128ac`), retained base
  (`3d127a0`) and 128 scratch words (`3d0fae0`). Source extents describe the 32-bit image,
  not native structure sizes. PE section metadata confirms all four spans are virtual-tail
  zero initialization. Record 8 is part of this array, with no second normal/UV owner.
- `src/guest/polybuff_owners.hpp` maps every source record word to the corresponding
  canonical field. Word/byte availability and guest pointer identity conversion remain
  reference-only. The legacy spellings borrow those views, without numeric copies.
  Scratch SourceBytes borrows the device Scratch; isolated fixtures alone allocate their
  private Scratch. Explicit incomplete specializations in the legacy headers prevent
  accidental primary-template scratch storage. All three legacy state owners have deleted
  copying and user-declared destructors, preventing implicit moves and dangling self-views.
- `src/guest/native_game_reference.cpp:67` loads nine words per record, not just the old
  four-word prefix. Retained base and scratch loads also reach the same owners. Frame Clear
  and device setup use canonical fact views; polygon consumers retain a live accessor.
  Callback rereads survive both canonical Word and existing HLE optional-word bindings.
  Compatibility four-word iteration does not truncate the canonical nine-word storage.

## Algorithm and binding

- `src/engine/polybuff_initialization.hpp:50` matches the full original `78e720` assembly:
  retain base; allocate a wrapping u32 `vertex_count * 36`; publish even null; read name,
  FVF and stride before the live configured count for each of nine records; invoke the
  helper; then clear all 128 words. Base is retained without dereference; ignored argument
  32 does not affect the algorithm. The original caller uses vertex count 24576.
- `src/engine/ninja_system_init.cpp:26` actually calls the adapter and removes the old
  allocation/loop/clear body. `src/guest/polybuff_initialization_reference.cpp:26` preserves
  lazy missing-fact instruction addresses and read order; lines 55–72 preserve helper
  receipt, pending-call and owner-failure diagnostics. Its call at line 81 instantiates the
  shared engine body. Five shared-core corruptions detected by the unchanged old system
  judge substantiate this connection.
- `src/engine/polybuff_initialization_binding.cpp:10` calls recovered original `794540`.
  The helper sources are byte-identical to the reviewed dependency. Its ordered stores,
  ignored HRESULTs, allocation-failure prefix and untouched argument words remain intact.
  ConfiguredCounts is a borrowed array, fetched through Context per record. It introduces
  no dummy count owner; PE record 8 count 12000 remains distinct from configured 32000.
- The new game algorithm uses plain integer arithmetic, semantic state and ordinary ADL
  functions. No address-shaped game algorithm, fixture fields, registry or service
  hierarchy was added. Reference code contains availability/conversion/receipt mechanics,
  not a relocated copy of the initializer. Added seams are documented as NOTSA.

## ODR and coexistence

`src/guest/polybuff_initialization_reference.cpp:6` defines only the separate
`polybuff_reference::Context`; it defines no native Context, DeviceBuffer, BufferMemory,
native device functions, exact function or store functions. The standalone oracle's
different test Context is confined to its own executable/library closure.

Inspected `tools/oracles/check_polybuff_context_coexistence.py:45`, its probes and raw
proof/logs. The executable links the native initializer specialization, native binding,
reference binding and Root's actual device unit without dynamic lookup. The reference
adapter's recorded symbols belong to the reference domain. Frozen `83f0ded` Context header
and unit hashes match the proof. That Context contains one Display reference and Root
owns the complete resource types. The copied unit preserves its assertions; its renamed
main receives an explicit return 0.

The executable runs nine reference calls and Root's device unit. **Native full
initialization is linked but unexecuted**; test-only unbound count/allocation hooks throw.
This is coexistence evidence, not product initialization proof.

## Evidence audit and STYLE section 12

The bounded hash audit is `REVIEW-evidence.json`. Both complete dossiers match their
recorded SHA-256 and evidence IDs:
`ev_da94eca7e26a899355bb6f1886c863ed55c8d3a262790876c5fde88647193940` and
`ev_b5a58dd382b6abd9cc8ae889d061434f82ecef8b1818dc67ba584b22d56cd784`.
The pinned EXE hash, complete function ranges, caller range and description table hash
also match. I read the original assembly rather than treating pseudocode as source.

All 27 proof-listed source hashes and 13 protected judge/probe/unit hashes match frozen
git objects. No pre-existing file under tests, tools/oracles or tools/gate changed relative
to `219aefa`. The new initializer verifier is byte-unchanged from `52ce302`. Input and
ordered/final receipt hashes match that checkpoint. No expectations, fixtures or tolerances
were weakened.

Audited producer outcomes, **not reviewer reruns**:

| Evidence | Recorded result |
| --- | --- |
| Full original initialization | 280 development / 1048 holdout exact; 427317 / 547793 ordered receipts; 141 checks; 5 mutants caught |
| Legacy unchanged judges | 600 system / 2000 device / 3969 frame / 3000 polygon exact; four legacy units |
| Ownership | 71 checks; 23 regression mutations caught, including 5 shared-core mutations through the old system judge |
| Strip dependency | 1014 / 2662 exact-value cases; **154 / 198 FP partials**, exit 2; 12 checks; 7 mutants caught |
| HLE bridge | 3006 polygon / 3021 viewport cases and existing mutations, with the producer's stated pre-template-generalization limit |
| Profile | Raw profile agrees: 8 reports, 56 addressed ranges, 0 UNHANDLED, no undetermined or opaque/unparsed ranges |

All 23 regression log hashes match and their available raw mismatch reports agree. The
no-count-reread mutation aborts at an unmapped fetch; it is explicitly an abort detection,
not an exact mismatch count. The SDK mutation passes the old probe but fails the new owner
unit. Historical system/device mutation specs were untracked: these are reconstructed
regressions, not verbatim historical specifications.

STYLE checks: source names/provenance and complete ownership trace to the evidence;
structured loops and source order are preserved; integer wrapping is explicit; maintained
table literals replace EXE reads in game code; no recovered callee is silently skipped.
The typed observation/device seams allow later composition without rewriting the loop.
The baseline diff only decreases debt (16 hex literals, 7 address identifiers, 1 address
call and 3 long lines removed). New initialization game files add no such debt. Provenance,
ownership, layering and readability passes are producer receipts; I did not rerun checkers.

## Limits and Root's remaining work

**Tests were not rerun.** No builds, apps, Ghidra, workers, broad tests, gate or source/Root
writes were performed. Read-only hashes and existing raw outcomes were audited. Producer
private linkage uses unchanged dependency archives, so it is not a source-only product proof.

Root must register sources/tests and link the adapters only in reference targets. Frozen
fixture TUs need `-include guest/polybuff_owners.hpp` on their own compilation command;
never inject it through global or inherited target-specific CXXFLAGS into game prerequisites.
Native product linkage must use the shared engine/native binding, excluding guest adapters
and unrelated unchanged legacy system/input algorithms.

Root still supplies actual Display storage, configured library State count borrowing,
native scratch allocation and lifetime, required registry/build additions, product binding,
native journeys, source-only closure and combined gate. Borrow `buffers.records[8]` rather
than creating another owner. Renderer stage `83f0ded` and bounded review `635df95` do not
remove its throwing `native_polybuff_owner_binding` getter. This review approves neither
that unimplemented binding nor actor pixels, GPU parity, N3b or gameplay. Any later round
requires Root's new full-context delegated review task.

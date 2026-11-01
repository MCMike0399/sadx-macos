# Final animation cohorts: three completed, consumer stopped

Frozen Source: 38e1c69ecae029e38bf1d51c404e6a6358a75025, Source fix d82d0ac,
seal a833f75. Before Source: 53da4de17a1c1f21745d8403cd5bf6068afeb9f5 /
foundation b0e53ed. Definitions, probes, compiler dependency closures and binaries
were frozen in commit 0026c8a before generating any final case.

The one-time authorization was consumed. All 9,216 inputs were generated and
saved first. The process exited **1**: 8,194 cases were attempted, 8,193 completed,
and consumer case 1 raised an original-emulator error before native execution.
No Source or test definition was edited, no expectation was adjusted, and no
cohort was rerun. This is **incomplete final qualification**, not Source acceptance.

| Cohort | Seed | Generated | Completed / attempted | Verdict | Exact FP partial IDs |
|---|---:|---:|---:|---|---|
| table-retry | 2026121701 | 2,048 | 2,048 / 2,048 | Stored/interface exact | Empty |
| motion-state | 2026121702 | 4,096 | 4,096 / 4,096 | Partial: 466 FP-only cases | Every entry in motion-state.json `fp_partial_ids` |
| blend-backing | 2026121703 | 2,048 | 2,048 / 2,048 | Partial: 744 FP-only cases | Every entry in blend-backing.json `fp_partial_ids` |
| consumer-attachment | 2026121704 | 1,024 | 1 / 2 | Original fixture-domain fault; incomplete | Case 0, startup observation |

The JSON lists are complete, sorted, untruncated case identities with input
hashes. Full original/native flags and words remain in the output streams.
No FP partial is a pass. No measured Source stored-value regression was found.
`summary.json` and `output-completeness-audit.json` distinguish completed work,
execution failures and unattempted inputs. The raw runner's generic
`MISMATCH_STOPPED` label is classified by `consumer-fault-analysis.json`;
it does not establish a candidate Source mismatch.

## Consumer failure and representation limits

The frozen generator independently randomized player presence for Sonic entry 2.
The inherited original fixture supports null players under action-check entry 1;
its stock run-Sonic entry 2 fixtures keep the player present. Case 1 combined
entry 2, no player, nonnull collision and forced mode. Original execution raised
`UC_ERR_READ_UNMAPPED` before any native probe or later observation for that case.
The fixture generator's unsupported cross-product is my test-side error; Source
was neither changed nor blamed for it.

Pinned bytes `66 85 45 06` at 0x49aa50 are `testw %ax, 0x6(%ebp)`.
The frozen input makes EBP null and reaches that instruction, implying a read at
address 0x6. This boundary is **inferred**, not a captured runtime EIP. The unchanged
original harness raises before returning observations; the wrapper saved its full
exception but did not save guest registers, memory, flags or pending state on
that error. Those fault-state outputs are unavailable. They were not reconstructed
or rerun. The exact input and traceback are output record 1 in
`consumer-attachment.outputs.jsonl.gz`. IDs 2 through 1023 are unattempted and all
remain saved in `consumer-attachment.inputs.jsonl`.

Consumer case 0 compared its Sonic action-check prefix, complete startup outputs
and snapshots, initialized-view identity, and all 91 imported Gamma rows exactly
between old/current interfaces. Startup flags differ source 0 versus native 32,
so this completed case is partial. Its attachment action is 69; both sides stop
as unimplemented at 0x49ba36, without calling animation. **No completed consumer
case reached the actual Sonic updater route.** Gamma shared identity is observed
for this one completed case only, not 1,024 executed ownership cases.

Two additional limits were declared before execution: Sonic action-0 initialized
animation-table publication is not reached past the existing startup boundary,
and Gamma table assignment uses its actual initialized view rather than linking
the complete runtime Context. No new callback, guessed availability, gameplay or
CharacterPack-to-playback bridge was supplied. PSetMotion remains UNRECOVERED.

## Source, native contract and fixture coverage

Table-retry executes the actual initializer twice per case with fresh export
identities. Every original read/write event and final table byte is retained.
Both old/current reference interfaces equal the same original-backed lazy
availability projection for old rows, masks, retries, nulls and missing exports.
Unavailable memory is not executed in the original. These are reference-contract
fixtures; the plain-native table initializer is not separately exercised here.
The table API exposes no arithmetic context, so no native hardware FP flag or
pending-state comparison is claimed for it.

Motion and blend each execute the original harness on every fresh numeric case.
Complete old/current reference and current plain-native numeric outputs match,
including their inherited software flags. Original-defined values, controls,
completion and boundaries match; original/native flag differences remain the
listed partials. The same cases also carry paired interface-only alias,
availability, nested failure, backing and ownership observations against the same
immutable before/current source snapshots. These observations are not mislabeled
as original memory execution. All twelve masked controls are covered.

Both invocation snapshots retain full result/work/nested boundaries, state/header
knownness, canonical references, allocation identities, arena block metadata and
losslessly encoded bytes/known masks over all 16 MiB plus sentinel. The three
allocation-prefix cases each reach the intended first-invocation result 39 times:
0 bytes available fails at 0x44a880; 64 fails at 0x44a898; 128 publishes null data
and returns. `motion-output-audit.json` records these reached results.

The original numeric harnesses expose their existing bounded result words, not a
complete hardware register/stack/tag dump. Original transition malloc is the
unchanged explicit 32-byte fixture supplier. Native arena identity observations
come from the paired interface probe; physical allocator parity is unclaimed.
The pending field is the derived unmasked exception-bit set of the current
immediate-exception API, not deferred hardware timing. Controls are masked.

## Artifacts, integrity and Root integration

All large streams, all 9,216 complete inputs, libraries, immutable source snapshots,
compiler logs and original exit file remain under this checkout's ignored
`build/animation-final-cohorts/`. `local-artifacts.json` pins 60 local artifacts.
`freeze.json` pins source Git trees, executable/imported inputs, libraries and all
successful compiler dependencies. `toolchain-and-setup.json` additionally pins
compiler/Python/Unicorn binaries and describes pre-freeze setup failures honestly.
`REPRODUCTION.md` records commands and the consumed-authorization restriction.

Only this packet and additive test helpers are committed. No existing judge,
expected value, tolerance, old raw stream, original baseline, thirteen mutant
intents, constant-substitution contract or manifest entry changed. The retained
252-constant regression receipt is inspected historical evidence, not a new run.
The 1,536 debt denominator and intrusive current code remain unchanged. No full
gate, native archive campaign, product closure or journey ran; STATUS, HANDOFF,
global inventory, Main and candidate working trees/builds/Git were not written.
No delegate, provider, network, storage, service operation or push occurred.

Root should:

1. Import the definition and receipt commits onto its integration packet; keep the
   frozen failed inputs and execution marker. Do not rerun or silently narrow them.
2. Resolve the consumer generator-domain and fault-state-capture deficiencies in a
   separate evidenced judge change, plus the declared attachment coverage gaps.
   Any new execution needs a separate explicit authorization and distinct identity;
   this final-cohort authorization is consumed. Do not count the unattempted 1,022.
3. Review `proposed-bindings.json` and merge only its eight additive protected helper
   hashes with the exact parent Source/Main union. No global manifest refresh was
   attempted in this checkout with missing historical corpora.
4. Retain fresh Source/integration acceptance, global inventory/constexpr lineage,
   source-only product/consumer closure, journeys and the coherent gate. This
   packet qualifies neither whole animation/player nor the campaign.

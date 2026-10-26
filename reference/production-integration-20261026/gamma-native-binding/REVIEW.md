# Independent native Gamma binding and query review

**APPROVE, bounded to the native composition through the explicit CCL_Init stop and
the two complete predicate leaves. No blocking findings on either review axis.**

Reviewed `54e159a..f6fb4ce` (`packet/gamma-native-binding`), concentrating on
`372e849`, `9b20b85` and `2e15b87`. Independently reviewed the new query source,
probe, original-instruction oracle, unit, dossiers and five mutation receipts at
`f806211` (`packet/selection-player-queries`). Also reviewed the narrow integration
addendum `f6fb4ce..d7fd300`, including `a6b73cc`'s harness copies and `d7fd300`'s
registration. The integration addendum has no `src/` changes.

Read AGENTS, GOAL, STYLE including section 12, STATUS's top and HANDOFF's last
entry. Reviewed both standards and specification in this one reviewer, as requested;
no agents were spawned. Approved dependencies were checked for identity and binding,
without reopening unrelated legacy algorithms. Only this review and its evidence
were authored in `packet/review-gamma-native-binding`.

**Tests NOT RERUN.** No compiler, unit, oracle, mutant, app, Ghidra or full gate was
launched. Recorded results below are the authors' results, checked against source
and available receipts. Root owns integration, builds and the full gate.

## Standards

No blocking standards finding in the new composition or leaves (STYLE sections
5–8 and 12). The native link-time bindings use semantic names, borrowed owners and
explicit failing lower seams. Source callee provenance stays in the frozen core;
the new query/native wrappers do not add duplicate source-function annotations.
No address dispatch, executable parsing, guest fallback, invented pose, static
matrix fallback or silent lower-call success was introduced in this slice.

`InitializedPlayerView` in `src/player/player_initialization.hpp:99` exposes
references, with legacy optional checks isolated in `player_startup.cpp:427`.
The `372e849` diff preserves the initialization assignments and their order;
the unsigned timer reference aliases the corresponding signed integer type.
The Gamma game algorithm receives ordinary initialized fields, rather than new
fixture-availability branches. This is a bounded seam over existing legacy state,
not approval of that legacy module's broader readability.

The current-owner migration, frozen Gamma core/probe/oracle/mutants and approved
asset implementation remain byte identical to their reviewed dependencies.
The startup unit, probe and original oracle remain byte identical to Main.
Inherited arithmetic and FP limitations are retained, not relabeled as passes.

## Specification and fidelity

No blocking specification finding within the stated stop boundary.

* The actual preview factory selects `exec_selection_gamma`; its implementation
  (`native_selection_preview.cpp:189`) constructs a borrowed Context and invokes
  the frozen core. The same scheduler owns the callback and PInitialize's real
  auxiliary task. Display, destruction and auxiliary callback bodies throw when
  reached; registering their identities does not execute or fake their bodies.
* `native_gamma_binding.cpp:33` follows the arena motion reference into
  `Registry.allocations`. It does not infer the player from the current slot or
  registry index. The frozen core captures/publishes the old player before
  initialization and reloads its local player afterward. There is no post-init
  current-player refresh or SuperSonic store. Null publication is exercised by
  the recorded composed unit; existing-player lower paths remain explicit stops.
* `native_gamma_binding.cpp:123` delegates to existing PInitialize with imported
  typed parameters, actual `bootstrap.event_work`, real task/motion ownership and
  the canonical eight-slot Registry. The constructor's table/request-zero stores
  precede `native_gamma_CCL_Init`; later callback stores, collision, textures,
  welds, materials, effects and animated-matrix work are not reported complete.
  Source teardown and the common tail have not been reordered or bypassed.
* `native_gamma_actor_assets.hpp` resolves loaded resources through the same
  CharacterPack, validates complete table extents and Stand metadata, and keeps
  genuine NULL declaration 61 distinct from unavailable payloads. Unavailable
  legacy action optionals remain unresolved; detailed importer boundaries are
  retained separately. This approves table binding, not later animation consumption.
  The pack hash, original 3,302 encoded records and all 22 descriptor bindings
  match the approved asset packet. Gamma free work and PInitialize's auxiliary
  allocation remain distinct; typed E102WK fields retain allocation identity.
* Query dossier `ev_b71958f1…` contains the complete 14-byte EV_CheckCansel body:
  one pointer read, no dereference, normalized EAX return. Dossier `ev_17a5334b…`
  contains both IsTimeZero return paths across 36 bytes: frames, seconds, minutes
  tested in that order. Source-image and range hashes match the oracle pins.
  Native predicates match these policies. The consumer uses bootstrap's actual
  cutscene task and timer bytes, independently distinguished by the binding unit
  from active/control/work and the unrelated game timer.
* The new oracle executes original instructions without callee interception and
  checks original reads, absence of writes, return PC and restored stack before
  comparing native predicate results. Its probe does not dereference the synthetic
  task identity. Recorded development/holdout results are 4,867/1,795 cases with
  zero mismatches; the unit covers 16,777,216 timer combinations. Five receipts
  reject inverted task nullness, each omitted timer byte and any-zero logic.

## Registration and evidence

The query probe, unit and oracle copied by `a6b73cc` are byte identical to
`f806211`. `d7fd300` appends explicit isolated compiler targets and the unit row.
Only Makefile and the unit manifest change among pre-existing protected inputs;
no protected inputs are removed. Read-only comparison of all 902 manifest entries
against the integration worktree found zero drift. No existing judge expectation,
tolerance, fixture or algorithm was changed by this addendum.

[evidence.json](reference/production-integration-20261026/review-gamma-native-binding/evidence.json)
records 67 matching hash/byte comparisons: frozen sources, dependency identities,
new harness copies, original ranges/dossiers, pack identity/prefix and local
recorded binding/startup/closure/frontier receipts. These are read-only evidence
checks, not rerun tests.

The binding unit log reports success. The actual app receipt records exit 1 at
frame 1382, `native_gamma_CCL_Init`; its log and screenshot hashes match proof.json.
I inspected the retained frontier PNG: outgoing title, no demonstrated actor pixels.
The force-load closure receipt reports success without guest/HLE/PE/emulator closure.
The unchanged startup oracle records 2,004 cases, zero value/boundary/resource/control
failures and 924 FP differences: **partial**, despite the legacy script's exit 0.
The frozen core retains its 1,422/1,080 cases and 498/690 FP differences, both exit 2:
**partial**, not fresh lower-function qualification.

This approval is not actor rendering/update/destruction qualification, GPU parity,
N3b completion, a gate result, gameplay, physical controller qualification, 63 FPS
or the connected Sonic story route. Those remain open. Root may integrate this
bounded composition and the two leaves, then continue at the actual CCL_Init seam.

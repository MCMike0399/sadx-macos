# Independent Gamma selection core review

**Verdict: APPROVE, bounded to mode 0 and the mandatory common tail; one nonblocking P3 naming nit.**

Reviewed frozen commit `3477fda7227555a2e657fcdce70bcb108e91d6f2` on
`packet/gamma-selection-core`. This review is authored only in the assigned
`packet/review-gamma-selection` worktree. No source checkout was changed.
Tests were **NOT RERUN**. No build, oracle, app, Ghidra, worker, gate, or product
binding was launched. Root's live full gate is outside this review.

## Finding

**P3 — CPU comparison names remain in game code.**
`src/player/gamma_selection_core.cpp:45` and `:174` directly name
`X87Comparison::less/equal/greater`. This is not an emulator operation sequence,
`X87Context`, or an address-shaped algorithm, and does not invalidate the bounded
fidelity evidence. However, the brief's literal “no X87 mentions” condition is
not met. If that stricter naming condition is required, expose semantic ordered
comparison predicates in the existing CPU fidelity API and use them here,
preserving unordered behavior and the retained Shadow value. Qualify that later
change with the same oracle and mutants; do not simplify the Shadow return to
float. STYLE §7 permits named fidelity helpers, and §12.4 requires the most
readable arithmetic that the evidence supports. No source behavior fix is needed.

## Fidelity review

- Entry captures motion, old player, and primary in source order. The prior hover
  read precedes current-primary/current-old-player publication; hover stores and
  EV_CheckCansel/IsTimeZero follow. Primary status is read after IsTimeZero.
  The floor-follow condition is ordered speed <= 0, including signed zero and
  excluding unordered inputs. The collision prefix retains signed-mode tests,
  damage nibbles, eyes height, and record 2's enable bit.
- Mode 0 captures player index before storing character 6. PInitialize remains
  the existing initializer. The local player reload uses captured motion; it is
  not republished as the current player. Primary, motion, registry and player
  ownership are borrowed, with no Gamma current-player or Super shadow stores.
  The real ID 2 unit calls TaskScheduler/PInitialize and distinguishes its
  auxiliary allocation from Gamma's free work. Actual product bindings remain
  Root's obligation; the probe's player lookup does not qualify those bindings.
- Complete animation-table binding/request 0, CCL_Init(3, 0), live parameter
  height, texture character 6, weld binding, destructor then display stores,
  CAlloc(1, 580), null allocation publication/timer 0, and cursor call precede
  the source fault. No ID 2 initialization bypass or successful null skip exists.
- NJS_POINT3COL stores are points, null texture, count 2, colors in the actual
  instruction order. The mutable endpoint array has one owner. Five live material
  lookups preserve row order 1/5/4/3/2, material indices 6/2/2/2/2, aliases, masks,
  alpha, and the final headlight pointer. ID 2 skips only the ID 1 HUD branch.
- The common tail retains PSetMotion, PJoinVertexes, laser targets, matrices
  54/30/39, Z rotation 16384, matrices 2/37, fan, E102Display, target/gun effects,
  optional held task, Shadow(1.5), ground attributes/timer, running effects,
  collision range/list, underwater work, and LoopTaskC, in source order.
  Post-display extra reads are live; the ground extra local is captured before
  Shadow and survives later pointer replacement. The source FST retains Shadow
  for subtraction; ExactFloat preserves the 1 + 2^-24 counterexample.
- Other mode bodies and all unrecovered lower algorithms remain plain named
  seams. The probe supplies declared receipt outputs, not hidden implementations
  of those algorithms. No direct event 580, guessed 7cdd60 producer, TimeZero
  interpretation of 4c5420, fake actor factory, or static pose was introduced.

## Source, assets, profile and judge audit

Read AGENTS/CLAUDE, GOAL, STYLE including §12, STATUS/HANDOFF, and canonical
`native-gamma-selection.md`; then the frozen game files, packet/context/proof,
source manifest, unit, probe, oracle, mutation driver and producer-proof script.
Used retained assembly, not a new provider session.

The read-only audit in `review-evidence.json` independently matches:

- EXE SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
- All dossier IDs/file hashes and complete body ranges: E102 483430–4841ce,
  InitGammaAnimData 7ca300–7cb55f, InitGammaWeldInfo 7cb560–7cb66a.
  Their half-open range hashes exactly match the committed source manifest.
- All seven frozen source/probe/tool hashes in proof.json; retained development
  and holdout probe-library hashes; all seven final mutant-library hashes and
  failure counts. The final bit-26 mask mutant is meaningful, unlike the retired
  equivalent mask variant.
- Original CHRMODELS_orig.dll hash `76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`,
  all 78 declarations with genuine null 61, Stand action/object/motion
  1020e344/10207290/1020e338, 16 frames, channels 3, interpolation 2, five material
  record hashes, and weld object pointers for rows 0/24/25/26/27. Retained original
  producer metadata has 91 recipes, Stand object count 77, constructed row 72,
  weld counts 2/2/0/0, modes 4/5/6/7, and null sentinel. Constructed row 72 remains
  a separate dependency, not an imported substitute. No producer was executed.
- Exact CCL/colors and lower setup range hashes. Source setup callees remain
  neutral contracts. The existing pack's selection export 2 is not Stand export 0.
- Retained three-range and final one-range profile reports have zero unhandled
  and undetermined overlaps. A separate read-only interval audit finds zero direct
  patch overlaps across all three ranges. Input 2.6.1/Speedrun 2.0 stay pinned;
  the profile records E102NGonFix inactive. The global 79 active unhandled patches
  are not claimed solved by this local zero-overlap result.
- Protected manifest grows from 871 to 875 inputs, adding the new report, probe,
  oracle and mutation driver. No prior protected hash changes except the
  append-only unit manifest. All 53 restored ignored blobs match their unchanged
  pre-existing manifest hashes. No expectations, tolerances or old judges changed.

## Retained qualification and its limits

The real native unit's retained output reports success, exit 0 in proof.json.
Development: **1,422 cases, 0 value/boundary/prefix mismatches, 498 FP partials,
exit 2**. Holdout: **1,080 cases, 0 value/boundary/prefix mismatches, 690 FP partials,
exit 2**. These are partials, not passes. Final mutants caught **7/7**, with
1,044 / 1,140 / 1,134 / 858 / 114 / 1,062 / 71 value failures respectively.
Successful per-case traces are compared by the harness in memory; retained JSON
contains aggregate results and failure samples, not a complete successful trace
archive. This review checked that comparison code and retained report integrity;
it did not independently reexecute the cases or prove the holdout ran only once.

The harness covers declared lower failures, missing material rows, null allocation,
publication/ID/index mutations, material aliases, extra replacement, selected
nonzero dispatch boundaries, and retained Shadow. It compares snapshots before
lower receipts and on final/boundary exit. It does not prove lower algorithms,
all possible missing binding facts, or every primary/motion argument identity:
several receipt argument checks intentionally retain only player identity. Root
must qualify those mappings through actual canonical owners during integration.

## Manual STYLE §12 disposition

1. No callee-address calls, goto, address labels, address identifiers or EXE reads
   in the new game files. Addresses occur in provenance annotations. Retained
   readability check has zero regressions; the new files have no baseline allowance.
2. E102/Display/Destruct and late_DrawLine3D names agree with New.h; descriptive
   lower/global names are justified in overrides.tsv with the E102 evidence ID.
3. Structured named steps preserve the assembly's stores/calls and pointer lifetimes.
4. ExactFloat is justified for retained Shadow and ordered FP comparisons; see P3.
5. Named constants/literals and imported resource contracts; no runtime EXE constants.
6. Lower behavior is declared as STUB seams. Receipt implementations are test-only.
7. Old judges unchanged; seven final meaningful mutants have retained caught receipts.
8. Existing named lifecycle/lower seams offer integration boundaries; this packet does
   not claim a new enhancement framework or enhancements qualification.
9. Added views/helpers are NOTSA-tagged. No new service hierarchy, registry, manager,
   duplicate player/pose, or fixture-availability optionals enter the game core.
10. No new platform replacement is implemented in this core, so platform tags are N/A.

Approval does not certify the full controller, actual lower calls/E102Display,
product/importer binding, GPU output, all preview actors, N3b, story, full gate,
unmasked hardware FP delivery, or 63 FPS. Root must preserve these explicit
boundaries when consuming the packet. Tests and apps were deliberately skipped
in this review as instructed; no gate success is recorded.

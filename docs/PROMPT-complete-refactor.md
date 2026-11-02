# Complete the readability refactor before further game development

**Scheduling superseded:** [ADR-0010](decisions/ADR-0010-readable-native-checkpoints.md)
restores native checkpoint delivery with required system refactors alongside it.
This document retains the exhaustive refactor scope and completion contract; its
former requirement to finish all that scope before gameplay is inactive. Use
`GOAL.md` for current work order. Open debt is not declared complete.

This is an implementation goal for sadx-macos. Make the entire existing maintained
codebase readable, modular and maintainable before resuming new gameplay, route
advancement or feature development. Continue through implementation, verification
and integration; an audit or a pilot alone does not complete this goal.

The product remains native Apple Silicon SADX (2004 PC) with Metal, assets imported
from the user's own supported disc/install, and the pinned Input/Speedrun profile:
63 FPS, CCEF, EHCF, Skipma and the other behaviors specified in `docs/SOURCES.md`.
Preserve speedrun quirks, exact behavior and existing capabilities. This refactor
does not change the full game's acceptance criteria or claim gameplay performance
that has not been measured.

## 1. Establish scope and preserve the baseline

Read `AGENTS.md`, `docs/STYLE.md`, `docs/ARCHITECTURE.md`, `docs/GOAL.md`, the top of
`docs/STATUS.md`, the latest `docs/HANDOFF.md`, and ADR-0005 through ADR-0008.
Read `docs/RESEARCH-complete-refactor-2026-10-31.md` for the initial investigation,
then verify its observations against the current checkout. Use `docs/JUDGE.md`
and `docs/TESTING.md` when establishing verification.

Record an ADR for this user-directed prerequisite campaign. It temporarily
supersedes GOAL's route-first scheduling and ADR-0008's requirement that alternating
steps advance journeys; it preserves their regression checks, proportionate
verification and prohibition on counting relocations as rewrites. This prompt
authorizes that scheduling change. Keep the original product goal and milestone
definitions intact, and make the active priority clear in the standing brief.

Inspect HEAD, worktree changes, preserved packet branches and actual test receipts.
Keep existing work and saves intact. Establish a fresh baseline with the required
gate and affected oracle results before changing behavior-bearing code. Capture
the real native frontier and existing reference/native journey results. If the GUI
or another prerequisite blocks a test, record that limitation; an interrupted or
failed gate is not a green baseline.
For missing or inadequate system coverage, establish a source-backed comparison
before rewriting that system. Freeze the identities and outputs of inherited
failing cases so new mismatches cannot hide inside an unchanged fail verdict.

Completion of this phase means a versioned inventory accounts for every maintained
source module, including code excluded from `native-sources.txt`, production app
and platform code, tooling, and game behavior currently living in `guest`/`hle`.
Classify each entry as already conforming, needing migration, justified reference
infrastructure, or demonstrably obsolete. Identify canonical owners, consumers,
destination module, judge and acceptance check. Track unrecovered behaviors in a
separate STUB inventory. Third-party code and historical evidence are not rewrite
targets; inspect their boundaries and preserve provenance.

## 2. Make the inventory and enforcement honest

Use the existing readability, provenance, ownership, layer and native-closure
checks. Verify implemented tooling before following stale TODOs in GOAL.
Strengthen demonstrated gaps in separate evidenced judge commits.

Audit addresses by meaning, not spelling: dispatch targets in any argument,
address aliases disguised as constants or enums, address suffixes, raw global
storage, register/stack transports, instruction-site tracing and executable reads.
Include fixture-availability optionals, duplicate ownership, large session/service
interfaces, opaque state-machine numbers, and game algorithms hidden in reference
composition. The existing first-argument dispatch regex misses Sonic's calls.

Distinguish original executable addresses from legitimate colors, bit masks,
format tags, numeric coefficients and fidelity-library encodings. Address
provenance in annotations remains useful. Numerical compatibility internals in
`src/cpu` require evidence-based review, not a blanket ban on hexadecimal or x87.
Inspect generated constexpr tables too: the Sonic animation operation table stores
instruction-shaped recipes that the current hex exemption hides. Native game
initialization must express the underlying behavior and typed data directly.
Every exception must name its purpose, location and verification; generic directory
exemptions must not conceal game logic.

Completion means the inventory covers actual semantics beyond lexical counts,
checker blind spots have discriminating regression cases, and the remaining work
is ordered by shared ownership and dependencies.

## 3. Refactor complete systems under their existing judges

Start with the dependency foundations needed by the migration, then work through
engine/task/render/texture systems, input and player physics/collision/animation,
game flow/save/audio/profile/UI, and native composition. Adjust the order from the
inventory; the scope remains exhaustive. Review already-readable modules without
rewriting them unnecessarily. Fog, frame control, game mode and scene position
provide existing migration patterns; `native_player_system` is a system-owner seed.

For each system:

1. Build the source context from symbols/types, callers, callees, ownership,
   existing REA evidence and profile patches. Use the owned REA/Ghidra workflow
   only for missing evidence. Preserve unknown meanings explicitly.
2. Express game behavior through named functions, typed parameters, meaningful
   state and enums, structured control flow, native callbacks and resource IDs.
   Replace generic address dispatch with real calls. Existing unrecovered callees
   become individually named STUB seams with explicit native failures and reference
   bindings. Retain all already implemented behavior.
3. Migrate each state owner and all consumers together. Preserve aliases, callback
   mutation, allocation identity/lifetime, lazy reads, partial stores and observable
   ordering. Game state uses optional values only for genuine game absence.
4. Keep one shared implementation of recovered game behavior. Reference adapters
   marshal original memory/registers, preserve diagnostic boundaries and call that
   implementation. Extract already-recovered game decisions out of legacy reference
   composition. Moving an unchanged algorithm into `guest` is not completion.
5. Use source literals for compiler constants, imported typed records for content,
   and named function tables for code structure. PE/container decoding belongs in
   the importer or reference tools. The native app and its ordinary source build
   must remain independent of original EXE/DLLs and guest execution.
6. Apply STYLE's arithmetic hierarchy per expression: plain arithmetic where the
   source/control-word evidence and unchanged oracles allow it, otherwise named
   fidelity helpers or readable exact-value expressions. Preserve precision,
   rounding, exceptional values and store order. Document unavoidable low-level
   sequences at a narrow numerical boundary.
7. Organize by system using ARCHITECTURE's destination map. Split oversized owners
   such as `native_session` into cohesive system state with explicit borrowing;
   keep Session as composition. Prefer ordinary calls and narrow seams over a new
   generic service framework. Update build discovery, source lists, dependencies,
   tests and closure checks together when introducing nested directories.
8. Verify the native interface and reference adapter against the same behavior.
   Use the unchanged oracle cases/verdicts, existing mutants and holdouts, plus
   focused composition checks for changed ownership and lifetimes. Record residual
   failures individually: preserving only an aggregate failure count is insufficient.
   Obtain independent STYLE §12 review for each coherent system migration.

Keep maintained tests, importers and reference adapters understandable too. Their
legitimate address knowledge should be localized and named. Preserve the original
judge coverage when source paths, probe bindings or mutation locations change;
any necessary judge migration is a separate evidenced change under `docs/JUDGE.md`.
Never weaken expectations, tolerances, inputs or mutants to accept the refactor.

Use durable committed packets for implementation and review, with one owner per
shared state and coordinated builds. Run focused checks during iteration and a
full gate at coherent integration checkpoints. Capture the actual make exit status,
including through log pipelines. Report partial oracles as partial, known failures
as failures, and skipped checks as skipped. Preserve Metal output, audio/input/save
behavior and measured baseline performance; compare timing under equivalent load.

## 4. Completion criteria

All of the following are required:

- Every inventoried module is reviewed and dispositioned, with zero unresolved
  readability migrations. Already-conforming modules have recorded justification.
  No recovered behavior was discarded, replaced by a stub, excluded from the build
  to hide debt, or relocated unchanged to evade the checks.
- Game code has zero executable-address dispatch, address-based game storage or
  resource identity, guest-register interfaces, fixture-availability state or PE
  reads. Original addresses are confined to provenance and justified development
  boundaries. Remaining numeric compatibility details have narrow documented uses.
- Existing game behavior in reference files is shared through readable native
  implementations. Genuine emulator, HLE, import and oracle infrastructure remains
  functional and separate. Native closure checks cover every migrated system,
  including systems not yet reached by the current app.
- Canonical ownership and layer/provenance/profile checks pass. Modules have clear
  public interfaces, meaningful names and an updated system map. A fresh reviewer
  can locate Sonic's jump, rendering submission, save progression and speedrun
  behavior and identify their tests without resolving addresses. Where behavior
  is not yet recovered, its named seam and limitation are equally discoverable.
- The final integrated revision has a real green gate, preserved existing journey
  content/state/audio checks, successful clean native source build and runtime
  independence checks for the currently supported route. Every changed system has
  current oracle/mutant evidence, with no unexplained regression against baseline.
  Inherited failures and FP partials remain explicitly bounded; they are not passes.
- The final report reconciles the full inventory, before/after debt, justified
  exceptions, test results, performance evidence, remaining behavior STUBs and the
  actual native frontier. Regenerate STATUS from real receipts and keep HANDOFF
  entries concise. Commit with Conventional Commits; push only when asked.

“Complete” means the complete refactor of the code that exists, including recovered
code outside the product's current route. It does not require inventing unrecovered
gameplay, completing M5/M6, or proving 63 FPS in stages the native app cannot run yet.
Maintainability must leave future hooks possible without building speculative
enhancements now.

## 5. Consult the research thread

The user authorizes the implementing agent to contact the original research thread
for architectural advice, evidence interpretation and review coordination:
[read and research deeply the docs. The goal is the...](t3-thread://v1/05626ae4-5585-4cd9-8b36-84b1d3c65806).

Discover the available T3 tools. Read that thread with `t3_thread_read` before
asking about its decisions. To ask for advice, use `t3_thread_send` with:

```json
{
  "threadId": "05626ae4-5585-4cd9-8b36-84b1d3c65806",
  "mode": "auto",
  "message": "Refactor consultation. Reply to <implementing thread ID/link>. Worktree: <path>; branch and commit: <identity>. System: <name>. Question or proposed decision: <specific issue>. Evidence: <paths and results>. Alternatives and tradeoff: <brief>. Requested help: <advice or review coordination>."
}
```

Include exact commits, evidence paths, test verdicts and limits so the advisor can
inspect the same candidate. Ask at substantial ownership/arithmetic decisions,
unresolved evidence conflicts and system review boundaries; keep independent work
moving while a response is pending. Delivery is not approval or a completed review.
If these tools are unavailable, report that and retain the question in the handoff.

Formal independent reviews still use a fresh `delegate_task` with the candidate
diff, evidence, unchanged judges and STYLE §12 criteria; do not reuse a completed
review child with `t3_thread_send`. This research thread can advise and coordinate
that review, and can reply through `t3_thread_send` to the supplied implementing
thread. Keep implementation ownership with the implementing agent.

Continue across module boundaries, gates and commits until these criteria are met
or a concrete external blocker prevents progress. Apply the project's three-attempt
limit to a stuck function, record its evidence, and work on independent migrations;
unresolved migrations remain open. When the refactor is complete, report the exact
checkpoint from which the original game-development goal can resume.

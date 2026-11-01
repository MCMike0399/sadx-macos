# Independent first blend cluster review

Decision: **APPROVE this bounded cluster's fidelity and design.** No new blocking
correctness or design finding. This is not approval of the whole render-state
migration, ADR-0009 campaign completion, or integration qualification.

Reviewed candidate `7fe9f364e852470c50dd350d091154904a7685f3` on
`packet/render-state-refactor`, against Main
`e6c5a490ddcde25a98f15bd3d289f9713ebfd48c`, using
`git diff e6c5a49...7fe9f36`. The maintained worktree was clean at inspection.
Review date: 2026-11-01. Implementation and review are separate packets.

## Findings and remaining debt, in priority order

There are zero new actionable defect findings in this cluster. The following are
open work and qualification limits, not evidence of a regression introduced here.

1. **Whole-system completion remains open.** `src/engine/ninja_render.hpp:20` and
   `:52` retain fixture availability, address-shaped fields and source diagnostics;
   `ninja_render.cpp:74` retains the legacy address-suffixed compatibility entry and
   instruction-site mapping. The packet expressly leaves these as debt. STYLE
   sections 1, 2, 6 and 12.1 are not satisfied by the complete legacy module. Finish
   canonical owner/interface and consumer migration before dispositioning that
   module as conforming. The new header contains the shared blend decisions; no
   behavior was moved into guest/HLE or excluded from the build to reduce debt.
2. **Depth and texture-stage migration/coverage remain open.**
   `ninja_render.cpp:42`, `:129`, `:138` and native display's depth consumers are
   outside this blend algorithm migration. The common render-state publication
   helper affects their path, but their algorithms were not rewritten. Preserve
   device-before-depth-table reads, masked comparison index and full-width depth
   write (including 42). Establish independent source-backed depth/stage coverage
   before rewriting them; blend parity alone cannot qualify them.
3. **Integration qualification remains open.** The current paired blend receipts
   fingerprint this candidate, including its new header. The nine consumer and
   native archive-overlay receipts identify source `618b0dd`, before the equivalent
   constant naming change. They borrow unchanged Root archives/platform objects.
   A clean updated source build, current integrated journeys and a coherent green
   gate remain integration work. Main's green baseline does not certify this packet.
4. **Judge coverage has explicit limits.** The unchanged blend oracle compares the
   packed byte, command device/state/value trace and stop boundary. It does not
   compare every exported result word, source-call field or HRESULT receipt; its
   backend always completes, and its differential runs use 88-byte tables. Existing
   units inspect missing/incomplete renderers, pending commands, ignored negative
   HRESULTs and unavailable 56-byte contiguous backing. This review checked those
   source paths manually; the 8,209-case count is not comprehensive receipt or
   lifetime certification. No separate inherited blend holdout or historical blend
   mutation specification was located in the submitted proof. Future judge work
   must stay separate. The new header is independently hashed by `proof.json`
   because the inherited oracle source lists omit it; retain that coverage until
   any separate judge change handles transitive implementation sources.

## Fidelity assessment

- `ninja_blend_internal.hpp:11` borrows the caller's byte. Both adapters publish
  the nibble before lower work; selectors other than 0/1 do not access it. Only
  the stored nibble is masked. The original full-width lower mode and selector
  survive. No second cache or owner was introduced.
- `ninja_blend_internal.hpp:35` uses unsigned `mode <= 1`, equivalent to the old
  unsigned `< 2` and to the source's equality checks for 0 and 1. These modes
  ignore selector/tables and emit source then destination. Zero emits 6 then 5;
  one emits 5 then 6. Failure of the first issue returns before reacquisition.
- `:41` reacquires the live device after the first callback. Native acquisition
  returns the current writer; legacy acquisition returns a shared handle by value.
  Each issued command copies that handle before callback invocation, so replacing
  or clearing the current device cannot invalidate its published command. The
  destination acquisition observes replacement, absence or null explicitly.
- `:46` sequences mapped value, device acquisition and target lookup in distinct
  statements. Lookups are invoked lazily, not evaluated while constructing the
  callbacks. A native acquisition callback can alter target data before target
  lookup while the mapped value stays captured, matching the baseline. The
  legacy device acquisition performs no external callback between these reads.
- The two adapters preserve actual contiguous backing for modes 12 through 15.
  Absent backing and out-of-domain inputs stop at the existing boundaries; there
  is no nibble-based lower-index normalization or invented fallback value.
- `ninja_render.cpp:8` publishes the command before checking backend availability.
  Missing/incomplete output retains the pending command and exact call boundary;
  completed receipts append even a negative or absent original HRESULT. The
  boolean issue result represents completion, not HRESULT success. The shared
  helper checks it only where another special-mode write would follow.
- The local `ReadStopped` catch handles only adapter read failures. Prior packed
  stores and completed commands remain visible. Renderer exceptions are not
  swallowed. Old load/dereference/call sites match the cached assembly, including
  each second-write boundary. Removing the legacy mapped path's redundant device
  recheck adds no intervening callback or observable read hook.

## STYLE section 12 assessment

1. The new shared implementation has semantic identifiers, structured control
   flow and no executable addresses or fixture optionals. The compatibility
   facade remains explicit debt as listed above. No relocation hides it. Submitted
   readability/provenance/layer receipts report no regressions; I did not rerun them.
2. Symbols and constants agree with the source lookup receipt and cached headers:
   `njColorBlendingMode`, `njColorBlendingMode_`, Ninja's paired-alpha modes in
   `ninja.h:356-357`, and D3DBLEND factors 5/6 in `d3d8types.h:213-214`.
3. Side effects, lazy reads, ignored selector quirks and partial stores remain
   ordered as described above. The ordering comments explain the callback hazard.
4. This change uses plain fixed-width integer arithmetic; no FP expression,
   precision or control-word change.
5. Named literal masks, source modes, device factors and render states replace
   unexplained integers; no new EXE read or content source.
6. No recovered callee was replaced with a stub, silent no-op or successful
   missing backend. Existing availability boundaries remain explicit.
7. Six supplied judge/unit/probe fingerprints match both the candidate files and
   Main. Recorded valid mutants remain discriminating; details below.
8. Existing native writer/device and service seams remain available for composition
   or future hooks. This cluster adds no enhancement registry or public service API.
9. Added shared helpers/roles are tagged NOTSA. The four compile-time callbacks
   separate real native access from legacy diagnostic access and preserve lazy
   ordering. Although there are only two adapters, the active PROMPT section 3.4
   specifically requires one shared recovered implementation. This private,
   bounded extraction serves that requirement without speculative framework work.
10. No new macOS platform replacement is introduced. Existing writer/backend
    boundaries are retained; the helper expresses recovered command selection.

## Evidence inspected and qualification

Read AGENTS, active GOAL/ADR-0009, PROMPT-complete-refactor, STYLE, ARCHITECTURE,
STATUS's top, latest HANDOFF, and `render-state-packet.md`. Inspected all three
changed source files, unchanged public headers, both blend probes/oracle scripts,
both existing unit sources and the baseline/shared evidence directories.

Original evidence was read from Root's local `reference/inventory-raw/` because
the packet worktree does not contain that ignored cache: bridge-blend-4030d0,
bridge-blend-77ec60, runtime-tables-blend and runtime-tables-depth pseudocode;
the two bridge assembly listings were rendered with the existing read-only
`ghidra_asm.py`. Also inspected `render-symbols.json`, cached mod-loader headers
and the baseline profile receipt (0 UNHANDLED overlaps). No provider was started.

Independently recalculated all three source fingerprints, the six inherited
judge/unit/probe fingerprints, both restored-report fingerprints and both restored
private dylib hashes. They agree with the submitted proof. All ten available
mutant-report fingerprints and mismatch counts also agree. These are evidence
integrity checks, not independent compilation or reruns.

Recorded unchanged blend judges each cover 8,209 cases with zero mismatches.
Valid mutation results, reference/native mismatches respectively: retained old
device 4/4; swapped special source factor 1,030/1,030; clobbered destination nibble
4,026/4,026; packed store after device 0/10; wrong contiguous modes 0/1,536.
Every valid mutation compiled and was caught by an affected judge; the two
native-only mutations appropriately leave the reference implementation exact.
The initial unused-constant compile failure is explicitly invalid and uncounted.
Restored judges are exact. No inherited oracle, fixture or expectation was edited.

The earlier consumer evidence records nine scripts exiting 0: four passes and
five FP partials (sprite queue, queue consumer, game view, lifecycle and native
queue). Baseline/shared reports for the first three partials differ only in
source/library fingerprints, not their recorded outcomes. The exact-case receipt
records unchanged 80-word outputs for all 600 cases per lifecycle backend, with
the same 402 FP flag mismatch identities each. I inspected that receipt, not a
fresh execution of the frozen outputs.

The overlay force-load receipt passes. Its app exits diagnostically with status
1 at `native_gamma_load_setup_resource`, frame 1382, preserving the baseline
outgoing-title screenshot hash. This is the existing frontier, not actor/GPU,
N3b, story, gameplay 63 FPS or new physical-controller qualification.

No builds, executable tests, new oracle runs, mutations, full gate, SSH, storage
helper calls or other agents/providers were used for this review. No maintained
source, HANDOFF, STATUS or Main edits. Only this review document is committed in
the review packet. Judge-enforcement candidate review remains separate.

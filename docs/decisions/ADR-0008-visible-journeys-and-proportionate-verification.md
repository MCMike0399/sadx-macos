# ADR-0008: Visible journeys, proportionate verification, and the Direct3D FPU precision

Date: 2026-10-16. Status: accepted direction (user review of the 2026-10-15/16 rounds).
Refines ADR-0005 (independence), ADR-0007 (readable code) and the per-step loop in
`docs/GOAL.md` §7. It changes neither the fidelity policy, the licence, nor what is committed.
The evidence is in [`docs/REVIEW-2026-10-16.md`](../REVIEW-2026-10-16.md).

## Context

After ADR-0007, agents spent 2026-10-16 on eleven "initialize X from native source" steps
(gates 151–162). Each moved 7–1,869 constants out of `sonic.exe` reads. Each step cost 17–36
minutes, mostly verification ceremony, and two commits. Across the series, game-tier source
was 1.7 % of the added lines and judge/evidence files about 92 %. The readability debt did not
move for eleven gates in a row (hex 11,628 / `_at_` 1,796). Earlier debt reductions came
mostly from relocating legacy files into `src/guest`, which the ratchet does not count.

Nothing in those rounds changed what the user can see. The gate's `app-smoke` runs 120 frames
and stops on the SEGA logo. Launched by hand, the app shows the SEGA logo, then a black screen
(the intro movie is not drawn) at about 6 frames per second, so the title takes about 20
minutes unless Enter is pressed. With Enter, the reference app reaches the title in 24 seconds
and file select and character select after that. Past character select, the 3D world draws
as a solid colour: the Metal guest renderer draws only pre-transformed 2D vertices, and the
game's untransformed vertices (FVF `0x142`, XYZ | DIFFUSE | TEX1) are skipped. The game keeps
running underneath.

The game tier is still hard to read in its physics and Ninja engine modules: 41 of the 47
`src/player` sources are written as x87 emulator calls. STYLE §7 allows plain `float` only
when the control-word census shows `0x007f`, and the census measured `0x023f` (53-bit). The
census ran in the hybrid guest, whose Direct3D 8 `CreateDevice` (`src/hle/hle_d3d8.cpp`) does
not model the FPU side effect of the real one. Without `D3DCREATE_FPU_PRESERVE`, which SADX
does not pass (794060/794000, HANDOFF "Floating-point observations"), Direct3D sets the calling
thread to single precision and round-to-nearest. The Windows capture saw the earliest (main)
thread at `0x007f`. The judge has therefore been running the game's math at a precision the
real game does not use. That blocks the readable arithmetic tier and probably affects M4 trace
parity.

## Decision

1. **Visible journeys are the spine of the plan.** `docs/GOAL.md` §3 defines a ladder of
   journeys, each one an automated check on a frame the app draws, plus a `make` target the
   user can run to see it. Reference-app journeys (R1–R6) come first because they make the
   game visible and give later native work its judge. Native journeys (N1–N5) follow. At
   least every other `/goal` step must advance a journey row or the work that row is blocked
   on. Steps report what is now visible.
2. **Verification is proportionate to risk.** Each kind of change has one required level of
   verification (GOAL §7):
   * moved constants: a unit test against the PE bytes;
   * platform work: journey checks;
   * exact ports and refactors: oracle, mutants and holdout;
   * independent review: only for a recovered system or a pilot.

   One full gate runs per step, and each step gets one HANDOFF entry. New tests and probes are
   committed with the code they judge. Edits to *existing* judge files remain separate
   `fix(judge)` commits. A step commits its gate record and a short evidence summary. A
   reusable harness goes to `tools/`. Throwaway harness scripts are not committed.
3. **Step size has a floor.** A step is a whole system, a whole module rewrite, or a journey
   row. Remaining numeric-initialization families are batched into one step with an exit
   criterion: no numeric executable reads left in `NativeGame::initialize_source_facts`.
   Resource references go to the importer.
4. **Model Direct3D's FPU side effect in the reference.** `CreateDevice` (and `Reset`)
   without `D3DCREATE_FPU_PRESERVE` sets the guest thread's precision control to 24-bit with
   round-to-nearest. This is a reported assumption, backed by Microsoft's documentation and
   the Windows capture. The guest digests that change because of it are re-baselined in one
   `fix(judge)` commit that cites this ADR. Then the census is rerun. Wherever it shows
   `0x007f`, STYLE §7 level 1 (plain `float`) is the default for rewritten game code. M4
   remains the final word on Windows parity.
5. **Readability counts rewrites, not relocations.** Moving game behaviour into `src/guest`
   or `src/hle` is not readability progress. STATUS reports the lines of game behaviour still
   in reference-tier composition files next to the ratchet totals. Generated tables
   (`sonic_animation_operations.hpp`, `sonic_animation_table_data.hpp`) are counted
   separately until they become named data or imported content.
6. **Short context.** The mandatory reading before a step is AGENTS.md, GOAL.md, STYLE.md and
   the top of STATUS.md. The other documents are references to consult when the step touches
   their subject. Stale guides move to `docs/history/`.

## Consequences

* The user sees progress on screen: first the reference app in 3D and playable, then the
  native app drawing the same screens without `sonic.exe`.
* A Metal fixed-function (transform and lighting) renderer becomes early work. Its design
  must serve the native Ninja renderer later, so it consumes a device-level command stream,
  not guest memory.
* Changing the guest FPU precision changes recorded digests once (determinism, override
  digests, controller parity). The re-baseline is a deliberate, evidenced judge change. It is
  not a fix to pass a test.
* Throughput rises because each step spends more of its time on code than on ceremony. The
  tamper evidence is kept: existing judges still change only in separate commits.

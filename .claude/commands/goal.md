---
description: Do one step of the sadx-macos goal (native SADX port) — analyse with REA, write readable game-tier code + reference adapter, verify, gate, handoff, commit
argument-hint: "[item from docs/GOAL.md §6, an address, or a slug; empty = the first open item]"
---

You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS). The
product is readable, modular C++ organised by game system, which anyone can clone, build and
play with assets imported from their own disc or install. It needs no sonic.exe, no original
DLL execution and no x86 runtime (ADR-0005). Names come from the symbol database, addresses
appear only in provenance annotations (ADR-0006), and everything address-shaped lives in the
reference tier (ADR-0007, docs/STYLE.md). Fidelity to the Windows reference is proved by
differential tests, and the hybrid guest is the judge, not the product. New features later
attach as opt-in enhancements over the faithful game.

Item for this step: **$ARGUMENTS** (empty means the first open item of docs/GOAL.md §6).

Read, in order, before touching code:

1. `AGENTS.md` (rules; CLAUDE.md is a symlink to it)
2. `docs/GOAL.md` (the brief: tracks, how REA is used, where things stand, the ordered items,
   the loop)
3. `docs/STYLE.md` (how game-tier code and reference adapters are written; the review checklist)
4. `docs/STATUS.md` (generated from the last gate; do not hand-edit)
5. `docs/ROADMAP.md` (the milestone whose table is not green is the current one)
6. `docs/ARCHITECTURE.md`, `docs/METHOD.md` (§0 is the tool cookbook), `docs/TIPS.md`, and
   `docs/decisions/ADR-0001..0007`
7. the last three entries of `docs/HANDOFF.md` (and `docs/NEXT-AGENT-PROMPT.md` item 1 when the
   step binds an override)

Then do exactly ONE step of the loop in `docs/GOAL.md` §7:

* Name the item and its track (R readability, S recovery, I independence, V verification,
  E enhancements) in one line. If `$ARGUMENTS` names an address or slug, locate it first in
  the symbol database, `reference/inventory/functions.json`, the handoff and the override
  tables.
* Build the context bundle first (METHOD §0, GOAL §7 step 2): names, signatures, structs and
  enums from the symbol database and the mod loader headers; callers and callees with names,
  and which are native; named pseudocode through REA's Ghidra (owned runner
  `reference/ghidra-live.mjs` + `reference/ghidra-inventory-config.json`, or the CLI with
  `--snapshot`); xrefs; profile status; the nearest game-tier module as a style model. Cite
  every `evidence_id`. Pseudocode is evidence, not source.
* Write **game-tier code** (STYLE): the system module, semantic names, `FUNCTION`/`GLOBAL`
  annotations, structured control flow over enums, literal constants, the most readable exact
  arithmetic the oracle accepts, and `STUB` seams for unrecovered callees. Write a **reference
  adapter** holding every guest address, register marshalling step and fixture unknown.
  Invent nothing: every field, callee and guard has evidence, and added helpers are tagged
  `NOTSA`. Respect the layer order and one slug per module.
* Verify at the level the roadmap asks:
  * unit test + manifest row;
  * oracle + verdict + mutants, with dev cases while iterating and holdout cases at the end;
  * corpus replay + bridge/core mutants + unchanged live digests for overrides;
  * goldens/replays for M2/M3;
  * for a refactor, the same oracle's verdict and counts unchanged.

  Break the code once and watch the test fail. Never edit a tolerance, fixture, mutant or
  harness to get your own code through; judge fixes are separate `fix(judge)` commits. Three
  failed approaches → record the unknown (HANDOFF + `docs/TIPS.md`) and move on. A recovered
  system or pilot refactor gets an independent subagent review with the evidence and STYLE §12.
* Gate: `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. Record only on rc 0 with
  `tools/gate/record_gate.py`, then `python3 tools/gate/gen_status.py`. Rerun and track the
  oracles of any ported code you touched. "Partial" is reported as partial, never as pass.
* Append a short `docs/HANDOFF.md` entry using the template in its header (evidence ids,
  changes, verdicts with counts, readability delta, what was NOT run, next). Commit with a
  Conventional Commit subject, no attribution lines, no push.

Do not stop at a plan or a summary checkpoint. The step ends with a recorded gate and a
commit, or with an explicit statement of what blocked it and what was left unverified.
Report the outcome first: gate name and rc, test counts, readability delta, unverified
scope, skipped tests, and the next item from §6.

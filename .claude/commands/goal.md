---
description: Do one step of the sadx-macos goal (native SADX port): advance a visible journey row or remove readability debt, verify proportionately, gate once, handoff, commit
argument-hint: "[item from docs/GOAL.md §6, a journey row (R2…N5), an address, or a slug; empty = the first open item]"
---

You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS). The
product is readable, modular C++ organised by game system, which anyone can clone, build and
play with assets imported from their own disc or install. It needs no sonic.exe, no original
DLL execution and no x86 runtime (ADR-0005). Names come from the symbol database, addresses
appear only in provenance annotations (ADR-0006), and everything address-shaped lives in the
reference tier (ADR-0007, docs/STYLE.md). Fidelity is proved by differential tests; the
hybrid guest is the judge, not the product. Progress is measured on screen (ADR-0008).

Item for this step: **$ARGUMENTS** (empty means the first open item of docs/GOAL.md §6).

Read before touching code: `AGENTS.md`, `docs/GOAL.md`, `docs/STYLE.md`, the top of
`docs/STATUS.md`, and the last entry of `docs/HANDOFF.md`. Open METHOD, TIPS, ARCHITECTURE,
ROADMAP, JUDGE or an ADR only when the step touches its subject.

Then do ONE step of the loop in `docs/GOAL.md` §7:

* In one line, name the item, its track (J journeys, R readability, S recovery, V
  verification, E enhancements) and its outcome: the journey row it advances, or the debt it
  removes (file and counts).
* A step is a whole system, a whole module rewrite or a journey row (GOAL §3.3). It is never
  a single constant family.
* For game logic, build the context bundle first (METHOD §0, GOAL §7 step 2) and cite every
  `evidence_id`. Pseudocode is evidence, not source. Platform work (renderer, input, audio,
  app, importer) starts from the D3D8/Win32 contract and the guest's call stream instead.
* Write game-tier code (STYLE): semantic names, `FUNCTION`/`GLOBAL` annotations, structured
  flow, literal constants, plain arithmetic where STYLE §7 allows it, `STUB` seams. Put every
  address, marshalling step and fixture unknown in the reference adapter. Invent nothing.
* Verify at the level in the GOAL §7 step 4 table, no more and no less. Break the code once
  and watch the test fail. Never edit an existing judge to pass; judge fixes are separate
  `fix(judge)` commits. Three failed approaches → record the unknown and move on.
* Gate once: `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. Record only on rc 0
  (`tools/gate/record_gate.py`), then `python3 tools/gate/gen_status.py`. "Partial" is never
  "pass".
* Look at the screen for journey work: take a screenshot with `sadx-app --screenshot` and read
  it.
* Write one HANDOFF entry of at most 15 lines (template in its header) and commit with a
  Conventional Commit subject. No attribution lines, no push.

Report in plain language. Start with the outcome: what the user can now see, with the command
to see it, or the debt removed. Then give the gate name and rc, the unverified scope, the
skipped tests, and the next item from §6.

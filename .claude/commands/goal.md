---
description: Continue the sadx-macos goal (native SADX port): advance the next native story-route checkpoint with readable game code, verify proportionately, gate at the checkpoint, handoff, commit, continue
argument-hint: "[item from docs/GOAL.md §6, a checkpoint row (N3b…N10, P), an address, or a slug; empty = the first open item]"
---

You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS).
The product is readable, maintainable C++ organised by game system, which anyone can clone,
build and play with assets imported from their own disc or install. It runs no sonic.exe,
no original DLL and no x86 runtime (ADR-0005). Names come from the symbol database and
addresses appear only in provenance annotations (ADR-0006). Everything address-shaped lives
in the reference tier (ADR-0007, docs/STYLE.md). The hybrid guest is the judge, not the
product.

The acceptance target is one connected, normal-input NATIVE Sonic story route
(docs/GOAL.md §1.2): start → save → character select → Chaos 0 → Emerald Coast → train →
Egg Hornet → Wind Stone → Windy Valley, at a measured 63 FPS, with audio, keyboard and
Switch 2 Pro input, and the pinned Input/Speedrun behaviour. Progress is the next native
checkpoint in GOAL §3.1, shown on screen and proved by an automated check.

Item: **$ARGUMENTS** (empty means the first open item of docs/GOAL.md §6).

Read before touching code: `AGENTS.md`, `docs/GOAL.md`, `docs/STYLE.md`, the top of
`docs/STATUS.md`, and the last entry of `docs/HANDOFF.md`. Open METHOD, TIPS, ARCHITECTURE,
ROADMAP, JUDGE, TESTING or an ADR only when the work touches its subject.

Then work through the loop in `docs/GOAL.md` §7:

* In one line, name the item, its checkpoint row and the outcome: the boundary that moves,
  or the debt removed (file and counts).
* Integrate before you recover. Run the native app to its current stop, and let the first
  missing call decide the work. Keep the checkpoint's dependency table (GOAL §8) current.
* Write readable game-tier code (GOAL §4, STYLE):
  * semantic names, `enum class` and `FUNCTION`/`GLOBAL`/`STUB` annotations;
  * structured flow, literal constants, plain arithmetic where the oracle accepts it.

  Banned in the game tier: no `call(0x…)`, no `op.at`, no `fp.*_memory` emulator calls,
  no `*_at_<hex>`, and no constants fetched from the EXE. When the route reaches a legacy
  module, migrate it under its unchanged oracle; never link a guest adapter into
  production. Invent nothing; cite every `evidence_id`.
* Verify at the level in the GOAL §7 table. Checkpoint checkers must check drawn content,
  not just image headers. Break the code once and watch the check fail.
* Never edit an existing judge to pass. Judge fixes are separate `fix(judge)` commits.
  After three failed approaches, record the unknown and move on.
* Gate at the checkpoint: `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. Record
  only on rc 0 (`tools/gate/record_gate.py`), then run `python3 tools/gate/gen_status.py`.
  "Partial" is never "pass".
* Reviewed work lives in git (`packet/<name>` branches in worktrees), never only in /tmp.
* Write one plain-sentence HANDOFF entry (at most 15 lines, spaces between words and
  numbers). Commit with a Conventional Commit subject, no attribution lines, no push.

Report in plain language:

* what the native app now does, with the command to see it;
* the gate name and rc;
* the unverified scope and skipped tests;
* the next missing call.

Then continue with the next item until the route is done or you are truly blocked.

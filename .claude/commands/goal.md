---
description: Do one step of the sadx-macos goal (native SADX port) — analyse with REA, implement, verify, gate, handoff, commit
argument-hint: "[item from docs/GOAL.md §6, an address, or a slug; empty = the first open item]"
---

You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS), built by
exact reverse engineering and proven by automated tests. Metal for graphics, GameController for
Xbox and Switch 2 Pro pads, fidelity to the Windows reference over convenience. The finished
game builds from maintained source and runs from imported assets without sonic.exe, original
Windows/mod DLL execution or an x86 runtime (ADR-0005); the hybrid app is the development reference.

Item for this step: **$ARGUMENTS** (empty means the first open item of docs/GOAL.md §6).

Read, in order, before touching code:

1. `AGENTS.md` (rules; CLAUDE.md imports it)
2. `docs/GOAL.md` (the brief: history, how REA is used, where things stand, the ordered next items, the loop)
3. `docs/STATUS.md` (generated from the last gate; do not hand-edit)
4. `docs/ROADMAP.md` (the milestone whose table is not green is the current one)
5. `docs/ARCHITECTURE.md`, `docs/METHOD.md`, `docs/decisions/ADR-0001..0006` (0005 and 0006 define
   the destination: executable-independent native source, semantic names with address provenance
   annotations)
6. the last three entries of `docs/HANDOFF.md` (and `docs/NEXT-AGENT-PROMPT.md` item 1 when the
   step binds an override: it holds every bridge's stack reserves, store phases and boundaries)

Then do exactly ONE step of the loop in `docs/GOAL.md` §7:

* Name the item in one line. If `$ARGUMENTS` names an address or slug, locate it in
  `reference/inventory/functions.json`, the handoff and the override tables first.
* Build the context bundle first (GOAL §7 step 2): names, signatures and structs from the symbol
  database / `reference/sadx-mod-loader/SADXModLoader/include/*New.h`, callers/callees and which
  are native, profile status, the nearest recovered module as a style model.
* Analyse with REA's Ghidra through the owned runner (`reference/ghidra-live.mjs` with
  `reference/ghidra-inventory-config.json`; results in `reference/inventory-raw/`). Hopper is not
  used. Exact bytes only via `tools/lib/pe_image.py`. Cite every `evidence_id` in the handoff entry.
* Implement in the right layer (`cpu < loader < math < engine < input < player < game < runtime <
  guest < hle < platform < apps`), one slug per module, semantic names with ADR-0006 provenance
  annotations (`// FUNCTION: SONIC 0x411e30 dsInitInt`), one owner per source address, guest
  addresses only in reference adapters, tagged failures with the source boundary, assumptions in
  `HleProcess::assumptions`. Pseudocode is evidence, not source.
* Verify at the level the roadmap asks (unit + manifest row; oracle + verdict + mutants for exact
  ports; corpus replay + bridge/core mutants + unchanged live digests for overrides; goldens/replays
  for M2/M3). Break the code once and watch the test fail. Never edit a tolerance, fixture,
  mutant or harness to get your own code through; three failed approaches → record the unknown
  and move on (AGENTS.md "Decompiling with an LLM").
* Gate: `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. Record only on rc 0 with
  `tools/gate/record_gate.py`, then `python3 tools/gate/gen_status.py`. Rerun and track the oracles
  of any ported code you touched; "partial" is reported as partial, never as pass.
* Append a `docs/HANDOFF.md` entry (evidence ids, what was done, counts, limits, what was NOT run),
  commit with a Conventional Commit subject, no attribution lines, no push.

Do not stop at a plan or a summary checkpoint. The step ends with a recorded gate and a commit, or
with an explicit statement of what blocked it and what was left unverified. Report outcome first:
gate name and rc, test counts, unverified scope, skipped tests, and the next item from §6.

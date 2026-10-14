# Working on sadx-macos (read this first)

This repository is a native macOS (Apple Silicon) port of *Sonic Adventure DX* (2004 PC,
`sonic.exe`) with the pinned speedrun mod profile. It is built almost entirely by AI
agents, across many sessions. These rules keep every session pointed at the same goal.

## The goal, in one sentence

A native app that plays SADX with the speedrun profile faithfully enough for speedrun
practice, proven by automated tests (`docs/ROADMAP.md`), not by someone playing it.

## Orientation (5 minutes)

1. `docs/STATUS.md` — where we are (generated from the last gate; do not hand-edit facts).
2. `docs/ROADMAP.md` — the milestones and their tests; pick work from the current milestone.
3. `docs/ARCHITECTURE.md` — where things live and the ownership rules.
4. `docs/METHOD.md` — how to analyse the binary (REA + Ghidra), how to port, how to test.
5. `docs/SOURCES.md` — the inputs (exe, profile DLLs, disc) and their hashes.
6. `docs/decisions/` — decisions already made; do not relitigate them silently. Add an ADR
   if you change direction, and say so in your report.
7. `docs/HANDOFF.md` — the per-step journal (append an entry when you finish a step).
8. `docs/GOAL.md` — the standing brief `/goal` loads (`.claude/commands/goal.md`): history, how
   REA is used, the ordered next items and the per-step loop.

## Non-negotiables

* **Fidelity over convenience.** The reference is the Windows game under the pinned
  profile (`docs/SOURCES.md`). When a behaviour is unknown, the code says so
  (`std::optional`, tagged failure results, reported assumptions) — it never guesses a
  value or masks a mismatch to make a test pass.
* **Never commit binaries or game assets.** `game/`, DLLs, `.exe`, disc images, large
  reference captures stay local and ignored. `tools/gate/check_repo_hygiene.py` runs in the gate.
* **One owner per source global.** A `*_at_<hex>` member lives in exactly one struct;
  `tools/gate/check_global_owners.py` enforces it. Borrow with references/pointers.
* **Tagged results, explicit unknowns.** Failures carry the source boundary (the
  instruction address) and the fact that was missing. Unported callees are reported, not
  silently skipped, except under the survey policy, which is navigation, not behaviour.
* **Report honestly.** A gate is recorded only with its real exit status. "Unverified"
  scope is named. Tests that were skipped are listed as skipped.
* **Profile awareness.** Every ported range is checked against
  `tools/gate/profile_patches.json` (`tools/gate/check_profile_patches.py`, 0 UNHANDLED). Under the
  profile the loader and mods replace game code; the port implements what actually runs.
* **No attribution lines in commits** (no Co-Authored-By / Generated-with).

## The loop for any piece of work

1. Read `docs/STATUS.md`; choose the next item of the current milestone (or the item the
   user named). Say which one you are doing.
2. Analyse with REA's Ghidra provider through the owned runner (`docs/METHOD.md` §1).
   Hopper is not used (demo build hangs). Raw bytes via `tools/lib/pe_image.py` only for exact
   byte checks and oracle fixtures.
3. Implement in the right layer (`docs/ARCHITECTURE.md`: cpu < loader < math < engine <
   input < player < game < runtime < guest < hle < platform < apps; include only downward,
   `tools/gate/check_layers.py` enforces it). One slug per module:
   `src/<layer>/<slug>.{hpp,cpp}` + `tests/unit/<slug>.cpp` (+ a `tests/unit/manifest.tsv`
   line when it takes arguments). An oracle is `tests/probes/native_<slug>_probe.cpp` +
   `tools/oracles/verify_original_<slug>.py`; the target `oracle-<slug-with-dashes>` and the
   report `build/oracles/<slug>.json` exist by themselves (`tools/oracles/manifest.tsv`
   only when it needs other inputs than the exe and the probe).
4. Verify at the level the roadmap asks for: unit test; oracle with mutants for exact
   ports; override-equivalence / replay / golden tests for runtime work.
5. Run the gate: `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. Record it only
   when rc is 0: `python3 tools/gate/record_gate.py --log build/gate.log --command "make -j10 gate"
   --exit-status 0 --limit "…" --output reference/production-integration-<date>/<name>-gate.json`,
   then `python3 tools/gate/gen_status.py` to refresh `docs/STATUS.md`.
   If you touched ported code, also run its oracles (`make oracle-<slug>`, or `make -j6
   oracles` for all, ~10 min), read them with `make oracles-verdict`, and track the ones
   you reran with `python3 tools/gate/track_reports.py <slug>…` (size-bounded summaries in
   `reference/oracle-reports/`). "partial" (values exact, FP flags differ) is not "pass":
   say which it is.
6. Append a `docs/HANDOFF.md` entry: evidence ids, what was done, test counts, limits.
7. Commit with a Conventional Commit subject (`feat(guest): …`, `fix(ninja): …`,
   `docs: …`). Push only when asked.

## Pitfalls that cost time before

* The build uses `-Werror -Wmissing-field-initializers`: aggregate initialisers must list
  every field (`{}` for the rest).
* A literal like `0x3d0842e+0x18` is one preprocessing number; write `0x3d0842e + 0x18`.
* Fastcall callees pop their stack arguments; oracle fixtures must model that.
* Do not map page 0 in an oracle when a null-pointer fault must be reproduced.
* Probe and native must start from identical initial values; never mask a mismatch.
* `make` mtime races: after editing a source, the probe `.dylib` must be newer than it
  before an oracle runs (`tools/gate/mutate.py` sleeps for this reason).
* Unicorn 2.1.4 raises no SSE exception flags and applies the x87 NaN rule to SSE
  operands; oracles compare MXCSR control bits only and keep one NaN source per case.
* Game paths contain spaces; quote `$(GAME_EXE)`.

## Decision rights

Agents decide implementation details, test design, and the order within a milestone.
Changing a milestone's definition of done, the fidelity policy, the licence, or what is
committed needs an ADR and the user's confirmation.

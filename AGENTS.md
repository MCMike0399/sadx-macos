# Working on sadx-macos (read this first)

This repository is a native macOS (Apple Silicon) port of *Sonic Adventure DX* (2004 PC,
`sonic.exe`) with the pinned speedrun mod profile. It is built almost entirely by AI
agents, across many sessions. These rules keep every session pointed at the same goal.

## The goal, in one sentence

A native app that plays SADX with the speedrun profile faithfully enough for speedrun
practice, proven by automated tests (`docs/ROADMAP.md`), not by someone playing it, and
built from readable, maintained source that needs neither `sonic.exe` nor the mod DLLs to
build or run (assets are imported once from the user's own disc/install; ADR-0005, ADR-0006).
The emulated hybrid runtime is the development reference that proves the native code right;
it is not the product. The source must read like a game codebase, organised by game system, so
anyone can download it, play with their own assets, maintain it, and extend it with opt-in
features that leave the faithful game untouched (ADR-0007, `docs/STYLE.md`).
Progress is measured on screen: the journey ladder in `docs/GOAL.md` §3 (the reference app
first, then the native app showing the same screens without `sonic.exe`). ADR-0008 is why.

## Orientation (5 minutes)

Read before every step (about 1,000 lines):

1. `docs/STATUS.md`, the top section: where we are (generated from the last gate; do not
   hand-edit facts).
2. `docs/GOAL.md`: the standing brief `/goal` loads (Codex: `/goal implement @docs/GOAL.md`;
   Claude Code: `.claude/commands/goal.md`). It holds the journey ladder, the ordered next
   items, the step size and verification rules, and the per-step loop.
3. `docs/STYLE.md`: **how game code is written** (two tiers, names, control flow, seams,
   arithmetic, constants, enhancements, review checklist). ADR-0007 is why.
4. The last entry of `docs/HANDOFF.md`.

Consult when the step touches the subject:

* `docs/METHOD.md` §0: the tool cookbook (REA + Ghidra, symbols, oracles).
* `docs/TIPS.md`: SADX/MSVC idioms and lessons. Add to it when you learn something.
* `docs/ARCHITECTURE.md`: layers, ownership, where things live.
* `docs/ROADMAP.md`: milestone test tables.
* `docs/SOURCES.md`: inputs and their hashes.
* `docs/JUDGE.md`: the judge manifest and judge commits.
* `docs/TESTING.md`: the test layers, what they do not catch, and the complete suite to build.
* `docs/decisions/`: decisions already made. Do not relitigate them silently. Add an ADR if
  you change direction, and say so in your report. ADR-0005..0008 define the destination and
  the current way of working. `docs/RESEARCH-native-recompilation.md` is why, and
  `CONTEXT.md` is the vocabulary.

`docs/history/` and the `REVIEW-*` files are history, not instructions.

## Non-negotiables

* **Fidelity over convenience.** The reference is the Windows game under the pinned
  profile (`docs/SOURCES.md`). When a behaviour is unknown, the code says so
  (`std::optional`, tagged failure results, reported assumptions) — it never guesses a
  value or masks a mismatch to make a test pass.
* **Never commit binaries or game assets.** `game/`, DLLs, `.exe`, disc images, large
  reference captures stay local and ignored. `tools/gate/check_repo_hygiene.py` runs in the gate.
* **Names in identifiers, addresses in annotations** (ADR-0006). New and migrated code is
  named from the symbol database (`docs/GOAL.md` §4.4; the mod loader's `*New.h` headers first)
  in the project's style, with `// FUNCTION: SONIC 0x411e30 dsInitInt`-style provenance on the
  line above. Never put a `sub_<hex>`/`FUN_<hex>` function or a Ghidra `DAT_`/`local_`/`param_`
  name into `src/`. No new `*_at_<hex>` identifier either: `tools/gate/check_provenance.py`
  runs in the gate. Guest addresses, register
  marshalling and override registration exist only in the reference adapters (`src/guest`,
  `src/hle`).
* **Readable game tier, address-shaped reference tier** (ADR-0007, `docs/STYLE.md`). Game code
  uses semantic names, enums, structured control flow, literal constants, readable exact
  arithmetic and named `STUB` seams for unrecovered callees. It never contains callee-address
  calls (`call(0x…,0x…)`), address labels or `goto`, constants fetched from the EXE, or
  `std::optional` fields that model "the fixture did not supply this". Those belong in the
  reference adapter (`src/guest`, `src/hle`). Invent nothing: every field, callee and guard
  traces to evidence, and helpers that are not in the original are tagged `// NOTSA:`. A
  transliterated draft may exist only inside a step and is refactored under the same oracle
  before the commit. `tools/gate/check_readability.py` runs in the gate, and its per-file
  baseline may only go down. Moving game behaviour into `src/guest` or `src/hle` is not
  readability progress (ADR-0008 §5). Rewrite it, or leave it where it is and say so.
* **One owner per source address.** Legacy `*_at_<hex>` members live in exactly one struct
  (`tools/gate/check_global_owners.py`); annotated `GLOBAL`s are checked the same way
  (`check_provenance.py`). Borrow with references/pointers. Migrate an owner
  whole (name + annotation + check) or not at all.
* **Tagged results, explicit unknowns.** Failures carry the source boundary (the
  instruction address) and the fact that was missing. Unported callees are reported, not
  silently skipped, except under the survey policy, which is navigation, not behaviour.
  In new and refactored code the unknowns are reported at their boundary: the reference
  adapter (missing guest/fixture fact), a `STUB` seam (unrecovered behaviour), or the
  importer (missing asset). They are not threaded through game logic (ADR-0007).
* **Report honestly.** A gate is recorded only with its real exit status. "Unverified"
  scope is named. Tests that were skipped are listed as skipped.
* **Profile awareness.** Every ported range is checked against
  `tools/gate/profile_patches.json` (`tools/gate/check_profile_patches.py`, 0 UNHANDLED). Under the
  profile the loader and mods replace game code; the port implements what actually runs.
* **No attribution lines in commits** (no Co-Authored-By / Generated-with).

## Toolbox: what to use for what

Paths are relative to the repo. **Exact command lines for every row are in the cookbook,
`docs/METHOD.md` §0**, with details in §1–2 and `docs/GOAL.md` §4.

| Question | Tool |
|---|---|
| What is this address called, what is its signature/struct? | the symbol database (`make symbols`, then the lookup snippet in METHOD §0 over `build/symbols/symbols.json`); the declaration via `grep -in 0x0*ADDR reference/sadx-mod-loader/SADXModLoader/include/*.h`. Treat names as hypotheses; record decisions in `reference/symbols/overrides.tsv` |
| Is this idiom/quirk known? | `docs/TIPS.md` |
| Pseudocode / dossier / CFG of functions (batch of new questions) | the owned runner `reference/ghidra-live.mjs reference/ghidra-inventory-config.json` (REA → Ghidra, snapshot cached); `batch_decompile` for clusters, `analyze_function` for what you port |
| One quick or cached question | REA CLI `node reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs function\|decompile\|xrefs\|instructions …` with `--snapshot` (GOAL §4.2); `rea doctor --provider ghidra` before blaming Ghidra |
| Who reads/writes a global | runner `xrefs`; `tools/analysis/xscan.py ADDR…` for raw `.text` references |
| Callback/vtable targets Ghidra missed; names into Ghidra | the seed TSV (`address<TAB>function\|label<TAB>name`), then one fresh snapshot (seed digest is part of the profile) |
| Assembly of a stored result | `tools/analysis/ghidra_asm.py reference/inventory-raw/<label>.json` |
| Exact bytes / constants | `tools/lib/pe_image.py` (`PEImage(path).read(va, n)`), never a listing |
| Call graph, sizes, hot paths, FPU sites | `reference/inventory/{functions,hot-paths,fpu-sites}.json` (`tools/inventory/run_inventory.py`) |
| Does a mod patch this range? | `tools/gate/check_profile_patches.py` (0 UNHANDLED), `tools/gate/profile_patches.json`, mod loader source in `reference/sadx-mod-loader/` |
| Embedded models/landtables/animations/texlists in the exe | X-Hax sa_tools `GameConfig/PC_SADX` mappings (fetch pinned, ignored; GOAL §4.4); `tools/lib/extract.py` is only a bring-up extractor |
| Is my native code equal to the original? | oracle: `tests/probes/native_<slug>_probe.cpp` + `tools/oracles/verify_original_<slug>.py`, `make oracle-<slug>`, `make oracles-verdict`, `tools/gate/mutate.py`; overrides: corpus replay + live digests (`make m1`) |
| Runtime facts (values, order, what actually runs) | the guest (`sadx-app --guest …`, `make m1` targets); HLE assumptions printed by the runtime |
| Generated tables from source data | `tools/emit/` emitters (check them into the gate like any test) |
| Gate, status, journal | `make -j10 gate`, `tools/gate/record_gate.py`, `tools/gate/gen_status.py`, `tools/gate/track_reports.py`, `docs/HANDOFF.md` |

Not used: Hopper (ADR-0002), REA live renames (`set_addresses_names` does not survive the
session and blocks the snapshot save; use the seed file), Ghidra GUI edits (not reproducible).

## Decompiling with an LLM (you): rules that keep it honest

These come from projects that ran agent decompilation at scale (GOAL §4.3) and from this
project's own sessions.

* **Context first, code second.** Build the context bundle of GOAL §7 step 2 (names, structs,
  callers/callees and which are native, pseudocode, profile status, a style model) before
  writing a line.
* **Pseudocode is evidence, not source.** Rewrite it as named C++ with types; do not paste
  Ghidra's `DAT_`, `local_`, `param_` or pointer-arithmetic shapes into `src/`. Keep exact
  arithmetic, operation order and FP width where the oracle says they matter.
* **One function or one tight cluster per attempt**, with the differential judge (oracle,
  override-equivalence, replay) as the only definition of "works". A *step* is larger: a whole
  system, a whole module rewrite or a journey row (GOAL §3.3), made of several such attempts.
  Reread `docs/STYLE.md` before writing; instructions decay over long sessions.
* **Readable but wrong is the default failure** of agent decompilation (RESEARCH third pass).
  Readability is never judged by eye: every readability change reruns the same oracle and
  mutants. Iterate on dev cases, and keep holdout cases for the final run.
* **Never game the judge.** Do not edit a tolerance, an expected value, a fixture, a mutant, the
  oracle harness or the gate scripts to get your own code through; do not narrow a test's
  inputs to dodge a mismatch. If the judge is wrong, fix it in a separate commit with evidence
  (`fix(judge): …`), never mixed with a port. Agents in other projects edited verifiers and
  hashes when they could, so the judge is tamper-evident (`docs/JUDGE.md`).
* **Bound dead ends.** After three failed approaches on one function, stop; record the unknown
  (evidence id, what was tried, the mismatch) in the handoff and move on.
* **Independent review** for a recovered system or pilot refactor: a fresh subagent reads the
  evidence, the oracle results and the diff, and works through `docs/STYLE.md` §12 (masked
  mismatches, invented facts, address-shaped code, over-abstraction) before the gate. A
  reviewer without objective criteria approves wrong code.
* **Parallelise by independent clusters**, never two agents on one owner; each writes its own
  HANDOFF lines with evidence ids.

## The loop for any piece of work

1. Read `docs/STATUS.md`; choose the next item of the current milestone (or the item the
   user named). Say which one you are doing.
2. Look up names and types (symbol database / mod loader headers), then analyse with REA's
   Ghidra provider through the owned runner (`docs/METHOD.md` §1, GOAL §7 step 2).
   Hopper is not used (demo build hangs). Raw bytes via `tools/lib/pe_image.py` only for exact
   byte checks and oracle fixtures.
3. Implement in the right layer (`docs/ARCHITECTURE.md`: cpu < loader < math < engine <
   input < player < game < runtime < guest < hle < platform < apps; include only downward,
   `tools/gate/check_layers.py` enforces it). One slug per module:
   `src/<layer>/<slug>.{hpp,cpp}` + `tests/unit/<slug>.cpp` (+ a `tests/unit/manifest.tsv`
   line; every test needs one to run in the gate). Game-tier code in the system's module plus
   a reference adapter for everything address-shaped (`docs/STYLE.md`); semantic names +
   provenance annotations (ADR-0006). An oracle is `tests/probes/native_<slug>_probe.cpp` +
   `tools/oracles/verify_original_<slug>.py`; the target `oracle-<slug-with-dashes>` and the
   report `build/oracles/<slug>.json` exist by themselves (`tools/oracles/manifest.tsv`
   only when it needs other inputs than the exe and the probe).
4. Verify at the level the change needs (the table in GOAL §7 step 4): a byte-comparison
   unit test for moved constants; journey checks for platform work; the unchanged oracle for a
   rewrite; oracle, mutants and holdout for a new exact port. Do not add ceremony that the table
   does not require.
5. Run the gate: `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. Record it only
   when rc is 0: `python3 tools/gate/record_gate.py --log build/gate.log --command "make -j10 gate"
   --exit-status 0 --limit "…" --output reference/production-integration-<date>/<name>-gate.json`,
   then `python3 tools/gate/gen_status.py` to refresh `docs/STATUS.md`.
   If you touched ported code, also run its oracles (`make oracle-<slug>`, or `make -j6
   oracles` for all, ~10 min), read them with `make oracles-verdict`, and track the ones
   you reran with `python3 tools/gate/track_reports.py <slug>…` (size-bounded summaries in
   `reference/oracle-reports/`). "partial" (values exact, FP flags differ) is not "pass":
   say which it is.
6. Append one short `docs/HANDOFF.md` entry (template in its header): the outcome (what is
   visible, or the debt removed), test counts, limits.
7. Commit with a Conventional Commit subject (`feat(guest): …`, `fix(ninja): …`,
   `docs: …`). Push only when asked.

## Pitfalls that cost time before

* `app-smoke` stops at frame 120 on the SEGA logo, and the intro movie is drawn black. To see
  the game, skip the movie with Enter: `./build/sadx-app --guest
  reference/updated-pc-reference/game reference/windows-system --keys 450:13` (or `make play`
  once GOAL §6 item 1 adds it). Look at a screenshot before claiming visible progress.
  Automated runs use the harness (`docs/TESTING.md` §3.0):
  * `sadx-app --guest … --offscreen --script FILE` for screenshots and game-state JSON at any
    frame, with no window;
  * `tools/journey/sadx_driver.py` to play step by step (`press`, `stick`, `advance`,
    `screenshot`, `state`).

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

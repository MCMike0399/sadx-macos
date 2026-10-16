# The goal, for `/goal`

This is the standing brief that an agent session loads when the user types `/goal` in
`~/Developer/sadx-macos`. `.claude/commands/goal.md` reads it, then `docs/STATUS.md`. It was
last rewritten on 2026-10-15, after the user reviewed the source and asked for code that is
readable, modular and maintainable enough to extend with new features. That request is
[ADR-0007](decisions/ADR-0007-readable-game-code-and-extensibility.md). It builds on
[ADR-0005](decisions/ADR-0005-native-source-and-executable-independence.md) (native,
executable-independent) and [ADR-0006](decisions/ADR-0006-symbol-names-and-provenance-annotations.md)
(names in code, addresses in annotations). Current numbers and completed gates live in
`docs/STATUS.md`. Numbers below are labelled with their date.

## 0. The prompt (what `/goal` injects)

```
You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS).
The product is readable, modular C++ that anyone can clone, build and play with assets
imported from their own disc or install: no sonic.exe, no original DLL execution, no x86
runtime (ADR-0005). The code reads like a game codebase, not a disassembly. Names come from
the symbol database, addresses appear only in provenance annotations (ADR-0006), and
everything address-shaped lives in the reference tier (ADR-0007, docs/STYLE.md). Fidelity
to the Windows reference is non-negotiable and is proved by differential tests. The hybrid
guest is the development judge, not the product. New features later attach as opt-in
enhancements on top of the faithful game.

Read, in order: AGENTS.md, docs/GOAL.md, docs/STYLE.md, docs/STATUS.md, docs/ROADMAP.md,
docs/ARCHITECTURE.md, docs/METHOD.md (§0 is the tool cookbook), docs/TIPS.md, ADR-0001..0007,
and the last three docs/HANDOFF.md entries. Then do ONE step of the loop in GOAL §7: name the
item (the one the user passed, else the first open item of §6), build the context bundle with
the symbol database and REA's Ghidra, write game-tier code plus its reference adapter,
verify with the differential judge, run the gate, record it only on rc 0, regenerate STATUS,
append a HANDOFF entry, and commit (no push, no attribution). Report honestly: name the
unverified scope, list skipped tests, never call a "partial" oracle a pass, and keep every
unknown explicit at its boundary. Never edit the judge to pass. Do not stop at a plan: a step
ends with a recorded gate or an explicit statement of what blocked it.
```

## 1. The goal, exactly

A native arm64 macOS game, built from readable, maintained source, that plays *Sonic
Adventure DX* with the pinned speedrun profile faithfully enough for speedrun practice. It is
proved by automated tests, and it is structured so that the community can maintain it and
extend it. Anyone can download the source. The user supplies the game assets from their own
disc or installation through a one-time importer.

Four requirements, each with its own acceptance tests:

* **Native execution.** All shipped game and pinned-profile code, cold paths included,
  compiles for arm64. Unicorn, x86 execution and original-code fallbacks belong only to the
  development reference.
* **Executable independence.** Building uses maintained source and declared native
  dependencies. Playing uses imported assets. Neither opens `sonic.exe` or runs an original
  DLL. A one-time import may read embedded data from the user's own EXE/DLL containers and
  emits independent, typed, hashed data (M5).
* **Readable, maintainable source** (ADR-0006, ADR-0007, `docs/STYLE.md`). Game code is
  organised by game system (player, camera, objects, stages, UI…). It uses names from the
  symbol database, recovered types, structured control flow, literal constants and readable
  exact arithmetic. An address appears only in a `// FUNCTION:`/`// GLOBAL:` annotation, and
  everything address-shaped (guest marshalling, callee-address dispatch, fixture unknowns)
  lives in the reference tier. A developer can change Sonic's physics without knowing where it
  lived in `sonic.exe`.
* **Extensible without losing fidelity** (ADR-0007 §8, ROADMAP M6). The faithful game is the
  default and the only thing fidelity tests measure. Enhancements (practice tools, overlays,
  widescreen, new features) are opt-in, attach at named hook points, and are proved not to
  change the game when off. Replay-neutral ones are proved not to change it when on, either.

What readable means, concretely:

```cpp
// before (legacy interpreter: src/guest/game_mode_reference.cpp)
case 0x01:
    r=ret(0x40c124,0x42cc70);
    if(r==1){call(0x40c130,0x411e30,1,1);goto L3bc;}
```

```cpp
// after (game tier; STYLE.md §11)
case GameMode::Logo: {                                  // MD_LOGO
    const int logo_result = run_logos();                // RunSegaSonicTeamLogos (STUB seam)
    if (logo_result == 1) {
        frame_control.set_frame_rate_mode(1, 1);        // dsInitInt
        return enter_title(state);
```

The game-mode oracle still judges the "after" code. The probe calls it through a reference
adapter that maps `run_logos` to 42cc70 in the guest. Exact arithmetic and observable ordering
remain part of fidelity whatever the code looks like.

## 2. History, and what it taught

| Days | What happened | Gates |
|---|---|---|
| 2026-10-10/11 | Port everything, function by function, each with a Unicorn oracle, using Hopper | `render` … `endscene-98` |
| 2026-10-12 | 6 KB of 3.85 MB verified per day; review; ADR-0001 hybrid runtime, ADR-0002 Ghidra via REA, ADR-0003 repo move, ADR-0004 layers | 99–104 |
| 2026-10-13 | M1: loader, mods and SDL2 in the guest; boot → FMV → title → attract, 7,228 deterministic frames; Metal app; keyboard, pads; 40 overrides | 105–129 |
| 2026-10-14/15 | more override bridges; ADR-0005/0006; symbol database (2,166 / 8,875 procedures named); provenance checker; native frame-control pilot built and tested with no guest; screen-init bridge | 130–140 |

**The method change, not more effort, made the day-3 jump.** The hybrid runtime made the
whole intro and title run from the original bytes, with exact ports bound in as overrides.
That produced a judge for everything: oracles, override-equivalence, determinism and replay.
It did not produce a readable native game.

**Why the code became unreadable** (measured 2026-10-15 at `f72a05d`, game layers
`engine input player game runtime`, about 21,300 lines): about 7,700 hex literals, 1,950 uses
of `*_at_<hex>` identifiers, 249 callee-address calls, 73 `goto`s, about 700 lines over 160
characters, and 16 provenance annotations. The causes are in ADR-0007. In short, the method
told agents to transliterate (`METHOD.md` §3 asked for `_at_<hex>` members and instruction
order), the hybrid runtime leaked callee addresses into game code, x87 exactness was written as
emulator calls, constants were fetched from the EXE by address, and fixture unknowns were
threaded through every read. All five are now fixed in the method, and the existing code is
fixed by refactoring under its oracles (track R).

**What cost time, and the rule that came out of it** (details in HANDOFF):

* Hopper demo hangs → Ghidra only, batched (ADR-0002).
* A per-instruction count passed to Unicorn made it 30× slower → wall-clock timeouts only.
* The gate did not run oracles, and 20 silently pointed at stale paths → `make oracles` +
  `make oracles-verdict` when ported code changes; partial is reported as partial.
* Tests outside `tests/unit/manifest.tsv` do not run → every test gets a manifest row.
* stdcall pop counts are load-bearing; code writes into the guest must flush Unicorn's
  translation cache.
* Gates are recorded only with their real rc. Interrupted gates stay interrupted.

**What the wider field learned in the same months** (RESEARCH third pass): agent
decompilation produces code that is *readable but wrong* unless an objective, tamper-proof judge
checks every step (momo5502's 15-agent run). Agents edit the judge when they can (Snowboard
Kids 2). Introduced fields, callees and guards are where hidden divergence lives (arXiv
2609.05370). Readability is best tracked as its own number, judged by the same check
(SM64DS: "readability never costs a match"). Address-bound state peaks before it falls, and
removing it needs a dedicated phase (OpenLoco 342 → 157 → 0).

## 3. Strategy: five tracks

Every `/goal` step belongs to one track. The order in §6 interleaves them.

| Track | What | Judge |
|---|---|---|
| **R — Readability** | refactor existing verified ports into game-tier form plus reference adapters, under their unchanged oracles; build the ratchet and the tooling that makes readable work cheap | the module's oracle verdict and counts unchanged, mutants still caught, the readability baseline goes down |
| **S — System recovery** | recover new systems directly in game-tier form (`METHOD.md` §3), one function or tight cluster per attempt, grouped into coherent systems | a new oracle with mutants and dev/holdout cases; override-equivalence in the live guest |
| **I — Independence** | the asset importer, native session composition (boot → title → stage with no guest), separate production/reference build graphs, frozen reference corpora | M5 rows: `native-source-build`, `asset-import`, `native-runtime-independent`, `reference-corpus-frozen` |
| **V — Verification** | M1–M4: override-equivalence, goldens, replays, audio events, Windows trace parity | ROADMAP tables |
| **E — Enhancements** | later (M6): hook and settings registry, first replay-neutral enhancements | enhancements-off parity, replay-neutral proofs |

The recovery pipeline (stages, mapped to tracks):

* **A. Symbol database** (done): names for 2,166 / 8,875 procedures plus seeds into Ghidra.
* **B. Provenance checker + pilot** (done): `check_provenance.py`; native frame control with
  no guest.
* **R. Readability foundation**: style guide (done in this revision), `.clang-format`, the
  readability ratchet, lookup and context-bundle tools, a tamper-evident judge, the
  control-word census, and pilot refactors (§6 items 3–6).
* **C. System recovery** (track S). For one system at a time: (1) the context bundle, including
  profile hooks, callback targets and every unresolved edge; (2) state meaning, record layouts,
  aliasing, initialisation and callback ownership, then the named interface and its seams;
  (3) every dependency classified as native behaviour, imported content, constant/initial
  state, platform service, or unresolved; (4) game-tier code plus reference adapter, proved
  through the adapter (oracle + mutants + override-equivalence), then from a native caller
  with the guest absent; (5) report remaining seams. Metrics: `STUB` vs `FUNCTION`,
  address-bound owners remaining, guest instructions per frame in replays.
* **E. Importer** (track I): `tools/import/` reads the user's install or disc once. sa_tools
  mappings are the schema, and committed descriptors are the source of truth. It emits typed,
  hashed packs with resolved references. Verify by re-serialising each record to its source
  bytes, resolving every `pointer=` site, and reproducing known pack digests. Packs stay local
  and ignored.
* **F. Freeze the reference**: before a system's last guest dependency goes, record its oracle
  fixtures, replay end states and trace digests into `reference/` (numbers and hashes only).
* **G. Independence gate**: the M5 rows.
* **H. Enhancements** (track E, M6): the hook registry, settings, and the first enhancements.

A static recompiler (Ghidra CFG → C, or M-HT/SR's x86 → LLVM) may still help as a temporary
speed path behind the reference adapter, never as shipped logic (ADR-0005).

## 4. Tools

### 4.1 The cookbook

`docs/METHOD.md` §0 maps each question to a tool and an exact command: symbol lookup, structs
and enums, callees with names, one function's pseudocode (REA CLI with snapshot), clusters
(`batch_decompile` through the runner), xrefs, assembly, bytes and constants (`pe_image`),
profile patches, sa_tools data mappings, runtime facts from the guest, and the oracle judge.
Start there. `docs/TIPS.md` is the knowledge base of SADX and MSVC idioms (x87 compare masks,
constant pools, tail merging, usercall registers). Add to it when you learn something.

### 4.2 REA: facts that matter

* **The owned runner** `reference/ghidra-live.mjs` starts `rea mcp`, opens `sonic.exe` once with
  REA's Ghidra provider, replays a JSON request file and saves a snapshot on close.
  * Allowed tools: `analyze_function`, `inspect_native_instruction`,
    `resolve_native_call_targets`, `trace_native_values`, `xrefs`, `binary_session`,
    `list_procedures`, `procedure_callers`, `procedure_callees`, `procedure_references`,
    `procedure_info`, `batch_decompile`. Add a read-only tool to the `allowed` set when a step
    needs it, one line each. Candidates: `procedure_pseudo_code`, `get_call_graph`,
    `trace_call_path`, `inspect_native_api` (jump tables), `inspect_native_data_type`,
    `inspect_analysis_view` (page a stored dossier by evidence id), `list_strings`,
    `search_strings`, `compare_functions`.
  * Config: `reference/ghidra-inventory-config.json` (snapshot
    `reference/sonic-ghidra-inventory-snapshot.json`, seeds `build/symbols/rea-seeds.tsv` from
    `make symbols`).
* **The CLI** (`node reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs`, REA 6.4.0):
  `function`, `decompile`, `xrefs`, `instructions`, `doctor`. An exact snapshot hit is answered
  before Ghidra starts; a miss re-imports `sonic.exe`, which takes minutes. Use the runner for a
  batch of new questions, the CLI for one question or a cached one, and `doctor` before blaming
  the provider. Set `REA_GHIDRA_SEED_FILE` to the generated seeds, or the snapshot profile will
  not match.
* **Names enter Ghidra only through the seed file** (`address<TAB>function|code|label<TAB>name`).
  The seed digest is part of the analysis profile, so a new seed set means one fresh,
  unattended re-analysis. Keep the old snapshot under a backup name. Live renames
  (`set_addresses_names`, `annotate_native_function`) do not persist and block the snapshot
  save.
* **Types cannot be pushed into Ghidra through REA.** Struct layouts live in project headers
  and are checked by oracles and by `static_assert`s in adapters.
* **Limits:** one serial bridge queue; p-code capped at 3,000 ops per function;
  `trace_native_values` is static def-use, not emulation; computed calls stay unresolved; no
  compile-to-source, struct inference or emulation for PE. Runtime facts come from the guest.
* **Evidence ids:** every result carries `evidence_id`. Cite it in the handoff and in
  `overrides.tsv` decisions.
* **Health:** `rea doctor --provider ghidra` with the environment's `GHIDRA_INSTALL_DIR`
  (`reference/tools/ghidra_12.1.4_PUBLIC`) and `JAVA_HOME` (Homebrew OpenJDK 27) was green on
  2026-10-13. Raw results: `reference/inventory-raw/` and `reference/ghidra-game-loop-recovery/`
  (ignored). Tracked products: `reference/inventory/{functions,hot-paths,fpu-sites}.json`.

### 4.3 What comparable projects teach

Evidence and links: [`RESEARCH-native-recompilation.md`](RESEARCH-native-recompilation.md)
(three passes). Every project that reached *readable, binary-free* native code got there by
decompilation with names, behind a judge. The static recompilers that ship still need the
original binary and keep `sub_<hex>` code.

| Project | What SADX takes from it |
|---|---|
| re3 / gta-reversed | the hybrid phase: named C++, addresses bound once in a hook table; `NOTSA` tags; reversible bug-fix registry; an AI tips file |
| OpenRCT2 / OpenLoco | state migration is the long pole and peaks before it falls; a dedicated integration phase; library/app split for headless tests; freeze the differential evidence before the original leaves |
| isledecomp / reccmp / isle-portable | `// FUNCTION: MODULE 0xADDR name` annotations with a linter; platform replacements tagged at their sites; macOS arm64 port of the same source |
| sm64-port, Ship of Harkinian, Torch, libultraship | one-time asset import driven by committed descriptors; `should(VanillaBehavior…)` hooks keep vanilla the default; resource overrides by id |
| TRX | faithful gameplay, platform rewritten first; never hard-code game-specific options |
| SM64DS decomp | readability measured as its own number, gated by the same judge |
| momo5502, Snowboard Kids 2, Mizuchi, LLM4Decompile, SK2Decompile, arXiv 2609.05370 | tamper-proof objective judge; invent nothing; one function per fresh context; a cap on attempts; skeleton first, then names; an adversarial reviewer *with* evidence |
| SA1 Dreamcast decomp (new) | equivalence-first with dev/holdout case splits; a future name cross-reference for shared Ninja/game code |

### 4.4 The symbol database (ADR-0006, implemented)

`make symbols` → `build/symbols/{symbols,conflicts,coverage}.json` and `rea-seeds.tsv`.
Sources, in order of preference: the mod loader's `*New.h` headers (original symbols), the
classic headers, sa_tools labels (`reference/sa-tools` at `b72d52f3`), Ghidra boundaries, and
the tracked decisions in `reference/symbols/overrides.tsv`. Coverage on 2026-10-15: 2,166
named procedures out of 8,875 (8,716 nonexternal); 215 of 418 legacy owner addresses named;
2,823 conflicts listed, never silently resolved. A name is a hypothesis until the recovered
behaviour agrees. The mod loader headers are GPL: use names and layouts as facts and write our
own declarations.

## 5. Where things stand (2026-10-15, gate 140 + screen-init bridge)

`docs/STATUS.md` is authoritative. In outline:

* **M0 complete. M1 nearly complete:** guest boot, frames, determinism, profile-applied,
  fpu-shims and guest-fps are green. Override-equivalence is green for the bound owners and
  open for the rest (§6 item 8).
* **The guest** boots `sonic.exe` with the real loader, model DLLs, Input and Speedrun mods
  and SDL2. It runs 7,228 deterministic frames from the SEGA logo through FMV and title into the
  attract demo, at a host 78–82 FPS and the 63 FPS virtual title cadence.
* **The app** draws every guest frame with Metal and reaches "Select a Saved File". The
  screenshot was checked by eye, not against a golden.
* **Oracles** (104): 48 pass, 46 partial (values exact, FP flags differ under Unicorn), 9 fail
  (Unicorn transcendental/SNaN limits; settling them needs an Intel capture, M4), 1 missing.
* **Native path:** the frame-control pilot builds and runs from maintained source with no guest,
  PE loader or original binary (`make native-frame-control`, 2,672 frozen cases). Native
  boot/title, the importer and gameplay are open.
* **Readability baseline** (see §2): about 7,700 hex literals, 1,950 `_at_` uses, 249
  callee-address calls, 73 `goto`, and 16 annotations in the game layers. The worst files are
  `src/game/game_mode.cpp`, `src/runtime/game_runtime.cpp`, `src/game/game_mode_leaves.cpp`,
  `src/player/player_mesh_matrix.cpp` and `src/player/sonic_player.cpp`. Generated tables such
  as `src/player/sonic_animation_operations.hpp` (1,027 address-keyed rows) are reference
  evidence that must become named data or imported content.

## 6. What is next, in order

Pick one bounded step. Definitions of done are the ROADMAP tables and the judges in §3. Track R
comes first, because every later port should copy a readable model, not the legacy shape.

1. **Symbol database (stage A): done 2026-10-15.** `reference/symbols/coverage.json`,
   `rea-verification.json`. Licensing of upstream sa_tools labels is unresolved
   (`docs/SOURCES.md`); generated outputs stay ignored.
2. **Dependency inventory, provenance checker, frame-control pilot (stage B): done
   2026-10-15** (gates 139–140). `docs/NATIVE-INDEPENDENCE.md`; `check_provenance.py` with
   `reference/symbols/legacy-identifiers.json`; 11 named GLOBAL owners; `make
   native-frame-control` with 2,672 frozen cases.
3. **Readability ratchet and tooling (R).**
   Formatting, the lexical ratchet, fixture/mutant tests and STATUS metrics landed in the
   2026-10-15 readability step (see HANDOFF). The separate provenance extension accepts
   STUB reasons and ordinary member functions. Lookup/context-bundle tools now join symbols,
   source candidates, the direct-call graph, profile overlaps and cached evidence (METHOD §0).
   Missing function extents remain explicit and require REA evidence. Next: item 4.
   The requirements below document this item's scope.
   * Add `.clang-format` (LLVM base, 4 spaces, 100 columns). Apply it to new and migrated files
     only.
   * Add `tools/gate/check_readability.py` to `make check`. In game-tier files it counts hex
     literals not inside a provenance annotation, `*_at_<hex>` identifiers, callee-address call
     patterns, `goto` and address-named labels, and lines over 120 characters or with several
     statements. It compares the counts against a tracked per-file baseline
     (`reference/readability-baseline.json`) that may only go down. New files must be zero.
     Reference-tier paths (`src/guest`, `src/hle`, `tests/probes`, `tools/`) are exempt.
     Include fixture tests and a mutant: a hex literal added to a clean file must fail. The
     120-character limit is deliberately looser than `.clang-format`'s 100, so legacy
     multi-statement lines are caught without flagging formatted code.
   * Extend `check_provenance.py` as a separate `fix(judge)` change: accept ADR-0006's
     `// STUB: … <reason>` text, and decide whether seam members inside structs are
     allowed. Ordinary member functions are allowed; callable fields are rejected.
   * Teach `gen_status.py` to print the totals as the "readable" metric.
   * Add `tools/symbols/lookup.py ADDR…` and `tools/symbols/context_bundle.py ADDR` (names,
     signatures, callees and callers with names, native status from annotations and override
     tables, profile-patch overlap, and cached inventory-raw files). The snippets in METHOD §0
     are the starting point.
4. **Tamper-evident judge (R).** Record SHA-256s of the judge in a tracked
   `reference/judge-manifest.json`: `tools/oracles/**`, `tools/gate/**` (minus generated
   files), `tests/probes/**`, frozen corpora, and `reference/oracle-reports/**` summaries. The
   gate fails when the manifest is stale. Updating it is a separate `fix(judge): …` commit with
   evidence, never mixed with a port. Propose a project `.claude/settings.json` PreToolUse hook
   that refuses Edit/Write on those paths unless the step is a declared judge change. Settings
   changes need the user's approval, so ask; do not apply it silently.
   Manifest/checker workflow: [JUDGE.md](JUDGE.md). The prepared hook settings remain an
   inactive proposal until approved. Report-summary refreshes use a separate judge commit
   after the verified game-source commit, as that workflow specifies.
   Manifest and checker delivered at gate 144 (406 files, 42 Python tests). Hook protocol
   fixtures pass; project settings and live activation await the user's approval.
5. **Control-word census (R, decides STYLE §7).** Record `FPCW` (and MXCSR) at the entry of
   every bound override across the 7,228-frame run, the controller replay and any available
   stage replay, and also at the ported player-physics entry points (add temporary trace hooks
   in the reference tier). Write `reference/fp-control-census.json` (address, call count,
   distinct control words). Plain-float game-tier arithmetic is allowed only for call sites
   with a single `0x007f`. Report the sites that differ.
   Census of available journeys (gate 145): `reference/fp-control-census.json`, 212 instrumented addresses
   (52 actual bindings plus a conservative 164 player/dependency targets, with overlap).
   Original and fully bound native intro/controller digests match their untraced references.
   Reached entries show `0x023f` / MXCSR `0x1f80`; ground acceleration remains unobserved,
   and no stage replay is registered. This does not establish the plain-float premise.
6. **Pilot refactors under the judge (R), one per step, each with an independent STYLE review.**
   Preparation: the legacy mode/leaf interpreters and their PE-backed boot composition are
   relocated to `src/guest` so the clean mode interface can be added without upward layer
   dependencies. The semantic game-mode rewrite now follows this preparation.
   (a) **Game mode 40c090** → `GameMode` enum from `MD_GameMode`, `STUB` seams (plain function declarations) named
   from the symbol database, structured control flow, and the adapter holding every callee
   address. **Done at gate 147:** native plain-state core plus reference adapter; unchanged
   `game_mode` pass (24,000 cases/252 paths), three preserved mutants caught, 2,048 holdout
   cases exact, native-only test and independent STYLE review. Missing native callees remain
   explicit STUB stops; this does not complete native boot/title. STYLE §11 now quotes real code.
   (b) **Ground acceleration 44c270 `PGetAcceleration`** → **done at gate 149**: native
   plain-field core and shared friction use `fidelity::ExactFloat` with named literal defaults;
   reference adapters hold numeric boundaries, constant decoding and fixture unknowns.
   Ground/default alternate/explicit attribute20000 each retain 29,952 exact value/call/CW
   comparisons and their existing flag-only partial verdicts. The 3,840-case holdout, six
   mutations, callback/scratch/store-prefix tests, native-only closure and independent STYLE
   review pass. All 24 literal encodings match PE bytes. Native mutation/yaw composition and
   live stage precision remain open; no plain-float or full-native-movement claim.
   (c) **Frame control** → **done at gate 150**: plain native values and typed callbacks;
   fixture optionals, `CounterWords` and numeric diagnostics live in the reference adapter.
   Native and portable adapter builds use separate isolated closures; all prior missing-fact
   assertions and 2,672 frozen comparisons remain intact. Limiter 1,799 comparisons/one
   existing exclusion exact, holdout 2,048/zero, all six preserved mutations caught; independent
   STYLE review and full gate pass. Defaults come from PE virtual-zero tails and two literal
   settings 1/3. Eight coupled legacy CPPs moved unchanged into reference; their semantic
   rewrites remain in the queue. STYLE §11 quotes actual reset code. No physical timing,
   native boot/title or full-game independence claim. Next: item 7.
7. **Refactor queue (R), one module per step, worst first:** `game_runtime` (legacy composition now in `guest/native_game_reference`; EXE-read
   initialisation → literals, typed tables, or importer seams), `game_mode_leaves`,
   `game_reset*`, `game_loop`, `game_startup`/`winmain`, the `engine/ninja_*` modules, then
   player modules in census order. Each step updates the baseline. Expect the address surface to
   rise briefly when adapters split out (OpenLoco), and report it honestly.
8. **M1 override-equivalence (V/S), with every new binding written in two-tier form.** Ported
   owners still unbound (reconciled at gate 130; 22 after gate 137): 401000 401830 401a20
   401ac0 40bf40 40c030 40c090 410510 4208e0 4250d0 77f0a0 77f150 77f610 780c30 781070 785190
   789bd0 791610 791bd0 791df0 792100 793e70. 780c30 has a bridge from the screen-init step.
   Rerun its full gate first, because the last attempt was interrupted at the user's request.
   The render-queue consumer 4086f0 needs its other record-type bodies ported; 4073b0 has no
   port yet.
   * Every new entry needs a `<slug>.cases.bin` corpus in `OVERRIDE_CORPORA`, replay through the
     real HLE owners, source-return continuations with their exact reserve, bridge and core
     mutants, an unchanged live boot digest, and unchanged 900-frame controller parity.
   * Bridge state notes: `docs/NEXT-AGENT-PROMPT.md` item 1 and the gate 119–140 handoff
     entries.
9. **M1 close-out:** register or retire the four unregistered tests (metal-sprite,
   metal-sprite-composition, metal-texture, player-task); rerun `make -j6 oracles` +
   `make oracles-verdict`; record the M1 gate with `make m1` in its command; mark M1 complete.
10. **Importer first slice (I, stage E).** sa_tools is pinned at `reference/sa-tools`. Write
    committed descriptors for Emerald Coast act 1 (landtable, models, texlist, death zones).
    Emit a typed, hashed pack with resolved references under an ignored directory.
    * Tests: every record re-serialises to its source bytes; every `pointer=` site resolves; an
      unknown exe hash fails with a tagged error.
    * Then make the Metal stage path read the pack instead of `tools/lib/extract.py`'s mesh.
    * Reimplement the formats; do not link the C# code. SA3D.Modeling is a format reference.
11. **Native session to the title screen (I).** Make the game tier a library that links without
    the app, guest or PE loader. Compose the recovered boot and frame systems with the engine
    device interface and the Metal backend. Replace each EXE read in
    `NativeGame::initialize_source_facts` with literals, typed tables or pack resources. Fill
    the remaining seams with `STUB`s that fail loudly, and list them. This is the first
    "native-only" journey beyond frame control.
12. **M2 `golden-frames`, `app-smoke` to 300 frames, `state-dump-ui`** (V). Goldens are game
    imagery: track only their hashes, or generate them from the local install in the test.
    Unify `metal_guest_frame` and `metal_sprite` behind one device backend.
13. **M3 input** (physical Xbox/Switch 2 Pro pads, mapping tests against the Input mod tables,
    deadzone policy, vibration), **M3 audio** (DirectSound → CoreAudio; WMA decision as an
    ADR), **M3 replays** (Emerald Coast, Egg Hornet, speedrun-profile assertions).
14. **M4:** the Windows collector for trace parity, reference screenshots, D3DCAPS8,
    HeapAlloc, CTRLFLAG0, and Intel transcendental/RCP tables, which settle the nine failing
    oracles.
15. **M5 full independence (S/I):** recover complete systems in game-tier form (player
    common, then Sonic, camera, collision, objects of the first stages, UI flow) with their
    content. Implement the pinned Input/Speedrun behaviour natively under `game/profile`.
    Freeze reference corpora per system. Separate the production and reference build graphs.
    Readability ratchet at zero outside the reference tier.
16. **M6 enhancements (E):** a hook registry (events plus `should(VanillaBehavior…)`), typed
    settings, the fix registry (default off), the all-off parity test, then the first
    replay-neutral enhancements: a practice overlay (speed, position, timer, input display).
    Then the gameplay-affecting practice tools that speedrun practice wants (savestates beyond
    the Speedrun mod's quick save, segment practice), clearly labelled non-faithful.

## 7. The loop for one `/goal` step

1. **Name the item** (from `$ARGUMENTS` or §6) and its track. Read the last three HANDOFF
   entries and reread `docs/STYLE.md`. Work on one function, tight cluster or module per step,
   in a fresh context. Long sessions lose their instructions.
2. **Context bundle before code** (METHOD §0, §3):
   * names, signatures, structs and enums for every address (symbol database, then headers);
   * callers and callees with names, and which are native;
   * named pseudocode (`batch_decompile` for the cluster, `analyze_function` for the ports) and
     `xrefs` for every global;
   * assembly and exact bytes (`ghidra_asm.py`, `pe_image`);
   * profile status (0 UNHANDLED);
   * the nearest module already in game-tier form as the style model.

   Pseudocode is evidence, not source.
3. **Implement in two tiers** (STYLE): game-tier code in the system module (semantic names,
   annotations, structured flow, literals, readable exact arithmetic, `STUB` seams), and the
   reference adapter holding every address, marshalling step and fixture unknown. Invent
   nothing: every field, callee and guard has evidence. A transliterated draft may exist only
   inside the step and is refactored under the same oracle before the commit.
4. **Verify** at the roadmap's level:
   * unit test (+ manifest row);
   * oracle + `verdict.py` + 3–7 mutants for ports, with dev cases while iterating and a
     holdout set (a different seed or range, written before the final run) run once at the
     end, since a port counts only when both pass;
   * corpus replay + bridge/core mutants + live digests for overrides;
   * goldens/replays for M2/M3.

   Break the code once and watch the test fail. Never change a tolerance, expected value,
   fixture, mutant or harness to pass. After three failed approaches, stop, record the unknown
   with its evidence in HANDOFF and `docs/TIPS.md`, and move on. A recovered system or a pilot
   refactor gets an independent review by a fresh subagent *with the evidence and the oracle
   results*, against the STYLE §12 checklist. A reviewer without objective criteria approves
   wrong code.
5. **Gate:** `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?` (it opens a window for
   app-smoke and needs a logged-in GUI session). On rc 0 only: `python3
   tools/gate/record_gate.py --log build/gate.log --command "make -j10 gate" --exit-status 0
   --limit "…" --output reference/production-integration-<date>/<name>-gate.json`, then `python3
   tools/gate/gen_status.py`. If ported code changed: `make oracle-<slug>` (or `make -j6
   oracles`), `make oracles-verdict`, `python3 tools/gate/track_reports.py <slug>…`.
6. **Journal:** append a short HANDOFF entry using the template in its header: item and track,
   evidence ids, what changed, verdicts with counts, readability delta, what was not run, and
   next. Commit with a Conventional Commit subject, no attribution lines, no push.
7. **Report** to the user: outcome first, gate name and rc, what is unverified, the next item.

Pitfalls that cost time are in AGENTS.md and `docs/TIPS.md`. Performance notes (no
instruction count to Unicorn, `reserve_hook_region` for dense traps, translation flush on code
writes, `timeout 0` for long runs) are in `docs/NEXT-AGENT-PROMPT.md`.

## 8. Inputs on this Mac (all git-ignored)

| Path | What |
|---|---|
| `game/Sonic Adventure DX/` | the working install (sonic.exe `6b0937ca…`, system/, mods/ with loader + Input/Frame Limit/CCEF/EHCF) |
| `game/pristine/sonic.exe` | the untouched speedrun-page download, same hash |
| `game/disc/sonic-adventure-dx-directors-cut_202202/` | retail disc BIN/CUE images (Disc 1 exe differs by the CD-check byte; Disc 2 = SoundData) |
| `reference/updated-pc-reference/game/` | the pinned profile install: the guest's install directory |
| `reference/windows-system/` | 32-bit msvcp140/vcruntime140 14.51.36247.0 the guest runs as real code (`PROVENANCE.md`) |
| `reference/sadx-mod-loader/` | mod loader source at eb4cdcca (submodule); `SADXModLoader/include/*New.h` + classic headers = the symbol source |
| `reference/sa-tools/` | X-Hax sa_tools at `b72d52f3`: `GameConfig/PC_SADX/*.ini` data mappings and `sonic_labels.txt` |
| `reference/python/` | venv with Unicorn 2.1.4 and capstone 5.0.7 |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra for REA; JDK = Homebrew OpenJDK 27 |
| `reference/tools/rea/node_modules/rea-agents/` | installed REA 6.4.0; `$SADX_REA_MODULES` overrides; `/private/tmp/sadx-rea/node_modules` legacy fallback |
| `~/Developer/rea/` | REA source checkout (docs in `docs/`), not built |
| `reference/inventory-raw/`, `reference/ghidra-game-loop-recovery/`, `reference/sdl-controller-recovery/` | raw Ghidra results (tracked product: `reference/inventory/`) |
| `reference/windows-transfer/` | the Windows capture collector for M4 |

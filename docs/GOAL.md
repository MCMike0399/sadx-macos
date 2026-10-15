# The goal, for `/goal`

This file is the standing brief an agent session loads when the user types `/goal` in
`~/Developer/sadx-macos` (`.claude/commands/goal.md` reads it, then `docs/STATUS.md`). It
replaces the per-step `docs/NEXT-AGENT-PROMPT.md` as the entry point: that file still holds the
detailed state notes of the last M1 steps and is linked from section 6. The destination was
clarified by the user on 2026-10-15 (ADR-0005); a second pass the same day added the symbol
database, provenance annotations, the asset importer plan and the lessons of comparable
projects (ADR-0006, §4.3–4.5, `docs/RESEARCH-native-recompilation.md`). Historical measurements
below are labelled; current numbers and completed gates live in `docs/STATUS.md`.

## 0. The prompt (what `/goal` injects)

```
You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS), built by
exact reverse engineering and proven by automated tests. Metal for graphics, GameController for
Xbox and Switch 2 Pro pads, fidelity to the Windows reference over convenience. The finished
game builds from maintained source and runs from imported assets without sonic.exe, original
Windows/mod DLL execution or an x86 runtime. The hybrid app is the development reference.
New and moved code uses semantic names from the symbol database with the source address in a
provenance annotation, never in an identifier (ADR-0006).

Read, in order: AGENTS.md, docs/GOAL.md (this brief: history, how REA is used, the next items),
docs/STATUS.md (generated from the last gate), docs/ROADMAP.md (the milestone whose table is not yet
green is the current one), docs/ARCHITECTURE.md, docs/METHOD.md, docs/decisions/ADR-0001..0006, and
the last three entries of docs/HANDOFF.md. Then do ONE step of the loop in docs/GOAL.md §7: name the
item (the one the user passed, else the first open item of §6), analyse with REA's Ghidra through the
owned runner, implement in the right layer, verify at the level the roadmap asks, run the gate,
record it only on rc 0, regenerate STATUS, append a HANDOFF entry, commit (no push, no attribution).
Report honestly: unverified scope named, skipped tests listed, "partial" oracles never called pass,
every HLE fact the reference machine would have supplied kept as an assumption in
HleProcess::assumptions. Do not stop at a plan; a step ends with a recorded gate or an explicit
statement of what blocked it.
```

## 1. The goal, exactly

A native arm64 macOS application, built from readable, maintained source, that plays
*Sonic Adventure DX* with the pinned speedrun profile faithfully enough for speedrun practice,
proven by automated tests. The user supplies game assets from their own disc or installation;
the finished game does not require downloading or retaining `sonic.exe` to play.

There are three separate requirements:

* **Native execution:** all shipped game and pinned-profile code, including cold paths,
  compiles for arm64. Unicorn, x86 JIT/interpreter execution and original-code fallbacks
  belong only to the separate development reference.
* **Executable independence:** ordinary source builds use maintained source and native
  dependencies. Playing uses imported assets. Neither operation opens `sonic.exe` or
  executes original Windows/mod DLLs. One-time import may read embedded data from the
  user's own EXE/model DLL containers; it must produce independent data resources.
* **Maintainable source:** game systems expose meaningful types, operations and ownership.
  Developers change frame control, player physics, collision or speedrun features through
  those interfaces. Source addresses remain available as provenance and reference-adapter
  metadata rather than defining production storage and dispatch.

**What "readable" means, concretely** ([ADR-0006](decisions/ADR-0006-symbol-names-and-provenance-annotations.md)):
an address is a *comment*, never an *identifier*. Today a field reads
`std::optional<std::int32_t> frame_multiplier_at_389d7dc`; the destination reads

```cpp
// GLOBAL: SONIC 0x3b11188 FrameMultiplier
std::int32_t frame_multiplier = 1;
```

and a function reads `// FUNCTION: SONIC 0x411e30 dsInitInt` above
`void set_frame_rate_mode(FrameRateMode mode, int frame_increment)`. Names come from the
symbol database (§4.4: the mod loader's symbol-based headers first), types from recovered
struct layouts (`task`, `taskwk`, `playerwk` …), and a gate checker reads the annotations to
keep one owner per address. This is the isledecomp/OpenLoco convention (§4.3), and it is what
lets a developer change player physics without knowing where it lived in `sonic.exe`.

The decision is [ADR-0005](decisions/ADR-0005-native-source-and-executable-independence.md);
the acceptance tests are [M5](ROADMAP.md#m5--native-source-and-executable-independence).
The vocabulary is in [CONTEXT.md](../CONTEXT.md). Exact arithmetic and observable ordering
remain part of fidelity even when the source is reorganized. The existing one-owner checker
and address-named state remain in force until migrated with equivalent ownership/provenance
checks; a cosmetic rename is not a native architecture change.

The existing implementation is an intermediate reference, as these historical measurements show:

| Requirement | Where it is defined | State at gate 129 |
|---|---|---|
| All game code native; build and play independent of original executables | ADR-0005; M5 | not achieved: guest runs boot → SEGA logo → intro FMV → title → attract → file select; 40 native overrides |
| Metal rendering | M2 | `sadx-app --guest` draws the guest's D3D8 stream (clears, DrawPrimitiveUP, textures, blend, depth) |
| Keyboard, Xbox and Switch 2 Pro controllers | M3 `input-mapping`; the Input mod's SDL path | keyboard forwarded; a synthetic XInput/HID pad reaches the pinned SDL2; GameController adapter samples the first extended profile; no physical pad verified |
| Speedrun profile (mod loader, Input 2.6.1, Speedrun 2.0, 63 FPS) | `tools/gate/profile_patches.json`, M3 `speedrun-profile` | loader and both mods run as real code in the guest; 119/119 active patch sites applied; 63 FPS limiter cadence measured on the virtual clock |
| Fidelity to the Windows reference | AGENTS.md non-negotiables, M4 | every unknown platform fact is a reported assumption (23 in the native survey, ~40 in the HLE); no Windows capture yet |
| Proven by automated tests | ROADMAP milestone tables, `make gate`, `make m1` | 129 unit programs, 101 probes, 102 oracles (48 pass / 44 partial / 9 fail / 1 missing), 82 recorded gates |

## 2. Four days in numbers

| Day | Where | What happened | Gates |
|---|---|---|---|
| 2026-10-10/11 | `~/Developer/rea/_reference/sadx` + `rea/sadx-native` | port-everything: boot order ported function by function (WinMain settings, library/device/screen init, GameLoop startup, one full frame of owners), each with a Unicorn oracle; Hopper demo as the reader | `render` (10-11 20:14) … `endscene-98` (10-12 19:47) |
| 2026-10-12 | same, then `~/Developer/sadx-macos` | the user measured the pace (6 KB of 3.85 MB verified per day, a late-2027 playable) and asked for a review; ADR-0001 hybrid runtime, ADR-0002 Ghidra via REA, ADR-0003 repo move, ADR-0004 layers; oracle health audit; first guest smoke (7899a0 from original bytes equals the port) | 99–104 |
| 2026-10-13 (26 commits) | `sadx-macos` | M1 4b–4f: loader + mods + SDL2 initialise in the guest; boot to first Present; override table; 590 FPU shim sites; 300 → 7,228 deterministic frames; headless DirectShow FMV; title screen; `sadx-app` draws the guest; keyboard; timing; shared D3D caps; XInput + raw-input HID path for SDL; device/heap/task/queue bridges (40 overrides) | 105–129 |

Measured totals (gate 129): `src/` 295 files / 29,811 lines in 12 layers; `src/hle` 3,888 lines
written in one day; whole-program inventory 8,875 procedures with 8,623 callee lists; 618
profile patches evaluated; 208 distinct REA evidence ids cited in `docs/HANDOFF.md`.

## 3. How the agents worked, and what it taught

**What produced the jump on day 3 was the change of method, not more effort.** Two days of exact
per-function porting verified 0.15 % of `.text`. One day of hybrid runtime made the whole intro
and title run from the original bytes, with the ported code bound back in as overrides. This
made a useful reference sooner; it did not recover an independent native game. Exact ports
and reference adapters remain verification tools (`docs/METHOD.md` §3). Recovery now also
groups functions into coherent native systems and explicitly accounts for cold code and data.

**What cost time, and the rule that came out of it** (all recorded in HANDOFF entries):

* Hopper demo hangs under load, one function per query → ADR-0002, Ghidra only, batched.
* A per-instruction count passed to Unicorn instruments every instruction (30× slower) → wall-clock
  timeouts only, `timeout 0` for long runs.
* The gate did not run oracles, so 20 oracle scripts silently pointed at pre-move paths → `make
  oracles` + `make oracles-verdict` are part of closing a session; `partial` is reported as partial.
* Tests outside `tests/unit/manifest.tsv` do not run in the gate even with no arguments (four such
  tests still exist: metal-sprite, metal-sprite-composition, metal-texture, player-task) → every
  test gets a manifest row.
* An HLE function's stdcall pop count is load-bearing because continuations resume the guest at the
  caller: one wrong count (CreateWindowEx, 60 vs 48) returned into heap memory.
* Hosts writes into guest code must flush Unicorn's translation cache; an INT3-based FPU trap was
  rejected because the loader copies code bytes into trampolines.
* Gates were recorded only with their real rc; an interrupted gate (rc 130) stays in the handoff
  as interrupted; the first gate after adding a test was rerun when the test was found unregistered.

**Honesty conventions that held across sessions:** assumption lists printed by the runtime,
tagged failures with the source boundary, "unverified" scope named in every handoff entry, mutants
for every new oracle or bridge, and the oracle verdict vocabulary pass / partial / fail.

## 4. How REA has been used, and how to use it from here

### 4.1 What exists

* **The owned runner** `reference/ghidra-live.mjs` is an MCP stdio client that starts `rea mcp`,
  opens `sonic.exe` (or `SDL2.dll`, `reference/ghidra-sdl-config.json`) with REA's Ghidra provider
  once, replays requests from a JSON file and saves a snapshot on close. Allowed tools:
  `analyze_function`, `inspect_native_instruction`, `resolve_native_call_targets`,
  `trace_native_values`, `xrefs`, `binary_session`, `list_procedures`, `procedure_callers`,
  `procedure_callees`, `procedure_references`, `procedure_info`, `batch_decompile`, plus `export`
  (evidence bundle). Current config: `reference/ghidra-inventory-config.json` (snapshot
  `reference/sonic-ghidra-inventory-snapshot.json`, seeds
  `build/symbols/rea-seeds.tsv` (`make symbols`); the original 64 callback/game-loop seeds
  are preserved from `reference/ghidra-game-loop-seeds.tsv`).
  The former full-cache snapshot is preserved locally as
  `reference/sonic-ghidra-inventory-before-frame-input-seeds.json`; adding seeds
  changed the analysis profile, so the current snapshot was rebuilt without reusing it. The older
  `ghidra-game-loop-config.json` snapshot is tied to the pre-move binary path; do not use it.
* **Results on disk:** `reference/ghidra-game-loop-recovery/` (314 files: preloop, frame, wm, dbg,
  startup, proj, queue, fog …) and `reference/inventory-raw/` (8,717 files: the callee sweep plus
  the `bridge-*`, `caps-*`, `timing-*`, `fmv-*` analyses of the M1 sessions). Both are git-ignored;
  the tracked products are `reference/inventory/{functions,hot-paths,fpu-sites}.json`.
* **Usage so far** (from the handoff): `analyze_function` for every ported or bridged function,
  `list_procedures` once (8,875 procedures), `procedure_callees` for 8,623 of them (35 minutes
  unattended through `tools/inventory/run_inventory.py`), `xrefs` for data references,
  `trace_native_values` once. `batch_decompile` and `procedure_callers` are allowed but almost
  unused. Pseudocode is written next to each result as `<label>.pseudocode.txt`;
  `tools/analysis/ghidra_asm.py <result.json>` prints the assembly.
* **REA itself:** the runner imports `rea-agents` 6.4.0 from `$SADX_REA_MODULES`, otherwise
  `reference/tools/rea/node_modules`, with `/private/tmp/sadx-rea/node_modules` as a legacy
  fallback (`docs/METHOD.md` §1). `~/Developer/rea` is the source checkout inspected for this
  goal revision. Do not assume that source HEAD and the installed package are identical.
* **Ghidra readiness is green:** `rea doctor --provider ghidra` with the environment file's
  `GHIDRA_INSTALL_DIR` (`reference/tools/ghidra_12.1.4_PUBLIC`) and `JAVA_HOME` (Homebrew OpenJDK 27)
  reports every check healthy (verified 2026-10-13).

### 4.2 The REA CLI, and what to do with it

`rea` (`node reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs`) exposes the same
workflows as the MCP server, one process per call, with `--snapshot` reuse:

```sh
export GHIDRA_INSTALL_DIR="$PWD/reference/tools/ghidra_12.1.4_PUBLIC"
export JAVA_HOME=/opt/homebrew/Cellar/openjdk/27/libexec/openjdk.jdk/Contents/Home
make symbols
export REA_GHIDRA_STARTUP_TIMEOUT_MS=600000 REA_GHIDRA_SEED_FILE="$PWD/build/symbols/rea-seeds.tsv"
REA="$PWD/reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs"
EXE="game/Sonic Adventure DX/sonic.exe"; SNAP="$PWD/reference/sonic-ghidra-inventory-snapshot.json"
node $REA doctor --provider ghidra --json                      # readiness, no target
node $REA function  "$EXE" 0x40c090 --provider ghidra --snapshot "$SNAP" --json   # one dossier
node $REA decompile "$EXE" 0x40c090 --provider ghidra --snapshot "$SNAP" --json   # pseudocode only
node $REA xrefs     "$EXE" 0x3b11188 --provider ghidra --snapshot "$SNAP" --json  # who touches a global
node $REA instructions "$EXE" 0x781060 --provider ghidra --snapshot "$SNAP" --json
```

Rules of thumb, from REA's own docs (`~/Developer/rea/docs/cli.md`, `ghidra-provider.md`):

* An exact snapshot hit is answered before Ghidra starts; a miss imports the binary again (one
  import of `sonic.exe` is minutes). So: **the runner for a batch of new questions, the CLI for a
  single question or a cached one, and `doctor` before blaming the provider.**
* Functions reached only through pointers (task callbacks, vtables, the D3DX dispatch table) are
  not auto-defined: add them to the seed TSV (`address<TAB>function[<TAB>name]`) and keep growing it.
* Every result carries `evidence_id`, limitations and the target SHA-256. Cite the id in the
  handoff entry (the convention is already followed: 208 ids so far).
* `batch_decompile {addresses[]}` and `procedure_callers` are cheaper than one `analyze_function`
  per function when only pseudocode or the caller set is needed; use them for surveys.
  `batch_decompile` has no CLI form and returns `items[].pseudocode`; the runner writes
  `.pseudocode.txt` only for single results, so split the items yourself (a small loop).
* **Names enter Ghidra through the seed file, durably.** The seed TSV accepts
  `address<TAB>function|code|label<TAB>name`; a named function seed renames the function, a
  label seed names a global (also in `.bss`). Seeded names survive into snapshots, so the
  pseudocode of every later query arrives named (`FrameMultiplier` instead of `DAT_03b11188`).
  The seed file's digest is part of the analysis profile: a new seed set means one fresh,
  unattended re-analysis and a new snapshot (keep the old one under a backup name).
* **Live renames do not persist.** `set_addresses_names` and `annotate_native_function` work on
  Ghidra but only for the session, and after any edit `close_binary` refuses to save a snapshot
  (export the evidence bundle, then close without `snapshot_path`). The runner does not allow
  them today. Prefer the seed file.
* **Types cannot be pushed into Ghidra through REA** (no struct/signature/header import;
  `inspect_native_data_type` only reads). Struct layouts therefore live in the project's own
  headers and are checked by oracles and `static_assert`s on offsets, not by Ghidra.
* Read-only tools worth adding to the runner's `allowed` set when a step needs them (one line
  each): `list_names`, `list_strings`, `search_strings`, `procedure_pseudo_code`,
  `get_call_graph`, `trace_call_path`, `inspect_native_api` (jump tables / switch cases),
  `inspect_native_data_type`, `read_function_instructions`, `inspect_analysis_view` (page one
  facet of a stored dossier by evidence id instead of re-running it), `compare_functions`.
  The unknowns ledger (`record_unknown`, `list_unknowns`, `verify_unknown_resolution`) fits the
  "explicit unknowns" rule if a step wants REA-side bookkeeping.
* Limits that matter: one serial bridge queue (a "batch" is serial inside Ghidra); p-code capped
  at 3,000 ops / 12,000 def-use edges per function; `trace_native_values` is static def-use, not
  emulation; computed calls stay unresolved; REA has **no** compile-to-source, struct-inference
  or emulation feature for PE targets. Runtime facts come from the project's guest.

### 4.3 What comparable projects teach

The [source investigation](RESEARCH-native-recompilation.md) has the evidence and links. The
short version: every project that reached *readable, binary-free* native code got there by
decompilation with names, not by static recompilation; every static recompiler that ships
(LASRecomp, N64Recomp/Zelda64Recomp, XenonRecomp/UnleashedRecompiled) still needs the
original binary to build or run and names its game code `f_<hex>`/`sub_<hex>`.

| Project | Path it took | What SADX takes from it |
|---|---|---|
| LASRecomp | static lift of the NSO to C at first launch, `f_<hex>` functions, `Cpu*` registers, original modules still loaded | separate platform/game code; artifact identity checks. Its generated code is the shape ADR-0005 rejects |
| pokeheartgold | LLM-agent matching decomp, batch per address range, journal per batch (`DECOMP-NOTES.md`), whole-ROM SHA-1 as the judge | batch journal (our HANDOFF); "a source-language percentage is not binary verification" |
| re3 / gta-reversed | DLL injected into the exe replaces functions one by one (`RH_ScopedInstall(Name, 0xADDR)`), per-function reversed flag, later standalone exe | this is exactly our hybrid phase; hook registration lives in one table, game code is named C++ |
| OpenRCT2 / OpenLoco | hooks into the original exe, then all functions native; OpenLoco then moved globals off fixed addresses (`loco_global<T,addr>` 342 → 157 → 0) before native 64-bit | **state migration is the long pole**: track the count of address-bound globals as a metric. Address survives only as `// 0x…` comments (3,047 in OpenLoco) |
| isledecomp + reccmp (LEGO Island) → isle-portable | `// FUNCTION: LEGO1 0x100b12c0` above a named function; `GLOBAL`, `VTABLE`, `STUB`, `LIBRARY` annotations; a linter checks them; portable fork runs on macOS arm64 | **our annotation scheme** (ADR-0006) and its checker |
| OpenRCT2 TestPaint | differential harness against the original paint code, later removed because it rotted | **freeze the reference** into recorded corpora before the exe leaves the loop (stage F) |
| sm64-port, Ship of Harkinian, OoT decomp | assets described by offset/name/type (OoT: 763 XML files), extracted once from the user's ROM into a native archive (`.o2r`) | the one-time importer (stage E), driven by sa_tools' SADX data mappings |
| LLM decomp case studies (Rock Band 3, Melee, Klonoa, Snowboard Kids 2; LLM4Decompile) | one function or cluster per task, names/types from prior work in context, fast unforgeable equivalence check, independent review agent, attempt caps; agents game loose metrics | the agent loop rules in §7 and AGENTS.md; our judge is differential execution (oracles, override-equivalence, replay), not byte matching |

### 4.4 The symbol database and provenance annotations (ADR-0006)

**The project already has most of the names and it has not used them.** The mod loader
submodule (`reference/sadx-mod-loader/SADXModLoader/include/`, pinned at eb4cdcca) carries:

* `SADXFunctionsNew.h`, `SADXVariablesNew.h`, `SADXStructsNew.h`, `SADXEnumsNew.h`, headed
  "based on symbols": original-style names (`dsInitInt` 411e30, `stHWSetVSync` 7899a0,
  `texLoadTexturePvmFile` 421180, `now_saving` 3b291a4, `task`/`taskwk`/`motionwk`/`playerwk`).
* `SADXFunctions.h`, `SADXVariables.h`, `SADXStructs.h`, `SADXEnums.h`: the older community
  names (`SetFrameRateMode`, `FrameMultiplier`, `GameModeHandler` 40c090, `LoadPVM`).
* Together roughly 3,800 distinct function addresses (of 8,875 Ghidra procedures), 3,150
  globals and 470 structs, as `FunctionPointer(ret, name, (args), 0xADDR)`,
  `DataPointer(type, name, 0xADDR)`, `DataArray(...)`, `TaskFunc(name, 0xADDR)` macros.

Outside the repo, X-Hax **sa_tools** (`GameConfig/PC_SADX/`, 140 files) maps 13,160 data items
embedded in `sonic.exe` and the model DLLs: 9,416 basic models, 1,680 animations, 480 chunk
models, 456 texlists, 62 landtables, 57 death zones, sound lists, object lists, physics data,
each with `address=`, `type=`, output filename and `pointer=` (the code sites that reference
it); `sonic_labels.txt` adds ~243,000 data labels. These are the schema for the importer
(stage E) and a second source of data names. Fetch it at a pinned commit into an ignored
path when stage A or E starts; reimplement the formats, do not link the C# code.

Rules (full text in ADR-0006):

1. A generated **symbol database** (`tools/symbols/`, stage A) merges, with a source tag per
   entry: the `New` headers (preferred), the classic headers, sa_tools labels, Ghidra
   boundaries from the inventory, and the project's own decisions
   (`reference/symbols/overrides.tsv`, tracked). Conflicts are listed, never silently resolved.
   Community names are hypotheses: a name is adopted when the evidence for the function's
   behaviour agrees with it.
2. The database emits the REA seed TSV, so Ghidra pseudocode arrives named.
3. **Code names are semantic, in the project's style** (`snake_case` functions/fields,
   `CamelCase` types); the original symbol is in the annotation. Recovered record types follow
   the symbol layout (`taskwk` → `TaskWork`) with `static_assert`ed offsets when guest memory
   is still marshalled.
4. **Provenance annotations** sit on the line above the declaration:
   `// FUNCTION: SONIC 0x411e30 dsInitInt`, `// GLOBAL: SONIC 0x3b11188 FrameMultiplier`,
   `// DATA: SONIC 0x88af78 <label>` (content the importer supplies), `// PATCH: SPEEDRUN
   +0x10f0 → 7899e0` (pinned-profile behaviour, module + offset), `// STUB: SONIC 0x… <reason>`
   (declared but not recovered), `// LIBRARY: SONIC 0x… _ftol` (CRT/D3DX). Module tags:
   `SONIC`, `CHRMODELS`, `ADV00`…, `LOADER`, `INPUT`, `SPEEDRUN`, `SDL2`.
5. A gate checker (`check_provenance.py`, stage B) enforces one `GLOBAL` owner per byte range
   and one `FUNCTION` per entry address, and validates names against the database. It replaces
   `check_global_owners.py` **only when** it covers everything that checker covers; until then
   both run, and existing `*_at_<hex>` owners stay valid. Migrate one system at a time and move
   the owner and its annotation together.
6. Address-keyed override registration stays in `src/guest`/`src/hle` reference adapters (one
   table, like gta-reversed's hook list); production game code never takes a guest address.

### 4.5 The recovery pipeline

REA's Java bridge returns Ghidra decompiler C text and an analysis dossier; neither is a
compiler or proof of behaviour. Treat truncated or unresolved results as open work. The
pipeline, each stage a series of `/goal` steps:

* **A. Symbol database** (§4.4): generated map + seeds + one fresh snapshot. Metric: named
  procedures / 8,875, named globals referenced by ported code.
* **B. Annotations and checker**: `check_provenance.py` in the gate; first migrate an
  already-verified module (frame control) from `*_at_<hex>` to named fields + annotations,
  proving the checker catches a duplicated owner (mutant).
* **C. System recovery** (the long pole): for one system at a time —
  1. gather callers, callees, data references, assembly and named pseudocode (runner,
     `batch_decompile` for the cluster); include profile hooks and callback targets; record
     evidence ids and unresolved edges;
  2. recover state meaning, record layouts, aliasing, initialisation and callback ownership;
     define the named native interface; keep binary marshalling in the reference adapter;
  3. classify every dependency: native behaviour, imported content, constant/initial state,
     platform service, or unresolved; resolve data pointers to resource identities;
  4. implement with direct native calls; prove it through the reference adapter (oracle +
     mutants, override-equivalence, replay), then from a native caller with the guest absent;
  5. report recovered behaviour and remaining dependencies. Metrics: address-bound globals
     remaining (OpenLoco's), functions with `FUNCTION` vs `STUB`, guest instructions per
     frame in the reference replays.
* **D. The agent loop for one function or cluster**: the context bundle (§7 step 2), one
  bounded attempt, the differential judge, an independent review for recovered systems.
* **E. Importer**: `tools/import/` (later a native `sadx-import` tool) reads the user's install
  or disc once, uses sa_tools mappings as the schema (address, type, `pointer=` sites) and
  emits typed, hashed packs (models, landtables, animations, texlists, physics, object/sound
  lists, strings, initial-state tables) with references resolved to ids. Tests: re-serialise
  each record and compare to the source bytes; every `pointer=` site resolves; an unknown
  variant fails explicitly. Packs stay local and ignored. `tools/lib/extract.py` is a
  bring-up artifact, not this importer.
* **F. Freeze the reference**: before a system's last guest dependency goes, record its oracle
  fixtures, replay end states and trace digests into `reference/` (numbers and hashes only,
  `check_repo_hygiene.py` green) so verification survives without `sonic.exe`. OpenRCT2's
  TestPaint rotted because this was not done.
* **G. Independence gate**: the M5 rows (`native-source-build`, `native-runtime-independent`, …).

A static recompiler (Ghidra CFG → C) may still help as a *temporary* speed path behind the
reference adapter, never as shipped logic; it would need its own instruction/flow/memory/FP
validation (ADR-0005). The hybrid runtime already gives what recompilation would give us
(the whole game running) so its value here is speed, not the goal.

## 5. Where things stand (gate 129, 2026-10-13 22:25)

`docs/STATUS.md` is authoritative; the shape is:

* **M0 complete.** **M1 nearly complete:** `make m1` runs guest-imports, guest-dlls, guest-boot,
  guest-overrides, hle-device/heap/task/queue-bridge, guest-fpu-shims, guest-frames, guest-input,
  guest-timing, guest-controller and app-smoke. Of the roadmap's M1 table, `guest-boot`,
  `guest-frames`, `determinism`, `profile-applied`, `fpu-shims` and `guest-fps` are green;
  `override-equivalence` is green for the 40 bound owners and open for the rest (§6).
* **The guest:** boots `sonic.exe` from entry 643f96 with the real loader, CHRMODELS_orig, model
  DLLs, Input and Speedrun mods, SDL2 (MSVCP140/VCRUNTIME140 as real code; everything else HLE in
  `src/hle`), runs 7,228 frames SEGA logo → FMV (headless DirectShow) → title and on into the
  attract demo deterministically; host 78–82 FPS, virtual 63 FPS title cadence.
* **The app:** `sadx-app --guest GAME WINSYS [--keys F:VK[:N],…] [--pad F:MASK[:N],…] [--exit-after N]
  [--screenshot out.ppm]` draws every frame with Metal and reaches "Select a Saved File"; the
  main-menu screenshot was inspected by eye only, not against a reviewed golden.
* **Oracles:** 48 pass, 44 partial (values exact, FP flags differ: Unicorn 2.1.4 raises no SSE flags
  and applies the x87 NaN rule to SSE), 9 fail (host-double FPATAN and SNaN loads in Unicorn, plus
  `sse_math` and `d3dx_perspective` classified), 1 missing (`game_reset_composed`, the probe
  rejects the composed fixture). Settling the fails needs an Intel capture (M4), not a port change.
* **Native-only runtime (`sadx-boot`)** still stops at frame 2 (LoadPVM 421180) and is now the
  override provider, not the main path.

## 6. What is next, in order

Select one bounded `/goal` step from this order. Definitions of done are the roadmap tables.
M1 is still the current incomplete milestone; the first two items deliberately pull M5
architecture work forward (stages A and B of §4.5). M1–M4 evidence remains useful, but
completion of more guest bridges is not a prerequisite for establishing the native path.

1. **Symbol database (§4.4, stage A) — implemented 2026-10-15.**
   See `reference/symbols/coverage.json` and `rea-verification.json`: 2,166 named
   procedures / 8,875 inventory entries (8,716 nonexternal); 222 / 429 legacy
   owner addresses named, 207 explicitly unnamed. Four fixture tests cover every
   supported macro, types, conflicts and seed preservation. `make check-symbols`
   runs in the gate. REA applies 3,758 function seeds and 248,999 labels (two
   unmapped), and the three requested decompilations expose preferred header names.
   Unknown ABI sizes remain null; upstream sa_tools label licensing is unresolved
   (`docs/SOURCES.md`); generated labels/database remain ignored. The specification
   below remains the reference for this delivered stage.

   **Specification:** `tools/symbols/build_symbol_db.py` parses the mod
   loader's `*New.h` and classic headers (the `FunctionPointer`/`VoidFunc`/`ThiscallFunctionPointer`/
   `FastcallFunctionPointer`/`StdcallFunctionPointer`/`UsercallFunc*`/`TaskFunc`/`DataPointer`/
   `DataArray` macros, struct and enum definitions) into a generated database under `build/`
   (address, module, kind, name, signature/type, size, source header, alternative names) plus a
   tracked `reference/symbols/overrides.tsv` for project decisions and a conflicts report. A unit
   test parses fixture lines of every macro form; a gate check asserts the database is
   regenerated from the pinned submodule. Emit a named seed TSV for REA (merge with the existing
   callback/game-loop seeds, never drop a seed), rebuild the snapshot once (backup the old one),
   and spot-check that `batch_decompile` of 411e30/7899a0/40c090 now prints `FrameMultiplier`
   and friends. Report coverage: named / 8,875 procedures; named globals among the addresses the
   current `*_at_<hex>` owners use (list the unnamed ones). No gameplay code changes in this step.
2. **Native independence inventory, then a frame-control pilot (stage B).** First track the currently
   observed EXE/DLL dependencies of `sadx-app`, `load_boot_config`,
   `NativeGame::initialize_source_facts`, `tools/lib/extract.py` and the build generators:
   code, assets, constants/initial state, callbacks and platform services. Give each a source,
   destination and verification method; leave unexamined dependencies explicit. Then take
   `game_frame_control` (mode/input callback and 63 FPS limiter) as the first bounded native
   slice: meaningful state/API, direct native callback, reference adapter to the existing
   oracle, and a deterministic host-driven test with original binaries and Unicorn absent.
   Add `tools/gate/check_provenance.py` (ADR-0006 annotations, one owner per byte, names
   validated against the symbol database; a duplicated-owner mutant must fail it) before moving
   existing address-owned state, and migrate the frame-control owners to named fields with
   `GLOBAL`/`FUNCTION`/`PATCH` annotations (`set_frame_rate_mode` ← 411e30 dsInitInt, the
   Speedrun 2.0 limiter as `PATCH: SPEEDRUN`). Names are decided from evidence: the headers call
   3b11188 `FrameMultiplier`/`vsync_const` and 389d7dc `g_CurrentFrame`/`DeltaTimeMultiplier`,
   while today's code calls 389d7dc `frame_multiplier_at_389d7dc`; settle such conflicts in
   `reference/symbols/overrides.tsv` with the evidence id, never by taste. The pilot
   must link without guest/HLE/PE-loader code, use the same core through both callers, and
   report that it proves frame control only, not gameplay or full asset independence.
   Subsequent steps extend native composition toward boot/title and a playable stage;
   reuse verified modules rather than maintaining a second gameplay implementation.
3. **M1 override-equivalence where it supports recovery or closes the existing milestone.**
   Reference adapters serve the native source, rather than becoming the production API.
   The handoff says 20 of the
   address-named list from the 4d entry remain unbound; grepping the bridges for entry addresses
   gives these ported owners with no `OverrideEntry` (reconciled at gate 130; 22 after gate 137):
   401000 401830 401a20 401ac0 40bf40 40c030 40c090 410510
   4208e0 4250d0 77f0a0 77f150 77f610 780c30 781070 785190 789bd0
   791610 791bd0 791df0 792100 793e70. The game_frame_control entries 411e30/40f010/411e20
   were bound together at gate 137. 402f40 and 772615 were bound at gate 130, 78dbe0 at gate 131, 40f350 at gate 132, 40f3a0 at gate 133, 419030 at gate 134, 7912e0 at gate 135, 77dfc0 through its common body 77dfc7 at gate 136. The render-queue
   consumer 4086f0 cannot be bound until its other record-type bodies are ported (it stops on them,
   and the live title produces them); 4073b0 has no native port yet. Those ports, then the frame owners. Every new entry: a `<slug>.cases.bin`
   corpus in `OVERRIDE_CORPORA`, replay through the real HLE device/heap/task owners, source-return
   continuations with their exact reserve, bridge input mutants and core mutants, live boot digest
   unchanged, `guest-controller` plain-vs-native 900 frames unchanged. State notes for every bridge
   built so far (stack reserves, store phases, boundaries) are in `docs/NEXT-AGENT-PROMPT.md` item 1
   and the gate 119–129 handoff entries.
4. **M1 close-out:** register the four unregistered tests (metal-sprite, metal-sprite-composition,
   metal-texture, player-task) or retire them with a reason; rerun `make -j6 oracles` and
   `make oracles-verdict` on the final M1 tree; record the M1 gate with `make m1` in its command
   and mark M1 complete in ROADMAP (an ADR is not needed; the table is unchanged).
5. **Importer first slice (stage E).** Fetch X-Hax sa_tools at a pinned commit into an ignored
   path (record the commit and licence in `docs/SOURCES.md`), parse `GameConfig/PC_SADX` entries
   for one stage (Emerald Coast act 1: landtable, its models, texlist, death zones), and emit a
   typed, hashed pack with resolved references under an ignored directory. Tests: re-serialise
   every record to the source bytes; every `pointer=` site resolves to the record; an unknown
   exe hash fails with a tagged error. Then make the Metal stage path read the pack instead of
   `tools/lib/extract.py`'s bring-up mesh. Formats are reimplemented, the C# code is not linked.
6. **M2 `golden-frames`:** offscreen renders of fixed guest frames (SEGA logo ~120, "CREATED BY
   SONIC TEAM" ~340, title ~7,900, file select via `--keys 450:13,800:13`) compared to reviewed
   goldens with a perceptual diff. Goldens are game imagery: keep them under an ignored path with a
   hash manifest tracked, or generate them from the local install in the test and track only the
   hashes; `tools/gate/check_repo_hygiene.py` must stay green. Then `app-smoke` to 300 frames and
   `state-dump-ui` (overlay = headless report). Unify `metal_guest_frame` with `metal_sprite` where
   both decode the same PVR formats.
7. **M3 input:** physical Xbox and Switch 2 Pro pads through GameController (`sadx-metal
   --controllers` lists them; `reference/switch2-usb-interfaces.json` has the USB probe), mapping
   unit tests against the Input mod's own tables, Apple deadzone policy decided and reported,
   vibration output (785190/pdVibMxStop_r path) or an explicit "not supplied" assumption.
8. **M3 audio:** DirectSound HLE → CoreAudio, the 40ff50 stream thread (cooperative scheduling at
   blocking Win32 calls), WMA decode decision as an ADR (bundled decoder vs one-time conversion),
   `audio-events` trace test.
9. **M3 replay:** `replay-emerald-coast` and `replay-boss-egg-hornet` input scripts with
   guest-memory assertions; the 63 FPS/FrameMultiplier model is already measured, make
   `speedrun-profile` assert quick save/load and CCEF camera traces.
10. **M4:** the Windows collector (`reference/windows-transfer`) for trace parity, reference
   screenshots, D3DCAPS8, HeapAlloc addresses, CTRLFLAG0, and the Intel transcendental/RCP tables
   that settle the 9 failing oracles.
11. **M5 full independence:** recover complete systems and their content dependencies; use
   runtime profiles to prioritize work, and track cold/unreached paths separately. Implement
   pinned Input/Speedrun behavior natively, import embedded resources, separate production
   and reference build graphs, and satisfy every M5 acceptance test. Retain the hybrid
   runtime as an optional oracle. Near-zero guest time alone does not finish the project.
   Each system recovered moves its owners from `*_at_<hex>` to annotated named state (stage C);
   freeze the reference corpora of a system before its last guest dependency goes (stage F).

## 7. The loop for one `/goal` step

1. Say which item you are doing (from `$ARGUMENTS` or §6). Read the last three HANDOFF entries.
2. **Analyse and build the context bundle** before writing code: (a) names, signatures and
   struct layouts for every address involved from the symbol database (until it exists: grep
   `reference/sadx-mod-loader/SADXModLoader/include/*New.h` then the classic headers for
   `0x0*<ADDR>`); (b) callers/callees and which are already native (`reference/inventory/
   functions.json`, the override tables); (c) pseudocode and assembly from the owned runner
   with `reference/ghidra-inventory-config.json` (`batch_decompile` for a cluster,
   `analyze_function` for the ones you port, `xrefs` for every global touched);
   `tools/analysis/ghidra_asm.py` for the listing; exact bytes only through
   `tools/lib/pe_image.py`; (d) the profile status of the range (`check_profile_patches.py`:
   0 UNHANDLED; the mod loader and mod sources for patched ranges); (e) the nearest
   already-recovered module as a style model. Pseudocode is evidence, not source: never paste
   it as the implementation.
3. **Implement** in the lowest layer that has the dependencies (`docs/ARCHITECTURE.md`); one slug
   per module; tagged failures with `source_boundary` and `required_fact`; assumptions into
   `HleProcess::assumptions`. **New** state and functions use semantic names plus ADR-0006
   provenance annotations; touching an existing `*_at_<hex>` owner either leaves it as is or
   migrates the whole owner (name + annotation + checker coverage) in the same step; until
   `check_provenance.py` is in the gate a new owner may keep the legacy `*_at_<hex>` form so it
   stays checked (AGENTS.md). Game
   behavior sits behind named interfaces; original-address marshalling, guest `uint32_t`
   addresses and override registration live only in reference adapters (`src/guest`,
   `src/hle`) (ADR-0005).
4. **Verify** at the roadmap's level: unit test (+ manifest row), oracle + `verdict.py` + mutants
   for exact ports, corpus replay + bridge/core mutants + live digest checks for overrides,
   golden/replay for M2/M3. Break the code once and watch the test fail. The judge is
   differential execution against the reference; you may not change a tolerance, a fixture's
   expected value, a mutant or the oracle harness to make your own code pass. After three
   failed approaches on one function, stop, record the unknown with its evidence and move on.
   A recovered system (not a single leaf) gets an independent review by a fresh subagent that
   reads the evidence and the diff before the gate.
5. **Gate:** `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?` (opens a window for app-smoke;
   needs a logged-in GUI session). On rc 0 only: `python3 tools/gate/record_gate.py --log
   build/gate.log --command "make -j10 gate" --exit-status 0 --limit "…" --output
   reference/production-integration-<date>/<name>-gate.json`, then `python3 tools/gate/gen_status.py`.
   If ported code changed: `make oracle-<slug>` (or `make -j6 oracles`), `make oracles-verdict`,
   `python3 tools/gate/track_reports.py <slug>…`.
6. **Journal:** append to `docs/HANDOFF.md` (evidence ids, what was done, counts, limits, what was
   not run), commit with a Conventional Commit subject, no attribution lines, no push.
7. **Report** to the user: outcome first, gate name and rc, what is unverified, the next item.

Pitfalls that cost time before are in AGENTS.md; performance notes (no instruction count to
Unicorn, `reserve_hook_region` for dense traps, translation flush on code writes, `timeout 0` for
long runs) are in `docs/NEXT-AGENT-PROMPT.md`.

## 8. Inputs on this Mac (all git-ignored)

| Path | What |
|---|---|
| `game/Sonic Adventure DX/` | the working install (sonic.exe `6b0937ca…`, system/, mods/ with loader + Input/Frame Limit/CCEF/EHCF) |
| `game/pristine/sonic.exe` | the untouched speedrun-page download, same hash |
| `game/disc/sonic-adventure-dx-directors-cut_202202/` | retail disc BIN/CUE images (Disc 1 exe differs by the CD-check byte; Disc 2 = SoundData) |
| `reference/updated-pc-reference/game/` | the pinned "updated speedrun" profile install: the guest's install directory (loader, Input + Speedrun mods, profiles) |
| `reference/windows-system/` | 32-bit msvcp140.dll / vcruntime140.dll 14.51.36247.0 the guest runs as real code (`PROVENANCE.md`) |
| `reference/sadx-mod-loader/` | mod loader source at eb4cdcca (git submodule); `SADXModLoader/include/*New.h` + classic headers = the symbol source (§4.4) |
| *(to fetch, stage A/E)* X-Hax `sa_tools` | `GameConfig/PC_SADX/*.ini` data mappings (13,160 embedded items) and `sonic_labels.txt`; pin the commit, keep it ignored |
| `reference/python/` | venv with Unicorn 2.1.4 (oracles and the in-process guest library) and capstone 5.0.7 |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra for REA's provider; JDK = Homebrew OpenJDK 27 |
| `reference/tools/rea/node_modules/rea-agents/` | installed REA 6.4.0; `$SADX_REA_MODULES` can override the modules root; `/private/tmp/sadx-rea/node_modules` is a legacy fallback |
| `~/Developer/rea/` | REA source checkout (docs in `docs/`, skill in `.agents/skills/reverse-engineer-anything/`), not built |
| `reference/inventory-raw/`, `reference/ghidra-game-loop-recovery/`, `reference/sdl-controller-recovery/` | raw Ghidra results (tracked product: `reference/inventory/`) |
| `reference/windows-transfer/` | the Windows capture collector for M4 |

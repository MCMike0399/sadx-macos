# The goal, for `/goal`

This is the standing brief an agent loads for `/goal` in `~/Developer/sadx-macos` (Codex:
`/goal implement @docs/GOAL.md`; Claude Code: `.claude/commands/goal.md`). It was rewritten on
2026-10-16 after the user reviewed five days of agent work. That review is
[REVIEW-2026-10-16](REVIEW-2026-10-16.md), and the decision is
[ADR-0008](decisions/ADR-0008-visible-journeys-and-proportionate-verification.md). The user
said two things. Rounds stopped showing progress on screen. And the code is still hard to
read. Both are fixable, and both are now part of the definition of a good step. ADR-0005
(native and executable-independent), ADR-0006 (names in code, addresses in annotations) and
ADR-0007 (readable game tier, address-shaped reference tier) still hold. Current numbers live
in `docs/STATUS.md`.

## 0. The prompt (what `/goal` injects)

```
You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS).
The product is readable, modular C++ that anyone can clone, build and play with assets
imported from their own disc or install: no sonic.exe, no original DLL execution, no x86
runtime (ADR-0005). The code reads like a game codebase, not a disassembly (ADR-0006/0007,
docs/STYLE.md). Fidelity to the Windows reference is proved by differential tests; the
hybrid guest is the development judge, not the product.

Progress is measured on screen. Every step either advances a row of the journey ladder
(GOAL §3: the app shows more of the game, first through the reference app, then natively),
or removes a real piece of readability debt by rewriting code, or unblocks one of those. Say
which before you start.

Read AGENTS.md, docs/GOAL.md, docs/STYLE.md and the top of docs/STATUS.md. Open other docs
only when the step touches their subject (METHOD §0 for tools, TIPS for idioms,
ARCHITECTURE for layers, ROADMAP for milestone tables, the last HANDOFF entry for context).
Then do ONE step of the loop in GOAL §7 on the first open item of GOAL §6 (or the item the
user passed). A step is a whole system, a whole module rewrite or a journey row, not one
constant family. Verify at the level GOAL §7 sets for that kind of change, run the gate once,
record it only on rc 0, regenerate STATUS, write one short HANDOFF entry and commit (no push,
no attribution). Report what is now visible or what debt was removed, plus the unverified
scope. Never edit an existing judge to pass. Never call a "partial" oracle a pass.
```

## 1. The goal, exactly

A native arm64 macOS game, built from readable, maintained source, that plays *Sonic
Adventure DX* with the pinned speedrun profile faithfully enough for speedrun practice. It is
proved by automated tests and structured so that the community can maintain and extend it.
Anyone can download the source. The user supplies the game assets from their own disc or
installation through a one-time importer.

* **Native execution.** All shipped game and pinned-profile code, cold paths included,
  compiles for arm64. Unicorn, x86 execution and original-code fallbacks exist only in the
  development reference.
* **Executable independence.** Building uses maintained source. Playing uses imported assets.
  Neither opens `sonic.exe` or runs an original DLL. A one-time import may read the user's own
  EXE/DLL/PVM/PRS files and emits typed, hashed data (M5).
* **Readable, maintainable source** (`docs/STYLE.md`). Game code is organised by game system,
  uses names from the symbol database, structured control flow, literal constants and plain
  arithmetic wherever the oracle accepts it. A developer can change Sonic's physics without
  knowing where it lived in `sonic.exe`.
* **Extensible without losing fidelity** (ADR-0007 §8, ROADMAP M6). Enhancements are opt-in,
  attach at named hook points, and are proved not to change the game when off.

What readable means. This is the user's example of what must go, `src/engine/ninja_fog.cpp`
today:

```cpp
op.at(0x787293);const auto scaled=fp.multiply_float_memory(fp.from_double(mantissa),65536.0F);
op.at(0x78729c);const auto truncated=ftol2_644340(fp,scaled);
```

and what the game tier should look like (`src/game/game_mode.cpp` today):

```cpp
const auto result = run_action(flow);
if (result == -2) {
    write_save_ready(flow, 0);
    clear_to_title(flow);
} else if (result == 1) {
    write_mode(flow, GameMode::Adventure);
```

Physics should end up as `speed.x * air_resist + acceleration.x` in plain `float`, with a
named fidelity helper only where the oracle shows that the x87 result differs. §6 item 2
removes the blocker for that.

## 2. History, and what it taught

| Days | What happened | Visible result |
|---|---|---|
| 10-10/11 | Function-by-function ports with Unicorn oracles (Hopper) | nothing |
| 10-12 | Throughput review (6 KB of 3.85 MB per day); ADR-0001 hybrid runtime, ADR-0002 Ghidra via REA | nothing |
| 10-13 | Guest boots sonic.exe with loader, mods, SDL2; Metal app; overrides | SEGA logo → title → file select |
| 10-14/15 | Override bindings; ADR-0005/6/7; symbol database; provenance, readability and judge checkers | same |
| 10-16 | FPU census; game-mode and ground pilots; 11 constant-initialization steps (gates 151–162) | same |

**Lessons (details in REVIEW-2026-10-12 and REVIEW-2026-10-16):**

* A method change made the jump on 10-13, not more effort. The hybrid runtime ran the whole
  intro from the original bytes and gave every later step a judge.
* Ceremony without a visible target stalls a project quietly. On 10-16 the agent passed 22
  gates in a row and changed nothing the user can see:
  * game-tier source was 1.7 % of the lines it added, and judge/evidence files about 92 %;
  * the readability debt did not move for eleven gates.
* Moving legacy files into `src/guest` lowers the ratchet without making anything readable.
  Count rewrites, not relocations.
* A reference that models the platform wrong blocks readable code. The guest's Direct3D 8
  `CreateDevice` never set the FPU to single precision as the real one does. So the census
  said 53-bit, and STYLE §7 kept every physics module in emulator-style x87 calls.
* Hopper demo hangs, so the project uses Ghidra only (ADR-0002). Wall-clock timeouts only;
  never a per-instruction count in Unicorn. Tests outside `tests/unit/manifest.tsv` do not
  run. Gates are recorded only with their real rc.
* The wider field (RESEARCH third pass): agent decompilation is *readable but wrong* unless an
  objective, tamper-evident judge checks it. Agents edit judges when they can. Readability is
  tracked as its own number and judged by the same oracle. The address-bound state peaks
  before it falls.

## 3. Strategy: the journey ladder, plus readability

### 3.1 The journey ladder (the spine)

A journey is something the user can watch, and an automated check proves it. Each row
needs:

* a `make` target that opens the app at that point (for the user);
* a gate test that drives the same journey headless or windowed and checks properties of the
  drawn frame. Like `tools/gate/check_app_smoke.py`, a check uses frame properties and locally
  generated golden hashes, never committed game imagery.

**Reference app** (`sadx-app --guest`: the original code in the guest, drawn by our Metal
backend). This lane comes first. It makes the game visible, and it produces the judge (state
traces, replays, digests) that native systems are later compared against.

| Row | Journey | State on 2026-10-16 |
|---|---|---|
| R1 | SEGA logo | done (`app-smoke`, 120 frames) |
| R2 | Title "Press ENTER" (Enter at 450 skips the movie) | works by hand (frame 700, 24 s); no check, no `make play` |
| R3 | File select → main menu → character select | works by hand (frames 800–1,700); no check |
| R4 | 3D world drawn: Station Square / Emerald Coast with geometry, textures, characters | **blocked by the renderer only.** The game reaches Station Square (mode 5, stage 26) by frame ~1,950, and Sonic moves under stick input (`sadx_driver.py`). But the Metal guest renderer draws only XYZRHW 2D vertices; 3D draws (`FVF 0x142` …) are skipped, and the screen is a solid clear. |
| R5 | Sonic playable: keyboard/pad moves Sonic through Emerald Coast act 1; a recorded input replay reaches the goal with state assertions (ROADMAP M3 `replay-emerald-coast`) | open; needs R4 and a stage frame-rate measurement |
| R6 | Sound and the intro movie (CoreAudio for DirectSound; movie drawn or cleanly skipped) | open. The movie is black and ran at ~6 fps in the app. `DirectSoundCreate8` returns `DSERR_NODRIVER`, so the game takes its no-sound paths, which the Windows reference does not. Settle this before trusting traces in M4. |

**Native app** (`sadx-app --native`, added by §6 item 7: game-tier code only, no guest, no
`sonic.exe`, assets from the user's install). These rows are the product.

| Row | Journey | Builds on |
|---|---|---|
| N1 | SEGA logo drawn natively: native frame control + game mode + logo task + PVM texture load from `system/*.PVM` + sprite draw through the shared Metal device | frame control, game mode (done); `ninja_sprite*`, `metal_texture` (ported); logo task (to recover) |
| N2 | Title screen natively, Enter goes to the next mode | N1 + title task |
| N3 | Menus to character select natively | N2 + menu tasks (`AVA_*` assets) |
| N4 | Emerald Coast act 1 geometry drawn natively from an imported pack | importer slice (§6 item 9) + the R4 renderer |
| N5 | Native Sonic runs in Emerald Coast; the first N frames of the R5 replay match the guest's state trace | player systems composed natively |

Rule: **at least every other step advances a journey row, or the work a row is blocked on.**
STATUS lists the rows and their state.

### 3.2 Tracks

| Track | What | Judge |
|---|---|---|
| **J — Journeys** | the rows above: renderer, input, audio, session composition, importer slices | journey checks + unit tests; replays/state traces from R5 on |
| **R — Readability** | rewrite existing ports into game-tier form under their unchanged oracles; whole modules, worst first | same oracle verdict and counts, mutants still caught, ratchet totals go **down** |
| **S — System recovery** | recover new systems directly in game-tier form (`METHOD.md` §3) | new oracle with mutants and holdout; override-equivalence in the guest |
| **V — Verification** | M1–M4 tables: override-equivalence, goldens, replays, audio events, Windows trace parity | ROADMAP tables |
| **E — Enhancements** | later (M6): hook and settings registry, replay-neutral overlays | enhancements-off parity |

Recovery stages that are done: the symbol database (A), the provenance checker and frame-control
pilot (B), the readability ratchet, the lookup and context-bundle tools, and the tamper-evident
judge manifest. Still to come: system recovery (C), the importer (E), freezing reference corpora
per system (F), the independence gate (G) and enhancements (H). See ROADMAP M5/M6.

### 3.3 Step size and proportionate verification

* **A step is a whole thing.** That means a whole system, a whole module rewrite, a journey row,
  or a coherent slice of one: the renderer's transform path, or the importer for one stage. A
  step is never a single constant family or a single function, unless that function is the
  whole of a module.
* **The verification level follows the kind of change** (§7 step 4 has the table). Constants get
  a byte-comparison unit test. Platform work gets journey checks. Exact logic gets an oracle with
  mutants and a holdout set. Independent review is only for a new system or a pilot.
* **One full gate per step, one HANDOFF entry per step.** Edits to *existing* judge files
  (tolerances, fixtures, harnesses, gate scripts, recorded digests) are always a separate
  `fix(judge): …` commit with evidence (`docs/JUDGE.md`). Purely *added* tests and probes
  should travel with the code they judge. `check_judge.py` cannot do that yet (§6 item 1,
  last bullet). Until it can: commit the source, refresh the manifest for the added files, run
  the gate **once**, then commit the refresh.
* **Evidence that is committed is small.** It is the gate record plus a short JSON or Markdown
  summary. A reusable harness goes to `tools/` with a test. One-off harness scripts, constructor
  dumps and "historical comparison" reruns are not committed.

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


## 5. Where things stand (2026-10-16, gate 162)

`docs/STATUS.md` is authoritative for numbers. In outline:

* **Reference app.** The guest boots `sonic.exe` with the real loader, model DLLs, Input and
  Speedrun mods and SDL2. It runs 7,228 deterministic headless frames (gate) at a host 77 FPS
  headless. In the app, Enter reaches the title, file select and character select. The 3D world
  loads but is not drawn (§3.1 R4). Intro movie: black, ~6 fps in the app.
* **Verification.**
  * 116 oracles: 60 pass, 46 partial (values exact, FP flags differ under Unicorn), 9 fail
    (Unicorn transcendental/SNaN limits; settling them needs an Intel capture, M4), 1 missing.
  * 71,115 override comparisons; two-boot determinism; 900-frame controller parity.
  * The judge manifest protects 505 files.
* **Native path.**
  * Frame control runs natively (`make native-frame-control`).
  * The semantic game-mode core is native, with STUB seams.
  * Ground acceleration and friction use the `ExactFloat` tier.
  * Twelve families of load-time constants have native sources.
  * The native app draws nothing yet, and there is no `--native` mode.
* **Readability** (ratchet totals, unchanged since gate 151): hex 11,628 (7,044 of them in two
  generated animation tables), `_at_` 1,796, address calls 75, goto 2, long lines 1,489,
  multi-statement lines 2,184. Emulator-style x87 code remains in 41 of the 47 `src/player` and
  8 of the 28 `src/engine` sources. `src/guest/native_game_reference.cpp` (1,652 lines) holds the
  address-shaped session composition that was relocated, not rewritten.
* **Working tree.** An unfinished "input device initial state" step from the last session is
  uncommitted (`src/input/device_initial_state.*`, its probe/oracle/test and edits to
  `native_game_reference`). §6 item 0 says what to do with it.

## 6. What is next, in order

Take the first open item. Each item is one or more whole steps (§3.3). The visible items (J)
and the readability items (R) alternate from item 4 on.

0. **Close the in-flight work (one short step).** Finish the uncommitted input-device
   initial-state step only if its tests already pass, as part of item 6's batch. Otherwise
   stash it with a note in HANDOFF. Do not start another single-family initialization step.
   As of 2026-10-16 it fails `check_provenance.py`: a new legacy identifier
   `device_at_3d08820` in `src/guest/native_game_reference.cpp`. Its judge files are also
   unmanifested, and its manifest row has no test source. So `make gate` fails until it is
   finished or stashed.
1. **`make play` and journey checks R2–R3 (J, small).**
   * `make play` opens `sadx-app --guest` with the pinned directories so the user can play
     with live keyboard and controllers.
   * The game itself skips the movie on Enter or Start. The launch line says so, because
     the movie is drawn black until R6.
   * Print one line on launch saying which keys do what.
   * Wire `tools/journey/run_journeys.py` into the gate as `make journeys`. It was added
     2026-10-16 and runs SEGA logo, title and character select as required journeys, plus
     `world-3d` as an open journey (`docs/TESTING.md` §3.1). Move it under `tools/gate` with
     a judge-manifest entry, and give it a fixture test for its frame-property logic.
     Character select needs the reference save in `reference/updated-pc-reference/game/savedata`
     or a documented equivalent.
   * A windowed app freezes while its window is hidden or covered: the `MTKView` stops
     drawing, and the guest advances only when a frame is drawn. `sadx-app --offscreen`
     (added 2026-10-16, with the harness in `src/apps/app_harness.hpp`) does not depend on a
     window. Move `run_journeys.py` onto `--offscreen --script`, so all its screenshots come
     from one run, and the gate can run it unattended.
   * The quick test fixes in `docs/TESTING.md` §3.7:
     * register the four unregistered tests;
     * run `game_reset_composed` once;
     * rebuild the override corpora when the native code changes;
     * record each test's wall time.
   * Keep `app-smoke` as R1.
   * `fix(judge)`: let `check_judge.py --update --additions-only` record newly added judge
     files while `src/` is dirty, and refuse it if any existing protected file changed or was
     removed. Add fixtures for both cases. Then a step with new tests is one commit and one
     gate (`docs/JUDGE.md` "Additions").
2. **Direct3D FPU precision in the reference (V/R, ADR-0008 §4).**
   * In `src/hle/hle_d3d8.cpp`, make `CreateDevice` and `Reset` without
     `D3DCREATE_FPU_PRESERVE` (behaviour bit `0x2`) set the calling guest thread's x87
     precision control to 24-bit and rounding to nearest, leaving the exception masks as
     they are. That gives `0x007f` from `0x027f`/`0x023f`. Print it as a reported
     assumption that cites Microsoft's `D3DCREATE_FPU_PRESERVE` documentation and the
     Windows capture (HANDOFF "Floating-point observations": main thread `0x007f`).
   * Unit-test the behaviour flag both ways.
   * Re-baseline the guest digests that change, in one `fix(judge)` commit that cites
     ADR-0008: two-boot determinism, first-600 override digests, controller parity, timing.
     Report the old and new values. Oracles that fix their own control word are unaffected;
     rerun them to show it.
   * Rerun the census (`reference/fp-control-census.json`) and add a stage entry once R4/R5
     reach one.
   * Update STYLE §7 with the result. Expected: `0x007f` at game entries, so plain `float`
     becomes the default for rewritten game code. If the census disagrees, record why and stop
     there.
3. **Metal fixed-function 3D renderer for the reference app (J, R4; several steps).**
   * Draw the guest's untransformed draws:
     * world/view/projection from `SetTransform`;
     * FVFs with XYZ, NORMAL, DIFFUSE, SPECULAR and TEX1/TEX2;
     * `DrawPrimitive`, `DrawIndexedPrimitive` and the `UP` variants, with vertex and index
       buffers;
     * D3D8 fixed-function lighting (directional/point lights, materials, ambient, colour
       sources);
     * fog (table and vertex), alpha test and blend;
     * texture stage states;
     * Z-buffer and cull.
   * Count every unsupported state as `not drawn` (the existing convention), never silently.
   * Design the interface as a device-level command stream that the native Ninja renderer will
     also feed (N1–N5). Do not design it around guest memory: guest decoding lives in an
     adapter.
   * Split it into steps by feature. The first step's check is that the first frame after
     choosing Sonic has no `not drawn` and is not a single colour. Then add golden frames for
     the first Station Square or Emerald Coast frame (ROADMAP M2).
   * Measure and record the guest's frame rate in a stage.
4. **Readability: rewrite the worst x87-style modules under their oracles (R; one module per
   step).**
   * Start with `src/engine/ninja_fog.cpp` (the user's example). Then do the player modules in
     ratchet order: `player_scene_position`, `sonic_player`, `player_mesh_registry`,
     `player_animation`, `player_contact_response`, `player_startup`, and so on.
   * Each rewrite uses plain `float` where item 2's census allows it and the oracle accepts it.
     It uses named fidelity helpers only where the oracle shows a difference (STYLE §7).
     Constants are literals. Seams replace address calls. The reference adapter takes
     everything address-shaped.
   * Done means: the same oracle verdict and counts, the mutants still caught, and the file's
     ratchet counts down by most of its debt. Alternate these steps with items 3 and 5.
5. **Sonic playable in the reference app (J, R5).**
   * Keyboard and pad drive Sonic through Emerald Coast act 1.
   * Record an input replay (title → Sonic → Emerald Coast 1 → goal) with state assertions
     read from guest memory at checkpoints: level, mode, player position within the
     tolerance ROADMAP M3 states.
   * Make it deterministic (two runs, same digests).
   * This replay becomes the judge for N5 and for the player modules.
   * If the stage frame rate is too low to play, list the hot functions (inventory
     `hot-paths.json` + a guest profile) and bind native overrides for the ones that are
     already ported, starting with the ones item 4 has made readable.
6. **Finish the numeric initialization in one batch (R/I, one step).**
   * Exit criterion: `NativeGame::initialize_source_facts` makes no numeric reads from the
     executable; each remaining read is a resource reference with an importer seam.
   * Use one shared byte-comparison test for all the remaining families, not one oracle per
     family.
   * Afterwards, the per-family history lives in HANDOFF, not in this file.
7. **Native session to the SEGA logo and title (J, N1–N2).**
   * Add `sadx-app --native GAME_DIR`. It composes native frame control, the semantic game
     mode, the logo and title tasks (recover them in game-tier form) and texture/sprite drawing
     through the shared Metal device. Textures come from the user's `system/*.PVM`. This is the
     first importer slice: reading files from the user's install, not from the EXE.
   * Fill the remaining seams with STUBs that fail loudly. List them.
   * The journey checks compare frame properties against the reference app's R1/R2 frames.
8. **Native menus to character select (J, N3)**, with the same checks against R3.
9. **Importer first slice (I, N4).**
   * sa_tools (pinned at `reference/sa-tools`) gives the schema. Write committed descriptors
     for Emerald Coast act 1: landtable, models, texlist, death zones.
   * Emit a typed, hashed pack under an ignored directory.
   * Tests:
     * every record re-serialises to its source bytes;
     * every `pointer=` site resolves;
     * an unknown exe hash fails with a tagged error.
   * The native app draws the stage from the pack using the item 3 renderer.
   * Reimplement the formats; do not link the C# code.
10. **Native Sonic in Emerald Coast (S/J, N5).** Compose the player systems natively. The first
    N frames of the R5 replay must match the guest state trace. This is where the remaining
    player recovery happens (track S), one system per step.
11. **M1 close-out (V).**
    * Bind the remaining ported owners only when a journey or a rewrite needs them.
    * Register or retire the four unregistered tests: metal-sprite, metal-sprite-composition,
      metal-texture and player-task.
    * Rerun `make -j6 oracles` + `make oracles-verdict` and record the M1 gate.
12. **M3 rest:**
    * input on physical pads (deadzones, vibration);
    * audio: DirectSound → CoreAudio, with an ADR for WMA;
    * the Egg Hornet replay;
    * speedrun-profile assertions.
13. **M4:**
    * the Windows collector for trace parity, reference screenshots and the stage FPU control
      words;
    * Intel transcendental and RCP tables, which settle the nine failing oracles.
14. **M5 full independence:**
    * recover the remaining systems in game-tier form;
    * implement the pinned Input/Speedrun behaviour natively under `game/profile`;
    * freeze the reference corpora per system;
    * separate the production and reference build graphs;
    * readability ratchet at zero outside the reference tier.
15. **M6 enhancements:**
    * a hook registry (events plus `should(VanillaBehavior…)`) and typed settings;
    * the all-off parity test;
    * a practice overlay (speed, position, timer, input display);
    * later, clearly labelled non-faithful practice tools.

## 7. The loop for one `/goal` step

1. **Name the item, its track and its outcome** in one line: the journey row it advances, or
   the debt it removes (file and counts). Read the last HANDOFF entry and reread
   `docs/STYLE.md`.
2. **Build the context bundle before writing code** (METHOD §0, §3). Do this only for game
   logic. Platform work (renderer, input, audio, app) needs the D3D8/Win32 contract and the
   guest's call stream instead.
   * names, signatures, structs and enums for every address (the symbol database,
     `tools/symbols/lookup.py`, `tools/symbols/context_bundle.py`);
   * callers and callees, and which ones are native;
   * named pseudocode (`batch_decompile` for clusters, `analyze_function` for ports) and
     `xrefs` for globals;
   * assembly and exact bytes (`ghidra_asm.py`, `pe_image`);
   * profile status (0 UNHANDLED);
   * the nearest game-tier module as a style model.

   Pseudocode is evidence, not source.
3. **Implement in two tiers** (STYLE).
   * The game tier holds semantic names, annotations, structured flow, literals, plain
     arithmetic where allowed, and `STUB` seams.
   * The reference adapter holds every address, every marshalling step and every fixture
     unknown.
   * Invent nothing. Every field, callee and guard has evidence. A transliterated draft may
     exist inside a step, but it is refactored under the same oracle before the commit.
4. **Verify at the level the change needs. No more, no less:**

   | Change | Required | Not required |
   |---|---|---|
   | Constants or tables moved to native source | one unit test comparing every value with the PE bytes (`pe_image`), one deliberate break | oracle, mutant suite, holdout, independent review |
   | Platform work (renderer, input, audio, app, importer) | unit tests for the decoding/state logic, the journey check(s), unchanged guest digests, one deliberate break | oracle, independent review |
   | Readability rewrite of a ported module | the same oracle verdict and counts as before, the existing mutants still caught, the ratchet goes down | new holdout, new evidence files |
   | New exact port or recovered system | oracle + `verdict.py` + 3–7 mutants, dev cases while iterating, holdout once at the end; override-equivalence when bound; independent STYLE §12 review | — |
   | Judge fix | a separate `fix(judge)` commit with the evidence, per `docs/JUDGE.md` | — |

   * Break the code once and watch the test fail.
   * Never change an existing tolerance, expected value, fixture, mutant or harness to pass.
   * After three failed approaches, stop. Record the unknown with its evidence in HANDOFF and
     `docs/TIPS.md`, and move on.
5. **Gate once:** `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. It opens a window for
   the app checks and needs a logged-in GUI session.
   * On rc 0 only: `python3 tools/gate/record_gate.py --log build/gate.log --command "make -j10
     gate" --exit-status 0 --limit "…" --output reference/production-integration-<date>/<name>-gate.json`,
     then `python3 tools/gate/gen_status.py`.
   * If ported code changed, run `make oracle-<slug>`, then `make oracles-verdict`, then
     `python3 tools/gate/track_reports.py <slug>…`.
   * New tests and probes registered by the step go into the judge manifest. See §3.3 for the
     interim two-commit, one-gate flow until `--additions-only` exists.
6. **Journal and commit.**
   * One HANDOFF entry, at most 15 lines, plain words with spaces between numbers and words.
     Cover: item, outcome (what is visible, or debt removed), verdicts with counts, what was not
     run, next.
   * One commit with a Conventional Commit subject, no attribution lines, no push.
   * Do not append per-step paragraphs to GOAL, ARCHITECTURE or TIPS. Update them only when a
     rule or a structure changes.
7. **Report** to the user in plain language. Start with the outcome: what they can now see
   (with the `make` command to see it) or what debt was removed. Then the gate name and rc,
   what is unverified, and the next item.

Pitfalls that cost time are in AGENTS.md and `docs/TIPS.md`. The performance notes are in
`docs/METHOD.md`: no instruction count in Unicorn, `reserve_hook_region` for dense traps, a
translation-cache flush when the guest's code is written, `timeout 0` for long runs. The
override-bridge notes are in `docs/history/NEXT-AGENT-PROMPT.md` item 1.

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

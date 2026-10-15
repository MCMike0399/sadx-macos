# The goal, for `/goal`

This file is the standing brief an agent session loads when the user types `/goal` in
`~/Developer/sadx-macos` (`.claude/commands/goal.md` reads it, then `docs/STATUS.md`). It
replaces the per-step `docs/NEXT-AGENT-PROMPT.md` as the entry point: that file still holds the
detailed state notes of the last M1 steps and is linked from section 6. Facts here were measured
on 2026-10-13 at gate 129 (commit `f68880f`); numbers that move live in `docs/STATUS.md`.

## 0. The prompt (what `/goal` injects)

```
You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS), built by
exact reverse engineering and proven by automated tests. Metal for graphics, GameController for
Xbox and Switch 2 Pro pads, fidelity to the Windows reference over convenience.

Read, in order: AGENTS.md, docs/GOAL.md (this brief: history, how REA is used, the next items),
docs/STATUS.md (generated from the last gate), docs/ROADMAP.md (the milestone whose table is not yet
green is the current one), docs/ARCHITECTURE.md, docs/METHOD.md, docs/decisions/ADR-0001..0004, and
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

A native arm64 macOS application that plays *Sonic Adventure DX* from the user's own install
(`docs/SOURCES.md`) with the speedrun profile that real runners use, faithfully enough for
speedrun practice, and that proves it by tests rather than by someone playing:

| Requirement | Where it is defined | State at gate 129 |
|---|---|---|
| Runs the whole game natively on Apple Silicon | ADR-0001: hybrid runtime; M5 drives the emulated fraction to zero | guest runs boot → SEGA logo → intro FMV → title → attract → file select; 40 native overrides |
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
and title run from the original bytes, with the ported code bound back in as overrides. The
exact-port method stays for hot paths and the platform boundary (`docs/METHOD.md` §3); it is no
longer applied in boot order to cold code.

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
  `reference/sonic-ghidra-inventory-snapshot.json`, 72 MB, seeds
  `reference/ghidra-game-loop-seeds.tsv`, 62 pointer-only functions). The older
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
* **REA itself:** the runner imports `rea-agents` 6.4.0 from **`/private/tmp/sadx-rea/node_modules`**
  (installed 2026-10-11). No `rea` binary is on PATH, `~/Developer/rea` is a source checkout
  without a build (`npm ci && npm run build:cached` would build it), and no REA MCP server is
  registered in Claude Code (`~/.claude.json` has none; `rea doctor` reports config drift for every
  client). Every REA call so far went through the runner script, never through an agent MCP tool.
* **Ghidra readiness is green:** `rea doctor --provider ghidra` with the environment file's
  `GHIDRA_INSTALL_DIR` (`reference/tools/ghidra_12.1.4_PUBLIC`) and `JAVA_HOME` (Homebrew OpenJDK 27)
  reports every check healthy (verified 2026-10-13).

### 4.2 The REA CLI, and what to do with it

`rea` (`node /private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs`) exposes the same
workflows as the MCP server, one process per call, with `--snapshot` reuse:

```sh
export GHIDRA_INSTALL_DIR="$PWD/reference/tools/ghidra_12.1.4_PUBLIC"
export JAVA_HOME=/opt/homebrew/Cellar/openjdk/27/libexec/openjdk.jdk/Contents/Home
export REA_GHIDRA_STARTUP_TIMEOUT_MS=600000 REA_GHIDRA_SEED_FILE="$PWD/reference/ghidra-game-loop-seeds.tsv"
REA=/private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs
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

**Housekeeping (item 1 done at gate 130: REA lives in `reference/tools/rea/`, the runner reads
`$SADX_REA_MODULES` with that default; item 2 still open):**

1. The runner's hardcoded `/private/tmp/sadx-rea/...` import path is one reboot away from
   disappearing (macOS purges `/private/tmp`). Install REA somewhere stable and point the runner at
   it: `npm install --global rea-agents@6.4.0` (then `rea` is on PATH and the runner can import
   from `$(npm root -g)/rea-agents`), or `npm install --prefix reference/tools/rea rea-agents@6.4.0`
   (git-ignored under `reference/tools/`). Make the path an environment variable with the current
   default so old notes still work; record the chosen path in `docs/METHOD.md` §1.
2. Optionally register REA as an MCP server for Claude Code in this project only
   (`.mcp.json`, `rea mcp` with the Ghidra environment above) so an agent can ask one question
   inline without writing a request file. Keep the runner as the batch path; `open_binary` with
   `snapshot_path` gives the same cache. Do not run `rea setup` for every client; the user's other
   agents' registrations are theirs to decide.
3. Keep Hopper out (ADR-0002). REA's IDA provider is irrelevant here.

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

Each item is one `/goal` step (one gate). Definitions of done are the roadmap tables.

1. **M1 override-equivalence for the remaining ported owners.** The handoff says 20 of the
   address-named list from the 4d entry remain unbound; grepping the bridges for entry addresses
   gives these ported owners with no `OverrideEntry` (reconciled at gate 130; 25 + 3 after gate 133):
   401000 401830 401a20 401ac0 40bf40 40c030 40c090 410510
   419030 4208e0 4250d0 77dfc0 77f0a0 77f150 77f610 780c30 781070 785190 789bd0 7912e0
   791610 791bd0 791df0 792100 793e70, and 411e30/40f010/411e20 of game_frame_control (they need a
   native input-callback bridge). 402f40 and 772615 were bound at gate 130, 78dbe0 at gate 131, 40f350 at gate 132, 40f3a0 at gate 133. The render-queue
   consumer 4086f0 cannot be bound until its other record-type bodies are ported (it stops on them,
   and the live title produces them); 4073b0 has no native port yet. Those ports, then the frame owners. Every new entry: a `<slug>.cases.bin`
   corpus in `OVERRIDE_CORPORA`, replay through the real HLE device/heap/task owners, source-return
   continuations with their exact reserve, bridge input mutants and core mutants, live boot digest
   unchanged, `guest-controller` plain-vs-native 900 frames unchanged. State notes for every bridge
   built so far (stack reserves, store phases, boundaries) are in `docs/NEXT-AGENT-PROMPT.md` item 1
   and the gate 119–129 handoff entries.
2. **M1 close-out:** register the four unregistered tests (metal-sprite, metal-sprite-composition,
   metal-texture, player-task) or retire them with a reason; rerun `make -j6 oracles` and
   `make oracles-verdict` on the final M1 tree; record the M1 gate with `make m1` in its command
   and mark M1 complete in ROADMAP (an ADR is not needed; the table is unchanged).
3. **M2 `golden-frames`:** offscreen renders of fixed guest frames (SEGA logo ~120, "CREATED BY
   SONIC TEAM" ~340, title ~7,900, file select via `--keys 450:13,800:13`) compared to reviewed
   goldens with a perceptual diff. Goldens are game imagery: keep them under an ignored path with a
   hash manifest tracked, or generate them from the local install in the test and track only the
   hashes; `tools/gate/check_repo_hygiene.py` must stay green. Then `app-smoke` to 300 frames and
   `state-dump-ui` (overlay = headless report). Unify `metal_guest_frame` with `metal_sprite` where
   both decode the same PVR formats.
4. **M3 input:** physical Xbox and Switch 2 Pro pads through GameController (`sadx-metal
   --controllers` lists them; `reference/switch2-usb-interfaces.json` has the USB probe), mapping
   unit tests against the Input mod's own tables, Apple deadzone policy decided and reported,
   vibration output (785190/pdVibMxStop_r path) or an explicit "not supplied" assumption.
5. **M3 audio:** DirectSound HLE → CoreAudio, the 40ff50 stream thread (cooperative scheduling at
   blocking Win32 calls), WMA decode decision as an ADR (bundled decoder vs one-time conversion),
   `audio-events` trace test.
6. **M3 replay:** `replay-emerald-coast` and `replay-boss-egg-hornet` input scripts with
   guest-memory assertions; the 63 FPS/FrameMultiplier model is already measured, make
   `speedrun-profile` assert quick save/load and CCEF camera traces.
7. **M4:** the Windows collector (`reference/windows-transfer`) for trace parity, reference
   screenshots, D3DCAPS8, HeapAlloc addresses, CTRLFLAG0, and the Intel transcendental/RCP tables
   that settle the 9 failing oracles.
8. **M5:** move hot guest functions to native overrides by runtime profile (replace
   `hot-paths.json` static depth with measured frame time per function), each proven by
   override-equivalence on corpora recorded from play.

## 7. The loop for one `/goal` step

1. Say which item you are doing (from `$ARGUMENTS` or §6). Read the last three HANDOFF entries.
2. **Analyse:** write a request file, run the owned runner with
   `reference/ghidra-inventory-config.json`, read `<label>.pseudocode.txt` and
   `tools/analysis/ghidra_asm.py`. Exact bytes only through `tools/lib/pe_image.py`. Check the
   range against `tools/gate/profile_patches.json` (`check_profile_patches.py`: 0 UNHANDLED).
3. **Implement** in the lowest layer that has the dependencies (`docs/ARCHITECTURE.md`); one slug
   per module; `*_at_<hex>` globals owned once; guest addresses stay `uint32_t`; tagged failures
   with `source_boundary` and `required_fact`; assumptions into `HleProcess::assumptions`.
4. **Verify** at the roadmap's level: unit test (+ manifest row), oracle + `verdict.py` + mutants
   for exact ports, corpus replay + bridge/core mutants + live digest checks for overrides,
   golden/replay for M2/M3. Break the code once and watch the test fail.
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
| `reference/sadx-mod-loader/` | mod loader source at eb4cdcca (git submodule) |
| `reference/python/` | venv with Unicorn 2.1.4 (oracles and the in-process guest library) and capstone 5.0.7 |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra for REA's provider; JDK = Homebrew OpenJDK 27 |
| `/private/tmp/sadx-rea/node_modules/rea-agents/` | REA 6.4.0 as the runner imports it today (see §4.2 item 1) |
| `~/Developer/rea/` | REA source checkout (docs in `docs/`, skill in `.agents/skills/reverse-engineer-anything/`), not built |
| `reference/inventory-raw/`, `reference/ghidra-game-loop-recovery/`, `reference/sdl-controller-recovery/` | raw Ghidra results (tracked product: `reference/inventory/`) |
| `reference/windows-transfer/` | the Windows capture collector for M4 |

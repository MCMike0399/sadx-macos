# Prompt for the next agent session

Copy the block below into a new Claude Code session opened in `~/Developer/sadx-macos`
(REA lives in `~/Developer/rea`; this repo's `CLAUDE.md` imports `AGENTS.md`).

```
You are continuing the sadx-macos project: a native Apple Silicon port of Sonic Adventure DX
(2004 PC) with the pinned speedrun mod profile, built by exact reverse engineering and proven
by automated tests. Read AGENTS.md, then docs/STATUS.md, docs/ARCHITECTURE.md, docs/ROADMAP.md,
docs/METHOD.md and docs/decisions/ADR-0001..0004 before touching code. The previous session's
handoff entries are the last eleven in docs/HANDOFF.md (from "Oracle health" to "Step 5: sadx-app
shows the guest"); read them — they record every HLE assumption and the bugs found on the way.

Where things stand (gate 111): the in-process guest runs sonic.exe from its entry point with the
real mod loader, CHRMODELS_orig, the model DLLs, the Input and Speedrun mods and SDL2 (MSVCP140 /
VCRUNTIME140 as real code from reference/windows-system, everything else HLE in src/hle), through
WinMain, window and D3D8 device creation, to 300+ deterministic frames at 63 fps virtual (the Speedrun
limiter on the virtual QPC), ~60 guest frames/s of host time. sadx-app --guest draws those frames with
Metal (PRESENTED BY SEGA, CREATED BY SONIC TEAM); make app-smoke is in the gate. Four frame-loop
overrides are equivalence-tested; the FPU shims replace every confirmed FPATAN/FSINCOS/FPTAN/FSIN/FCOS.

Current milestone: M1 — finish its definition of done (docs/ROADMAP.md). Do these in order, one gate
each (make -j10 gate — it now opens a window for app-smoke; record with tools/gate/record_gate.py only
on rc=0; python3 tools/gate/gen_status.py; a docs/HANDOFF.md entry; commit; do not push unless asked):

1. The intro FMV. At frame ~389 the game builds a DirectShow FilterGraph (CoCreateInstance of
   CLSID_FilterGraph stops by name; the call site is near the movie player that reads system\*.MPG /
   the WMV path through WMVCORE's WMCreateReader). Model it headlessly and honestly: find exactly which
   interfaces/methods the game uses (IGraphBuilder::RenderFile, IMediaControl, IMediaEventEx,
   IMediaSeeking, IVideoWindow, IBasicAudio…), give the graph the movie's real duration from the file,
   deliver EC_COMPLETE on the virtual clock, draw nothing (a reported assumption), and keep every
   unmodelled method stopping by name. Then run until the title screen; extend guest-frames to 600
   frames (the roadmap's SEGA logo → title).
2. profile-applied: 111 of 119 active patch sites are written by the first Present and still after 300
   frames; the 8 left are hudscale/SkyChaseFixes sites the inventory marks "unconditional at startup"
   while the profile has EnableUIScaling false. Decide from the loader source (eb4cdcca) and the pinned
   DLL which is right; fix tools/gate/profile_patches.json or find what the guest misses. The
   guest-boot test then asserts all active sites.
3. Override equivalence for every ported module with an oracle (src/guest/override_table): extend the
   bridges beyond the four frame-loop ones, starting with the owners whose state is plain globals,
   then the ones that need guest↔native object bridges (task graph, render queue, heaps). Each entry:
   equivalence on generated cases (the oracle's input classes) + mutants + the live-boot check in
   guest-boot. Record which ported modules are still unbound.
4. FPU shims for F2XM1 / FYL2X(P1): vendor the Bochs routines at the pinned revision (third_party
   bochs_fpatan, upstream.json), add X87Transcendental wrappers, shim the 7 confirmed sites, and compare
   on the guest-fpu-shims test like the others. Decide RCP/RSQRT honestly (the IEEE-division model is
   an assumption, keep it reported).
5. Input: feed host input to the guest (DirectInput mouse/keyboard device state and buffered data, the
   Input mod's SDL2 path — SDL's joystick backends currently find no devices), with an input script for
   the determinism test (identical per-frame hashes for the same script), then keyboard/controller in
   sadx-app.
6. A `make m1` target that runs exactly the roadmap's M1 tests, and the M2 golden-frames start
   (offscreen render of the SEGA frame against a reviewed golden).

Performance notes: never pass an instruction count to uc_emu_start (Unicorn then instruments every
instruction — 30x slower); per-address code hooks are scanned linearly on each hooked instruction, so
use X86Guest::reserve_hook_region for dense trap ranges; host writes into guest code must invalidate
translations (X86Guest::write does).

Analysis rules: use REA's Ghidra through reference/ghidra-live.mjs (docs/METHOD.md §1; now repo-relative
and allowing list_procedures/procedure_callees/…); the whole-program inventory is in reference/inventory
(functions.json, hot-paths.json, fpu-sites.json) — use it. capstone is in the reference venv for byte-level
checks. Never commit anything under game/, the windows-system DLLs, screenshots or the large reference
material; tools/gate/check_repo_hygiene.py runs in the gate. Report honestly: unverified scope is named,
skipped tests are listed, gates are recorded only with their real exit status, "partial" oracles are not
called passing, and every HLE fact the reference machine would have supplied is an assumption in
HleProcess::assumptions.
```

## Inputs available on this Mac (all git-ignored)

| Path | What |
|---|---|
| `game/Sonic Adventure DX/` | the working install (sonic.exe `6b0937ca…`, system/, mods/ with loader + Input/Frame Limit/CCEF/EHCF) |
| `game/pristine/sonic.exe` | the untouched speedrun-page download, same hash |
| `game/disc/sonic-adventure-dx-directors-cut_202202/` | retail disc BIN/CUE images (Disc 1 exe differs by the CD-check byte; Disc 2 = SoundData) |
| `reference/updated-pc-reference/game/` | the pinned "updated speedrun" profile install — the guest's install directory (loader, Input + Speedrun mods, profiles) |
| `reference/windows-system/` | 32-bit msvcp140.dll / vcruntime140.dll 14.51.36247.0 the guest runs as real code (PROVENANCE.md has hashes and the copy command) |
| `reference/sadx-mod-loader/` | mod loader source at eb4cdcca (git submodule) |
| `reference/python/` | the venv with Unicorn 2.1.4 (oracles and the in-process guest library) and capstone 5.0.7 |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra for REA's provider |
| `reference/inventory-raw/` | raw Ghidra list_procedures / procedure_callees results (the tracked product is reference/inventory/) |
| `reference/ghidra-game-loop-recovery/` | all Ghidra results so far (`<label>.json` + `.pseudocode.txt`) |
| `reference/windows-transfer/` | the Windows capture collector for M4 |

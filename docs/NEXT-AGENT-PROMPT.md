# Prompt for the next agent session

Copy the block below into a new Claude Code session opened in `~/Developer/sadx-macos`
(REA lives in `~/Developer/rea`; this repo's `CLAUDE.md` imports `AGENTS.md`).

```
You are continuing the sadx-macos project: a native Apple Silicon port of Sonic Adventure DX
(2004 PC) with the pinned speedrun mod profile, built by exact reverse engineering and proven
by automated tests. Read AGENTS.md, then docs/STATUS.md, docs/ARCHITECTURE.md, docs/ROADMAP.md,
docs/METHOD.md and docs/decisions/ADR-0001..0004 before touching code. Read the last handoff entries
in docs/HANDOFF.md (from "M1 4e completion" on) — they record every HLE assumption and bug found.

Where things stand (gate 114): the in-process guest runs sonic.exe from its entry point with the real
mod loader and mods (MSVCP140/VCRUNTIME140 as real code, everything else HLE in src/hle) from the SEGA
logo through the intro movie (a headless DirectShow graph, src/hle/hle_dshow: no video/audio drawn, a
reported assumption) to the title screen (frame 7168) and on into the attract demo; two boots are
identical. Host keyboard input reaches the guest as WM_KEYDOWN/WM_KEYUP (HleProcess::host_key; the
Input mod's window subclass maps them): Enter skips the movie and opens "Select a Saved File".
sadx-app --guest draws all of it with Metal and forwards the Mac keyboard (--keys scripts presses).
The FPU shims cover all 590 confirmed FPATAN/FSINCOS/FPTAN/FSIN/FCOS/F2XM1/FYL2X/RCP/RSQRT sites.
profile-applied is 119/119. The override table binds 12 owners (frame loop, startup registrars, the
D3DX math leaves), equivalence-tested on 6 oracle corpora + enumerated cases with mutants, live in the
boot and over 600 frames. `make m1` runs exactly the roadmap's M1 tests.

Current milestone: M1 — what is left (one gate each: make -j10 gate — it opens a window for app-smoke;
record with tools/gate/record_gate.py only on rc=0; python3 tools/gate/gen_status.py; a docs/HANDOFF.md
entry; commit; do not push unless asked):

1. Controllers: the Input mod reads game controllers through SDL2 (XInput / RawInput / HID backends; the
   HLE reports none connected). Model one XInput pad (XInputGetState/GetCapabilities for user 0, and
   whatever SDL's detection needs: device notifications, its joystick thread) fed from the host
   (GameController framework in sadx-app; an input script in a test).
2. The title screen's virtual frame time is ~19.5 ms (51 fps) against 15.9 ms on the logo; all virtual
   time comes from the Speedrun limiter's QPC polling (213c115b). Find why (frame work vs the limiter's
   interval, the 1 µs-per-query counter model) before trusting gameplay timing.
3. Override equivalence for the rest of the ported owners with oracles (46 address-named ones are
   unbound: see the 4d handoff entry for the list). Next: the D3D-device users (781250, 791170, 403180,
   fog 7949b0/794e40/794ec0, 77ec60) need a guest-device bridge (the HLE IDirect3DDevice8 as the
   native NinjaRenderState/transform owner callbacks); then heaps (780f60/780fb0), the task graph and
   the render queue. Give each oracle a <slug>.cases.bin (the verify scripts write it; add it to
   OVERRIDE_CORPORA) instead of enumerating cases.
4. The native runtime (game_runtime.cpp) still declares only D3DPRASTERCAPS_WFOG; with those caps the
   title screen's mipmapped PVRs hit the fatal 789dd0 (see the 4f entry). Align it with the guest's
   caps assumption.
5. M2 golden-frames: offscreen render of fixed guest frames (SEGA logo, title screen, file select)
   against reviewed goldens (perceptual diff), using sadx-app --guest --keys/--screenshot.

Performance notes: never pass an instruction count to uc_emu_start (Unicorn then instruments every
instruction — 30x slower); per-address code hooks are scanned linearly on each hooked instruction, so
use X86Guest::reserve_hook_region for dense trap ranges; host writes into guest code must invalidate
translations (X86Guest::write does); long tests must call with timeout 0 (no wall-clock limit) or a
loaded machine cuts the run short.

Analysis rules: use REA's Ghidra through reference/ghidra-live.mjs with
reference/ghidra-inventory-config.json (the game-loop snapshot no longer matches the binary profile;
results land in reference/inventory-raw/); the whole-program inventory is in reference/inventory
(functions.json, hot-paths.json, fpu-sites.json); capstone is in the reference venv. Never commit
anything under game/, the windows-system DLLs, screenshots or the large reference material;
tools/gate/check_repo_hygiene.py runs in the gate. Report honestly: unverified scope is named, skipped
tests are listed, gates are recorded only with their real exit status, "partial" oracles are not called
passing, and every HLE fact the reference machine would have supplied is an assumption in
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

# Prompt for the next agent session

Copy the block below into a new Claude Code session opened in `~/Developer/sadx-macos`
(REA lives in `~/Developer/rea`; this repo's `CLAUDE.md` imports `AGENTS.md`).

```
You are continuing the sadx-macos project: a native Apple Silicon port of Sonic Adventure DX
(2004 PC) with the pinned speedrun mod profile, built by exact reverse engineering and proven
by automated tests. Read AGENTS.md, then docs/STATUS.md, docs/ARCHITECTURE.md, docs/ROADMAP.md,
docs/METHOD.md and docs/decisions/ADR-0001..0004 before touching code. Read the last handoff entries
in docs/HANDOFF.md (from "M1 4e completion" on) — they record every HLE assumption and bug found.

Where things stand (gate 117): the in-process guest runs sonic.exe from its entry point with the real
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

Timing is now checked: guest-frames asserts 60 steady title frames after the full intro at
15.873 ms (63 FPS); guest-timing scripts a movie skip, then follows title -> attract -> title.
Attract uses FrameMultiplier 2 (31.746 ms per Present); loading can contain many limiter waits
before one Present. Every observed limiter wait matches multiplier*10 MHz/63, and every
virtual tick after boot is accounted for by QPC reads (1 us per query). The old ~19.5 ms
title report was not reproduced; its measurement scope is unavailable. Windows wall-time
fidelity remains unverified: keep the QPC-step model reported as an assumption.

Native and guest GetDeviceCaps now use engine/d3d8_caps: the same 0xd4-byte HAL,
WFOG and mipmap/cube-map texture-cap assumption, reported in both runtimes. Tests check
native startup's stored caps, both guest COM methods, and original 78cfa0's MIPMAP branch.
This aligns the platform fact; the native runtime's unported texture-init callees remain explicit.

Current milestone: M1 — what is left (one gate each: make -j10 gate — it opens a window for app-smoke;
record with tools/gate/record_gate.py only on rc=0; python3 tools/gate/gen_status.py; a docs/HANDOFF.md
entry; commit; do not push unless asked):

1. Finish controllers through the real Input mod / SDL path. hle_xinput now models a synthetic user-0
   wired pad (state/caps/#100, packet changes, enable/disable; no vibration actuators) with host samples,
   but SDL STILL ENUMERATES ZERO JOYSTICKS. The pinned SDL2 (25003b5b, base 6c940000) enables raw input
   by default (6ca6f120); 6ca73cf0 disables classic XInput when 6ca70900 reports raw input enabled.
   Supply the virtual Xbox-style HID/raw-input device facade (GetRawInputDeviceList/Info, HID metadata,
   device handles and WM_INPUT data/events), then any notification/thread behaviour SDL reaches.
   Do not change the profile's SDL hints to route around this. Raw attach 6ca6e6e0 requires a type-2
   device whose path contains IG_; it reads device info/preparsed data, opens the device and reads HID
   manufacturer/product strings. Analysis is in reference/sdl-controller-recovery/ (owned runner now
   accepts config.binary_path, defaulting to sonic.exe). Add GameController host sampling in sadx-app
   and a script that drives movie skip/title/file select through controller Start, with determinism
   and axis/button assertions. The direct XInput unit test is NOT proof of SDL/game input delivery.
2. Override equivalence for the rest of the ported owners with oracles (46 address-named ones are
   unbound: see the 4d handoff entry for the list). Next: the D3D-device users (781250, 791170, 403180,
   fog 7949b0/794e40/794ec0, 77ec60) need a guest-device bridge (the HLE IDirect3DDevice8 as the
   native NinjaRenderState/transform owner callbacks); then heaps (780f60/780fb0), the task graph and
   the render queue. Give each oracle a <slug>.cases.bin (the verify scripts write it; add it to
   OVERRIDE_CORPORA) instead of enumerating cases.
3. M2 golden-frames: offscreen render of fixed guest frames (SEGA logo, title screen, file select)
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

# Prompt for the next agent session

Copy the block below into a new Claude Code session opened in `~/Developer/sadx-macos`
(REA lives in `~/Developer/rea`; this repo's `CLAUDE.md` imports `AGENTS.md`).

```
You are continuing the sadx-macos project: a native Apple Silicon port of Sonic Adventure DX
(2004 PC) with the pinned speedrun mod profile, built by exact reverse engineering and proven
by automated tests. Read AGENTS.md, then docs/STATUS.md, docs/ROADMAP.md, docs/METHOD.md and
docs/decisions/ADR-0001..0003 before touching code. The previous session's handoff entry is
the last one in docs/HANDOFF.md ("Repository move and the hybrid-runtime decision").

Current milestone: M1 — the whole game runs headless on the hybrid runtime
(docs/ROADMAP.md). The guest CPU skeleton exists (src/guest/x86_guest.*, test
tests/unit/guest_smoke.cpp: 7899a0 from original bytes == native port, override works).

Do these in order, one gate each (make -j10 gate; record with tools/record_gate.py only on
rc=0; python3 tools/gen_status.py; append a docs/HANDOFF.md entry; commit; do not push unless asked):

1. Hygiene first: 19 comparison reports copied into reference/production-integration-20261012/
   post-repair-comparisons/ are mutant leftovers (see docs/STATUS.md "failing" list). Rerun those
   oracles (make reference-<x>-test), recopy the reports, rerun index_comparisons.py, confirm
   STATUS.md shows 60/60.
2. Oracles for what landed without one: d3dx_perspective_7716df, d3dx_matrix_inverse_7757e0
   (seed the dispatch slot 389b7c0 = 7757e0 as the SSE tables do), and the Bochs FSINCOS/FPTAN
   wrappers (compare against Unicorn's FSINCOS/FPTAN on random ext80 inputs; expect and
   document differences — QEMU uses host libm — and record which model is Intel-exact once an
   Intel capture exists). Then port 791170/781250/781570 (the view/projection WinMain span;
   analysis already in reference/ghidra-game-loop-recovery/wm-*.json and proj-791170.json)
   so 3d129c0 stops being zero natively.
3. M1 guest, step by step, each with a test in tests/unit/guest_*.cpp:
   a. Import resolution: parse sonic.exe's import table (ExecutableImage needs an
      imports() accessor; tools/pe_image.py has the Python version) and bind every import to a
      trap address in a reserved page; an HLE registry (src/hle) maps trap -> native function
      with a logged call record. Unbound imports are explicit failures with the import name.
   b. DLL loading in the guest: the mod loader (reference/updated-pc-reference/game/mods/
      .modloader/SADXModLoader.dll), Input mod, Speedrun mod, CHRMODELS; relocation via
      ExecutableImage::rebased; their imports through the same HLE; DllMain/Init export calls
      in the loader's order (profile_patches.json documents the patch sites they write — the
      profile-applied test verifies them in guest memory).
   c. WinMain to the first Present with the D3D8 COM vtables HLE'd onto D3D8StateOwner
      (src/runtime/game_runtime.hpp) — guest-boot test.
   d. Override table: bind the already-ported owners (start with the frame-loop ones in
      src/ninja and src/game) and run override-equivalence on the oracle corpora.
   e. FPU shims: tools/inventory script for FPATAN/FSINCOS/FPTAN/F2XM1/FYL2X/RCPSS/RCPPS/
      RSQRT* sites; hook them in the guest and compute with src/cpu models; test over the list.
   f. guest-frames + determinism + guest-fps tests; record FPS in the gate.
4. Keep sadx-app showing the real state: once the guest produces frames, render its D3D8
   command stream (clears, polygons, sprites via platform/mac/metal_sprite) and add
   `make app-smoke` to the gate.

Analysis rules: use REA's Ghidra through reference/ghidra-live.mjs (docs/METHOD.md §1);
Hopper is not used. Build a whole-program inventory (docs/METHOD.md §2) early — it is cheap
to start unattended and every later step needs it. Never commit anything under game/ or the
large reference material; tools/check_repo_hygiene.py runs in the gate. Report honestly:
unverified scope is named, skipped tests are listed, gates are recorded only with their real
exit status.
```

## Inputs available on this Mac (all git-ignored)

| Path | What |
|---|---|
| `game/Sonic Adventure DX/` | the working install (sonic.exe `6b0937ca…`, system/, mods/ with loader + Input/Frame Limit/CCEF/EHCF) |
| `game/pristine/sonic.exe` | the untouched speedrun-page download, same hash |
| `game/disc/sonic-adventure-dx-directors-cut_202202/` | retail disc BIN/CUE images (Disc 1 exe differs by the CD-check byte; Disc 2 = SoundData) |
| `reference/updated-pc-reference/game/` | the pinned "updated speedrun" profile install (the DLLs the oracles pin) |
| `reference/sadx-mod-loader/` | mod loader source at eb4cdcca (git submodule) |
| `reference/python/` | the venv with Unicorn 2.1.4 (oracles and the in-process guest library) |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra for REA's provider |
| `reference/ghidra-game-loop-recovery/` | all Ghidra results so far (`<label>.json` + `.pseudocode.txt`) |
| `reference/windows-transfer/` | the Windows capture collector for M4 |

> **Partly historical.** The entry point is `docs/GOAL.md` (`/goal`). This file is kept for the
> per-bridge state notes in item 1 (stack reserves, store phases, boundaries) and the performance notes.
> New bridges follow the two-tier form of `docs/STYLE.md`.

# Prompt for the next agent session

> Since 2026-10-13 the entry point is `/goal` (`.claude/commands/goal.md` → `docs/GOAL.md`). This
> file is kept for the detailed M1 state notes its item 1 holds (bridge reserves, store phases,
> boundaries); `docs/GOAL.md` §6 links here.

Copy the block below into a new Claude Code session opened in `~/Developer/sadx-macos`
(REA lives in `~/Developer/rea`; this repo's `CLAUDE.md` imports `AGENTS.md`).

```
You are continuing the sadx-macos project: a native Apple Silicon port of Sonic Adventure DX
(2004 PC) with the pinned speedrun mod profile, built by exact reverse engineering and proven
by automated tests. Read AGENTS.md, then docs/STATUS.md, docs/ARCHITECTURE.md, docs/ROADMAP.md,
docs/METHOD.md and docs/decisions/ADR-0001..0004 before touching code. Read the last handoff entries
in docs/HANDOFF.md (from "M1 4e completion" on) — they record every HLE assumption and bug found.

Where things stand (gate 129): the in-process guest runs sonic.exe from its entry point with the real
mod loader and mods (MSVCP140/VCRUNTIME140 as real code, everything else HLE in src/hle) from the SEGA
logo through the intro movie (a headless DirectShow graph, src/hle/hle_dshow: no video/audio drawn, a
reported assumption) to the title screen (frame 7168) and on into the attract demo; two boots are
identical. Host keyboard input reaches the guest as WM_KEYDOWN/WM_KEYUP (HleProcess::host_key; the
Input mod's window subclass maps them): Enter skips the movie and opens "Select a Saved File".
sadx-app --guest draws all of it with Metal and forwards the Mac keyboard (--keys scripts presses).
The FPU shims cover all 590 confirmed FPATAN/FSINCOS/FPTAN/FSIN/FCOS/F2XM1/FYL2X/RCP/RSQRT sites.
profile-applied is 119/119. The process override table binds 40 owners (frame loop, startup registrars, the
D3DX math leaves + blend wrapper/leaf, fog colour, view/projection/range and fog init/table/generation/switches + heap allocation/free + task-work initialization/allocation/free + task pool/creation/free/dispatch/lifetime + queue reset/allocation), equivalence-tested on 12 oracle corpora + enumerated cases with mutants, live in the
boot and over 600 frames. `make m1` runs the roadmap's M1 tests, including the device, heap and task-work HLE bridge corpora.

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

Controller delivery is implemented through the pinned SDL's default raw-input path:
hle_raw_input enumerates/attaches a synthetic Xbox-style HID device, sends WM_INPUT and arrival/removal
notifications, and supplies the HID parser interfaces. Original SDL handles mapping and XInput correlation.
guest-controller scripts Start at 450/800 to skip the movie/open file select, checks two identical boots,
six axes/fifteen buttons, and reconnect. sadx-app samples the first supported GameController extended
profile; --pad FRAME:BUTTONS[:FRAMES],... scripts XInput masks (Start = 0x10).
Limits: synthetic device identity/report schema and packet policy are assumptions; Apple's deadzones
remain applied; no physical-controller/Windows HID parity or vibration output is claimed.
SDL needs Win32 message dispatch (its video subsystem is not initialized) and a stable correlation
interval before independent triggers are available after reconnect. See the controller handoff.

Current milestone: M1 — what is left (one gate each: make -j10 gate — it opens a window for app-smoke;
record with tools/gate/record_gate.py only on rc=0; python3 tools/gate/gen_status.py; a docs/HANDOFF.md
entry; commit; do not push unless asked):

1. Override equivalence for the rest of the ported owners with oracles (20 of the previously listed address-named ones are
   unbound: see the 4d handoff entry for the list). Next: render-queue producers/consumer and remaining owners; the pool, allocation/creation/child/free owners are now bound. Heap owners 780f60/780fb0 are now bound through hle_heap_bridge; projection range through hle_device_bridge. The bridge currently binds
   403180/4030d0/77ec60/7949b0/781250/791170/794e40/794ec0/787260/411af0/411b40, verifies the HLE vtable target, invokes the actual synchronous COM handler,
   preserves call trace/partial-store order and rereads the live device. process_overrides combines it
   with the 12 guest-only, 2 heap and 13 task owners; sadx-app installs that table. Bind the remaining render-queue owners. Give each oracle a <slug>.cases.bin (the verify scripts write it; add it to
   OVERRIDE_CORPORA) instead of enumerating cases. Blend and fog corpora are now generated dependencies;
   hle-device-bridge replays 16,418 wrapper/leaf cases +300 fog-colour cases and 3,000 view +3,000 derived projection cases plus 400 fog-init/1,200 fog-table cases, including cold dispatch
   initialization, boundaries and mutants. D3DX dispatch unset calls original 77350c after the
   view/perspective prefix; preserve stack depth for its pointer-bearing HLE calls. Non-SSE targets stop.
   The EXP2 density branch now runs Bochs FYL2X, with the guest's canonical FLDLN2 constant.
   The broader fog oracle stays partial (2,410 FP-status differences); the remaining table-domain
   boundary is 78753a. All 4,700 fog corpus cases are replayed; 19 generator table-domain cases stop explicitly at 78753a.
   Continue with task/queue. Fog enable/disable callbacks also flush cache, mode,
   generated table and the one-byte 3abdcfe flag in source phases.
   Projection range replays all 3,000 cases, preserves entry ESP-0x40 matrix pointers, and flushes
   range/cache stores before GetTransform. Its call frame sits below the matrix (ESP-0x50).
   Live-changing/null/foreign devices and a range-input mutant are checked. The core oracle
   remains partial: no store/transform mismatches, 1,251 x87-status differences.
   Heap uses NinjaMemoryMethods to call the existing HLE allocator through verified IATs. Preserve
   HeapCreate(0,0x800000,0), the 389d628 store before HeapAlloc, raw HeapFree EAX, zero failures
   versus unavailable host calls, and source return sites. All 3,000 heap cases and live reuse/
   exhaustion checks pass; 3 core/2 bridge mutants caught. guest-controller now compares the plain
   guest with process_overrides over its 900-frame script on every gate run.
   task_overrides binds 40b1a0/40b220/40b310 with one persistent TaskWorkArena. Initialization
   runs original 64fc70 at CALL depth (redirect reserve 4), then real HLE HeapCreate/HeapAlloc
   at source sites, and native fill/headers. Reinitialization preserves sentinel padding.
   MetadataWriter serializes header updates; guest payload writes stay in place. All 16 task_work
   sequences (4,890 operations) match full arena bytes with payload mutations; 3 core/2 bridge
   mutants caught. External metadata corruption/unowned frees/size-wrap stop explicitly.
   Next task graph work must map native task identities/callbacks and import live work fields:
   Payload transport now reads/writes live guest work; cached references are checked against live words.
   TaskGraphState carries native slot indices, callbacks and owned references. Restore validates the
   partition/links/arena ownership before replacing state and rejects active walkers/destruction.
   40b460 runs original 51a740 with redirect reserve 8; source globals/free links are serialized.
   40b030/40b860/40b940/40b6c0 use the native scheduler. Callback addresses map to native tokens,
   not host pointer casts. Guest callbacks remain original and their changes are reconciled
   before the next native operation. All 17 lifecycle sequences (2,302 operations) replayed with those
   guest callees; full arena bytes/defined results/globals match. Three core and five graph mutants caught.
   Null-parent creation preserves its allocated prefix then stops at 40b95c. Unknown callback execution
   during native rollback is a named boundary, never skipped. Continue with guest callback dispatch and
   destruction; dispatch now executes the original guest callback through native cursor continuations.
   40b0c0/40b170/40b420 are bound: next captured before callback, source context phases preserved,
   all-lists order 8 then 0..6. Source-return redirects retain 40b10b/40b43f in the callback stack,
   with nested pending frames matched by ESP. Callback reserves are 0x14 index, 0x20 all, 8 children.
   Saved-register slots and incoming callback registers match source; 18 callback scenarios cover
   mutation/nesting/self-deletion/prepend and same-slot reuse. Lifecycle oracle now checks callback-
   time globals (18 sequences/2,311 operations); all three dispatch core mutants caught.
   Unknown execution inside a synchronous creation rollback remains a named boundary.

   Task lifetime follow-up (gate 128): 40b570/40b7e0 bind native lifetime cursors; original guest
   destructors and recursive child callees execute via source-return continuations before unlink/
   work cleanup/recycle. Source retired rows retain field30; owned controls update flag/task linkage.
   External work-reference resolution maps observed guest words to tracked native allocations,
   not pointer casts. Lifetime callback scenarios cover nested dispatch and prepend. Lifecycle
   corpus now has 19 sequences/2,316 operations, with callback observations of parent exec state.

   Queue lifecycle follow-up (gate 129): 407ab0/408630 invoke native reset/allocation.
   Heap calls preserve 40863f/408656 returns (reserve 4); allocation stores precede reset.
   Buffer/cursor/overflow and colour stores precede COM. Reread capture words and colour
   after COM, serialize buckets before final words/colour/mode/count clears. All 600
   lifecycle cases replay at both owners; three real heap reallocations, two bridge input
   mutants and three core mutants caught. Core oracle remains partial for 402 FP-status
   differences. Only observed 0x88000-byte reset arenas bind; other sizes stop explicitly.
   The owned Ghidra runner now buffers early piped commands while opening its provider.

2. M2 golden-frames: offscreen render of fixed guest frames (SEGA logo, title screen, file select)
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

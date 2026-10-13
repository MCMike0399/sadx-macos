# SADX native Apple Silicon port: complete agent handoff

**Start here:** [SADX_AGENT_GUIDE.md](SADX_AGENT_GUIDE.md) is the method guide for agents: goal, current status, REA usage, the step-by-step workflow, conventions, pitfalls and next steps. This file is the detailed step ledger.

This handoff records the investigation and implementation state on **2026-10-12**, in `/Users/burbujamc/Developer/rea` on **Macintosh**, the user's personal M5 Pro MacBook Pro with 24 GB RAM. Read the resume checklist first, then the subsystem sections relevant to your work. The appendices index the retained source, tests, probes, comparisons and provenance rather than replacing their detailed evidence.

**The goal is active and incomplete. There is no complete playable native SADX application, and no speedrun-readiness claim.** The strongest complete shared build checkpoint is a **historical 64-native-program / 4-Python-test gate**. Source changed afterward. Several newer modules have private validation; others are unfinished drafts. A green historical gate does not certify the current worktree.

## Standing instruction from the user: use REA's own tools and guides

For every reverse-engineering step of this goal, use the REA codebase's tooling: its **Hopper and Ghidra providers through the REA CLI/MCP** (Evidence with provenance, xrefs, decompilation, value flow, snapshots), following [AGENTS.md](AGENTS.md), [README.md](README.md), the guides in [docs/](docs/) (`ghidra-provider.md`, `cli.md`, `mcp-contracts.md`, `installation.md`) and the `reverse-engineer-anything` skill. `llvm-objdump` and Unicorn oracles stay for exact-byte checks and execution comparisons, not as a substitute for REA analysis. Recorded in `_reference/sadx/goal-amendments.json` → `rea_tools_and_guides_user_request_20261012`. The goal itself is unchanged: native Apple Silicon SADX, speedrun-ready with the updated speedrun mods.

## 0. Continuation progress (later on 2026-10-12, Claude Code session)

This section supersedes conflicting status statements below; older sections remain as history. Gate records and logs are in `_reference/sadx/production-integration-20261012/`, written by the new `sadx-native/tools/record_gate.py` (exact source/artifact SHA-256 per file).

| Gate | Native programs | What changed |
| --- | ---: | --- |
| `post-repair-gate.json` | 64 | Clean rebuild of the post-raw-MDATA-repair worktree (checklist step 4 done) |
| `integration-67-gate.json` | 67 | + `game_frame_control`, `ninja_texture_selection`, `game_scene_logic` in CORE/PROBE |
| `controller-68-gate.json` | 68 | + shared `game_controller` record, `scene_input_producer` (40fdc0/40f070) |
| `controller-analog-68-gate.json` | 68 | 40f170 WriteAnalogs borrows raw controller records |
| `consumer-69-gate.json` | 69 | + `ninja_sprite_display`, `ninja_queue_consumer` (CPU), `metal_sprite` in `libsadx-macos.a`; `build/metal-sprite-test` target |

All gates also ran the 4 Python extraction tests. Canonical comparisons were all rerun against the final probes into `post-repair-comparisons/` (indexed by `index.json`, produced by `tools/index_comparisons.py`):

- Repaired blend canonical: **7,944 cases, 0 stored, 1,668 status differences** (step 5 done). The pre-repair 2,808-difference report stays as history.
- Unchanged expected results reproduced: animation 16,440/0 stored; Sonic player 28,800/0; Sonic actions 30,192/0; startup, auxiliary effect, task work/lifecycle, position, render 6,160/0, sprite (24 seeded signaling-depth vertex differences retained, exit 1), queue, animation table, game logic 170/0, logic support.
- Frame control 2,672/0 and texture selection 683/0 on production probes (step 6 done).
- **Scene logic 413d40: first successful link, unit (optimized + ASan/UBSan) and oracle 800 cases/0 mismatches** (step 7 done). Scope: explicit owner fixtures and empty 40b170 lists.
- **Controller refactor (step 8) applied.** `GameLogicController` is now only the 52-byte optional-byte record in `game_controller.hpp`; `game_logic` 413dd0 reads/writes pressed via `read_u32/write_u32(0x10)`; `SonicPlayerControls` borrows `GameLogicEnginePads` + one `GameControllerButtonMasks`; WriteAnalogs `PlayerInputProducerRuntime::raw_controllers` borrows `GameLogicControllers` (new failures `unavailable_raw_controller` 0x40f185, `null_raw_controller`/`unavailable_raw_axis` 0x40f18c/0x40f190). `produce_scene_input` implements full local 40fdc0 (4146b0 classifier, ordered 909f94/909fa4 stores, `==1` copy of 13 raw dwords for slots 0/1 including unknown bytes, exact clear order, 40f170 owner, 40f070 slots 0..3). New oracle `verify_original_scene_input_producer.py`: **1,888 cases, 0 mismatches** (1,040 complete, 96 null-copy faults, 752 analog-owner stops). Input producer oracle unchanged: values match, exception flags retained.
- **Queue consumer (step 9 CPU part)**: the standalone fixture now states `projection.color_argb_at_3d0f7f0`; without it the test asserts the true 77e53b `missing_color` boundary. The whole consumer unit (lazy NEXT, dangling edge, unknown fact, colour clamp, two-pass) passes optimized and under ASan/UBSan. Assets `ninja-texture-returns.bin` (389d458, 16 B) and `ninja-queue-consumer-constants.bin` (7dcb0c+7dce50) are now source-extracted by `extract.py`, byte-identical to the private copies. `metal-sprite-test` passes on the real M5 Pro from production archives (`metal-sprite-production.json`).
- **New original-instruction oracles (step 9):** `verify_original_ninja_sprite_display.py` (full 77e390 incl. real 77f3d0/77f440/78cf20 texture binds, 781670, 78b840/791940, vertices, SetVertexShader/DrawPrimitiveUP call sites/args/bytes) — **536 cases × 12 CW = 6,432, 0 state mismatches**; `verify_original_ninja_queue_consumer.py` (real 404760 submissions then the whole 4086f0 body with every local callee unchanged) — **187 × 12 = 2,244, 0 state mismatches** covering 0–3 draws, fog 787240 stop, 411a20 bit0 early return vs body stop (411a30), mode-2 4071c0 stop, world 791a50 and missing-78dbe0 display stops. D3D8 device is a recording COM fixture; 78dbe0 is a seeded `fld; ret 4` or stop. FP status-flag differences retained.
- **Fidelity fix found by the consumer oracle:** 77ecb7 indexes the 12-word blend value table at 0x88ad6c with an unchecked nibble; values 12–15 read the following `.rdata` depth words at 0x88ad9c. `NinjaBlendTables::following_words_at_88ad9c` now carries that contiguous backing when decoded from 88 bytes (56 still accepted; then 12–15 stay an explicit `unsupported_value_index` boundary). Render oracle widened to nibbles 0–15: **8,209 cases, 0 mismatches**. Gate `display-consumer-69-gate.json`.
  REA Ghidra xrefs (fresh owned runner `ghidra-live.mjs` + `ghidra-game-loop-config.json`, retained as `ghidra-game-loop-recovery/blend-depth-xrefs-*.json`): 88ad64→only 77ecca (`ev_b48a1b83…`), 88ad6c→only 77ecb7 (`ev_c185e39d…`), 88ad9c→only 77ed0e (`ev_acf43dfa…`), 88ada0/88ada8→none. Region is in read-only `.rdata` (0x7db000–0x891b88).
- **CPU→Metal composition (step 9 done for the supported sprite path):** `tests/metal_sprite_composition.mm` / `build/metal-sprite-composition-test` runs native 404760+4086f0 with the real EFF_REGULAR record 130 upload, folds the emitted 10 render / 3 stage / 1 texture commands into a D3D8 state snapshot and renders through `metal_sprite` on the M5 Pro (quad (16,16)–(48,48), 481 covered pixels; log `metal-sprite-composition.log`). States no reconstructed owner produces yet are an explicit declared fixture: render 7,15,28,52,168,171(BLENDOP — 14 candidate `push 0xab` sites e.g. 53e643/573c2a/5f7e5e need REA analysis), stage0 1,2,3,5,6,16,17,18.

- **Step 10 started — main loop and Speedrun timing (gate `speedrun-timing-70-gate.json`, 70 programs):**
  - `game_loop.hpp/.cpp`: native 40cad0 split into `run_game_loop_startup` (40cad0–40caf9), `run_game_loop_iteration` (40cb00–40cca7, one source frame) and `run_game_loop_shutdown` (40ccaf–77fc50). Every callee is a required owner callback; exact read-before-store order (e.g. 3abdf58 stored only after the 3b22de4 load). It borrows `GameLogicState` for 3b11198/3b11194 (no duplicate owner). Oracle `verify_original_game_loop.py`: **688 cases, 0 mismatches**, mutation-tested (swapped 403180 args → 49 mismatches).
  - `run_frame_completion` = 411d00. Policy `speedrun_2_0` reproduces the live Speedrun 2.0 hook. `original_unported` stops at 0x411d1c (vanilla lag recovery via 7800e0/780120/780130 is not ported). Oracle `verify_original_frame_completion.py` maps the pinned DLL at 0x10000000, patches `jmp 0x100011d0` at 411d1c and executes the real hook back to 411da3: **2,964 cases, 0 mismatches**.
  - Speedrun DLL analysed with **REA Ghidra** (`rea decompile/xrefs --provider ghidra`, retained in `_reference/sadx/speedrun-dll-recovery/`): +0x11c0 `max(0, [3b11188]-1)` (`ev_5fc7b7a3…`); +0x11d0 stub jumps via file pointer +0x10010018 = 0x411da3; +0x10f0 limiter (`ev_9c9c4f7b…`): QPF→3d08538, interval = low64(sext(FM)·freq) / cadence (signed `__alldiv`), spin until signed (now−last) ≥ interval, then last=now (no deadline accumulation); QPF failure stores one QPC; last counter lives at 3d08550 (pointer +0x10010058); cadence global +0x1001001c (60 in file, written 63 by init at +0x10001211, which also writes 63.0f to 7dce58).
  - `speedrun::extra_game_ticks` now wraps exactly like the DLL (INT_MIN → INT_MAX; the old test asserting 0 was wrong); `limiter_interval`/`limiter_elapsed` replace the old non-wrapping `frame_interval_ticks`. `run_speedrun_frame_limiter` (game_frame_control) borrows the renderer frequency/last-counter and timing owners. Oracle `verify_original_speedrun_limiter.py` runs the real DLL function with stubbed QPF/QPC: **1,799 cases, 0 mismatches** (incl. the `__alldiv` divide fault).
  - `mac_clock` (libsadx-macos): 10 MHz virtual QPC from `mach_absolute_time`, limiter wait hint (sleep to ~4 ms before the deadline, then the source spin). `mac-clock-test`: 63 frames in 1.0002–1.012 s, no frame shorter than 158,730 ticks; sporadic host overshoot is reported (the DLL has no catch-up either). Gate `mac-clock-71-gate.json`.
  - **Input 2.6.1 owner (`input_mod_pad`, gate `input-mod-72-gate.json`)** from REA Ghidra on the pinned DLL (`_reference/sadx/input-dll-recovery/`): OnInput +0x6510 (`ev_5e46a6be…`), poll_controllers +0x4790 (`ev_a21b62bb…`), DreamPad::poll +0x4080 (`ev_92d691f1…`), convert_axes +0x4400 (`ev_c87c64b2…`). The shipped DLL is an `EXTENDED_BUTTONS` build (Back→0x800, LShoulder→0x1, RShoulder→0x100, LStick→E-held, RStick→persistent half-press), triggers are float SSE `(short)(axis/32767f*255f)`, threshold byte at pad+0x55, Old loaded before Held stored, SDL pointers at +0x1000e354/58. Pads 0–3 are copied whole to engine ControllersRaw 0x3b0e9c8 (+ DLL raw_input 0x1000e748); Support 0x3f0fff on connect; soft reset = held 0x606 & released Start & GameMode∉{1,8}; demo playback is an explicit unported boundary. Stick conversion reuses the already-verified `convert_input_mod_261_axes`; pad constants (`InputModPadConstants`: axes set + 255.0f@1000bba8 + 0.5f@1000bb78) are caller-supplied DLL bytes. `input_mod_on_input` composes the updated-profile 40f460: Input mod patches 40f460 to its RET at 40fdb3, the loader (eb4cdcca, built-in SDL off, source `sadx-mod-loader/SADXModLoader/Events.cpp`) replaces that RET with mod OnInput events (Input, then Speedrun's empty OnInput, REA `ev_3af47a88…`). Oracle `verify_original_input_mod_pad.py`: **1,044 poll cases, 0 value mismatches** (Unicorn never raises the SSE PE flag, so MXCSR differences are retained). Speedrun `OnFrame` (`ev_f0ffb142…`: Chao stat viewer, Gamma-timer counters) is a pending owner.
  - **Controller ownership correction (exact bytes, DLL `Jumps` export table at +0x1000e0b8):** 40f451 is the RET of vanilla InitControllers 40f3a0 (which points ControllerPointers[0..3] at ControllersRaw 3b0e9c8 after replicating a template record with an overlapping forward copy). The Input mod hooks that RET (+0x4be0 → +0x4b80) and **repoints all eight ControllerPointers at its own `raw_input` records (0x1000e748 + 0x34·i)**, so the engine reads `raw_input`, not ControllersRaw (`input_mod_init_raw_controllers`). `poll_controllers` polls pad i then copies it (interleaved) to raw_input[i] and, for i<4, ControllersRaw[i]. EnableController 40ef70 → +0x4bf0 and DisableController 40efa0 → +0x4c30 maintain the mod's own table 0x1000e73c, not 0x909fb4 (which 40fdc0 still reads directly — the two tables genuinely diverge); quirks: index ≥2 also sets/clears 0 and 1, index >8 recurses, Enable(8) writes the low byte of the keyboard normalized-R float at 0x1000e744, Disable(8) hits the invalid-parameter abort. Jumps entries: 785330→+0x8d80, 4bcbc0→+0x9150, 4bcc10→+0x91a0 (rumble), 40f343→+0x4b70 (WriteAnalogs, ported), 42d52d→+0x9240.
  - **Startup 40c030 survey (REA Ghidra, retained `ghidra-game-loop-recovery/startup-*.json` / `startup2-*`):** callees 420810 (→408630 queue arena alloc 0x88000 + 407ab0 reset, then 40f350 edge/key clears), 78c5a0 RET, 411e30(1,1) (native), 424c20 (86 B, calls 410510 = DirectSound/event init), 40bf40 (227 B, ~20 state-reset callees), **40f3a0 (ported: `init_controllers_40f3a0`, oracle 300 random-page cases / 0 mismatches / no stray writes, gate `controller-init-72-gate.json`)**, 40d0d0 (283 B, audio player objects), 40a8a0 (80 B), 789930 (39 B: register 410630 in a 32-slot callback list at 3d084b0). 40c090 is the 2,349-byte game-mode state machine. Loop-body owners: 420830 = store 3b280a8=1; 420840 = 3b280a8=0, 77ed00(3), 77ed20(1), 4128f0.
  - Queue lifecycle (pending): 407ab0 = cursor `(base+0x1f)&~0x1f`, zero 0x88000 bytes, 3abd9c8=0, 4128f0, 407330 (401420, 77ed20(1), 77ece0(1), 412400(0), 2048 bucket sentinels, captured words from 900840..48, 77ff90(&3ab9864), 3ab98ac=0), then 3b1117c=0 and counts 3abd9d4/3abd9dc=0. Done at the owners (gate `queue-reset-72-gate.json`): `NinjaRenderQueueArena::reset_407ab0(source_base_address)` (cursor `(base+0x1f)&~0x1f − base`, whole buffer known-zero, block ids never reused so stale handles are foreign) and `NinjaSpriteQueue::forget_records_after_arena_reset()` (bucket links stay stale until 407330's sentinels). Still pending: the 407ab0/407330/408630 composition with its 4128f0/401420/77ece0/412400/77ff90 owners.
  - **Mac controller → SDL domain (gate `input-chain-73-gate.json`, resumed session on Bedrock):** `sadx::input::mac::input_mod_gamepad_state(GCController*)` produces `InputModGamepadState` in SDL2 order. The reference SDL2.dll (25003b5b…) defaults `SDL_GAMECONTROLLER_USE_BUTTON_LABELS` on; the Input mod sets no hint and ships no gamecontrollerdb, so SDL A/B/X/Y follow the printed label (Switch “A” = east). The adapter resolves labels from each element's `sfSymbolsName` (`a.circle`…; observed on the snapshot profile) with an explicit positional fallback flag. Axes are trunc(v·32767) with Y negated, triggers 0..32767; Apple's deadzone is already applied, so this is not raw XInput/HIDAPI parity. Snapshot D-pads must be driven through `setValueForXAxis:yAxis:` (setting one direction button doesn't propagate). `tests/input_chain.cpp` composes keyboard → InputState → `input_mod_on_input` → raw_input via ControllerPointers → `produce_scene_input` → engine pads: Return+X give held/pressed 0x0c in engine pad 0, stick 127, mask 909f94 = 0x402, correct hold/release edges. No physical device exercised.
  - **Game-mode state machine 40c090 (gate `game-mode-74-gate.json`):** `game_mode.hpp/.cpp` `run_game_mode_40c090` is a direct translation of the exact instructions (REA decompile `startup-40c090` for orientation only): all 27 modes via the 0x40c9c0 table, the 5033b0 title-menu result table (0x40ca54 byte index → 0x40ca2c targets: −2, −1, 1, 100–104, 107), 16-bit `MOVSX` compares on 42f240/416610/4173b0/415af0 results, 42caa0's nonzero result stored as the new mode, rereads of 3abdc7c after callees (40c4e6, 40c733, 40c7d0, 40c8c2, 40c98d), and the common tail (785330/424320/590650 gate, then 42c4f0, 426050). Returns 0; the [esp+0xc] local is dead. It borrows `GameLoopState` (3b28114, 3b291a4, 3b291ad, 3abdcf8) and the caller's 3abdc7c owner; `GameModeState` holds the rest, including the 3b0ef4c level word (unknown/null stops at the MOVSX). All 154 native call sites were checked against the call instructions. Oracle `verify_original_game_mode.py` (every callee an owner fixture, scripted EAX, two global-rewriting mutators, fail-at-N, null level pointer): **24,000 cases / 252 distinct call paths, 0 mismatches**; two seeded bugs give 109 mismatches.
  - **InitGameLoop 40c030 (gate `startup-75-gate.json`):** `game_startup.hpp/.cpp` `run_startup_40c030` from the exact instructions. These differ from the REA decompile: the call at 40c068 targets 4253c0 (a jmp to 40d0d0), and the callback registration goes through 781000 (jmp 789930). Ported leaves: 40f350 (clears 3b0e3e0..3b0e6df, i.e. two bytes before edge row 0 and only byte 0 of row 255, plus 3b0e3b8..3ba and 3b0e990..9a0), 424c20's local stores around its 410510 owner (the 3b29ce8 counter stores prior+1, then 0 if prior>3), 40f3a0, 40a8a0 (48-byte 900e88→9008b4 copy when 900e88==0), 789930 (first empty slot of the 32-slot list at 3d084b0; returns 0 when full). Owners: 408630, 411e30(1,1), 410510, 40bf40, 40d0d0, 78c5a0 RET. When the Input mod is active, `input_mod_raw_input_at_1000e748` applies its 40f451 repoint. `GameLogicEdgeState` gains `bytes_at_3b0e3e0` and `byte_at_3b0e3b9`. Oracle `verify_original_game_startup.py` compares six randomized source pages byte for byte: **400 cases, 0 mismatches**; seeded bugs give 388. The 3b291a4 duplicate is resolved: `GameFrameInputState::save_phase_at_3b291a4` is now a reference borrowed from `GameLoopState` (frame-control oracle still 2,672/0).
  - **Native composition (gate `runtime-76-gate.json`):** `executable_image.hpp/.cpp` gives PE load-image facts (file bytes, zeros in the virtual tail, nullopt outside), so source globals start from the image (e.g. 7dce58 is **60.5f** in the file; Speedrun writes 63.0f). `game_runtime.hpp/.cpp` `NativeGame` owns every ported state structure (timing, logic, loop, game mode, edges, renderer timing, controllers, Input mod pads/raw_input, audio/menu/exit-callback startup state) and wires 40c030, 40cad0, 40c090, 40f460 → `input_mod_on_input` (updated profile), 411d00 with 781060 → `run_speedrun_frame_limiter`, 411e30, 77ed40 and 420830; every other callee is an explicit unported owner. `stop_at_unported` is real execution; `survey_unported_as_noop` lists the unported owners in first-reached order (navigation only, not game behaviour). `build/sadx-boot SONIC.EXE INPUT_MOD.DLL [frames] [--survey]` uses the Mac 10 MHz clock and the profile's pad settings; `tests/game_runtime.cpp` checks policy invariants. **Native execution currently stops at 40cad0>40c030>408630**; the 5-frame survey (`production-integration-20261012/sadx-boot-survey.txt`) lists 31 unported owners: startup 408630, 410510, 40bf40, 4253c0→40d0d0; loop 401410, 77f090 (message pump), 407ab0, 77ed00, 4250d0, 4086f0 (ported but unwired: needs a render backend), 40d4d0, 40eab0, 42f990, 40d3c0, 420840, 40ef30; 411d00's D3D8 frame calls 781010 (780030+7959a0), 781020 (EndScene path), 781030 (Present + device-lost 789dd0), 781040 (Clear), 781050 (BeginScene); 40c090's 415000, 42cea0, 421f70, 421f80, 4221e0 (.rel loader), 4f88a0, 42c7b0, 42cc70, 42c4f0, 426050.
  - **Render-queue lifecycle (gate `queue-lifecycle-77-gate.json`):** `ninja_queue_lifecycle.hpp/.cpp`: `ninja_queue_allocate_408630` (rebinds a fresh 0x88000-byte arena, copies 900858..60 → 90084c..54, then 407ab0) and `ninja_queue_reset_407ab0` (cursor `(base+0x1f)&~0x1f`, known-zero buffer, 3abd9c8=0, 4128f0 → 402f40 colour, 407330: 77ed00([8928c8]), 77ed20(1), 77ece0(1) = render state 27, 412400(0) → 3b12208/3abdbc0, 2048 sentinels, 900840..48 → 3ab9898..a0, 77ff90 copy, 3ab98ac=0; then 3b1117c, 3abd9d4, 3abd9dc = 0). 402f40 is now the shared `ninja_queue_color_402f40` used by both the consumer and the lifecycle (consumer oracle unchanged, 2,244/0). Oracle `verify_original_ninja_queue_lifecycle.py` (HeapAlloc/HeapFree and SetRenderState as fixtures): **600 cases, 0 state mismatches**, FP flags retained; seeded bugs give 552. `NativeGame` now owns the queue, render state and a `D3D8StateOwner` (records render/stage state for a future Metal backend; claims no draw), runs 408630 at startup and 407ab0/77ed00 each frame, and reports the HeapAlloc-alignment assumption. **Native execution now stops at 40cad0>40c030>410510** (audio init); the survey lists 28 unported owners.
  - **Audio streaming init 410510 (gate `audio-78-gate.json`):** `audio_stream.hpp/.cpp` from the exact instructions: if 3b0ef2c≠0 return 1; operator new(4) + 513e60 (zeroes the device pointer field) → 3b0ecc0; 44de80 (DirectSound: Release old, DirectSoundCreate, SetCooperativeLevel(hwnd 3a8b088, 2), primary format via 44d500(2 ch, 22,050 Hz, 16-bit); returns the failing HRESULT, else 0); on a negative HRESULT, return 0 with nothing else touched; otherwise 36× (3b0ec30[i]=0, CreateEventA → 3b0ece0[i]), zero 3b0ee98 and 3b0ed70 (36 words each), 3b0ef2c=1, CreateThread(40ff50) → 3b0ee00 (id 3b0ec28), InitializeCriticalSection(3b0ecc4), return 1. 424c20 ignores the return value. Oracle `verify_original_audio_stream.py` (operator new, 44de80 and the three Win32 import slots as fixtures; fs:[0] at linear page 0): **192 cases, 0 mismatches**. `NativeGame`'s device owner currently returns DSERR_NODRIVER (0x88780078), an explicit reported assumption that diverges from the Windows reference, which has sound. A CoreAudio device owner and the 40ff50 stream thread are the real fix. **Native execution now stops at 40cad0>40c030>40bf40.**
  - **CRT RNG and reset 40bf40 (gate `reset-79-gate.json`):** `crt_rand.hpp/.cpp`: the single-threaded MSVC CRT RNG (`srand` 6443b5 stores the seed at 2bcbdf8; `rand` 6443bf computes `seed=seed·0x343fd+0x269ec3` and returns `(seed>>16)&0x7fff`). Oracle **41,718 calls / 400 sequences, 0 mismatches**. `game_reset.hpp/.cpp` 40bf40: 25 owner calls (40cd10, 401410(1), 785330(0), 403e60, 4257c0, 4257e0, 424460, 410100(−1), 40cff0(0/1), 4208e0, 40d490(0), 40b1a0, 40b460, 4228f0(0..9), 421fb0, 411af0), stores 3abdf80=0 (borrowed from GameLoopState) and word 3b29d08=0, free+zero of 3b290d0/3b0e330 when nonzero, the 51f680 RETs, and **`srand(0xdead0cab)` on every reset** (speedrun-relevant RNG determinism). Oracle **300 cases, 0 mismatches**. Wired into NativeGame for startup and 40c090; native execution now stops at 40c030>40bf40>40cd10 (survey: 39 unported owners, `sadx-boot-survey.txt`).
  - **40bf40 leaves (gate `reset-leaves-79-gate.json`):** `game_reset_leaves.hpp/.cpp` `run_reset_leaf` ports, from exact instructions: 40cd10 (3abdfac=3abdf90), 401410 (depth mode 8928c8, borrowed), 403e60 (queue mode 3ab98ac=2), 4257c0/4257e0 (−1/0 stores at 912694..9126a8 and 3b29cf4/cf8), 421fb0 (3b11190=0, timing 3b1118c=0, tail-jump 504ed0 when 3c5e8b8≠0), 4228f0 (n=0 calls 422440/4224b0; n=5 keeps callback 0x592370; otherwise calls the 3b291b4 callback and clears it), 424460 (36 sound records: +0=0, +8|=0x10, 410100(i), 3b29b90[i]=0), 410100 (only the audio-uninitialized early return), 40cff0 (releases audio player 3abdf9c/3abdf94 via 411760/411720, 3abdfa8=0), 40d490(0) (3b0e338=0), and 785330 (a source quirk: the loop never advances, so entry 0 at 3d08428 is always the one tested; set to 3 when below 3, return 0, else −1). Composed oracle `verify_original_game_reset_composed.py` runs these as real source code inside 40bf40 over eleven randomized pages: **300 cases, 0 mismatches**. **Native execution now stops at 40c030>40bf40>4208e0.**
  - **View setup 4208e0 and FOV 7815c0 (gates `view-80-gate.json`, `fov-80-gate.json`):** `game_view.hpp/.cpp` `run_game_view_4208e0`: 403e60; 420860 when 3b280a8≠0 (407ab0, 402f10→780c00 clear colours 0xff000000, blends (0,8)/(1,6), 4128f0 colour, 4071c0 rect owner, 3b28114=0); 3abd9cc=0; x87 view products 3b28098=r_w·640, 3b2809c=r_h·480, ×0.5 into 3b280a0/a4; 788240 copies 3b28094/+0xc/+0x10 → projection 3d0f9e0/9ec/9f0; 402ed0(0x31c7) (calls 7815c0 only if 3aad0bc≠0x31c7, then 3ab98ec/3aad0bc); 3b2cbb4=0x31c7; the dead r·639/r·479 locals passed to the 78c5a0 RET are still computed for x87 state; 77ff60/77ff70 captured words; 402f40(&3b280ac); 403180(−1.0f, −3000.0f) owner; 77ed50(1) is a no-op (cases only 0/2/3: cull 1/3/2). Oracle `verify_original_game_view.py` **500 cases, 0 state mismatches**. `ninja_set_fov_7815c0` (moved out of the consumer, where it was dead code) computes scale = width 3d0f9e4 / (2·tan(angle/2)) with an f32 sine over an extended cosine from the source table. Oracle `verify_original_fov.py` **10,260 cases, scale bitwise equal**. **Hopper cross-checks via REA** (`HOPPER_LAUNCHER_PATH` set to the `~/Applications` install; snapshot `_reference/sadx/hopper-recovery/sonic-hopper-snapshot.json`): 4208e0 `ev_268824c0…` agrees with the port and names the constants (640/480/0.5/639/479); 40c030 `ev_73e5934d…` shows the 4253c0/781000 thunk calls that Ghidra folded. **Native execution now stops at 40c030>40bf40>4208e0>403180.**
  - **Projection range 403180 / 7815e0 (gate `projection-81-gate.json`):** `ninja_projection_range.hpp/.cpp`: 7815e0 clamps near to −1 when greater than −1, else to −65535 (0xc77fff00) when below, and the same for far, storing the negations into 3d0f808/3d0f968. 403180 stores X/Y into 90082c/900830; only when (X,Y) differs from the cache 3aad0c0/c4 does it update the cache, GetTransform(3), set m[10]=f32(Q) with Q=(−Y)/(X−Y) kept in extended precision, set m[14]=f32(−(Q·(−X))), and SetTransform(3). The x87 condition-code branch mapping was derived per instruction (TEST 0x41/JNE = only greater falls through; TEST 5/JP = only less; TEST 0x44/JP = only equal). Oracle `verify_original_projection_range.py` (GetTransform/SetTransform COM fixtures; each case run with a changed and an unchanged cache): **3,000 runs, 0 mismatches**; a seeded bug gives 1,747. `D3D8StateOwner` now records transforms; a read before any SetTransform returns the D3D8 creation default (identity) and is reported as an assumption. **Native execution now stops at 40c030>40bf40>40b1a0.**
  - **Ninja fog and the 40bf40 fog switch (gate `fog-82-gate.json`):** `ninja_fog.hpp/.cpp` owns the fog chain, read instruction by instruction through **REA Hopper** (`rea instructions/xrefs --provider hopper`, address-sorted listings) and cross-checked with the **REA Ghidra** owned runner (`ghidra-fog-table-requests.json` → `ghidra-game-loop-recovery/fog-table-*`, e.g. 794e40+7949f0 `ev_472b306b…`, 7949b0 `ev_126dea05…`; 3d128a4 xrefs `ev_e4278137…`: only 794ec0 writes it). Ported: **794ec0** device fog init (render state 28=0, with the 3d12798 store between the device load and its dereference; 3d128a4 = D3DCAPS8 RasterCaps bit 20 = WFOG (3d127e4) unless 795930 sees bit 0x20 in 3d08ae0, which 7958d0/793f70 read from registry value CTRLFLAG0; the WFOG path sets 34=0xffffff, 35=3, 36=0, 37=65535.0, otherwise 3d128a0=1.0; 3d12898=0xffffff, 3d0fce0=0x7e, 3d12950=0x7f; 3d12a00 w table `pow(2,i>>4)·(16+(i&15))/16`, exact on both CRT pow paths (6447a0 x87, SSE2 647c00); then 794db0(0x8010)); **7949b0** colour; **794db0** reciprocal table 3d12c00 (`K=1/(2^(p&31)·(p&0xff00)/65536)`, entry `1/(K·w)` when 2^-16 < K·w, else 65536.0; tail-jumps 7949f0); **7949f0** table-fog render states 35/36/37 (thresholds 0.3/0.6/0.9/1.0/65535) or the non-WFOG fallback density 3d128a0; **787260** njGenerateFogTable3 (frexp 7b5eef; **_ftol2 644340 modelled exactly**: FISTP under the current RC, then a one-step correction from the FST m32 signs; 128 entries X/w with X=−end when end/256>start, else 1.0; normalisation `1−(end+T[j])/(end+T[ref])` and zeroed tail); **794e40** table edges; and **411af0/411b40** with the full 4119b0 update (787240(0), 787260(T,−65535,−65535), 787250, cache 90a098/9c=−65535, then 787230 twice) and the full 411a20 level fog (3abdc60/64 negated when positive, colour 3abdc68, table regenerated only when the cache changes, then 787220). Explicit native boundaries: FYL2X EXP2 density (794afc), frexp of Inf/NaN (78728e), 787260 reference index past the table (78753a). Oracle `verify_original_fog.py` (whole PE incl. .data mapped, real CRT code, SetRenderState fixture with fail-at-call): **4,700 cases, 4,673 compared, 0 state mismatches** (27 native explicit boundaries); x87 flags differ only where native raises PE/IE and Unicorn does not. Seeded mutants are caught: thresholds 1,044, tail zeroing 915, table edge 721, CTRLFLAG 123, colour 262, and _ftol2 carry 183 / borrow 37 (after adding RC down/up control words and ends whose negation ends in 0xff). The queue consumer now calls the shared fog owner, still without a fog device, so its 787240/411a20 stops are unchanged (consumer oracle 2,244 cases, 0 state mismatches). `GameLoopState` now borrows 909eb4/3abdcfe read-only from the consumer state and queue runtime; they were duplicate owners (game-loop oracle 688 cases, 0 mismatches). All oracle scripts now resolve source files from their own location, so the Makefile `reference-*` targets run from `sadx-native/`. `NativeGame` runs 794ec0 as the native part of the pre-40cad0 device setup 791610 and reports three assumptions: the Metal device declares WFOG, CTRLFLAG0 is absent, and the rest of 791610 is unported. `d3d8()` exposes the recorded D3D8 state, and `tests/game_runtime.cpp` checks the fog state after 40bf40 (28=0, 34=0, 35=3, 36=65534, 37=65535). **40bf40 is now fully native; execution stops at 40c030>4253c0** (survey: 26 unported owners, `sadx-boot-survey.txt`).
  - **40c030 completes; keyboard poll 77f090 (gate `keyboard-83-gate.json`):** REA Hopper instruction listings (`ev_c4e9632d…` 4253c0/40d0d0, `ev_dc8c1b88…` 40ff10, `ev_8127d8f1…` 791bd0, `ev_e42544b9…` 791b10, `ev_83052091…` 77f0a0, `ev_359cdb54…` 77ffc0) and the owned REA Ghidra runner (`ghidra-keyboard-requests.json`: 780030 runs the 3d0fa40 callback list then the 3d0f7a4/a8/fa34/fa30 hooks; 77ffe0 removes; 401830 is called from 402c8d and 791610 from 793d80). **40d0d0** (the 4253c0 thunk at 40c068) is a `run_reset_leaf` case: when 40ff10 (=3b0ef2c, audio initialized) and config 3a7cf3c / 38f6ec0 are nonzero it creates the stream players 3abdf9c / 3abdf94 (operator new 6443e7 → 411970 → 4116a0: still an unported nested owner), otherwise it zeroes them, and always zeroes 3abdfa0/a4/98/a8. With the no-audio-device assumption it is the store-only path. Oracle `verify_original_stream_players.py`: **600 cases, 0 mismatches**; mutants caught. **40c030 now completes natively.** The loop's 401410 call uses the existing leaf. **`ninja_keyboard.hpp/.cpp`**: 77f090 tail-jumps to **791bd0**, the DirectInput keyboard poll: with no device (3d08820=0) only the stack cookie is checked; otherwise GetDeviceState(256) and, for each of the 3d0881c records at 3d08818 (the four 12-byte records at 3ce6e38), up to six new keys from DIK entries with bit 7 set through the 0xed-entry table 389d918 (kind 0 key code, kind 1 modifier bit), held keys kept in their previous order before new ones; on failure the records are cleared and Acquire retried while DIERR_INPUTLOST. **77f0a0** (from 401830 "Initialize Keyboard") initializes the records (pointers 3ce6e68+4i), runs **791b10** (3d08820 cleared, record pointer and count stored, CreateDevice(GUID_SysKeyboard) / SetDataFormat / SetCooperativeLevel(3d0fd30, 0xa), the 01 02 00 00 words) and **77ffc0**(77f090), which appends to the unbounded callback list 3d0fa40 / 3ce712c. Because 781010 runs that list through 780030, the reference polls the keyboard twice per frame (40cb06 and inside 411d00). Oracle `verify_original_keyboard.py` (COM fixtures, the real table): **3,000 cases, 0 mismatches**; mutants caught (modifier 183, six-key limit 377, Acquire loop 393, held order 174, zero hole 21 after adding holes). `NativeGame` runs 77f0a0 in the pre-40cad0 step (assumption reported) with a native device serving its public `keyboard_dik` DIK state (the macOS → DIK mapping is still to do). **Native execution now stops at 40cad0>40c090>415000** (survey: 23 unported owners).
  - **One owner per source global, checked (gate `owners-83-gate.json`):** `tools/check_global_owners.py` (run by `make test`) scans every owner struct in `src/*.hpp` for value members named `*_at_<address>` and fails when two structs own the same address. References and pointers are borrows, `.rdata` constants (0x7db000..0x891b88) may be copied, and call-argument snapshots are named `*_from_<address>`. It found six real duplicates; each became a borrow of the writing owner: the consumer's unused 3ab98ec copy was removed (`GameViewState` owns it); `InputModFrameState` 3abdc7c/3b2a2e4 (previously refreshed by the runtime before each use); `GameSceneLogicState` 3abdc7c; `PlayerAuxiliaryEffectRuntime` 3b1117c; `SonicPlayerGlobals` 3b29d08 (owned by `GameResetState`, read as int16). Now **235 owned addresses, 0 duplicates**. Oracles rerun: view 500, input mod 1,044, scene logic 800, audio 192, consumer 2,244 and auxiliary effect 43 cases with 0 state mismatches; sonic player 28,800 with 0 stored mismatches and the same 2,840 status-flag mismatches as its retained report. **Add every new global to exactly one owner and borrow it elsewhere; the checker enforces this.**
  - **40c090 game-mode leaves (gate `mode-leaves-84-gate.json`):** REA Hopper listings (`ev_4277b26b…` 415000, `ev_e693e5d4…` 42cea0, `ev_9d4cd008…` 421f90, `ev_5133e509…` 414470+46b230, `ev_682d0672…` 427f10, `ev_aae45a4b…` 427ee0, `ev_da2cd47f…` 413300, `ev_a3de700c…` 42f5d0, `ev_8593cd88…` 43b490, `ev_9225c0ba…` 425fe0, `ev_ef52b4b7…` 425af0, `ev_ba59dd7c…` 420e90, `ev_df6bb8df…` 4144e0, `ev_0214fe7d…` 427ec0, `ev_f43631d7…` 426050, `ev_79d95b69…` 4221e0, `ev_060c983d…` 4f88a0, `ev_85872ed4…` 42c7b0). `source_bytes.hpp` adds `SourceBytes<base,size>`, the owner of a contiguous run of globals with no typed owner yet (per-byte facts, little-endian access, bounds-checked). `game_mode_leaves.hpp/.cpp` `run_game_mode_leaf` ports, from exact instructions: 4144e0 (character word 3b22dc0), 421f70 (EAX 0xf), 421f80 (1), 4f88a0 (0), 4221e0 (its print is the 51f680 RET; 0), 426050 (3b0f13c += 3b11178), 42cea0 (zeroes 3b2a2f0..3b2a303 through the borrowed 3b2a2fa byte, plus 3b2b358), 421f90, 427ee0, 427ec0, 425af0, 43b490 (3b0f0fc = −1.0f), 42f5d0 (3b18608 block, event task 3b2c578 = null via the runtime's `PlayerScenePositionGlobals`, 3abe134 = 1, 914788 = −1), 413300 (seven zero fills over 3b18250..3b18daf leaving the 4-byte hole at 3b18a08, a null level pointer 3b0ef4c, 3b18db0, 3b18849 = 1), 427f10, 420e90 (per-character release lists 90ed54 / counts 3b290c4; the 403290 texture-release walk is a nested owner), 414470 (420e90(previous), character store, 422680, the 3b2b5fa byte into 3b0ef34 with the 425fe0/425af0 minimum 4, then the 46b230 tables), and the composites 415000 (…, 413c00, then `srand(0xdead0cab)`) and 42c7b0 (42c730, demo words 3b2a2e4/3b2a2e8 cleared, then 42d3b0). EAX is set wherever the source defines it. Oracle `verify_original_game_mode_leaves.py` (whole PE mapped; 422680/413c00/42c730/42d3b0/403290 as return-or-stop fixtures): **2,567 compared cases, 0 mismatches**; 33 release-walk cases are explicit; 7 seeded mutants caught. `PlayerAuxiliaryEffectRuntime` now borrows 3b22dc0. The owner checker counts scalar widths and whole `SourceBytes` blocks: **17,654 owned bytes, 0 duplicates**. **Native execution now stops at 40c090>415000>414470>422680** (per-character motion setup: a 0..7 jump table into routines that copy pointers out of the loaded character data, e.g. 3a7cf28 / 3aad104); survey: 19 unported owners.
  - **Model DLLs and 422680 (gate `model-dlls-85-gate.json`):** REA Hopper (`ev_248749d0…` 401ac0, `ev_94b3aacd…` 494820, `ev_10b7acf6…` 48d1c0, `ev_6c9818f3…` 4767f0) plus byte checks. The CRT static initializer **7c4e90** runs `401ac0(this=3ab9170)` before WinMain, then registers 7daed0 with atexit. **401ac0** `LoadLibraryA`s seven model DLLs (`SYSTEM\chrmodels.dll`, `adv00`, `adv01`, `adv01C`, `adv02`, `adv03`, `bosschaos0models.dll`) into `3ab9170+4i` and, for each DLL that loaded, `GetProcAddress`es its exports into 85 fixed globals (e.g. `___SONIC_ACTIONS` → 38f6e7c, `___BIG_ACTIONS` → 3aad104, `___KNUCKLES_ACTIONS` → 3a7cf28). The success/failure prints are 51f680 RETs. `bosschaos0models.dll` is absent from the install, so its slot is skipped. The installed `system\CHRMODELS.dll` is the **mod loader** (sha256 e40fac49…, no export table); it loads CHRMODELS_orig.dll (76bbaa1b…), and the native port resolves slot 0 against the original's exports (**assumption**: the loader's redirection was not analysed). Every model DLL, the loader and the Input mod prefer 0x10000000, so `ExecutableImage` gained export lookup (exact names, forwarders explicit) and `rebased()` (full mapping plus HIGHLOW relocations). `model_dlls.hpp/.cpp` maps the DLLs at ascending 64 KiB-aligned bases from 0x20000000 (**assumption**: the reference's load addresses were not captured) and binds them. The 85-entry table was transcribed from the 401ac0 bytes and is marked `// owner-table`, which the owner checker reads. Oracle `verify_original_model_dlls.py` (LoadLibraryA/GetProcAddress fixtures backed by the real export tables): **45 presence masks, 0 mismatches**; mutants caught. **422680** (per-character motion setup; a jump table) is now a game-mode leaf: characters 0 and 2 copy `[38f6e7c]+0x190/+0x194` to 3c566c0/d0, 7 copies Big's +0x10/+0x10/+0x74/+0x78, 3 copies Knuckles' +0xa0/+0xa4, 5 and 6 return, and 1, 4 and anything above 7 loop forever (PrintDebug then JMP 422700), which is an explicit `nonterminating` outcome. The leaves oracle now maps the relocated DLLs into Unicorn (an independent Python rebase) and runs the real 422680 code: **2,497 compared cases, 0 mismatches**; offset mutants caught. **Native execution now stops at 40c090>415000>413c00.**
  - **413c00, 783260 and 42c4f0; 40c090 completes (gates `mode-413c00-85-gate.json`, `fade-86-gate.json`):** REA Hopper (`ev_28d03cbf…` 413c00, `ev_f83578fe…` 783260, `ev_bb43dae7…` 40a340). **413c00** sets the game-mode word 3b22de4 = 0x15 (borrowed from `GameLoopState`), 3b22e1c/3b2c45c/3b18db8/3b22e20 = 0, runs 43b490, stores `~(3b2b5f9>>1)&1` into 3abe134, applies the 425fe0/425af0 minimum 4, calls 785330(0) (the runtime routes it to its existing reset leaf), and runs **783260(40a340)**, which stores the material callback's source identity into 3d08420 (40a340 is D3D8 GetMaterial, scale, SetMaterial; it is stored, not run). It returns 2 for signed stage ≤ 25, 1 for ≤ 34, 2 for ≤ 38, else 1. The runtime now owns a `SonicPlayerGlobals` (stage 3b22dcc), which the leaves borrow. **42c4f0** (the per-frame fade overlay) returns unless the alpha byte 3b29d5f is set; otherwise blends (0,8)/(1,6) go through `ninja_set_blend`, and a 4073b0 rectangle (0, 0, f32(8928c0·640), f32(8928c4·480), depth 0x46ac3dfb, colour = the 3b29d5c dword, 1) remains a nested owner. Its arguments are compared bit for bit, so nested calls now carry every stack argument and the leaves borrow the runtime's x87 context and `GameViewState` screen scale. Leaves oracle: **2,524 compared cases, 0 mismatches** (stage-boundary, width/height and alpha mutants caught). **40c090 now completes natively for the boot frames; execution stops at 40cad0>4250d0.**
  - **Sound-effect manager 4250d0 (gate `sound-87-gate.json`):** REA Hopper (`ev_692ed7fd…` 4250d0/424b10; helpers `ev_0aa613b8…` 4244a0, `ev_0cf83f9d…` 423c20, `ev_2bbc36b1…` 423c80, `ev_474422fb…` 423cc0, `ev_a780c51e…` 410260, `ev_8189ce2a…` 410050, `ev_e087f91c…` 423b20, `ev_99cf7046…` 4246c0, `ev_78acdf16…` 424590). `sound_manager.hpp/.cpp` ports 4250d0. When 3b29ce0 is nonzero it returns. Otherwise it increments the 3b29ce8 counter (above 3 it clears the 18-word 3b292a8 table and resets the counter), then walks the 36 records (stride 0x38 at 3b292fc).
  - **Queue consumer wired; Ninja matrix stack (gate `consumer-87-gate.json`):** REA Hopper (`ev_7569cc6f…` 7850f0 push, `ev_16b83a0e…` 7816e0 pop, `ev_28a34020…` 780180 njInitMatrix, `ev_d20ef0c8…` 781250 view reset). The matrix stack's globals are base 3d0fa1c, current 3d0f818, capacity 3d0fa28, depth 3d0fa2c and flag 3d0f7ac. **780180** (called by 401830 as `njInitMatrix(ecx=3a7cf48, edx=0x80, 0)`) stores them, sets depth 1, copies the 389d650 unit matrix into slot 0 and negates its m[0] and m[5]. `NativeGame` runs it in the pre-40cad0 step (assumption: other pre-40cad0 matrix use, such as 781250's view setup, is unported) and builds `PlayerCoordinates` over it. With that, the ported **4086f0** consumer is composed into the runtime: `NinjaTextureSelectionState` from the load image (null list/memory pointers), a `NinjaTextureSelectionRuntime` over the runtime's render state (the consumer's owner-identity check), `NinjaSpriteDisplayRuntime` and consumer constants decoded from the same `.rdata` runs as `tools/extract.py`, and texture binds, vertex formats and sprite draws recorded in `D3D8StateOwner`. The 78dbe0 projected-depth provider is left unset, an explicit missing owner. The consumer's maxima 3abd9d8/3abd9e0 now start from the image. **Native execution now stops at 40cad0>40d4d0**; the 5-frame survey lists 14 unported owners (`sadx-boot-survey.txt`).
  - **Per-frame loop owners (gate `frame-owners-88-gate.json`):** REA Hopper (`ev_92b3c565…` 40d4d0, `ev_052e6685…` 40eab0, `ev_4afd05f4…` 42f990, `ev_c6b535a7…` 40d3c0, `ev_62eb3c6c…` 420840, `ev_be9c98eb…` 40ef30, `ev_b394b7b4…` 40ef20, `ev_f44c8eee…` 40bcf0, `ev_b44ebd2e…` 402f00, `ev_ca3b0dbe…` 402ed0, `ev_98777ee9…` 4128f0). `game_frame_owners.hpp/.cpp` ports:
    - **40d4d0**: 3b2cbb4 = 402f00() (= 3ab98ec), depth 7/0, `402ed0(0x31c7)` (7815c0 only when 3aad0bc changes, then both FOV words), the 3abe13c draw list for 3b0e338 > 0 (nested owner at 40d520), the saved FOV restored when nonzero, depth 1/1, 3b0e338 = 0, then **40bcf0** (3abdf88 countdown; 40e2c0(3abdc18) and the 780fb0 free of 3abdf84 when it reaches 0).
    - **40eab0**: its effect body runs only for 3b0e998 > 0 (nested at 40eac0).
    - **42f990**: clears the 3b2c584 list pointer before its texture-release walk (nested at 42f9cc).
    - **40d3c0**: its 3abe150 release loop for 3b0e334 > 0 (nested at 40d3f5), then 3b0e334 = 0.
    - **420840**: 3b280a8 = 0, depth 3/1, then 4128f0 = 402f40(3b18230).
    - **Key queries**: 40ef30 returns byte 3b0e3b8; 40ef20(k) returns byte 3b0e3e0+3k (the edge-state owners).

    Oracle `verify_original_game_frame_owners.py` (real 7815c0/402f40/depth code with a COM SetRenderState fixture; the source stops at each nested body's first instruction): **2,100 cases, 0 mismatches**; 5 mutants caught. **Native execution now stops at 40cad0>411d00>781010**, the D3D8 frame calls (the Metal backend boundary).
    - **Active records** (flag bit 0) with id > 0x568 call 425710(id+0x159) and clear flags 0x11 and the timer. Lower ids go through 4246c0 and an unported body, so they end there natively and are an explicit boundary.
    - **Idle records with a positive timer** apply volume (flags 0x22 with 0x100 through 4244a0, with EAX = index, then 423c20; 0x200 from the record volume), frequency (0x800, ×0x4f, clamped to ±10000, then 410260), pan (0x2000 through 423cc0), then decrement the timer.
    - **A timer at 0** stops the sound (0x10 → 410100, routed to the existing reset leaf), then clears the record: +4 = 0, flags = 0, id = −1, entity 3b29b90 = 0.
    - **Tail**: when 3b29ce4 is 0 it jumps to 424b10, whose 3D-position work needs a listener at 3b2cbb0.

    The audio helpers are owners. New field owner `SoundRecordFields`; the timers and flags remain `AudioStartupState`'s and `ResetLeafState`'s. Oracle `verify_original_sound_manager.py` (every helper a recording fixture with case-derived EAX; 424b10's owner point is its with-listener continuation 424b26): **956 compared cases, 0 mismatches**; 6 mutants caught. **Native execution now stops at 40cad0>4086f0** (the queue consumer: ported but not yet wired into the runtime).
  - **D3D8 frame thunks and the loader's per-frame events (gate `frame-calls-89-gate.json`):** REA Hopper (`ev_6933ea38…` 78bab0, `ev_325e96c6…` 780030, `ev_e5920be5…` 7959a0, `ev_04e0aec7…` 78b960, `ev_cae92641…` 78ba30, `ev_e469fc56…` 78ba50, `ev_38577148…` 78b880, `ev_4f24b631…` 794800, `ev_15d97866…` 7912e0, `ev_06d80449…` 793cc0, `ev_4c2882b4…` 77dfc0, `ev_4b6ac674…` 795940) and retained Ghidra `callback-list-78007f` (`ev_6d8280d1…`). `ninja_frame_calls.hpp/.cpp` ports 411d00's five thunks (781010→78bab0, 781020→78b960, 781030→78ba30, 781040→78ba50, 781050→78b880):
    - **780030**: calls each 3d0fa40 slot while rereading 3ce712c after every call (it ends only on equality, so a callback that unregisters through 77ffe0 changes the walk), then the hooks 3d0f7a4, 3d0f7a8, 3d0fa34 (411e30's native 411e20 callable, run on `GameFrameInputState`) and 3d0fa30(3d0fa38). **7959a0** calls every nonzero hook in 3d08af0..3d08b2c. No `.text` code writes 3d0f7a4/3d0f7a8/3d0fa30/3d0fa38 (byte scan; no enabled DLL or `Codes.dat` names them), so they stay BSS zero.
    - **78b960 EndScene path** (when 3d08588 bit 0 is set): clears bit 0, 7912e0, 77ec60(0,8), 77ec60(1,6), saves render states 0xe/0x17 (GetRenderState +0xcc), sets 0/8, 793cc0 with ECX=1, restores both, EndScene (+0x8c). `ninja_set_blend_77ec60` is split out of `ninja_set_blend` (4030d0 = the 3ab9890 update + 77ec60).
    - **78ba30 Present**: `Present(NULL×4)` (+0x3c); on D3DERR_DEVICELOST (0x88760868) vanilla tail-jumps 789dd0. The pinned loader WriteJumps 78ba30 to `Direct3D_Present_r` (direct3d.cpp): Present, `direct3d::reset_device()` on DEVICELOST, then `onRenderSceneStart` (no enabled mod exports it). REA Ghidra xrefs on the pinned loader (`ev_13deb2fa…`: the reset-error string at 100dadb0 is used only at 10050400) and REA Hopper (`ev_f21c397d…` reset_device = 10050300, xrefs `ev_4b494d26…`, callers `ev_97e3f9ff…`) identify `Direct3D_Present_r` = **10050290**: `mov eax,[100f167c]` (= 3d128b0), device, vtable, `call [eax+0x3c]`, DEVICELOST → `call 10050300` at 100502ac, then the onRenderSceneStart vector 100f88f8..100f88fc (empty). The runtime uses `NinjaPresentPolicy::mod_loader` with those loader sites; reset_device is a nested owner. The oracle's `--loader` mode runs the loader's own instructions in a second machine: **800 cases, 0 mismatches**; a wrong-owner mutant gives 332. The loader's 78b960/78b880 trampolines run the original with no `OnRenderSceneEnd` subscribers.
    - **78ba50 Clear**: `Clear(0, NULL, 3d1289c&1 ? 7 : 3, 0, 1.0f, 0)` (+0x90), then **794800** on the nine 0x24-byte records at 3d0fe00: a null vertex buffer returns; otherwise +8 = +4 − +0xc before the buffer's Lock(0, size, &p, 0x2000)/Unlock (a nested owner at 794826).
    - **78b880 BeginScene path** (when bit 0 is clear): sets bit 0, BeginScene (+0x88), 77ec60(0,8)/(1,6); only when 3d128ac ≠ 0 it saves 0xe/0x17, sets 0/8, calls 77dfc0(3ce7164, 4, −1.0f, 0x22) and restores.

    Device methods are `D3D8FrameDevice` callbacks (BeginScene/EndScene/Present/Clear/GetRenderState) that a Metal backend implements; `NativeGame` uses a recording owner (counts in `D3D8StateOwner`, D3D_OK) unless `NativeGameConfig::frame_device` supplies them, and GetRenderState reads recorded states (ZWRITEENABLE/ZFUNC fall back to the D3D8 creation defaults 1/4 with a reported assumption). New owner `NinjaFrameCallState` (3d08588, 3d1289c, 3d128ac, the four hooks, the 3d08af0 table, the 3d0fe00 records); it borrows `NinjaCallbackList`, `GameFrameRendererState` (3d0fa34) and `NinjaRenderState`. Checker: 18,138 owned bytes, 0 duplicates. Oracle `verify_original_ninja_frame_calls.py` (COM device fixture, recording nested fixtures, a vertex-buffer COM object, a 3ce712c rewrite mid-list): **3,969 cases, 0 mismatches** (31 explicit `index_out_of_range` cases past the 8 modelled slots), 47 outcome kinds; **7/7 mutants caught** (restore swap, stencil flags, no count reread, record+8 operands, 15 hooks, BeginScene flag, blend order).
  - **Loader-profile fidelity in composed owners (same gate):** the reference loader is 3293cf35 (eb4cdcca), not the original install's e40fac49, and its source shows code patches that touch already-composed owners:
    - **426063** (426050's RET) → `ProcessCodes`: the profile has no `Codes.dat`/`Patches.dat`, `check_stack_balance` only prints, DebugScreen is off (empty message queue), so its effect is the Speedrun mod's **OnFrame** (+0x2e60, REA Ghidra `ev_f0ffb142…`), the only enabled `OnFrame` export (Input exports Init/Jumps/OnExit/OnInput; Speedrun Init/OnFrame/OnInput). `mod_loader_frame.hpp/.cpp` ports it: 100107e9 (QuickSave, false in the profile) → 10004f80; 100107e8 (Chao viewer) → the viewer body 10002e7f..10002f4e as a nested owner, then clear unless 3b22dcc == 0x27; outside game mode 7 with 3abdc7c ∈ {4, 9} the 10010818/19/1a timer rolls over with signed-byte `JL` compares (60/60/100, saturating at 59/59/99), otherwise it resets. 40c9b6 does not read EAX. Oracle `verify_original_speedrun_on_frame.py` runs the pinned DLL's own instructions: **6,000 cases, 0 mismatches**, **5/5 mutants caught**.
    - **40c045** → `EventGameLoopInit`: no enabled mod exports OnInitGameLoop and CrashGuard is off, so it is a no-op like 78c5a0's RET (40c04a does not read EAX); documented at the owner.
    - **401ae1/401ae2** → `nop; call LoadChrmodels` during 401ac0's own `LoadLibraryA` of the loader proxy: it loads CHRMODELS_orig.dll and stores that handle in 3ab9170 (`DataPointer chrmodelshandle`) before 401ae7 reloads `[esi]` for GetProcAddress. The runtime now stores CHRMODELS_orig's mapped base as slot 0's handle (it was 0x10000000), replacing the "redirection not analysed" assumption with a source-bound statement.
    - **Speedrun Init patches missing from `expected-hooks.json`** (REA Ghidra `ev_48a5ef54…` 10001080, `ev_790560b8…` 100013b0, `ev_95e73262…` 100012a0, `ev_f848167d…` 10007a50, `ev_3cdf9d0a…` 10004f80): 71adce → `jmp 10001060` (Chao-race hook that sets 100107e8); 426028 → a malloc'd buffer address (correction: 426028 is NOP padding after 425ff0's RET at 426023, so no executed instruction changes); 42608f → `call 10001280` + nop at 426094; 426134 → `call 10001290`; the buffer's first three words point at the timer bytes. None of these game ranges is ported yet. Recorded in `_reference/sadx/speedrun-dll-recovery/init-patch-inventory.json`; every future port must check its range against that file and the loader's unconditional effects (`updated-pc-reference/README.md`).
    - New assumptions printed by `sadx-boot`: natively the callback list holds only 77f090 and the hook table is empty, while the unported pre-40cad0 init also registers 77ffc0 callbacks (77f048, 77f20f, 785261, 78818f, 793e75) and 7959c0 hooks (789bd0 via 78123a>789c50, 795380 via 788007>795420); 3d128ac (writer 78e720) and the 3d0fe00 records (device setup) keep load-image zeros, so BeginScene skips 77dfc0 and Clear's 794800 walk finds no buffers; the D3D8 frame methods use a recording owner with no display backend.

    **A whole source frame now completes natively** (BeginScene 1, Clear 1 with flags 3, Present 1 in the stop run). **Native execution now stops at 40cad0>40c090>42c7b0>42c730 (call site 42c7b0)** in frame 2; the 5-frame survey adds 781020>7912e0 and 781020>793cc0.
  - **SEGA logo setup 42c730 (gate `sega-setup-90-gate.json`):** REA Hopper (`ev_2f20ba09…` 42c730, `ev_85872ed4…` 42c7b0, `ev_e5d538c4…` 42e470, `ev_faf4342e…` 402f10, `ev_3c539b87…` 780c00, `ev_82405003…` 42c4e0, `ev_1193c938…` 432ea0, `ev_bb8282b8…` 421180). `game_mode_leaves` ports **42c730** as a leaf (40c090 calls it at 40c900; 42c7b0 now runs it internally): 40b1a0, 40b460, 42e470(0) → 3b2c420 = 0, 402f10(0,0,0) → 780c00 with alpha ORed in (3ce7174..80 = 0xff000000, borrowed from `GameViewState`; without the borrow 402f10 is a nested owner), LoadPVM 421180("SEGALOGO_E" at 7df3d4, texlist 913a48), 3b29d6c = 1 and 3b29d68 = 0 before 411e30(1,1), 77ed40(1), 42c4e0(&four zero bytes) → 3b29d5c = 0, then **432ea0** (−1 into the 0x18-stride records 3b2c604..3b2c64c under a signed `JL` bound, then fixed words over 3b2c5e8..3b2c65c), and 78c5a0(3) (RET); EAX = 0 from 432ea0's XOR. The runtime now routes leaf-nested 40b1a0/40b460 (shared `task_reset`), 411e30 and 77ed40 to their ported owners. New owners in `GameModeLeafState`: 3b2c420, 3b29d6c, 3b29d68, `SourceBytes<0x3b2c5e8,0x78>`; checker 18,270 owned bytes, 0 duplicates. Leaves oracle (now with 42c730 and the real 42e470/402f10/780c00/42c4e0/432ea0/78c5a0; 40b1a0/40b460/421180/411e30/77ed40 as fixtures): **2,526 cases, 0 mismatches** (74 explicit); **6/6 mutants caught**.
    - **LoadPVM is the loader's in the profile.** `TextureReplacement.cpp` WriteJumps 421180 (LoadPVM) to `LoadPVM_r` and also replaces LoadSystemPVM, njLoadTexture, njLoadTexture_Wrapper, LoadPvmMEM2 and njLoadTexturePvmFile (77feb0). `LoadPVM_r`: the SS_MIZUGI and null-texlist prints, return when `nbTexture` is 0, a temp texlist over TexNameBuffer 3b28220 (300 names) via the game's njSetPvmTextureList, `LoadSystemPVM_r` (mipmap blacklist guard, `try_texture_pack` through the loader's replace-path map, else njLoadTexturePvmFile), then it grows the caller's texlist when the PVM has more textures and copies (attr, texaddr), carrying the last pair forward. Porting it means the loader's file-replacement map, the texture-pack probe (no enabled mod ships textures) and the game's PVM file loader.
    - **Native execution now stops at 40cad0>40c090>42c7b0>421180 (call site 42c756).**
  - **WinMain 4025b0 settings span (gate `winmain-91-gate.json`):** REA Hopper (`ev_38d5b305…` 4025b0, `ev_e98ea543…` 40ff20, `ev_ae027857…` 40ccf0, `ev_d86e7daa…` 40e8a0, `ev_ab504507…` 40e8b0, `ev_9a11df45…` 401a20, `ev_ab66d04a…` 780f60, `ev_415400bc…` 780fb0, `ev_f9d93b48…` 793d80, `ev_97036f6f…` 401830, `ev_2e4efe4b…` 77f610). **Boot order established:** the CRT static initializers (401ac0, and the object at 3ab918c), then WinMain 4025b0 = the settings span, CoInitialize, **781070(892940) → 781125 → 793d80 → 791610 (device setup)**, FindWindowA/SetWindowLongA (3a8b088 HWND, 38f6e90 old wndproc), **401830 (system init)**, the subsystem inits 5850f0/556e40/556d60/568d20/4f68e0/5adc40/600700 (each takes cliplevel 3abdcf0), 781570, 781250, 40e2e0, then **40cad0**. `NativeGame` now runs 401ac0 → WinMain settings → 794ec0 → 780180/77f0a0 (791610 before 401830; previously 794ec0 ran after 77f0a0).
    - `game_winmain.hpp/.cpp` ports 4025b0..402c29 as the profile runs it: LoadLibraryA("imm32.dll") → GetProcAddress("ImmDisableIME") → call (a null procedure faults at 4025e8), 3b0ef38/3b0f0e8 = 1 then the loader's **402614 SetLanguage** (VoiceLanguage/TextLanguage = its TestSpawn 1/1), 401a20 (CD-drive scan), the media-player MessageBoxA when 3ab918c or 3ab9190 is 0, and the `.\sonicDX.ini` `[sonicDX]` reads: framerate (default 2; ≤0 → 1, >3 → 3) → **89295c**; fogemulation (≤0 clears, ≥1 sets **909eb4 bit 0**); sound3d (clamped 0..1) → 40ff20 → 3b0ef28 = clamp(v,0,3); cliplevel → 3abdcf0 (raw store, then clamp 0..2 store); sevoice → 38f6ec0 (same pattern); voicev (default 100; ≤0 → 0 and 38f6ec0 = 0; >100 → 100) → 3abdfac = (⌊v·97/100⌋ − 100)·100 via the 0x51eb851f reciprocal, then 40ccf0(0) (3abdf90 = 3abdfac; reduce when > 0); bgm → 3a7cf3c; bgmv (default 0x59) → 909f28 = (⌊v·89/100⌋ − 100)·100, −10000 when 3a7cf3c is 0; screen (≤0 → 892950 = 1, else 892950 = 0 and ShowCursor(FALSE) until the count is ≤ 0); the loader's **40297a → 402a90** jump skips screensize; padconfig (GetPrivateProfileStringA, default "pad_default", 0x20) → buttons (clamped ≤ 13; nonzero fills 389d634's 13 words with 0xffff) → pad%d (fallback −2 → the nine local defaults 2,1,0xd,0xa,9,0xc,0x11,0x10,3 for i < 9) mapped through the 8929d8 table into 389d634; mousemode (clamped 0..1) → 40e8a0 → 3b0eae0; cmd0..cmd4 > 0 → 40e8b0(i, v). 40ff20, 40ccf0 and 40e8a0 are ported inline; 401a20 and 40e8b0 are nested owners.
    - `win32_profile.hpp/.cpp` is the native GetPrivateProfileIntA/StringA owner (case-insensitive section/key, trimmed values, sign + decimal digits, fallback when absent, size−1 truncation; quoted/hex/overflowing values are flagged). This is a Win32 re-implementation, reported as an assumption. The profile's `sonicDX.ini` (sha a630a440…, identical in the install and the reference tree) uses only plain decimals.
    - Owners: `GameWinMainState` (3b0ef38, 3b0f0e8, 3abdcf0, 909f28, 892950, 3b0ef28, 3b0eae0, 389d634's 13 words, the 8929d8 table, the 3ab918c/3ab9190 static-object members, the 8929b0/8929c4 message pointers); borrowed 89295c, 909eb4, 38f6ec0, 3abdfac, 3a7cf3c, 3abdf90. Checker: 18,316 owned bytes, 0 duplicates. Oracle `verify_original_game_winmain.py` runs the original span with both loader patches written in (IAT fixtures for the Win32 calls, a scripted INI, wsprintfA formatting, recording fixtures for 401a20/40e8b0, a null GetProcAddress source fault): **3,000 cases, 0 mismatches**, 105 outcome kinds; **8/8 seeded mutants caught** (plus 40ccf0's 3abdf90 copy, caught in a manual run after the scripted runner hit the make mtime race); the 40ff20 clamp mutant is equivalent here, since WinMain clamps sound3d to 0..1 first.
    - **Fidelity fixes:** the runtime previously used the load-image render factor 89295c = 3 and 909eb4 bit 0 = 0; the profile's sonicDX.ini gives framerate = 1 and fog emulation on. That changes 411e30's frame multiplier and 411d00's resync extra ticks (89295c − 1).
    - New assumptions: the Win32 profile semantics; imm32/ImmDisableIME as host tokens; the MessageBoxA reached because 3ab918c/3ab9190 keep load-image zeros (the CRT static constructor that sets them is unported); 401a20 skipped (its 3a8af78 stays empty). 780f60/780fb0 are njMalloc/njFree over a lazily HeapCreate'd heap whose handle lives in 389d628 (−1 in the image), so heap addresses are guest-visible; a native guest-heap owner is the next step. Native execution still stops at 40cad0>40c090>42c7b0>421180.
  - **Guest heap, njMalloc/njFree and the CD-drive scan (gate `heap-cd-92-gate.json`):** REA Hopper (`ev_ab66d04a…` 780f60, `ev_415400bc…` 780fb0, `ev_9a11df45…` 401a20, `ev_8c5220ee…` 401990). `guest_heap.hpp/.cpp` is the native owner of the Win32 heap: the HeapCreate handle (token 0x510000) and HeapAlloc blocks (from 0x50000000, 8-byte rounded spans plus an 8-byte gap) are native identities, since the reference's were not captured. Fresh bytes are unknown, writes stay inside blocks, and a call trace is kept. `ninja_memory.hpp/.cpp` ports **780f60 njMalloc** (when 389d628 = −1, HeapCreate(0, &"LastWater003", 0) into 389d628, then HeapAlloc(handle, 0, size)) and **780fb0 njFree** (HeapFree unless the pointer is 0). `game_winmain` ports **401a20**: 3a8af78[0] = 0; GetLogicalDriveStringsA(0, NULL) sizes an njMalloc buffer, a second call fills it (0 → free and return); each drive string goes to GetDriveTypeA, and a CD-ROM (5) passing **401990** (wsprintfA "%sSoundData\\SE\\V_AMY_J_BANK06.dat" + FindFirstFileA/FindClose) is lstrcpyA'd into 3a8af78; the skip loop and buffer free match the instructions. New owners: `NinjaMemoryState` (389d628), `CdDriveState` (`SourceBytes<0x3a8af78,0x104>`, which ends at the 3a8b07c model binding). The runtime runs it from WinMain with a host drive owner (one fixed C:\\, no CD-ROM with the disc; reported). Oracle `verify_original_cd_drive_scan.py` (the real 401a20/401990/780f60/780fb0 with Win32 IAT fixtures following the same rules and GuestHeap's identity scheme): **1,500 cases, 0 mismatches**, 17 outcome kinds; **4/5 mutants caught**. The surviving first-byte-check mutant is equivalent: Win32 returns 0 for an empty drive list before that check. Checker: 18,580 owned bytes, 0 duplicates. Mutation runs now use `time.sleep(1.1); os.utime(src, None)`, which avoids the make mtime race.
  - **Next, 781070 (Sonic Library Initialize, `ev_c28ffe1b…`):** copies the 892940 config (+0x28..+0x48 → 389d864..389d884, +0x10/+0x50/+0x54 → 38a5dc4/38a5dc8/38a5dcc), 794050 when +0x4c == 1, 793ef0(&window rect) (the loader's 789e50 CreateSADXWindow), 789f80, 78c5a0(0), 77ff40(0), **793d80 → 791610 + 7800a0**, 780f20(0,0), 780c30(+0x14,+0x18,+0x1c), 78c5a0, 77f150(0, 3cfc400, 3ce8550), 793e70, up to eight 788000(8, aligned 3ce71b8, 0x2c8, aligned 3cf4570) tries with a cleanup chain on failure, then 3d0f800 = 1.0f, 3d0f9c0 = (float)(u32)3d1278c / (float)(u32)3d12788 and 789c50 (registers the 789bd0 hook). **791610** (`ev_cf7f2751…`): 7899a0(1), zero 3d0fae0..3d0fcdf and 3d1289c, the nine 3d0fe00 records' first words, Direct3DCreate8(0xdc) → 3d11f60 (null → 789dd0), 3d128b0 = 0, 7943c0 (device creation), device +0x1c (GetDeviceCaps → 3d127c0), 794ec0, 78d0d0, 78c5a0, 796710, 791860, 7943f0, 78c5a0, Clear(0, NULL, 3d1289c&1 ? 7 : 3, 0, 1.0f, 0), SetViewport(3d12780) (+0xa4), 3d12948/3d1294c = half-extents, 3d0fd00..3d0fd18 = −1, 389d318 → 3d0fd80, 3d0fd20..3d0fd2c from the viewport, then a jump to 7914b0.
  - **Library and device init 781070 + 791610 (gate `library-device-93-gate.json`):** REA Hopper (`ev_c28ffe1b…` 781070, `ev_cf7f2751…` 791610, `ev_06e543ef…` 77ff40). `game_library_init.hpp/.cpp` ports **781070**: the window-rect words and config +0x28..+0x48 → 389d864..389d884, +0x10/+0x50/+0x54 → 38a5dc4/38a5dc8/38a5dcc (instruction order), 794050 only when +0x4c == 1, 793ef0(rect), 789f80, 77ff40(0), 793d80, 780f20(0,0), 780c30(+0x14,+0x18,+0x1c), 77f150(0, 3cfc400, 3ce8550), 793e70, the 788000(8, 3ce71c0, 0x2c8, 3cf4580) retry loop (−0x17/−0x21 run the 7881a0…789d30 cleanup chain and retry, 0 succeeds, any other result retries; eight failures run the chain once more), then the 3d1278c load, 3d0f800 = 1.0f, the 3d12788 load and 3d0f9c0 = height/width (x87 unsigned loads), and 789c50. Oracle `verify_original_library_init.py`: **2,400 cases, 0 mismatches**, 72 outcome kinds; **6/6 mutants caught**. `ninja_device_init.hpp/.cpp` ports **791610** (see the previous entry) with the real 7899a0: oracle `verify_original_device_setup.py` (Direct3DCreate8 IAT fixture 7db27c, a COM device for GetDeviceCaps/Clear/GetViewport, a 7943c0 fixture that installs the device or leaves it null, page 0 unmapped so a null device faults at the vtable load): **2,000 cases, 0 mismatches**, 14 outcome kinds; **7/7 mutants caught**. New owners: `GameLibraryInitState`, `NinjaDeviceState` (3d0fae0 block, 3d11f60, the D3DCAPS8 block 3d127c0..3d12893, the 3d12780 viewport, 3d12948/3d1294c, 3d0fd00..3d0fd18, 3d0fd80, 389d318, 3d0fd20..3d0fd2c) and the runtime's 3d0f7a0 (77ff40); `NinjaFogDeviceFacts` fields renamed to `*_from_*` (call snapshots), checker 19,353 owned bytes, 0 duplicates.
    - **Composition:** `NativeGame` now runs WinMain settings → **781070** → 793d80 → **791610** → 401830's 780180/77f0a0. 3d128b0 starts as the load image's null; the 7943c0 owner creates the native device (the loader's CreateDirect3DDevice_r behind 794000 is not ported, so this is reported), GetDeviceCaps declares only D3DPRASTERCAPS_WFOG in the caps block, and **794ec0 now reads RasterCaps from that block** (3d127e4). GetViewport gives the loader's 640×480 back buffer, so 3d0f9c0 = 0.75 and the 3d0fd20.. viewport floats are set before the loop. 791610's Clear goes to the recording owner (Clear count 2 in the stop run). Skipped pre-loop owners are listed in one assumption: window 793ef0, 789f80, 78d0d0, 796710, 791860, 7943f0, 7914b0, 7800a0, 780f20, 780c30, 77f150, 793e70, 788000 (taken to succeed on its first try) and 789c50 (the 789bd0 hook registration, pending its listing). The stop is unchanged (40cad0>40c090>42c7b0>421180).
  - **Next, 401830 (system init, `ev_97036f6f…`):** 77f240(38fcf08, 0x40000) (3d0f83c = arg0, 3d0f810 = arg1 & ~0x7ff), 780ea0(−32, 0, 0x160000, 0, 0x38000) (frees 3ce71b0, clamps negative args to 0, 3ce71a4 = (sum)·4, njMalloc → 3ce71b0), 780870(0,0,0) (3ce715c = 0xffbfbfbf, 3ce7154 = 8.0f, njMalloc(0x1000) → 3ce7158, 793c70(ecx = buffer, edx = 0x80, 0x800)), 7808c0(0xc) (3ce7154 = (float)(u16)arg), 780850(38fcf08, 0x6000) → 78e720(0x20, …) (writes 3d128ac), 7803b0 (ECX = 38f6ef8 → 3d0f740), 780180 (done), **77f610(3a8b0a0, 0x800)** (texture pool), 780140 (3ce7150 = 1, QueryPerformanceCounter(3ce7130)), 77ff60(8) (3d0f9c8), 402f10(0,0,0), 78c5a0, 77f150(0, 3aad140, 3a7ef48) (zeroes the 3ce6f48 records), 77f0a0 (done), 77efd0 (the 3ce6d88 records, 791d60, registers 77efc0 via 77ffc0 at 77f048), 40bb20([3d0fd34], [3d0fd30]) (DirectInput8Create, mouse CreateDevice/SetDataFormat/SetCooperativeLevel/Acquire), then two debug prints.
  - **Profile patch inventory and checker (subagent, 2026-10-12):** `sadx-native/tools/profile_patches.json` (618 entries: loader 82 active / 496 inactive, Input 23 active, Speedrun 16 active / 1 inactive; per entry va, size, owner, kind, target, condition, active, source_ref, original_bytes, handled_by) and `sadx-native/tools/check_profile_patches.py` (cross-checks every `build/original-*-comparison.json` declared sonic.exe range against active patches without `handled_by`; exits 1 on an unhandled overlap). First run: 4 overlaps — **77f0d7** (real divergence, below), 402ed0 (the loader's always-installed FixFOV trampoline calls the original at 4:3 and updates a loader-internal value), 78b922 (uiscale rewrites BeginScene's 77dfc0 call to an unhooked copy of the original: same behaviour) and 42608f (a false positive of the 0x40-byte default range; 426050 ends at 426063). Not visible to the checker: **785330** (routed by `game_runtime` to the vanilla reset leaf, but the Input mod's `Jumps` send it to `rumble::pdVibMxStop_r`) and **77e390** (`ninja_queue_consumer`/`ninja_sprite_display` port it, but the loader replaces it with its own `draw_particle`). Other always-installed loader effects found: FixFOV trampolines (7815c0, 402ed0), AutoMipmap (77fa10, 40d2a0), OnExit (64672f), the background/FMV fill-mode UI-scaling trampolines (77de10, 77dbc0, 404660, 404490, 77e9f0, 77dfc0, 720b10, 78b070, 77dc70, calls at 77ea10/78b922, 14 background trampolines), window patches for ScreenMode 0 (789e48, 789bd0, 402c61 (7 bytes), pointers 79426e/79427a), the CreateFileA IAT slot 7db0bc, an interpolation jump at 404c10, and HUD-scaling writes equal to the original bytes at 640×480. **Input mod Init patches missed by earlier sessions** (`_reference/sadx/updated-pc-reference/sources/input-mod.cpp`): 77f0d7 → `CreateSADXKeyboard` (KeyboardInputPointer 3b0e340 = the mod's SADXKeyboard; no 791b10 keyboard device), 77f03e NOP (77efd0's 791d60 mouse device), 77f205 NOP (77f150's 7928e0 DirectInput init), 437547 → GetEKey (overwrites the loader's FixEKey), and pointer repoints to `input::controller_enabled` at 40ef80/86/90, 40efb0/b6/c0, 40efd8, **40fe0d/40fe2f (inside the ported 40fdc0)** and 40f30c. The earlier statement that "40fdc0 still reads 909fb4 directly — the two tables genuinely diverge" is wrong under the profile: 40fdc0 reads the mod's table. These must be modelled before the profile can be called faithful; run the checker in every gate from now on.
  - **System init 401830 (gate `system-init-94-gate.json`):** REA Hopper (`ev_97036f6f…` 401830, `ev_c37c5606…` 77f240, `ev_daadcd99…` 780ea0, `ev_3eae34db…` 780870, `ev_1465b5f1…` 7808c0, `ev_3e49f1a4…` 780850, `ev_e51d96e7…` 78e720, `ev_23d7a475…` 7803b0, `ev_2e4efe4b…` 77f610, `ev_39f5c00b…` 780140, `ev_a08a11bb…` 77ff60, `ev_898a3f92…` 77f150, `ev_073071f2…` 77efd0, `ev_9141e510…` 40bb20) and REA Ghidra xrefs on the Input DLL (`ev_a0c80bd3…`: its KeyboardInputPointer slot 1000e068 is used only at 10006500 = CreateSADXKeyboard, whose bytes `a1 68 e0 00 10 c7 00 44 e9 00 10 c3` store **0x1000e944 (SADXKeyboard)**). `ninja_system_init.hpp/.cpp` ports 401830 with its leaves: 77f240 (3d0f83c, 3d0f810 = 0x40000 & ~0x7ff), 780ea0 (free the old 3ce71b0, clamp negatives, 3ce71a4 = 4·sum = 0x660000, njMalloc), 780870 (3ce715c = 0xffbfbfbf, 3ce7154 = 8.0f, njMalloc(0x1000) → 3ce7158, 793c70 fastcall), 7808c0 (3ce7154 = 12.0f), 780850 → 78e720 (3d127a0, **3d128ac = CRT malloc(0x6000·36)**, 794540 on the nine 3d0fe00 records with the 389d864/389d888/389d8ac/389d8d0 words, 3d0fae0 cleared), 7803b0 (ECX 38f6ef8 → 3d0f740), 780180 (the runtime's njInitMatrix owner), **77f610** (`ninja_texture_pool.hpp/.cpp`: the 0x800 × 0x44 texture memlist pool at 3a8b0a0, base 3d0f844, count 3d0f80c, 3d0fa24/3ce7128 cleared, 3d0f830, 3d0f860, 3d0fa14 = 2, 3d0f960, 3d0f7c0..3d0f7dc), 780140 (3ce7150 = 1, QueryPerformanceCounter → 3ce7130/34), 77ff60 (3d0f9c8 = 8, borrowed from `NinjaSpriteQueueRuntime`), 402f10 → 780c00, 77f150 (zeroes the 3ce6f48 and 3ce6e78 records with the +0x30 back-pointers, registers 77f110, tail-jumps 785240), 77f0a0, 77efd0 (zeroes the 3ce6d80 records — each writes +0x16..+0x1a, so the last spills into 3ce6de0..3ce6de2 — and the 3ce6df0 records; registers 77efc0) and 40bb20 (DirectInput8Create via 77dbb0/IAT 7db014 into 3abdc10, CreateDevice(GUID_SysMouse) into 3abdc14, SetDataFormat, SetCooperativeLevel(3d0fd30 = HWND, 6), Acquire; 3d0fd34 = HINSTANCE). **Input mod profile:** 77f0a0 stores 3b0e340 = 0x1000e944 instead of running 791b10 (no keyboard device, record pointer, count or 3ce6e68 words; `NinjaKeyboardProfile::input_mod`), and 77efd0's 791d60 / 77f150's 7928e0 calls are NOPs. 781070's own 77f150 call (781155) now runs the same owner, so the callback list matches the reference order: 77f110 (781070), 77f110 (401830), 77f090, 77efc0. The unused `keyboard_dik` front-end input was removed. A second `GuestHeap` (handle 0x520000, blocks from 0x60000000) serves the CRT malloc 645002. Oracle `verify_original_system_init.py` (the NOPs written in; Heap/QPC/CRT-malloc/DirectInput fixtures; 780180/77f0a0/793c70/794540/785240 recording fixtures with their fastcall stack pops): **600 cases, 0 mismatches** (the 0x22000-byte pool, the record blocks and heap traces compared), 19 outcome kinds; **9/9 mutants caught**. Checker: 159,580 owned bytes, 0 duplicates; profile-patch checker: 0 unhandled overlaps (77f0d7/77f03e/77f205 marked handled).
    - **Native execution now stops in frame 1 at `40cad0>411d00>781010>77f110`** (the controller callback 77f150 registers), and the survey adds 77efc0 (the mouse callback) and BeginScene's 77dfc0 (live now that 3d128ac is set). Hopper's demo hung three more times this session; bulk reads now go through the owned REA Ghidra runner (`_reference/sadx/ghidra-preloop-requests.json`).
  - **40fdc0's enable bytes under the profile:** the scene producer's `SceneInputProducerState::controller_enabled` is now a per-slot reader (was a 909fb4 array reference); the instructions at 40fe0b/40fe2d are `cmp byte [909fb4/909fb5],1`, whose operands the Input mod's Init repoints to `input::controller_enabled[0/1]` (1000e73c/1000e73d). The original-instruction oracle still reads 909fb4 (1,888 cases, 0 mismatches); `tests/input_chain.cpp` now reads `InputModControllerTable`. The 40fe0d/40fe2f patches are marked handled; 40ef80..40efd8 (inside the replaced EnableController/DisableController) and 40f30c remain to check when those ranges are ported.
  - **Registrations (gate `registrations-95-gate.json`):** REA Ghidra (`preloop-785240`, `preloop-789c50`, `preloop-7959c0` in `ghidra-game-loop-recovery/`, assembly checked). **785240** (77f150's tail jump): the four 0x18-stride words at 3d08428 (`ResetLeafState`, borrowed) = 0, then 77ffc0(785190) at 785261. **7959c0** (`ninja_hook_register_7959c0`): returns when the hook is already in 3d08af0..3d08b2c, else stores it in the first zero slot (nothing when full). **789c50** (781070's last call): 7959c0(789bd0), then SetForegroundWindow([3d0fd30]) (a host no-op). The callback list is now 77f110, 785190 (781070's 77f150), 77f110, 785190 (401830's), 77f090, 77efc0, and the hook table holds 789bd0 — the reference's registrations, so the 780030/7959a0 assumption that the lists differ is retired for these sites (77f048, 77f20f, 785261, 78123a>789c50 are now ported; 78818f, 793e75 and 788007>795420 remain inside unported owners). System-init oracle with the real 785240: **600 cases, 0 mismatches**. **Native execution stops at 40cad0>411d00>781010>77f110** (`jmp 792100`); the survey lists 785190, 77efc0, the 789bd0 hook and BeginScene's 77dfc0 next.
  - **Per-frame callbacks and hooks (gate `frame-callbacks-96-gate.json`):** REA Hopper (`ev_acb5a7fd…` 77f110 = `jmp 792100`, `ev_4355ef3f…` 77efc0 = `jmp 791df0`, `ev_ef189f68…` 789bd0, `ev_5e04ff13…` 785190, `ev_4f30de4f…` 792100, `ev_b078f952…` 791df0) and REA Ghidra on the Input DLL (`ev_76cfd0e6…` 10008d80 = `rumble::pdVibMxStop_r`).
    - `game_window.hpp/.cpp` ports the **789bd0 message pump**: PeekMessageA(&msg, [3d0fd30], 0, 0, PM_REMOVE), Translate/DispatchMessageA while messages arrive (rereading 3d0fd30), then 3d08534 = msg.wParam. When the first peek finds nothing the MSG buffer was never written, so 3d08534 takes an uninitialised stack word, which the native owner reports as **unknown**. Headless, the host queue is empty (reported); dispatched messages would reach the window procedure 401900 (a nested owner). Oracle (IAT fixtures 7db1ec/7db1fc/7db1f8, a scripted queue, stack garbage under the MSG local): **2,000 cases, 0 mismatches**, 578 explicit uninitialised cases checked as "unknown"; **3/3 mutants caught**.
    - `ninja_vibration.hpp/.cpp` ports **785190** (the per-frame vibration update over the 3d08428 entries; states 1/2 read the time through 793da0/793e00 and switch to 3 once the duration is reached, unsigned; state 1 starts 791f80(i, min(|strength|·0x594, 10000)) and counts; state 3 stops 792000(i) and returns to 0). Oracle: **4,000 cases, 0 mismatches**; **4/4 mutants caught**.
    - **785330 under the profile:** the Input mod's Jumps send every pdVibMxStop call (40bf40's 40bf51 and 413c00's 413c4c) to `rumble::pdVibMxStop_r` (DLL 10008d80): free each nonzero rumble task in the eight pairs at 1000ec60 through the game's 4b4160 and clear it, then for each DreamPad with nonzero rumble flags (+0x3f) clear the dword at +0x40, clear flag bits 0–1 and call SDL_GameControllerRumble(pad +0x34, 0, 0, 0); EAX = 0. It never touches the game's 3d08428 states — **the vanilla 785330 the runtime used set entry 0's state to 3, which made 785190 call 792000 natively** (a real divergence the survey exposed). `input_mod_vibration_stop` (input_mod_pad) ports it; `InputModDreamPad` gains `rumble_flags`/`rumble_word`; the runtime routes all 785330 calls through it. Oracle on the DLL's own instructions: **3,000 cases, 0 mismatches**; **3/3 mutants caught**; 785330 is marked handled in `profile_patches.json`.
    - `ninja_input_devices.hpp/.cpp` ports **792100** (returns when the joystick count 3d08814 is 0) and **791df0** (returns when the mouse device 3d08834 is null). The Input mod NOPs the device setups 7928e0/791d60, so both return at once; their device bodies (792122, 791e0b) are owners. Oracle: **400 cases, 0 mismatches**.
    - **Native execution now stops in frame 1 at `40cad0>411d00>781050>77dfc0`** (BeginScene's rectangle; live because 78e720 set 3d128ac). The 3ce7164 struct it reads is set by 780c30, which 781070 skips natively, so 780c30 must be ported with 77dfc0.
    - **789bd0 under the profile:** the loader WriteJumps 789bd0 to `HandleWindowMessages_r` (found by `check_profile_patches.py` after the port, which flagged the range UNHANDLED). It peeks with a NULL HWND (every thread window), runs TranslateAcceleratorA(parentWindow, accelTable) first (a handled accelerator skips Translate/Dispatch) and stores the same 3d08534 wParam. `GameMessagePumpProfile {original, mod_loader}` models both; the runtime uses `mod_loader`; the oracle covers the original path, the loader path is from source plus a unit test (no binary oracle). 789bd0 is marked handled in `profile_patches.json`.
  - **Screen setup, BeginScene polygon and D3DX projection (gate `screen-97-gate.json`):** REA Hopper (`ev_4c2882b4…` 77dfc0, `ev_d1afa200…` 78ea10, `ev_841b73bd…` 78dbe0, `ev_89408774…` 791940, `ev_0c108bfb…` 78bba0, `ev_808f1f0f…` 78b780, `ev_7c27121e…` 78b800, `ev_46db0679…` 770295, `ev_8f29afc4…` 770284, `ev_46fccd0e…` 77350c, `ev_0077033a…` 773700, `ev_69f8ce2d…` 773f67, `ev_76cf6972…` 772615, `ev_10f50993…` 774310, `ev_c7ac7947…` 774150, `ev_d1c52bfe…` 7914b0) and REA Ghidra (`ev_6c6c4a9c4…` 780c30, `ev_0db220f9a…` 7912e0, `ev_30fe19ed6…` 77ff70, `ev_a5bb92625…` 780090, `ev_42dd944a8…` 7735f0, `ev_53cc7aa64…` 772615, `ev_c195d08fa…` 791170).
    - `ninja_screen.hpp/.cpp` ports **780c30** (781070's 78113f; arguments config +0x14 flags, +0x18, +0x1c): 3d0f804/3d0f964/3d0f814; 7899a0(+0x1c); 3d0f9e4/3d0fa20 = 640/0x280 when flags & 0x10 else 320/0x140, 3d0f9e8/3d0f838 = 480/0x1e0 when & 0x20 else 240/0xf0; 3d0fa04/3d0fa08 = unsigned viewport width/height, 3d0fa0c/3d0fa10 = half; flags & 0x10000 doubles 3d0f9e4 and sets 3d0f9e8 = 480 only when it equals 240 (FUCOMPP, TEST AH,0x44); 3ce7160 = 2; 3d0f820/824 = 0, 3d0f828 = 3d0f9e4 − 1, 3d0f82c = 3d0f9e8 − 1 (and the TEST AH,0x41 re-stores); 7912e0; 77ec60(0,8), (1,6); **77ff70**(−1, 0) → 3d0f840/3d0f9c4; **780090** → 3ce712c = 0; then 3ce71b0/3ce71a4 and the 3ce7174 colours zeroed and the 77dfc0 block 3ce7164 = {3ce7184, 3ce7174, —, 4} with the points (0,0) (W,0) (W,H) (0,H). Owners: 3d0f9e4 stays `NinjaQueueConsumerState`'s, 3d0fa0c/10 `NinjaSpriteProjection`'s, 3d0f840/9c4 the queue runtime's, 3ce71a4/b0 `NinjaSystemInitState`'s, 3ce7174 `GameViewState`'s (all borrowed).
    - **7912e0**: only when 3d0fd1c == 1 (its sole writer is the loader's `reset_device`, direct3d.cpp:209): the 389d318 copy to 3d0fd80, 3d0fd1c = 0, the 3d0fd20..2c rectangle, SetViewport(3d12780), SetTransform(3, 3d129c0), MultiplyTransform(3, 3d0fd80). `NinjaScreenDevice` carries these methods plus SetVertexShader/DrawPrimitiveUP. It is ported but not yet wired at EndScene's 78b977.
    - **78dbe0**: z = 65535 when w < 0x37800080, else 1/w; returns out.z of D3DXVec3Project((0,0,z), 3d12780, 3d129c0, 389d318, 389d318).
    - **D3DX8 dispatch** (`d3dx_math.hpp/.cpp`): calls go through 389b778, which 77350c fills on first use (copy of 389b860, then 773f67, then 7738e5 for 3DNow!, else 773700 SSE2 or 7735f0 SSE). Both SSE tables give slot 3 (MatrixMultiply, 770804) = **774310** and slot 10 (Vec3TransformCoord, 77020b) = **774150**; slot 14 (Vec3Project, 770295) is the default **772615** on every path. Ported exactly: 774310's row-broadcast mulps/addps order; 774150's movaps/movlps paths (the `test al,0xf` alignment choice) and its RCPPS + one Newton step (2r − (r·v)·r, lane 3); 772615's NULL-argument switch (identity on the stack, single matrices, products) and its x87 viewport map with unsigned FILD/FADD 2^32. **RCPPS is CPU-specific** (Intel and AMD tables differ) and the reference CPU was not captured: the runtime models it as IEEE division (Unicorn's model) and reports this.
    - **77dfc0** as BeginScene's 78b922 reaches it (uiscale rewrites that call to the trampoline's unhooked original — "The backgound rectangle should not be scaled"; marked handled in `profile_patches.json`): −1/arg clamped to 1.0; 791940 (ECX = flags & 0x40: alpha blend + stage-0 ALPHAOP 4 or 3); 78bba0 (SetTexture(0, NULL) when 3d0fdf4 ≠ −1, then 3d0fdf4 = −1, 3d129a0 = 0); 78b780 (CULLMODE 1, LIGHTING 0, COLORVERTEX 1, point MAG/MIN/MIP filters); 78ea10 (count vertices x+0.5, y+0.5, z = 78dbe0(arg), rhw 1, colour; 78bba0; SetVertexShader(0x44); DrawPrimitiveUP(TRIANGLEFAN, count−2, 3d128ac, 0x14)); 78b800 (linear filters). The vertex bytes are carried in the draw command; the 3d128ac heap block's contents are not retained.
    - Oracles: **780c30 3,000 cases, 0 mismatches** (real 7899a0/7912e0/77ec60/77ff70/780090, COM fixture, both 3d0fd1c paths, null device); **77dfc0 3,000 cases, 0 mismatches** (real D3DX path, slots seeded as 77350c leaves them on an SSE CPU); **772615 direct 4,000 cases, 0 mismatches** (all 16 argument/viewport combinations, aligned and misaligned matrices). Limits: Unicorn applies QEMU's x87 NaN rule (larger significand) to SSE operands where hardware returns the first operand's NaN, so cases keep one NaN source (the first oracle run showed exactly that divergence: native kept hardware's rule); Unicorn raises no MXCSR exception flags, so only MXCSR control bits are compared. Mutants: 780c30/77dfc0 **18/21 caught** (survivors equivalent: an identical re-store, the second texture release after −1, an unused alignment flag); D3DX **6/8 caught** (survivors differ only in which of two NaNs propagates).
    - Runtime: 780c30 runs at 78113f; 77dfc0 runs at 78b922 with reads mapped to their owners; the sprite display's `projected_depth` now calls 78dbe0. **3d129c0 stays zero natively** because its writer 791170 is reached from WinMain's **781570**, and WinMain 402c92..402d7c (cliplevel inits, SetClip exports, 781570, 781250, 40e2e0) was never run — now reported as an assumption (it had been skipped silently). Projected depths are therefore NaN until that span is ported.
    - **Native execution now stops in frame 2 at `40cad0>40c090>42c7b0>421180`** (LoadPVM, the loader's `LoadPVM_r`).
  - **EndScene viewport refresh and the 793e70 registration (gate `endscene-98-gate.json`):** REA Ghidra (`ev_d306a0ea5…` 793cc0, `ev_03b0df100…` 787230, `ev_4c48e0c46…` 794950, and this step's 793e70, 77dd90, 77dde0, 794980, 787220, 793b20, 77ddf0, 793c70, 793be0, 793cb0 reads).
    - EndScene's **7912e0** (ported at gate 97) now runs at 78b977; with 3d0fd1c = 0 it returns at once, as on the reference outside a device reset.
    - `ninja_debug_text.hpp/.cpp` ports **793e70** (781070's 78115d): push 78c5a0, call 77ffc0, EAX = 0. 78c5a0 is a bare RET (byte c3), so the runtime runs that frame callback as a no-op. Oracle (real 77ffc0, seeded count and 32 slots): **500 cases, 0 mismatches**. The runtime's 780030 assumption now prints the live callback order: 77f110, 785190, 78c5a0, 77f110, 785190, 77f090, 77efc0 (77f150 runs in both 781070 and 401830).
    - **793cc0 is not ported yet, on purpose.** Its 77f3d0(38a5cf8) selects the debug-font list `{3d088ac, 1}`, whose entry is BSS until **793c70** (401830's 7808a2, skipped natively) → **793be0** builds the font texture: 3d08860 = −2, 3d08864 = −1, 7871a0(3d08874, 389dcf8 bitmap, 0x100×0x80×0x80), 7871c0(3d088ac, 3d08874, −2, 4.0f), 78cbd0(3d08860) when 3d088a0 is 0, 3d088a0 += 1, 3d088b4 = 3d08860. On the reference 78cf20 would otherwise dereference a null TEXMEM. 78cbd0 is the same texture-creation owner LoadPVM's chain needs, so 793c70/793be0/793cc0 belong with the texture-loading step. The rest of 793cc0 is read: 787230 (794980 fog off, 3d0849c = 0), 77dd90(1) (791940, 78b780, SetVertexShader(0x144), stage-0 COLOROP/ALPHAOP args 3), the 3d08854 queue loop (count 3d088bc; 77ddf0 → 78d3e0/78d140, then 793b20 per character → 77e970), 77dde0 (jmp 78b800), 787220 (794950 fog on, 3d0849c = 1), then 3d088bc = 3d088a8 = 0.
  - Next: the macOS keyboard/GameController adapter into `InputModKeyboardPad`/`InputModGamepadState` (exact SDL axis conversion), then real owners for 77f090/4250d0 and an AppKit/Metal app driving the loop phases.
  - Earlier next-step note: a native macOS clock owner (the reference QPC is 10 MHz, so a 10 MHz virtual counter from `mach_absolute_time` gives the reference's exact 158,730-tick 63 FPS interval), then real owners for 77f090/40f460/4250d0 and an AppKit/Metal app driving these phases.

The REA Ghidra runner was relaunched this session (owned `ghidra-live.mjs` with `ghidra-game-loop-config.json`); handle 47648 from the earlier session was not inherited. No Windows capture was requested.

## 1. Original goal and requirements that remain in force

The user asked to decompile/reconstruct the supplied PC Sonic Adventure DX game into buildable source and a native macOS Apple Silicon game, using native GPU rendering, preferably Metal. The supplied target is `Sonic Adventure DX/sonic.exe`, a patched executable with older speedrun-related files in the supplied tree. The user wants **updated required speedrun mods**, compatible mod behavior, **keyboard**, **Xbox controller**, and **Switch 2 Pro controller** support, and a game ready for speedrunning.

The user permits choosing between source integration, a native mod-loader port, or an adapter. The selected approach is **source-integrate the required speedrun behaviors and later expose native interfaces for source-rebuilt extensions and asset/config overrides**. Existing i386 Windows DLLs cannot be directly loaded by an arm64 process. Preserve the modded PC semantics through named native owners; source virtual addresses are evidence locations, not native function pointers.

The accepted precision target is **maximum practical fidelity to the exact supplied executable running with the selected updated mods on native Intel Windows hardware**. Higher GPU performance, a level viewer, a Dreamcast port, an emulator, or successful isolated numerical tests does not satisfy the full objective. Mac development/execution is local and independent of Windows availability; Windows is the verification reference.

Required final outcomes:

- Complete playable native **PC SADX** reconstruction with buildable source, including its characters, stages, gameplay, collision, cameras, animations, objects, menus, saves, audio and video.
- Effective updated speedrun behaviors, including Input, CCEF, EHCF, Skipma and the selected 63 FPS/tick policy, integrated into their real owning engine paths.
- Native GPU presentation and sustained performance, not just offscreen readback.
- Real in-game keyboard and both requested controller-family/transport verification.
- Matching relevant simulation/FP behavior, speedrun tricks, collision, timing/IGT, transitions, cutscenes and routes against the actively modded Windows reference.
- A usable native Mac `.app`, games category/fullscreen support, and an explicit native mod/asset/config strategy.
- Appropriate reference fixtures and stronger whole-game verification before claiming readiness or leaderboard acceptance.

The original wording says “2003”; the investigated PC speedrun resources/loader describe **2004 US**. Resolve this through the exact executable digest and bytes, not an assumed edition label. The supplied `sadx.exe` is a Super Sonic hack launcher and is **not** the chosen target/reference.

Authoritative amendments and obligations: [_reference/sadx/goal-amendments.json](_reference/sadx/goal-amendments.json), [sadx-native/README.md](sadx-native/README.md), [_reference/sadx/goal-obligations/ledger.json](_reference/sadx/goal-obligations/ledger.json). The goal tool only changes status; it cannot rewrite the objective. Six complete-deliverable obligations remain unproven. Obligation Evidence ID: `ev_2145efe7f95b590e0e04d144917e7eae846918bbee8b6794420cec5ce6d1cbb7`.

## 2. Resume checklist: execute in this order

1. Load instructions with `python3 ~/.codex/claude_memory.py --cwd "$PWD" --refresh` at a new task. After compaction/resume, omit `--refresh` to reload full context. Follow root `AGENTS.md`, canonical Claude instructions and relevant nested rules. This handoff supplements them.
2. Inspect `git status --short`, current source and the ledgers below. At handoff, tracked REA source is unchanged; `Sonic Adventure DX/`, `sadx-mod-loader/` and `sadx-native/` are untracked. The handoff itself is a requested new root Markdown file. No commit/push was requested.
3. Revalidate any session/process handle before using it. Root's Ghidra interactive runner **47648** was confirmed live during handoff; it is the sole active provider. Worker processes/oracles are terminal. Tool handles may not survive a new conversation or environment.
4. **Rebuild the current production archive after the raw-MDATA animation repair.** The 64-program gate predates the repair, newer render/queue interfaces, texture selection and proposed controller work. Preserve failed/historical reports; do not relabel them as current success.
5. Refresh canonical repaired animation blend, updater, startup/Sonic and affected queue/render comparisons against the newly built probes. Repaired private blend has zero stored differences, but canonical production blend still contains the pre-repair 2,808 stored differences. Keep FP-status differences explicit.
6. Integrate privately verified `game_frame_control` and `ninja_texture_selection` only after reconciling the shared source freeze and their current ABI. Neither is in the 64-program gate.
7. Finish root's `game_scene_logic` draft verification. Its first private link failed for missing `X87Context` constructor; therefore its unit/probe/original comparison did **not** run. Link it against a fresh production core archive or the actual x87 dependencies, then execute its tests/oracle. The source is not proven merely because its `.cpp` compiled.
8. Finish the shared controller/input ownership refactor before combining its independent drafts with existing `game_logic` or Sonic. Both old and proposed `GameLogicController` definitions currently exist independently; including them together would conflict. No shared controller refactor was applied at handoff.
9. Finish the queue-consumer/sprite-display tests, original-instruction probes and supported CPU-to-Metal composition. The next known consumer test failure is an **uninitialized test color** before the intended depth boundary, not a producer bug. Establish the explicit fixture color rather than weakening the implementation.
10. Connect actual frame/scene/input/task/Sonic/animation/collision owners into a complete native startup and game loop, then original camera/model/world rendering and a real AppKit/Metal application. Preserve required failures until the owning implementation exists.
11. Prepare real gameplay/reference traces and physical input verification when the native journey exists. Hardware may be disconnected. The user was told no further Windows capture commands were needed **for now**; do not repeat old neutral controller tests or ask for new captures without a concrete missing evidence question.
12. Audit the full original requirements before any completion claim. Keep the goal active while gameplay, presentation, timing, input, packaging, mods or verification remain incomplete.

The previous goal work made concrete source/evidence progress, not just plans. This documentation task did **not** request a goal pause. Workers held source at checkpoints to make this handoff consistent; resume their scopes when appropriate.

## 3. Environment, instructions and operating boundaries

- CWD: `/Users/burbujamc/Developer/rea`; shell zsh; timezone `America/Mexico_City`.
- Writable: workspace, `/private/tmp`, and the supplied temp directory. Canonical Claude memory is outside these roots. Durable technical context is retained here; optional memory duplication was not used to manufacture an approval blocker.
- Macintosh is the personal Mac; DiveMac is the work Mac. Preserve the user's CrossOver/Steam configuration and games. The user rejected pinning the display to 60 Hz.
- Address Tailscale machines by **MagicDNS**, never a literal IP. Windows peer: `burbuja.taile27c1c.ts.net` / `Burbuja`.
- Read-only native analysis is authorized. Source implementation/tests and reversible workspace fixes are authorized. GUI/GPU/JIT/filesystem access can still require the client's actual escalation; use concrete tool approval, and retry actual sandbox/JIT/network failures appropriately.
- Source game bytes remain immutable. Native runtime executes reconstructed native code, not a guest game interpreter. Unicorn is a development comparison harness only.
- Reuse explicit process/resource ownership. A PID/path alone does not establish ownership after exit/reuse. Retain resources/partial state when cleanup or a required owner fails.
- Preserve caller-selected paths, source digests, uncertainty, observed null versus unknown and partial facts. Source addresses/raw pointer words are not native pointers or GPU handles.
- Root REA `AGENTS.md` requires dependencies inward, format-aware parsing, clear semantic/resource ownership, meaningful malformed/cancel/cleanup/capacity/identity tests, and honest verification scope. Native C++ uses tagged result structs and explicit failures.
- Do not commit binaries, original assets, provider databases, credentials, `dist/`, `node_modules/` or local planning artifacts. The user explicitly requested this root handoff; no Git operation is authorized merely by its creation.
- REA reverse-engineering skill was applied: [.agents/skills/reverse-engineer-anything/SKILL.md](.agents/skills/reverse-engineer-anything/SKILL.md). Its step 6 explicitly authorized distinct parallel investigation scopes. Developer policy otherwise prohibits proactive delegation without applicable authorization. Existing workers were `/root/mod_compatibility`, `/root/port_sources`, `/root/transcendental`; do not spawn duplicate providers.
- `diagnosing-bugs` was used for actual collector failures. `writing-for-agents` was used for this handoff. An explicit `.md` request does not invoke Pages.
- Keep reasoning concise and tool work concrete. Earlier goal turns consumed excessive tokens in repetitive internal speculation; preserve current evidence and avoid restarting those loops. Send meaningful user progress updates at least every 60 seconds during work.

## 4. Workspace layout and authority

| Location | Role |
| --- | --- |
| [Sonic Adventure DX](<Sonic Adventure DX/>) | Immutable supplied executable/assets/mod files; exact source target |
| [sadx-mod-loader](sadx-mod-loader/) | Supplied/pinned X-Hax source/SDK, useful layouts/address candidates; i386 Windows ABI |
| [sadx-native](sadx-native/) | Reconstructed native C++17/Objective-C++ code, tests, probes, tools, third-party numerical libraries |
| [_reference/sadx](_reference/sadx/) | Ignored evidence, artifacts, comparisons, source constants, snapshots, ledgers and prepared reference |
| [_reference/sadx/continuation.json](_reference/sadx/continuation.json) | Accumulated navigation/status; some historical keys are stale |
| [_reference/sadx/production-integration-20261012](_reference/sadx/production-integration-20261012/) | Chronological gate logs/hashes and canonical production comparisons |
| [_reference/sadx/updated-pc-reference](_reference/sadx/updated-pc-reference/) | Independent prepared Windows copy, immutable manifest/config/source/archive inventories |
| [_reference/sadx/windows-runtime-captures](_reference/sadx/windows-runtime-captures/) | Derived selected facts from user-supplied native Windows JSON |
| [_reference/sadx/handoff-checkpoints](_reference/sadx/handoff-checkpoints/) | Durable copies of worker checkpoints, input disassemblies and Metal sprite report |
| `/private/tmp/sadx-*` | Private binaries/archives/tests/logs; temporary and potentially disposable |

The executable SHA-256 is **`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`**, 55,549,952 bytes, PE32 i386, base `0x400000`. Extractor/oracles reject differing bytes. Original character data `system/CHRMODELS_orig.dll` is **`76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`**, 7,647,232 bytes.

Treat current source and exact producer/probe hashes as authority. Older prose is often historical. In particular:

- `port-sources.md` describes the first binary64 approximation; current core uses explicit software ext80. It is historical, not current numerical policy.
- Early prepared-reference docs say “not executed”; the later user Windows snapshots establish running process/module/hook observations.
- `continuation.json.latest_verification` still describes a **53-program baseline**, with older private identities and obsolete Windows flags. Use newer gate files and current comparisons.
- Earlier `player_shadow` naming was corrected to **player_auxiliary_effect**. Old report filenames remain historical; source aliases were removed.
- Test counts 49, 53, 55, 56, 58, 60 and 64 refer to different frozen gates, not interchangeable claims.
- Worker reports sometimes say native Windows is unobserved within their new input scope. Global module/hook/thread snapshots exist; new physical-input/tick/math traces do not.

## 5. Build, runtime and verification state

Current root Makefile uses `xcrun clang++`, C++17, arm64, `-O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math`, minimum macOS 14. Objective-C++ uses ARC. SoftFloat/Bochs are built locally from retained source. Root REA Node/npm tooling follows `.nvmrc`/`packageManager`; native builds use Xcode/Python.

```sh
make -C sadx-native -j4 all test probes build/metal-texture-test
make -C sadx-native render
make -C sadx-native controllers
```

`all` builds `build/libsadx-core.a`, `build/libsadx-macos.a`, and `build/sadx-metal`. Tests/probes link the production core. The Metal stage executable is **offscreen, untextured Emerald Coast inspection geometry plus controller enumeration**, with no gameplay window or game loop. `controllers` is enumeration, not in-game hardware support. Separate foreground controller tester `.app`s were diagnostic only.

### Frozen shared gates

All under `_reference/sadx/production-integration-20261012/`:

| Gate | Native programs | Scope |
| --- | ---: | --- |
| `gate.json` | 55 | Auxiliary rename, grounded actions, real position binding |
| `texture-gate.json` | 56 | Original texture decoder |
| `render-gate.json` | 58 | Ninja blend and local sprite math/vertices |
| `game-logic-gate.json` | 60 | `413dd0` plus byte-edge/tick helpers |
| `task-order-gate.json` | 60 | Source task list order 7 then 0..6; Metal dependency include |
| `animation-queue-gate.json` | 64 | Queue producer and three animation units, **before raw-count repair/new shared interfaces** |
| `metal-texture-gate.json` | separate | Production Metal texture build and real bubble GPU readback |

Each has logs and exact source/artifact hashes. The 64 gate command was `make -C sadx-native -j4 all test probes build/metal-texture-test`; four Python extraction tests passed. Its first attempt failed an aggregate initializer warning; root fixed **`PlayerPhysicsState.animation_tail = {}`** at the owner, preserving optional/unknown tail facts rather than initializing them to observed zero. Actual calloc later establishes known zeros/nulls.

Current source includes post-gate repaired animation and renewed render/queue APIs. Fresh build/canonical comparison is required. New `game_frame_control`, `game_scene_logic`, `ninja_texture_selection`, controller/input drafts and display/consumer/Metal sprite code are **not** advertised by the 64 gate.

### Comparison commands and pitfalls

Python harness: `_reference/sadx/python/bin/python`, Unicorn **2.1.4**. Run from repo root:

```sh
_reference/sadx/python/bin/python sadx-native/tools/verify_original_player_animation_blend.py \
  "Sonic Adventure DX/sonic.exe" sadx-native/build/native-player-animation-blend-probe.dylib \
  --output _reference/sadx/production-integration-20261012/original-player-animation-blend-repaired-comparison.json
```

Analogous scripts exist for every probe; consult `--help`/source for exact args. `_reference/sadx/refresh-audit-reports.mjs` runs four independent requests from `refresh-audit-reports.json`, retains per-key `/private/tmp/sadx-ghidra-audit-*.log`, writes `refresh-audit-results.json`, and permits expected exit 0/1. The **configuration is mutable and currently contains the last selected batch**, not an evergreen exhaustive test suite. Save an old config/results before replacing them.

Unicorn JIT can terminate with exit 132 inside sandbox; rerun with authorized escalation, not altered math. Shell redirection can break an otherwise approved prefix match. Native Metal may report no device in the sandbox; actual M5 Pro tests used the authorized host environment.

SDK MCP client **2.3.1** takes `callTool(params, {timeout: ...})` as **two arguments**. A previous three-argument call silently lost the intended timeout and was fixed. An observation timeout is not a dead analysis process.

Boolean `passed=false` may intentionally retain status/nonfinite/extended-register disagreements even when stored outputs match. Conversely, a verifier may exit 0 for its stored-state gate while strict equality remains false. Always inspect the report fields and scope. Unit success is not hardware parity. Rebuilt probes may have different identities despite unchanged old semantics; historical reports remain tied to tested bytes.

## 6. Numerical implementation and retained differences

The core uses explicit `X87Context` and `SseContext`, caller-owned precision/rounding/masks, integer software ext80, real memory-operand semantics and explicit binary32/64 stores. It preserves source operation/spill order and partial mutations. It is **native code**, not a runtime x86 interpreter.

- Berkeley SoftFloat ext80 subset: BSD notices/source retained under `third_party/softfloat`.
- Separate SSE/8086-SSE SoftFloat subset: explicit MXCSR rounding, DAZ/FTZ, exceptions under `third_party/softfloat_sse`.
- Bochs P5/P6 integer FPATAN model under `third_party/bochs_fpatan`; **LGPL** code/notices in addition to BSD SoftFloat. Intel Ultra hardware equality remains unmeasured. Preserve notices and audit packaging obligations.
- Original quarter-sine table at **`0x88af78`**, **1025 f32 / 4100 bytes**, SHA **`3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed`**. FNV test identity `b047ecf880b993c2`. Interpolation uses source table and angle wrap, not host analytic sine/tangent.
- All 65,536 sin/cos integer angles were checked across 12 PC/rounding profiles. Earlier binary64 cancellation loss was fixed with ext80.
- Dot and squared-length summation orders differ and are preserved. Signed zero, subnormal, infinities, NaNs, payloads and temporary CW changes have distinct semantics.
- A signaling-NaN memory-operand bug was fixed at the arithmetic producer: an early native load had quieted it before the source combined operation.
- Some Unicorn FLD/FST/precision/exception/NaN behavior disagrees with documented hardware. These differences remain recorded; native code was not weakened to fit emulator behavior.
- CRT integer truncation, floor, sqrt, fabs, asin/acos and atan2 have distinct source dispatcher/control/error identities. Preserve source temporary precision/rounding and restored caller CW. Fabs/asin/floor/atan2 full comparisons remain false in some profiles; seeded suffix/dispatcher-bypass success does not close their whole contract.
- FPATAN approximation is a named model, not proof of physical transcendental results. Avoid a process-wide host FP default or host `atan2`/`tan` substitution.

Primary provenance: `x87-provenance.md`, `transcendental-provenance.md`, `crt-*-provenance.md`, `sse-math` and `input-mod-axes` reports, plus per-player numeric provenance indexed in the appendices.

## 7. Task, player, collision and Sonic source already reconstructed

### Task/resource semantics

`TaskScheduler` owns eight lists and the actual 4096-slot source pool/arena. Source task record is 0x34 bytes; native pointers/handles are distinct from source serialization. `run_index(8)` aliases list 7. Parent `RunTask` **`0x40b170` calls index 8 first, then 0..6**, not 0..7. `run_game_tasks()` now implements that order. Unit coverage includes same-frame append from list7 into list0 and stopping later lists after a list7 failure.

Walkers preserve when NEXT is sampled, callback mutations, child-only traversal, nullable callbacks, current-function/field30 contexts and cleanup ownership. `FreeTask` schedules destruction; it is not immediate host retirement. Interrupted cleanup retains owned resources and a resumption stage. Generic source arena bytes are allocation/provenance once typed live views are bound; they are not a second independently updated pose. Callback failures keep lower diagnostics. A PID or bare source address is not a resource identity.

Ghidra confirmed dedicated auxiliary task work is a distinct taskwk union. Treating taskwk+8 as player state corrupted the overlapping resource field and was removed. Source field30 remains owned/opaque, not an invented layout.

### Player numeric/control owners

Existing modules cover source parameter rows (8×132 bytes), startup prefix/registration/allocation, GetAnalog, WriteAnalogs/Input2.6.1 magnitude hook, coordinates/matrices, speed/heading, ground/air/friction/alternate/event acceleration, rotations, axis/floor projection, CRTs, path/direction reset, hold query, contact initialization/attributes/response, mesh/plane/bounds/query/registry and full local scene position bodies/helpers.

Shared `PlayerTaskState`, `PlayerPhysicsState`, `PlayerMotionState`, `PlayerEventState` and `PlayerParameters` preserve one mutable owner. Unknown fields, observed null resources, unresolved references and output-store masks stay distinct. Native callers cannot substitute identity transforms, flat floors, guessed event values, copied stale parameter state or successful missing callbacks.

Important source findings and retained checks:

| Slice | Evidence/result and scope |
| --- | --- |
| Movement composition | 49,380 instruction evaluations with real input/coordinate/math owners; stored velocities/matrices/calls match; status differences retained |
| Input producer | 121,200 evaluations; raw/normalized words match original+actual Input2.6.1 hook; enabled `==1` vs mod `!=0` preserved |
| Input Mod scalar axes | 109,056 actual-DLL evaluations; values match, MXCSR/status differences retained |
| Heading | 4,960 angle-policy +15,780 heading/matrix cases |
| Rotation | 30,456 stored/matrix comparisons |
| Air acceleration | 29,952 stored/call/matrix comparisons |
| Axis/projection/angle/cross | 39,240 scoped comparisons |
| Alternate attrs | 59,904 stored/call/control cases |
| Event ground/air | 44,256 evaluations; 24 NaN-sign disagreements retained |
| Local PSetPosition | 23,808 scoped parent cases; some callees were explicit fixtures, not real geometry |
| Collision shapes / initializer | 43,344 /57,600 scoped stored-result cases; malformed/required boundaries retained |
| Hold query | 133,410 source cases including 16 fault cases; live registered task views |
| Contact attributes | 105,852 stored/boundary/control comparisons; friction uses **BL bit7**, not decompiler full-word sign |
| Contact response | Five real owners `43cce0,43cf90,43d180,43d8b0,43d360`; 61,800 stores/returns/CW match; 50,196 status differences |
| Full local scene position | `449500/445d10`, 1,632 cases, 1,152 contacts; stores/returns/CW/frames match; 1,344 status differences |
| Scene helpers | 8,076 comparisons, 6,040 status differences |
| Actual ECoast composition | Native loader→registry→broadphase/query→response, not selected-node or flat-floor callbacks |

`player_scene_position_binding` passes **136 real Emerald Coast PSetPosition paths /96 retained responses**, forwards one live parameter owner and caller context, and preserves successful/failed scene diagnostics through later Sonic failures. Renderer `44499e` and nonnull moving-platform callbacks remain required. Geometry/source attrs `0x400002` filter water/horizontal records for ordinary Sonic; character byte 9==6 fixtures cover Gamma explicitly, not an invented universal floor.

Ghidra retrospective fixes: registry initializer `451030` returns EAX **1**; floor-to-world `43edd0` copies input before `4b8e80` may alias it; new typed source fields receive zero/byte facts only from actual producers; auxiliary task uses its proper work union. See `ghidra-retrospective-audit.md`, registry/contact audit files and retained dossiers.

### Sonic and auxiliary effect

Sonic main is at **`0x49a9b0`**. Initial real Hopper Evidence: `ev_770cabb55d61b4a23c3627b0124d5a90bdf99e7ea931779db6179db1ee3190c9`; its initial dossier contained 84 textual direct-call candidates. The actual action table at `49bfbc` has 88 entries /352 bytes. Full Main/common-tail/render/model/sound/all-action closure is still incomplete.

- Control/movement slice: **28,800** evaluations, no stored differences, **2,840** status differences. Includes explicit seeded suffix/prefix scopes, not a whole game tick.
- Grounded/running actions 1–5 and bounded jump/spindash predicates: **30,192** cases, zero stores/boundaries, **2,172** status differences. Exact mutable masks/raw held/pressed, live actor/hold/event/table facts and source timer widths are retained.
- Important local owners include `43bf40,4b83f0,4b8320,495fa0,492f90,494f70,495e60,493120,496ee0,442570,442680,494ff0,4429c0,43bfe0,4930d0`; source charge/action/sound/effects requiring unavailable owners remain tagged.
- Source call interface has explicit scalar slots and task-work argument identity, not a numeric guest-pointer cast. Source actor registration/live values outrank historical captured values when live facts are unknown.
- `player_auxiliary_effect` (formerly mislabeled shadow) covers `4a2b60→4a2ae0`, lookup `4144e0/441970`, update prefix `4a2570`, cleanup `4a0720/4a0760`, source table `9275a0` 1080 bytes, callback ownership and lifecycle. **516** stored/boundary/callback/CW comparisons match; **60** status differences remain.
- Its selected asset is **EFF_REGULAR bubble ordinal130/global0x184f**, frame `9279d8`, not established character-shadow rendering. Full effect update/display remains unfinished.
- Startup prefix has **2,004** state/resource/boundary/CW comparisons, **924** status differences; no complete startup return was observed. Actual successful 512-byte calloc produces newly known animation-tail zero/null fields, described below.

## 8. Animation: repair and current ABI are essential

Sources: `player_animation_state.hpp`, `player_animation.hpp/.cpp`, `player_animation_blend.cpp`, `player_animation_decode.hpp/.cpp`, `sonic_animation_table.hpp/.cpp`, generated `sonic_animation_table_data.hpp`, `sonic_animation_operations.hpp`, their emitters/tests/probes, and Sonic integration. Current repaired producers are frozen; **post-repair canonical archive/report refresh remains outstanding**.

### Actual resource facts

- Original `CHRMODELS_orig.dll` export `___SONIC_ACTIONS`: **149 export slots**, **146 distinct actions**, **139 motions**, **223 shared SceneGeometryObject records**, **3,650 key arrays**.
- Executable initializer **`7cec90`** is **7,674 bytes**, **882 actual MOV assignments**, **147 PL_ACTION records**. SDK count146 is not authoritative. **103** full-table source fixtures match bytes/FP.
- `3c56210` lies in executable virtual zero-fill; it is not readable as initialized file-backed data. `401ac0` actually resolves `___SONIC_ACTIONS` into global `38f6e7c`; `7cec90` builds descriptors. Source bytes/exports do not prove prior runtime initialization consumed them.
- The decoder resolves actual PE export names, preserves artifact-scoped IDs/file ranges, and reuses `SceneGeometryObject` rather than cloning a model graph. No DLL execution or source-pointer/native-pointer cast.
- `PlayerPhysicsState` owns existing words120/122/124/126/128. Tail owns only next12a, timer12c, flags12e, frame130, start134, table140 and buffer148 plus allocated resource types. `speed138/self13c` use actual startup arena aliases/live state; copied speed/self aliases were removed.
- Generic tail default `= {}` means unknown optionals. Successful **512-byte source calloc** establishes scalar zeros and **observed-null** table140/buffer148. Actual table assignment occurs in Sonic initialization with a caller-provided initialized table.
- Complete local `44a800` modes1–12 and `43f230/446910/43f9f0` tested in **16,440** cases: zero stored/boundary differences, **4,140** FP-status differences. These profiles excluded/unbound some blend/key-allocation work and are not full-game animation proof.

### The raw MDATA layout quirk and why old green units were insufficient

The historical/debug and first optimized canonical **`43f2a0`** comparisons found **7,944 cases, 2,808 stored differences, 1,032 status differences**. Both failed reports are preserved. The debug/native fixture rebuilt a 16 MiB arena each call, taking ~0.169 s/case and ~1,334 s total; CPU-live waiting was legitimate. Optimized canonical ran in ~4.39 s. Neither was restarted merely for elapsed time.

Actual `43f39c` reads **old node+8 as position count** even when old type7 uses **MDATA3**, where that word is a **scale pointer**. `43f549` consumes old node+0xc as angle count, which is MDATA3's position count. The normalized native key-count representation had accidentally repaired source behavior. Source can read beyond the declared position-key array into adjacent captured bytes; returning zero/clamping/wrapping by a normalized count is wrong.

The repaired representation retains consumed raw pointer/count words separately from normalized resource identities and supplies **one immutable caller-owned contiguous byte backing** for each key view. It performs bounded per-word reads with source operation/spill order, retains earlier output writes, and stops when required backing is unavailable. It does not invent adjacent memory, apply a native pointer cast, or assume uncaptured runtime mutations/relocations equal file bytes.

Repaired private v2 comparison: **7,944 cases, zero stored/CW differences, 1,668 low-six status differences**; strict `passed=false` remains. Regression: a capture restricted to 16 declared keys stops at actual next-frame load **`43f3cc`** after retaining frame0; explicitly supplied contiguous PE byte backing completes.

Actual outgoing type7 records: **19,27,47,58,59,61,66,67,68,142** (9 distinct motions). All ten real transitions into record0 were exercised at frame0.5 and nbFrame−0.5; optimized/UBSan real-DLL lifecycle tests pass across 12 CW profiles. See `animation-mdata3-quirk-records.json`, `player-animation-provenance.md`, `player-animation-ledger.json`, and preserved pre-repair ledger.

Canonical status at handoff:

- `production-integration-20261012/original-player-animation-blend-comparison.json` **still fails with the pre-repair 2,808 values**.
- `original-player-animation-blend-repaired-v2-comparison.json` is repaired **private** proof, not the current production artifact.
- Rebuild archive/probe and write a **new** canonical repaired report; keep both failed baselines.
- Current optimized/UBSan units validate resource transitions and cleanup but do not establish hardware FP flags, complete model rendering or the full Sonic common tail.

## 9. Input: implemented policy, physical evidence, and held refactor

### Existing shared input/adapter

`InputState` supports atomic keyboard/controller rebinding, physical/action separation, focus release, previous-held/pressed/released state, 8192-step Input2.6.1 keyboard ramp, signed direction and separate magnitude, and explicit software SSE sampling. `mac_input` translates real AppKit physical keycodes and stable GameController captures. **52 DLL keyboard goldens** pass through both shared state and Cocoa event objects.

Default bindings: arrows movement; J/L/I/M camera; X/Z/A/S actions; Return Start; Q/W camera triggers. Original Windows VK settings and SDL/native physical scancode namespaces are distinct. Apple normalized axes already have backend deadzone behavior and are **not** raw signed16 SDL axes; original-controller/hardware parity is unverified. Triggers use trunc(value×255)>threshold30. Face legends are physical-position labels, not automatic action swaps.

### Physical observations and deferral

- Xbox Series X physical USB: VID **045e**, PID **0b12**.
- Switch 2 Pro USB: VID **057e**, PID **2069**.
- Apple synthetic GamePad-1: VID045e/PID028e; not interchangeable with physical Xbox identity.
- Escalated nonexclusive IOHID open succeeded after sandbox failure0xe00002e2. Both devices satisfied supportsHIDDevice, which does not prove input events.
- A live Xbox extended profile was observed and its **neutral** snapshot accepted. No Switch 2 GameController object appeared in the observed interval.
- Windowless 300 s and foreground ~1,005 s diagnostic attempts captured no requested button/stick changes. User reported attempts; Xbox triggers were explicitly **not tested**. Those reports are not captured events.
- Source Switch descriptor reportID9 has four12-bit axes0..4095 and21 buttons. Cached zero values are not calibrated neutral/input proof. Xbox physical node lacked expected elements in one enumeration.
- Access differs by app identity: headless Granted0 versus foreground Unknown2. No permission request/change was performed; this does not prove a permission cause.
- Original tester was closed normally and logs retained. Prepared `ControllerTesterModern.app` is **unlaunched**. No listener/tester is left running. User may disconnect; avoid another unprepared capture request.
- Pinned SDL Switch2 driver commit `930cd0dbf8d1b6fdc5b91711d8d71542c1df0960` contains bulk/calibration/rumble/unknown-purpose initialization. It was inspected, **not executed or incorporated**. Identify exact transport/interface/endpoint/purpose/cleanup before any future handshake; do not wholesale copy its initialization.

Details and hashes: `controller-physical-provenance.md`, `connected-controller-hid-summary.json`, source plist and terminal JSONL logs. Diagnostic sample rates20/30 Hz are not game ticks or63 FPS proof.

### Current independent controller/input drafts: shared refactor NOT applied

Files `src/game_controller.hpp/.cpp`, `src/scene_input_producer.hpp/.cpp` strict-compile privately. **No new units, UBSan, original-instruction probe/comparison or ledger exists yet. `40fdc0` orchestration is not implemented.** Shared `game_logic`, Sonic controls and `player_input_producer` remain unchanged by this proposal.

The new controller draft preserves one **52-byte optional-byte record** with LE16/32 accessors; raw pointer words are opaque. Four optional raw controller refs and eight distinct engine-copy records correspond to real different storage. Existing `game_logic.hpp` still separately defines the old pressed-only `GameLogicController`; **do not include both definitions together**. The planned shared move/consumer adaptation is a deliberate pending operation.

Proposed next refactor:

1. Move the old definition to common `game_controller.hpp`; adapt413dd0 to read/write pressed at+0x10 on the sole record.
2. SonicControls borrows engine pads and one common mutable `GameControllerButtonMasks`, replacing copied held/pressed/mask arrays. Raw controllers remain distinct from engine pads at3b0e7f0.
3. WriteAnalogs uses borrowed raw-controller axes/control/enable views, preserving original909fb4 enable table versus patched InputMod table as distinct facts.
4. Implement40fdc0: exact4146b0 classifier→ordered909f94/fa4 stores→control==1→eight52-byte records; slots0/1 copy **13 DWORDs in forward source order** only when original enabled byte==1. Other slots or disabled control perform actual clear stores.
5. Clear preserves offsets+0,+4,+24,+28,+30; previous held+8 copies to+2c; then held0/offFFFFFFFF/pressed0/released0 and halfword zeros in order **1c,1e,1a,18,20,22**.
6. Invoke real40f170/InputMod magnitude owner, then40f070 slots0..3. Preserve aliasing, partial-copy facts, unknown/null and actual gating.

Independent40f070 draft: signed16 **Y+1e thenX+1c**, strict **±78** thresholds, directional-work stores before held+8/pressed+10 reads, signed32 repeat wrap with **>30 then−6**, control gate **!=0**. Known-null raw pointer leaves previous work/output untouched; unknown pointer is a boundary.40fdc0 and original WriteAnalogs instead require control **==1**.4146b0 returns1 only for stages27/28/29/2a. Physical mappings and complete composition remain unverified.

Durable draft disassembly copies: `handoff-checkpoints/sadx-scene-input-producer.asm`, `sadx-engine-pad-keys.asm`. Source Evidence:40fdc0 `ev_1b6db6b69540aa46e3646e6fd9cd0a059d82f7a61c9ccca304f20d2ffdacd627`;40f070 `ev_56c2f9ae5888c58debd7d47ac6fe8b4cb0c5cf92ef01923ceac8505d249ba742`.

## 10. Game-loop and frame-control progress

### Integrated local game logic

`game_logic_support` owns actual **40ec80** raw byte aggregation and **413920** tick-count calculation. One `GameLogicTimingState` owns words1178/117c/1180/1188/118c/1198 and2a2e4; outer413dd0 borrows it. Edge state owns256×3 raw optional bytes,256 aggregate bytes and separate3b0e3ba/3b0eae8 bytes; raw source namespace has no physical-key interpretation.

- 40ec80 modes0 copy,1 accumulate,2 restore; other raw mode values are complete no-op. Exact unrolled prefetch-before-current-store order preserves partial missing facts.
-413920 uses signed IDIV, INC and low32 IMUL wrap, and the actual source **six-tick clamp**. Divide zero/INT_MIN÷−1 are explicit faults, not guessed ticks.
- Final source comparison **1,737 edge profiles +4,117 timing profiles**, zero state/presence/boundary/trace differences; **288 expected divide faults** (256 zero +32 overflow). Earlier64 verifier address differences were corrected at40ecd6/40ece4/40ecf2, not patched into source math. Fresh CPU context per experiment avoids Unicorn carrying a fault into a later double fault.
- `game_logic` implements full local **413dd0** ordering: initial presses, edge0, activation stores, calculated scene ticks, input repoll+edge1 on extra ticks, tail mode change, press union, edge2. **170** original-instruction fixture cases match. Required scene/input/mode owners remain separate.
- Real helpers compose through one borrowed state; optimized/UBSan tests include retained divide-failure diagnostic, pointer replacement, cancellation/partial calls, and true task ordering.

### Privately verified frame control, not yet integrated

`game_frame_control.hpp/.cpp`, unit, probe and `verify_original_game_frame_control.py` cover **411e30**, **7899a0/77ed40**, **40f010**, **411e20**. **2,672** comparisons have zero stores/callback-identity/boundary differences; optimized and UBSan pass.

- Original multiplier helper reads frequency **high then low before FrameMultiplier store**; computes low64 multiplication and signed64 division by **60**. That legacy interval is distinct from the updated mod63 limiter. No captured Windows frequency or host clock is silently selected.
- Set-mode stores timing1188,1178 and pending117c0 before later requirements. Branches1/2 read renderFactor89295c; selected multiplier failure preserves both outer/nested source boundaries. It installs a real native callback, not a guest-address cast.
-411e20 calls actual792f60 (returns0) then tail-jumps40f010. That updates flags3b0eaa0 from savePhase3b291a4, mask3b0e3b8 and **existing edge59 lane0** at3b0e493. Optional missing facts remain explicit.
-40f010 initial raw word3b0e760 affects only incidental EAX return; actual780074 immediately overwrites EAX after callback, so it is excluded from the void observable model.
- Verifier fixes: data pages derive from actual field addresses; low-frequency read occurs after PUSH high word, so nested caller return is **ESP+4**, not ESP. These were harness mapping/diagnostic errors; native stored results matched.
- Renderer owner currently owns only frequency words3d0853c/38, FrameMultiplier389d7dc, interval3d084a8/ac, renderFactor89295c and callback3d0fa34. Input state owns flags/mask/save phase and borrows edge state. It does not duplicate queue globals909eb4/3abdcfe.

### New scene orchestration draft: next fix is linkage, not a claimed pass

`game_scene_logic.hpp/.cpp`, `tests/game_scene_logic.cpp`, `native_game_scene_logic_probe.cpp`, `verify_original_game_scene_logic.py` were just created. `.cpp` strict-compiles. Private unit and dylib link commands omitted the real **X87Context constructor**, failed, and created no executables. The attempted oracle then failed `dlopen` for missing `/private/tmp/sadx-game-scene-logic-probe.dylib`. **No original scene comparison or unit success exists.**

The draft implements local **413d40**:40fdc0→4014b0→conditional5919c0→real task order40b170→420700→456ec0→4511d0→42fb00→conditional426070 and two output bytes. Stage is borrowed as raw uint16 and compared as signed16. Source reads stage before query-result branch, reloads it only after426070, and rereads mode for the final conditional. Full input/context/collision/query owners remain required; empty tasks/external owner fixtures in the proposed oracle do not prove whole scene execution.

After a fresh core build, a suitable private link is:

```sh
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -arch arm64 -Isadx-native/src \
  sadx-native/tests/game_scene_logic.cpp sadx-native/src/game_scene_logic.cpp \
  sadx-native/build/libsadx-core.a -o /private/tmp/sadx-game-scene-logic-test
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -arch arm64 -dynamiclib -Isadx-native/src \
  sadx-native/tests/native_game_scene_logic_probe.cpp sadx-native/src/game_scene_logic.cpp \
  sadx-native/build/libsadx-core.a -o /private/tmp/sadx-game-scene-logic-probe.dylib
```

Run the executable before its oracle. Resolve actual new failures rather than recording these suggested commands as completed tests.

### Larger loop still unfinished

Actual `GameLoop40cad0`, `InitGameLoop40c030`, `411d00` frame completion, `413f00` post-logic wrappers and remaining owners are retained Ghidra source. FrameMultiplier update, intentional extra-tick policy, asynchronous input/presentation, save/UI branches, camera/collision/render/audio and complete startup are not fully connected. The63 source hook replaces411d1c with max(0,FrameMultiplier−1), not lag recovery; the surrounding original routine still owns state and presentation calls.

## 11. Textures, rendering and queue ownership

### Original assets and decoder

`ninja_texture` parses PVMH/GBIX/PVRT with file-range/ordinal/raw metadata ownership, preserves unknown chunks/padding and partial parsed prefixes. Needed profile:16-bit1555/565/4444, rectangle9, twiddled1, twiddled-mip2, VQ3; unsupported formats are explicit.

**532 actual records /540 levels** from EFF_REGULAR, BEACH01/02/03, OBJ_BEACH, BEACH_SEA, BG_BEACH, EC_CLOUDS match actual source metadata/packed16 instructions and declared PuyoTools integer RGBA policy.16 synthetic PVM flag profiles also match. Source PVM low bits are filename1, formats2, dimensions4, globalIndex8; primary PuyoTools named flags differed, so source bytes won.

PVRT parser **77f2f0** sets payload at chunk+16, lengthword−8. Bubble ordinal130/global0x184f is64×64, pixel0/data9, chunk8224B at0xda730;8192B pixels start0xda740 and16B unused tail remains. Never guessed payload alignment from nominal size. Source texture0x88ad... etc covered78bc30/78bd10/78bf50/78c570/78ca00.78c570 was undefined in Ghidra, but29 actual instructions were directly compared; no reimport was needed just for that leaf.

RGBA8 is an explicit **integer floor255/max policy**, not original GPU normalized precision/filter equivalence. Preserve packed16 texels as fidelity input.

### Real Metal texture upload

`metal_texture.hpp/.mm` is in macOS archive; `tests/metal_texture.mm` builds `build/metal-texture-test`. It owns one archive/record/resource, uploads packed16 **R16Uint**, normalizes in safe/precise shader to **RGBA32Float**, preserves mips and validates native queue/device/readiness/failure ownership.

Actual **Apple M5 Pro** GPU:532 records,540 levels, **16,883,028 FP32 channels**, zero differences from the declared float-ratio model; EFF_REGULAR also ran UBSan. Production bubble alone has16,384 matching channels and real readback/PNG under `production-integration-20261012/bubble-metal.*`. All-archive reports are in `metal-texture-readback/`. These establish native upload/normalization/readback, not Windows D3D filtering/color or whole-game rendering. Legacy macOS14 math path was not separately observed.

### Ninja geometry/render/queue producers

- `ninja_render` models actual4030d0→77ec60 packed blend, ordered native device commands, ignored/omitted HRESULTs and explicit missing owners. **6,160** comparison cases match. It now exposes one source-site-aware issue owner and distinct D3D8 texture-stage contract; reused depth/color methods retain order. These post-64-gate changes need production repinning.
- `ninja_sprite` covers77e3c9..77e5c2 local geometry and77e63c..77e703 /77e733..77e7ff vertex stores. **2,868** profiles: no local values/boundaries/CW differences; **24 seeded signaling-depth vertex differences** and **2,760 status differences** retained. Actual UV scale is rounded **1/255**, not1/256.
- Draw packets preserve112 bytes, source FVF, strip primitive5/count2 and stride24/28. Source angle is not consumed by77e390 and was not invented.
-`ninja_sprite_queue` owns caller-sized arena with **32-byte alignment**, real overflow log order,152-byte record/header capture,2048 sort buckets/sentinels, stable insert order and resource lifetimes. **17,664** source evaluations +256 immediate-state/color +63 texture-gate cases match stored/bucket/link/trace/CW; **17,256 FP-status differences** remain. Strict false.
- Arena header semantics differ from TaskWorkArena; no allocator reuse with wrong headers. Null allocation gives **immediate_required**, not successful rendering. Buckets unknown before explicit source initialization; resetting reachability does not fabricate resource cleanup.
- Queue copy/move deleted: sharing mutable record links across duplicated bucket owners was unsafe. Typed header+c texture-list capture now lives at its producer alongside raw provenance. Raw0 establishes null; nonzero unbound stays unknown; conflicting nullness fails **after raw store** with partial bytes retained. A downstream repair map was rejected.
- Lazy bucket-front/owned-record queries allow NEXT to be read **after draw callback408f24**, rather than prevalidating a snapshot and missing mutations.

### Texture selection: privately verified, not yet integrated

`ninja_texture_selection` reuses sole `NinjaTextureListOwner/NinjaTextureMemoryReference`, keeps raw source words separate from native ownership, and models77f3d0/77f440/78cf20/78cdc0/78b840. **683** profiles match state/calls/boundaries; optimized/UBSan pass. Palette unchanged/no-bank paths close; dirty revision store is retained and actual surface lock remains unavailable at **78ce5a**. Full texture allocation/refcounts/cache cleanup are not inferred.78b840 is exactly render states(22,1),(137,0),(145,1), not a guessed cache entry.

### New display/consumer/Metal sprite drafts held for handoff

Files `ninja_sprite_display.hpp/.cpp`, `ninja_queue_consumer.hpp/.cpp`, `metal_sprite.hpp/.mm`, tests/fixture files are **unintegrated WIP**. Detailed durable checkpoint: [handoff-checkpoints/queue-consumer-checkpoint.md](_reference/sadx/handoff-checkpoints/queue-consumer-checkpoint.md).

Actual4086f0 traverses buckets0..2047, optional two passes from909eb4 bits, captured state per record, then final side effects. Type3 dispatch408ab6→408b1f→77e390. New CPU draft handles supported null/default/type3 branches, matrices/colors/flags and partial failures; other type1/2/4..15 bodies remain unavailable. Final412400 transforms saved3b12208 as **(signed>>1)&63**, not exact restoration.411f30 invokes actual51f680RET despite misleading Ghidra switch-label names. Fog/FOV/buffer lifecycle remain incomplete.

Known pending CPU unit: first real source zero-depth sprite path passes, next standalone missing-depth fixture stops earlier at **77e53b missing color**. Set its explicit `projection.color_argb_at_3d0f7f0` first, remove temporary stderr diagnostic, then exercise intended77e637 depth failure and remaining mutation/two-pass tests. Do not bypass the correct producer failure. No reusable original77e390/4086f0 verifier exists yet.

Actual77e61c TEST AH,05 / JP: LESS alone takes **FLDzero**; greater/equal/unordered need actual78dbe0 after reciprocal FDIV/FST32. World FVF142 stops at791a50 absent transform owners. Source78c5a0 is RET. Address U/V use clamp3. Missing projection is not replaced by0,1/z,host divide or identity.

D3DX depth closure: generic772615 calls CPU-dispatched matrix/transform leaves. Static dispatch default770284→77350c; feature7/3DNow substitutes77bb57, unavailable on reported Intel. Intel/SSE2 leaves774310/774150 were retained after a seeded reimport; **774150 uses RCPPS plus Newton refinement, not DIVPS**. No approximate-reciprocal model/runtime-dispatch readback is established. Preserve that unknown; even inferred Intel features do not prove actual selected table/environment route.

Actual Metal sprite fixture supports finite screen FVF144/RHW1 strips, validates typed upload-owner/archive/ordinal/surface binding and explicit D3D8 state/filter/depth/blend maps, translates D3D8 integer pixel centers to Metal half-integer centers, and retains queued resources through real GPU completion. **16,384** bubble channels match packed16 ratios; draw ordering/LESS rejection/source-alpha blending and missing/partial-prefix tests pass on M5 Pro. Report copied durably to `handoff-checkpoints/metal-sprite-inspection.json`. This is an inspection fixture, not original Windows raster equality or full queue/game presentation. No arbitrary void cast or automatic bubble-index fallback.

Pending source dossiers/lifecycle:408630,407ab0,407330,412400; fog787240→7949b0,787250→794e40,787260.408630 allocates0x88000 then407ab0; reset aligns/zeroes buffer, clears warning, calls4128f0/407330 and resets display/counters. No such lifecycle is invented in the current consumer. Source7815c0 FOV uses angle÷2 truncation, Ninja sine f32 spill/cos ext80 and FDIVR; preserve source table/context, not host tan.

## 12. Updated mod/reference strategy and exact versions

Investigated resources: `https://www.speedrun.com/sadx/resources`, approved2018 bundle `/a5jc4`, PC executable `/zqvoh` (download returned403 during equality investigation). Original supplied hashes/versions in `local-inventory.json`, `mod-compatibility.md`. Public searches were bounded; they do not prove no other unpublished native project exists.

| Component selected on 2026-10-11 | Revision | Selected DLL SHA-256 |
| --- | --- | --- |
| X-Hax loader676 | eb4cdcca9637d611a98b910fcfed6ce18580451c | 3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba |
| Input2.6.1 | 68e2a91bec4a55967674c1420ecd9abf89bcd0ce | c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5 |
| Speedrun2.0 | 1833a11fccb93b96c15eb044d4c8b628039e6f8a | f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802 |
| SDL2 dependency | exact retained file | 25003b5bb2bda0d5b558d357cd8eb6c96fa20d776b815950b46ff3444791d1cf |

Release assets were queried live then pinned with archives/metadata/digests; a version tag alone is insufficient (Speedrun asset changed2025-02-20). Recheck current releases only when updating/releasing the profile, without silently changing the baseline.

Original supplied loader is a dirty3.7 build/master6edf5371+, not presumed2018; Input already2.6.1; separate old CCEF/EHCF/Good63 artifacts exist. No original loader activation config was found. Latest Speedrun2.0 integrates CCEF/EHCF/Skipma/limiter; enabling duplicates would change ownership.

Prepared isolated copy manifest SHA: **795918d0e40d30437a0fde905d40917cac861ef46b3e86dff0cffdac901f8cd8**, 6,272 game files. Original selected source copy scope6,229 files /1,393,992,524 bytes; independent bytes, no hard/symlinks back. Excluded alternate/hack launchers/oldmods, retained supplied saves/data. Static verifier: `python3 _reference/sadx/updated-pc-reference/verify_profile.py`.

Profile `updated-speedrun.json`: enabled order **Input then Speedrun**; built-in loader input=false and XInputFix=false. CCEF=true, Skipma=true, FreeCam=false, QuickSave=false, ForceLoad=false (source reads **ForceLoad**, schema says ForceReload). FrameLimit string**63FPS** parses `stoi` prefix63. EHCF unconditional. Chao stat viewer/Gamma timer setup unconditional; their effects are part of reference expectations.

Graphics640×480, backend0/DirectX8, Vsync=false, AA/AF0, forced filters/mips/UI scale=false, border disabled, resizablefalse, pause-on-inactive=true, background/FMV fill2/1. BASSmusic/SFX=false, SEVolume100, text/voice1. `sonicDX.ini` preserved exactly; audio/voice files were not removed. All18 optional loader patches false, EnabledGamePatches empty; no Codes.dat/Patches.dat inherited, no trainers/unlimited lives/custom physics/automatic save reloaders.

Input all8 slots: deadzones7849/8689, radialLtrue/Rfalse, threshold30, rumble1/MegaRumblefalse/minTime0, keyboardPlayer0, mouseDisabled/debugfalse, explicit VK defaults. No mapping DB was inferred for future devices.

Latest loader still unconditionally redirects files/bootstrap, patches windows/buffers, depth pointer, file selection/language/E camera/sound/texture/model registries, code/event/input/control dispatch and some rendering behavior. All optional patchesfalse does **not** mean vanilla. Key static sites and full source audit are in updated-reference README/sources: bootstrap401ae1/2, mod init402513/game loop40c045, window789e50/resolution40297a, depth38a5db8→38a5d94, dispatch426063/40fdb3/42f1c5/42f1e9/40ff00, GVR/Tails filter87cba8/ac. Angle interpolation defaults disabled until helper users enable it.

`speedrun` native helpers preserve CCEF OR**0x80000004**, Skipma held-Start/injected pressed-Start and movie heldA/Start checks, floor(FrameMultiplier×freq/cadence)60/63 interval and disabled lag recovery with intentional extra ticks. Surrounding camera/cutscene/clock/Egg Hornet owners remain required; static NOP patches cannot be transplanted into absent native functions. Leaderboard/community acceptance of the exact latest profile/native port is **unverified**; no moderator messages were sent.

## 13. Native Windows reference, collector repair and Tailscale

### Transfer and collector

Windows copy user ran: `C:\Users\maqui\Downloads\SADX-Windows-Reference\SADX-PC\game\sonic.exe` directly, no manager. ZIP `windows-transfer/SADX-Windows-Reference.zip`: **910,129,922 bytes**, SHA **ddfa7146721f717d2ee3bd464a5aaa8517eb32c7d3e83edcda26e1b31a52d13e**. Original ZIP preserved. Fix1 11,264B; Fix2 12,235B.

Canonical collector `updated-pc-reference/capture-runtime.ps1` SHA **3601c69deb0d9051cf86c4d83260637ae65b42b0aad75032558ce88cd780a7a0**; embedded C# SHA **bb474f4715f0c1e5525faeab0a34f148a6add1ba25f8a49237c0a87cddbd8c69**. C#5 warning-as-error/layout/rel32/14 device-path tests ran locally; actual Windows APIs were validated by the user's successful capture, not Mac tests.

Actual failures fixed at their producers:

1. PowerShell5.1 parameter default Join-Path `$PSScriptRoot` ran before root binding; use empty default then resolve in body.
2. ConvertFrom-Json top array piped into `@(...)` created nested inventory and false hash mismatch; direct `[object[]]` assignment fixes shape.
3. Toolhelp MODULE|MODULE32 reports executable twice; retain raw records and derive distinct mappings only when exact normalized path/base/size/hash agree. Conflicting hashes remain errors.
4. Intel `SysWOW64\DriverStore` reported path may not exist physically; resolve from observed module base using **GetMappedFileNameW + QueryDosDevice**, retain raw/resolved/device paths and actual errors. Added read-only PROCESS_QUERY_INFORMATION0x400, not memory write/injection.
5. Print real errors inline. Collector metadata backups and bundle-index hashes updated; immutable game manifest unchanged.

Collector does not launch/terminate/inject/write target memory or take cadence traces. Default thread suspension=false; explicit `-CaptureThreadState` briefly suspends each thread and perturbs execution. Capture clock windows are not simulation-tick traces. Runtime command line was not collected; runtime working directory not verified; disk hashes do not prove entire loaded-image identity; configs at capture do not prove Init-consumed values.

### Two distinct successful snapshots

Full original JSON bytes are **not retained as acquired report files**. User supplied JSON in conversation; selected facts were saved as **derived user-provided observations**, not REA acquisition. Do not invent original file digests/Evidence IDs or merge processes.

| Capture | Identity | Results |
| --- | --- | --- |
|2026-10-12T00:30:47.6897729Z..00:30:52.0303603Z | PID16480, creationFILETIME134362370837469313; PS5.1.26100.9168 | required6 modules and13 hooks match, end identity true, errors/cleanup empty; no thread sampling |
|2026-10-12T00:39:44.4668152Z..00:39:49.8089781Z | PID17384, creation134362391497835450; PS5.1.26100.9444 | same selected required facts, new mappings/system DLL hashes;39 thread contexts captured/resumed/closed, end identity true, errors/cleanup empty |

Files: `user-supplied-20261012-003047-selected-facts.json`, `user-supplied-20261012-003944-fp-selected-facts.json`. System update/version changes and ASLR mappings are separate identities.

Hardware: Intel **Core Ultra7 155H**,16 cores22 logical, Windows11 Home Single Language10.0.26200/build26200/x64, native machine8664 and process014c (WOW64), QPC**10,000,000 Hz**. Installed RTX4060 Laptop driver32.0.15.9282 and Intel Arc32.0.101.6556. **Selected game GPU unknown**; Intel driver DLL presence is not proof of selected adapter. AtlasOS is user-reported, not independently detected. Architecture excludes ARM64 translation, not every VM possibility.

All6 required module path/current-disk-hash checks true: sonic, CHRMODELS loader proxy, CHRMODELS_orig, Input, SDL2, Speedrun. Two raw sonic records represent one distinct mapping. All13 live16B readbacks match expected prefixes/relative-jump ownership:

| VA | Operation / owner |
| --- | --- |
|434870,438330 | CCEF camera OR /Speedrun |
|533939,440ef5 | EHCF NOP3/NOP2 /Speedrun |
|431517,5132b1 | in-game/movie Skipma jumps /Speedrun |
|7899e0 | frame limiter trampoline /Speedrun |
|411d1c | extra-tick policy jump /Speedrun |
|7dce58 | actual scalar63.0f (`00007c42`) /Speedrun |
|40f343,40f451 | analog/controller-init hooks /Input |
|40f460 | skip original controller update→40fdb3 /Input |
|40fdb3 | input event dispatch /loader |

00:39 mappings: Input726c0000, Speedrun726a0000, proxy72b60000; jump examples726c4b70/4be0,726a10f0/11d0/7a10/7a40, loader72bc3f70. Earlier00:30 bases were different and must not be transplanted.

### Floating-point observations and interpretation

Earliest thread17388 created18.1µs after PID17384 creation: CW**007f** =24-bit/nearest/all six exceptions masked; FSW0020 stickyPE, legacyTAGffff/all8 slots empty. MXCSR**1fa1** =control1f80/status21 (invalid+precision), DAZ/FTZoff. Other38 threads CW**027f** =53-bit/nearest/masked; MXCSRcontrol1f80,34 status0,4 precision1fa0. Exception flags are accumulated observations, not initialization defaults. Every sampled thread had suspend count0/resume count1 and successful resume/close/context/flags/clock.

First32 FXSAVE bytes: `7f0020000000000066ea7800000000000000000000000000a11f0000ffff0000`; FIP**78ea66** is actual **FSTP f32[ESI−0xc]** inside78ea10, called77e040 in77dfc0 2D triangle-fan rendering. Current EIP774c2f0e is ntdll wait. Earliest/main-render attribution is a supported inference, **not a physics/FPATAN sample**. CW may have changed after saved FIP; empty-tag register bytes are not live ext80 values.

Source CRT6466ca→callback2bcbdd0→643e5f→6462d0→64c57b/64c549 selects53-bit and FNCLEX. CreateDevice794060/794000 behaviors40/80/20 omit FPU_PRESERVE bit2; loader forwards behavior unchanged. Microsoft documents single/nearest Direct3D default, supporting the causal hypothesis for main24-bit, not proving its full execution history. Temporary CRT inverse/floor/integer controls remain separate and restore caller state. A linear disassembly FLDCW candidate522a04 was jump-table data, not a control owner.

Keep per-thread/per-phase `X87Context/SseContext`; use capture-tied candidate-main007f/1f80 comparison profile, retain all other12 tests, avoid global default changes or copying sticky status. Hardware math, gameplay controls across ticks and source DLL behavior still need stronger traces. Full analysis: `fp-control-ownership.md/.asm/-source.json`.

### Tailscale and remote authority

Last established peer **Burbuja** online; Fix2 Taildrop succeeded with:

```sh
/Applications/Tailscale.app/Contents/MacOS/Tailscale file cp \
  _reference/sadx/windows-transfer/SADX-Collector-Fix2.zip burbuja:
```

MagicDNS host `burbuja.taile27c1c.ts.net`. System DNS/nc failed; native `tailscale nc burbuja 22` resolved but timed out/502. **SSH/authenticated remote command execution was never established.** No SSH/service/firewall/DNS installation/change. Taildrop receipt does not authorize arbitrary Windows execution or prove a received report file. `file get` fetches all pending files; avoid scooping unrelated incoming transfers. Actual original report Taildrop was not obtained. User was told no more capture commands needed for now.

## 14. REA/Ghidra workflow and live resource checkpoint

REA published **6.4.0**, installed under `/private/tmp/sadx-rea/node_modules`, actual MCP catalog**170 tools**. Repo also6.4.0. The investigation used real Hopper/Ghidra evidence, xrefs, instruction bytes, call paths, p-code/value flow, snapshots/bundles and obligation ledgers. Simulated providers do not prove original behavior.

Hopper6.6 demo was installed with user authorization under personal Applications. IDA was not used; any attached GUI database stays read-only/open. Ghidra is portable **12.1.4** under `_reference/sadx/tools/ghidra_12.1.4_PUBLIC`; official archive569,649,598B, SHA `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db`. Original native decompiler did not support this host; official C++ was compiled with Xcode C++11 to `os/mac_arm_64/decompile`, SHA `54f4f60ebf68a5197ab211515723d56aaa497e28b7bc61a544fdebefce72008b`. Existing Java:

`/opt/homebrew/Cellar/openjdk/27/libexec/openjdk.jdk/Contents/Home`

Session-local config only: GHIDRA_INSTALL_DIR, JAVA_HOME, maxheap2G, startup timeout600000, explicit seed file. No unrelated Homebrew/Node/npm/Java/Ghidra upgrade or persistent shell/client change.

### One root-owned interactive provider, confirmed live at handoff

Tool exec session **47648**:

```sh
node _reference/sadx/ghidra-live.mjs _reference/sadx/ghidra-game-loop-config.json
```

Config directory `ghidra-game-loop-recovery`, snapshot `sonic-ghidra-game-loop-snapshot.json`, environment `tools/ghidra-game-loop-environment.json`, seeds `ghidra-game-loop-seeds.tsv`. Future session must revalidate47648; if it is absent/terminal, launch a fresh owned runner with matching config. Opening snapshot does not persist a real Ghidra database; first uncached query may reimport ~173–180 s. **Do not restart on observation timeout.**

`ghidra-live.mjs` stdin accepts:

```json
{"tool":"analyze_function","args":{"procedure":"0x40cad0"},"label":"a-unique-lowercase-label"}
{"tool":"batch","requests_file":"_reference/sadx/your-fixed-requests.json"}
{"tool":"export","label":"your-complete-snapshot"}
{"tool":"close"}
```

Supported static calls include analyze_function, inspect_native_instruction, resolve_native_call_targets, trace_native_values, xrefs, binary_session. Batch file avoids oversized PTY commands. Keep requests immutable once queued; one earlier edited batch was already loaded, so actual Evidence `parameters`/canonical procedure and `*-executed-requests.json` are authority over a later request-file edit.

Tool `isError` is retained/nonfatal; invalid JSON retained/nonfatal; transport failure closes in finally. Results `.json` and pseudocode files live in configured folder; exports preserve complete records. Function-address requests can resolve a containing function:401c00/401d20 both canonically401ac0. Seed entries are **caller assertions**, not Ghidra discoveries. Source byte/CFG/instruction facts decide correctness; custom-register aliases/pseudocode can be wrong.

Full496f50 dossier exceeded10MiB MCP transport after successful analysis. It remains retained complete in exported gameplay bundle; `sonic-action-checks-complete.evidence.json` extracted with genuine Evidence, pseudocode/body preserved, p-code truncation explicit. Avoid repeating large analysis solely for client transport truncation; use retained views/export.

Latest game-loop folder has original40cad0/40c030/411d00/413dd0/413d40/413f00, animation7cec90, SSE774310/774150, generic77080a, source exports401ac0,40ec80/413920/40be40, mode411e30/77ed40/77ff50, callbacks and xrefs, scene40fdc0/4014b0/42fb00/426070/40f070, consumer/fog dossiers. Some short bodies411e20/40f010 remain undefined in Ghidra; exact LLVM instructions were used instead. Earlier matching short-function failures do not justify silent source omission.

Prior closed/reusable evidence folders: `ghidra-session`, `ghidra-engine-recovery`, `ghidra-retrospective-recovery`, `ghidra-scene-helper-recovery`, `ghidra-shadow-recovery`, `ghidra-live-recovery`, `ghidra-gameplay-live-recovery`, `ghidra-action-fp-recovery`. Old gameplay/action-FP interactive runners are terminal; last action-FP close succeeded/exit0 and saved snapshot.

Close an owned provider with its protocol when finished. An early cleanup warning from sysctl/EPERM required checking actual JVM/private-runtime removal; successful process exit alone is not all cleanup evidence. Preserve warnings, retain ownership on failed cleanup, and avoid killing an arbitrary PID/path. No provider GUI was controlled and original executable bytes were not modified.

## 15. Missing work and completion audit

The source and tests are substantial but **the full deliverable remains missing**. Remaining work includes:

- Complete original startup/bootstrap, initialized scene/global/resource producers and full task/frame loop, including real input, collision, render/audio owners.
- Complete Sonic Main/action cases/common tail/sound/effects and all other characters, bosses, objects, stages, SET-driven behavior and game-mode transitions.
- Original camera/camera reset/persistence, source control gates, relevant math/transcendental/hardware boundary behavior.
- Character/basic/chunk model mesh decoding, skin/key/motion draw composition, stage materials/UV/textures/fog/lighting/blend/state, world transforms/projected-depth model, stable queue dispatch and buffer lifecycle.
- Complete Metal window/presentation/app event loop, focus/cadence, frame limiting/tick policy and sustained native performance; current offscreen/inspection code does not supply these.
- Audio/music/voices/SFX, movies/cutscene eligibility, menus/HUD, saves and original save/category fixtures. Source refcount/cache/resource cleanup and native mod/asset/config extension contracts.
- Actual Xbox/Switch2 USB protocol/backend input delivery, calibrated axes/deadzones/buttons/triggers/rumble and native in-game action verification; physical keyboard delivery/rebinding/config persistence.
- Windows game-boundary/tick traces with selected updated mods, raw input, action/position/velocity/collision/RNG/IGT/QPC/cadence, route/trick/skip/death/transition fixtures. Disk snapshots and independent function oracles are weaker evidence.
- Source licensing/packaging notices and a usable native games-category `.app`; no binaries/assets/provider databases should be committed casually.
- Current upstream resource/version/community acceptance checks before any “ready for speedruns” claim. Do not contact moderators without explicit user authority.

A full completion audit must match each original requirement to current authoritative evidence of its actual scope, not substitute current finite tests. If unresolved, continue work; do not mark goal complete/blocked merely because it is large. Only explicit user pause authorizes goal status paused. This handoff task did not pause it.

## 16. Source reuse investigations and boundaries

Checked revisions (bounded negative search): FraGag/sadx_decomp `ca40cd33e71e04fd21d3a4ff159ca80d5b5780dd` is CHRMODELS data, not full PC engine; doldecomp/sadx `4c2b833be9b4b63d57d761beb647cb8d9acbedb6` is limited GameCube/C-runtime work; SARecomp `d5288fe64e68711dd9f0e6718c1d0fb2eeca92f2` is Dreamcast PAL beta, different guest foundation with missing clean bootstrap/generated inputs/AVX2-FMA host constraints; sonicretro/sad_disasm `78386b53816129ff8d37ef1bce756613fdb5de0d` is SH-4 structural landtable reference. None inspected supplies a complete buildable native **PC** engine for this artifact. Licenses and target differences remain separate from useful structural references. Existing SDK declarations sometimes disagree; original bytes outrank names/counts.

## 17. Checkpoint and evidence navigation rules

- Read a subsystem's provenance/ledger when modifying it; compare actual hashes and scope before relying on old “passed” metadata.
- Use current `Makefile`/source to discover commands and arities. Read indexed reports' `limits`, `source_files/native_sources`, tested library hash, case counts and mismatch classes. Cases `.bin` files and full Evidence bundles preserve details absent from prose.
- Keep old failed probes/reports and the source version that generated them. Update current canonical outputs only after actual new producer verification, with distinct names/history when needed.
- `/private/tmp` paths are scratch. Critical worker checkpoint/Metal JSON/input listings were copied to `_reference/sadx/handoff-checkpoints/`; other private executables can be rebuilt from pinned source.
- Do not print giant inventory/bundle JSON wholesale. Transfer index alone is extremely large. Select relevant JSON fields/retained pages, and retain complete records on disk.
- Some helpers have no separate same-named header. Make's generic object rule is `build/%.o: src/%.cpp $(CORE_HEADERS) Makefile`; old duplicate-header prerequisite was removed. `.d` includes now cover core plus `mac_input.d` and `metal_texture.d`.
- Report in plain language what changed, why, how verified, and practical limits. Distinguish observations, derivations, inferences and unknowns.

The appendices below are generated from the handoff-time tree to make all current implementation/evidence locations discoverable. They are a snapshot, not perpetual automatic validation.

### Exact integration additions still required

- Repaired animation: current Make already includes `player_animation`, `player_animation_blend`, `player_animation_decode`, `sonic_animation_table` and their three probes; rebuild rather than duplicating module names. Unit args are original DLL + `player-animation-constants.bin`12B (f32 at7dcb2c/7dcb10/7dcb7c), decoder DLL-only, initializer no args.
- Frame control: add CORE/PROBE `game_frame_control`; unit has no asset arguments. Verifier EXE, dylib, `--output` as shown for other modules. It is currently privately verified only.
- Scene logic: add CORE/PROBE `game_scene_logic` only after linking/testing its draft. Unit has no asset args; proposed oracle uses empty real task lists and declared external receipts. Include actual x87/core dependencies.
- Texture selection: CORE/PROBE `ninja_texture_selection`; unit no args; verifier EXE,dylib,`--output`. Privately683 cases proved, not part of historical64 gate.
- Queue consumer: proposed unit args are sprite40B,blend56B,depth32B,trig4100B,unit64B,texture-return16B at389d458,consumer8B at7dcb0c+7dce50. Only first five are currently extracted by root Make assets; last two are private scratch artifacts and need source-bound extraction when released.
- `ninja_sprite_display`, `ninja_queue_consumer` and `metal_sprite` are WIP; use the durable worker checkpoint before adding build/test targets. Do not equate the passing Metal fixture with released CPU source conformance.
- Controller/input: independent `game_controller` and `scene_input_producer` cannot be wired by simply adding CORE names. First remove the duplicate class/old snapshot interfaces and adapt every producer/consumer/probe in the coordinated refactor described above.

Sonic constants52B are f32 words at7dcb2c,7dfff0,7dd554,7e0090,7dd57c,7df1bc,7e01a8,7dcb94,7e0844,7df1c8,7e00f4,7dcb34,7dd550. Sprite constants40B are frame9279d8(20B)+7db2c4/7dce6c/7e9b40/7dcb2c/7dcb10(each4B). Blend tables56B at88ad64 and depth tables32B at88ad9c. Keep extractor, code arities, probe schemas and source hashes consistent.

Game-loop profile source seeds57 entries, all caller-created before analysis; its original import took173.045s. Earlier seeded action-FP/gameplay profiles remain distinct. Direct source/evidence chains and artifact identities, not guessed function names, establish the newer frame/consumer facts. First root scene verification has no `.json` result because it never loaded a linked native dylib.


## Appendix A. Handoff source inventory and current build membership

Exact current source/test/tool hashes and sizes are in [worktree-state-20261012.json](_reference/sadx/handoff-checkpoints/worktree-state-20261012.json). This is a source snapshot, not a new passing build.

At handoff the Makefile CORE_NAMES are:

```text
ninja_math
speedrun
input
task
player_motion
player_state
player_speed
x87_math
x87_transcendental
player_coordinates
player_input
player_input_producer
crt_integer
crt_asin
crt_acos
floor_angles
player_movement
player_heading
player_acceleration_ground
sse_math
input_mod_axes
player_rotation
player_acceleration_air
player_projection
player_acceleration_friction
player_acceleration_alternate
player_acceleration_alternate_math
player_position
crt_floor
player_contact_initializer
collision_shapes
collision_normal
player_path
player_direction_reset
player_scene_broadphase
player_scene_decode
player_scene_registration
crt_sqrt
crt_fabs
crt_argument
task_work
player_mesh_math
player_mesh_matrix
player_plane
player_reaction
crt_atan2
player_mesh_query
player_hold
player_mesh_registry
player_startup
player_contact_attributes
player_contact_response
player_auxiliary_effect
sonic_player
player_scene_position
player_scene_position_helpers
player_scene_position_binding
ninja_texture
ninja_render
ninja_sprite
game_logic_support
game_logic
ninja_sprite_queue
player_animation
player_animation_blend
player_animation_decode
sonic_animation_table
```

At handoff the Makefile PROBE_NAMES are:

```text
math
player_speed
player_coordinates
player_input
player_input_producer
crt_integer
floor_angles
player_movement
player_heading
ground_acceleration
x87_memory
sse_math
input_mod_axes
player_rotation
air_acceleration
axis_projection
projection_angle
alternate_acceleration
player_position
crt_floor
player_contact_initializer
collision_shapes
player_path
player_direction_reset
player_scene
crt_sqrt
crt_fabs
crt_asin_binary64
task_work
task_lifecycle
player_mesh_math
player_plane
player_reaction
crt_atan2
player_mesh_query
player_hold
player_mesh_registry
player_contact_attributes
player_startup
player_contact_response
player_auxiliary_effect
sonic_player
player_scene_position
player_scene_helpers
sonic_action
ninja_texture
ninja_render
ninja_sprite
game_logic_support
game_logic
ninja_sprite_queue
player_animation
player_animation_blend
sonic_animation_table
```

All current source filenames (paired headers/implementation are intentionally listed, including unintegrated drafts):

```text
sadx-native/src/collision_normal.cpp
sadx-native/src/collision_normal.hpp
sadx-native/src/collision_shapes.cpp
sadx-native/src/collision_shapes.hpp
sadx-native/src/crt_acos.cpp
sadx-native/src/crt_acos.hpp
sadx-native/src/crt_argument.cpp
sadx-native/src/crt_argument.hpp
sadx-native/src/crt_asin.cpp
sadx-native/src/crt_asin.hpp
sadx-native/src/crt_atan2.cpp
sadx-native/src/crt_atan2.hpp
sadx-native/src/crt_fabs.cpp
sadx-native/src/crt_fabs.hpp
sadx-native/src/crt_floor.cpp
sadx-native/src/crt_floor.hpp
sadx-native/src/crt_integer.cpp
sadx-native/src/crt_integer.hpp
sadx-native/src/crt_inverse_internal.hpp
sadx-native/src/crt_sqrt.cpp
sadx-native/src/crt_sqrt.hpp
sadx-native/src/floor_angles.cpp
sadx-native/src/floor_angles.hpp
sadx-native/src/game_controller.cpp
sadx-native/src/game_controller.hpp
sadx-native/src/game_frame_control.cpp
sadx-native/src/game_frame_control.hpp
sadx-native/src/game_logic.cpp
sadx-native/src/game_logic.hpp
sadx-native/src/game_logic_support.cpp
sadx-native/src/game_logic_support.hpp
sadx-native/src/game_scene_logic.cpp
sadx-native/src/game_scene_logic.hpp
sadx-native/src/input.cpp
sadx-native/src/input.hpp
sadx-native/src/input_mod_axes.cpp
sadx-native/src/input_mod_axes.hpp
sadx-native/src/mac_controller_probe.mm
sadx-native/src/mac_controller_tester.Info.plist
sadx-native/src/mac_controller_tester.mm
sadx-native/src/mac_gc_only_probe.mm
sadx-native/src/mac_hid_access_probe.mm
sadx-native/src/mac_input.hpp
sadx-native/src/mac_input.mm
sadx-native/src/metal_sprite.hpp
sadx-native/src/metal_sprite.mm
sadx-native/src/metal_stage.mm
sadx-native/src/metal_texture.hpp
sadx-native/src/metal_texture.mm
sadx-native/src/ninja_math.cpp
sadx-native/src/ninja_math.hpp
sadx-native/src/ninja_queue_consumer.cpp
sadx-native/src/ninja_queue_consumer.hpp
sadx-native/src/ninja_render.cpp
sadx-native/src/ninja_render.hpp
sadx-native/src/ninja_sprite.cpp
sadx-native/src/ninja_sprite.hpp
sadx-native/src/ninja_sprite_display.cpp
sadx-native/src/ninja_sprite_display.hpp
sadx-native/src/ninja_sprite_queue.cpp
sadx-native/src/ninja_sprite_queue.hpp
sadx-native/src/ninja_texture.cpp
sadx-native/src/ninja_texture.hpp
sadx-native/src/ninja_texture_selection.cpp
sadx-native/src/ninja_texture_selection.hpp
sadx-native/src/player_acceleration.hpp
sadx-native/src/player_acceleration_air.cpp
sadx-native/src/player_acceleration_air.hpp
sadx-native/src/player_acceleration_alternate.cpp
sadx-native/src/player_acceleration_alternate.hpp
sadx-native/src/player_acceleration_alternate_math.cpp
sadx-native/src/player_acceleration_alternate_math.hpp
sadx-native/src/player_acceleration_friction.cpp
sadx-native/src/player_acceleration_friction.hpp
sadx-native/src/player_acceleration_ground.cpp
sadx-native/src/player_acceleration_ground.hpp
sadx-native/src/player_animation.cpp
sadx-native/src/player_animation.hpp
sadx-native/src/player_animation_blend.cpp
sadx-native/src/player_animation_decode.cpp
sadx-native/src/player_animation_decode.hpp
sadx-native/src/player_animation_state.hpp
sadx-native/src/player_auxiliary_effect.cpp
sadx-native/src/player_auxiliary_effect.hpp
sadx-native/src/player_contact.hpp
sadx-native/src/player_contact_attributes.cpp
sadx-native/src/player_contact_attributes.hpp
sadx-native/src/player_contact_initializer.cpp
sadx-native/src/player_contact_initializer.hpp
sadx-native/src/player_contact_response.cpp
sadx-native/src/player_contact_response.hpp
sadx-native/src/player_coordinates.cpp
sadx-native/src/player_coordinates.hpp
sadx-native/src/player_direction_reset.cpp
sadx-native/src/player_direction_reset.hpp
sadx-native/src/player_heading.cpp
sadx-native/src/player_heading.hpp
sadx-native/src/player_hold.cpp
sadx-native/src/player_hold.hpp
sadx-native/src/player_input.cpp
sadx-native/src/player_input.hpp
sadx-native/src/player_input_producer.cpp
sadx-native/src/player_input_producer.hpp
sadx-native/src/player_mesh_math.cpp
sadx-native/src/player_mesh_math.hpp
sadx-native/src/player_mesh_matrix.cpp
sadx-native/src/player_mesh_matrix.hpp
sadx-native/src/player_mesh_query.cpp
sadx-native/src/player_mesh_query.hpp
sadx-native/src/player_mesh_registry.cpp
sadx-native/src/player_mesh_registry.hpp
sadx-native/src/player_motion.cpp
sadx-native/src/player_motion.hpp
sadx-native/src/player_movement.cpp
sadx-native/src/player_movement.hpp
sadx-native/src/player_path.cpp
sadx-native/src/player_path.hpp
sadx-native/src/player_physics.hpp
sadx-native/src/player_plane.cpp
sadx-native/src/player_plane.hpp
sadx-native/src/player_position.cpp
sadx-native/src/player_position.hpp
sadx-native/src/player_projection.cpp
sadx-native/src/player_projection.hpp
sadx-native/src/player_reaction.cpp
sadx-native/src/player_reaction.hpp
sadx-native/src/player_rotation.cpp
sadx-native/src/player_rotation.hpp
sadx-native/src/player_rotation_result.hpp
sadx-native/src/player_scene_broadphase.cpp
sadx-native/src/player_scene_broadphase.hpp
sadx-native/src/player_scene_decode.cpp
sadx-native/src/player_scene_decode.hpp
sadx-native/src/player_scene_position.cpp
sadx-native/src/player_scene_position.hpp
sadx-native/src/player_scene_position_binding.cpp
sadx-native/src/player_scene_position_binding.hpp
sadx-native/src/player_scene_position_helpers.cpp
sadx-native/src/player_scene_registration.cpp
sadx-native/src/player_scene_registration.hpp
sadx-native/src/player_scene_resources.hpp
sadx-native/src/player_speed.cpp
sadx-native/src/player_speed.hpp
sadx-native/src/player_startup.cpp
sadx-native/src/player_startup.hpp
sadx-native/src/player_state.cpp
sadx-native/src/player_state.hpp
sadx-native/src/player_task.hpp
sadx-native/src/scene_input_producer.cpp
sadx-native/src/scene_input_producer.hpp
sadx-native/src/scene_resource_reference.hpp
sadx-native/src/sonic_animation_operations.hpp
sadx-native/src/sonic_animation_table.cpp
sadx-native/src/sonic_animation_table.hpp
sadx-native/src/sonic_animation_table_data.hpp
sadx-native/src/sonic_player.cpp
sadx-native/src/sonic_player.hpp
sadx-native/src/speedrun.cpp
sadx-native/src/speedrun.hpp
sadx-native/src/sse_math.cpp
sadx-native/src/sse_math.hpp
sadx-native/src/task.cpp
sadx-native/src/task.hpp
sadx-native/src/task_work.cpp
sadx-native/src/task_work.hpp
sadx-native/src/x87_math.cpp
sadx-native/src/x87_math.hpp
sadx-native/src/x87_transcendental.cpp
sadx-native/src/x87_transcendental.hpp
```

Tests/probes: [sadx-native/tests](sadx-native/tests/). Original-instruction verifiers/generators: [sadx-native/tools](sadx-native/tools/). The snapshot JSON lists every current test/tool path and hash; inspect exact argument schemas before running. Generated sonic-animation/matrix/plane source has corresponding emitters; preserve source/emitter/probe identities together.

## Appendix B. All subsystem provenance and ledgers

Open these when changing their subsystem. Historical names/scopes remain explicit; latest source and tested artifact hashes determine applicability.

- [axis-projection-provenance.md](_reference/sadx/axis-projection-provenance.md)
- [collision-normal-ledger.json](_reference/sadx/collision-normal-ledger.json)
- [collision-normal-provenance.md](_reference/sadx/collision-normal-provenance.md)
- [contact-attributes-ghidra-audit.md](_reference/sadx/contact-attributes-ghidra-audit.md)
- [contact-response-ledger.json](_reference/sadx/contact-response-ledger.json)
- [contact-response-provenance.md](_reference/sadx/contact-response-provenance.md)
- [controller-physical-provenance.md](_reference/sadx/controller-physical-provenance.md)
- [crt-asin-binary64-provenance.md](_reference/sadx/crt-asin-binary64-provenance.md)
- [crt-atan2-ledger.json](_reference/sadx/crt-atan2-ledger.json)
- [crt-atan2-provenance.md](_reference/sadx/crt-atan2-provenance.md)
- [crt-fabs-provenance.md](_reference/sadx/crt-fabs-provenance.md)
- [crt-floor-provenance.md](_reference/sadx/crt-floor-provenance.md)
- [crt-floor-review.md](_reference/sadx/crt-floor-review.md)
- [crt-sqrt-provenance.md](_reference/sadx/crt-sqrt-provenance.md)
- [fp-control-ownership.md](_reference/sadx/fp-control-ownership.md)
- [game-frame-control-ledger.json](_reference/sadx/game-frame-control-ledger.json)
- [game-logic-ledger.json](_reference/sadx/game-logic-ledger.json)
- [game-logic-support-ledger.json](_reference/sadx/game-logic-support-ledger.json)
- [game-logic-support-provenance.md](_reference/sadx/game-logic-support-provenance.md)
- [game-task-order-ledger.json](_reference/sadx/game-task-order-ledger.json)
- [ghidra-retrospective-audit.md](_reference/sadx/ghidra-retrospective-audit.md)
- [ghidra-startup-hold-audit.md](_reference/sadx/ghidra-startup-hold-audit.md)
- [ledger.json](_reference/sadx/goal-obligations/ledger.json)
- [input-mod-axes-provenance.md](_reference/sadx/input-mod-axes-provenance.md)
- [input-provenance.md](_reference/sadx/input-provenance.md)
- [input-sampling-provenance.md](_reference/sadx/input-sampling-provenance.md)
- [mac-input-provenance.md](_reference/sadx/mac-input-provenance.md)
- [metal-texture-ledger.json](_reference/sadx/metal-texture-ledger.json)
- [metal-texture-provenance.md](_reference/sadx/metal-texture-provenance.md)
- [mod-compatibility.md](_reference/sadx/mod-compatibility.md)
- [ninja-sprite-provenance.md](_reference/sadx/ninja-sprite-provenance.md)
- [ninja-sprite-queue-provenance.md](_reference/sadx/ninja-sprite-queue-provenance.md)
- [ninja-texture-ledger.json](_reference/sadx/ninja-texture-ledger.json)
- [ninja-texture-provenance.md](_reference/sadx/ninja-texture-provenance.md)
- [ninja-texture-selection-ledger.json](_reference/sadx/ninja-texture-selection-ledger.json)
- [ninja-texture-selection-provenance.md](_reference/sadx/ninja-texture-selection-provenance.md)
- [player-acceleration-alternate-provenance.md](_reference/sadx/player-acceleration-alternate-provenance.md)
- [player-acceleration-ground-provenance.md](_reference/sadx/player-acceleration-ground-provenance.md)
- [player-air-acceleration-provenance.md](_reference/sadx/player-air-acceleration-provenance.md)
- [player-animation-before-raw-count-repair-ledger.json](_reference/sadx/player-animation-before-raw-count-repair-ledger.json)
- [player-animation-ledger.json](_reference/sadx/player-animation-ledger.json)
- [player-animation-provenance.md](_reference/sadx/player-animation-provenance.md)
- [player-auxiliary-effect-provenance.md](_reference/sadx/player-auxiliary-effect-provenance.md)
- [player-contact-attributes-provenance.md](_reference/sadx/player-contact-attributes-provenance.md)
- [player-coordinates-provenance.md](_reference/sadx/player-coordinates-provenance.md)
- [player-heading-provenance.md](_reference/sadx/player-heading-provenance.md)
- [player-hold-ledger.json](_reference/sadx/player-hold-ledger.json)
- [player-hold-provenance.md](_reference/sadx/player-hold-provenance.md)
- [player-input-producer-provenance.md](_reference/sadx/player-input-producer-provenance.md)
- [player-input-provenance.md](_reference/sadx/player-input-provenance.md)
- [player-mesh-query-provenance.md](_reference/sadx/player-mesh-query-provenance.md)
- [player-mesh-registry-ghidra-audit.md](_reference/sadx/player-mesh-registry-ghidra-audit.md)
- [player-mesh-registry-ledger.json](_reference/sadx/player-mesh-registry-ledger.json)
- [player-mesh-registry-provenance.md](_reference/sadx/player-mesh-registry-provenance.md)
- [player-motion-provenance.md](_reference/sadx/player-motion-provenance.md)
- [player-path-reset-provenance.md](_reference/sadx/player-path-reset-provenance.md)
- [player-plane-provenance.md](_reference/sadx/player-plane-provenance.md)
- [player-position-ledger.json](_reference/sadx/player-position-ledger.json)
- [player-position-provenance.md](_reference/sadx/player-position-provenance.md)
- [player-reaction-provenance.md](_reference/sadx/player-reaction-provenance.md)
- [player-rotation-provenance.md](_reference/sadx/player-rotation-provenance.md)
- [player-scene-position-binding-ledger.json](_reference/sadx/player-scene-position-binding-ledger.json)
- [player-scene-position-binding-provenance.md](_reference/sadx/player-scene-position-binding-provenance.md)
- [player-scene-position-ledger.json](_reference/sadx/player-scene-position-ledger.json)
- [player-scene-position-provenance.md](_reference/sadx/player-scene-position-provenance.md)
- [player-scene-provenance.md](_reference/sadx/player-scene-provenance.md)
- [player-shadow-provenance.md](_reference/sadx/player-shadow-provenance.md)
- [player-speed-provenance.md](_reference/sadx/player-speed-provenance.md)
- [player-startup-provenance.md](_reference/sadx/player-startup-provenance.md)
- [port-sources.md](_reference/sadx/port-sources.md)
- [sonic-action-before-ledger.json](_reference/sadx/sonic-action-before-ledger.json)
- [sonic-actions-ledger.json](_reference/sadx/sonic-actions-ledger.json)
- [sonic-actions-provenance.md](_reference/sadx/sonic-actions-provenance.md)
- [sonic-player-ledger.json](_reference/sadx/sonic-player-ledger.json)
- [sonic-player-provenance.md](_reference/sadx/sonic-player-provenance.md)
- [task-lifecycle-provenance.md](_reference/sadx/task-lifecycle-provenance.md)
- [task-provenance.md](_reference/sadx/task-provenance.md)
- [transcendental-provenance.md](_reference/sadx/transcendental-provenance.md)
- [x87-provenance.md](_reference/sadx/x87-provenance.md)

## Appendix C. Retained comparison report inventory

This complete handoff-time inventory keeps status/value distinctions discoverable. Counts are per report scope, not additive whole-game coverage; similarly named variant reports may be historical duplicates. Read full profiles, limits and hashes. A blank scalar summary means results are nested, not that the report passed.

| Report | Selected scalar result fields |
| --- | --- |
| [original-air-acceleration-comparison.json](_reference/sadx/original-air-acceleration-comparison.json) | stored_values_calls_and_matrix_match=true, exception_flags_match=false |
| [original-alternate-acceleration-comparison.json](_reference/sadx/original-alternate-acceleration-comparison.json) | values_calls_cw_match=true |
| [original-attribute-20000-acceleration-comparison.json](_reference/sadx/original-attribute-20000-acceleration-comparison.json) | fixture_count=2496, values_calls_cw_match=true |
| [original-attribute-4000-acceleration-comparison.json](_reference/sadx/original-attribute-4000-acceleration-comparison.json) | fixture_count=2496, values_calls_cw_match=true |
| [original-attribute20000-acceleration-comparison.json](_reference/sadx/original-attribute20000-acceleration-comparison.json) | values_calls_cw_match=true |
| [original-attribute20000-comparison.json](_reference/sadx/original-attribute20000-comparison.json) | fixture_count=2496, values_calls_cw_match=true |
| [original-attribute4000-comparison.json](_reference/sadx/original-attribute4000-comparison.json) | fixture_count=2496, values_calls_cw_match=true |
| [original-axis-projection-comparison.json](_reference/sadx/original-axis-projection-comparison.json) | stored_vectors_and_matrix_match=true |
| [original-collision-shapes-comparison.json](_reference/sadx/original-collision-shapes-comparison.json) | stored_state_matches=true, passed=false |
| [original-contact-response-comparison.json](_reference/sadx/original-contact-response-comparison.json) | cases_per_entry_and_cw=1030, stored_values_return_control_match=true, passed=false |
| [original-crt-asin-binary64-comparison.json](_reference/sadx/original-crt-asin-binary64-comparison.json) | cases=692, returned_values_and_boundaries_match=false, full_emulator_match=false |
| [original-crt-atan2-comparison.json](_reference/sadx/original-crt-atan2-comparison.json) | cases=2368, returned_values_and_boundaries_match=false, full_emulator_match=false, local_control_and_suffix_matches=true |
| [original-crt-fabs-comparison.json](_reference/sadx/original-crt-fabs-comparison.json) | nan_policy=1, returned_values_control_boundaries_match=false, full_emulator_match=false |
| [original-crt-floor-comparison.json](_reference/sadx/original-crt-floor-comparison.json) | returned_values_and_boundaries_match=false, full_emulator_match=false |
| [original-crt-integer-comparison.json](_reference/sadx/original-crt-integer-comparison.json) | tested_values_match=true |
| [original-crt-sqrt-comparison.json](_reference/sadx/original-crt-sqrt-comparison.json) | matherr_control=1, returned_values_control_match=true, full_emulator_match=false |
| [original-event-air-acceleration-comparison.json](_reference/sadx/original-event-air-acceleration-comparison.json) | fixture_count=1844, values_calls_cw_match=false |
| [original-event-ground-acceleration-comparison.json](_reference/sadx/original-event-ground-acceleration-comparison.json) | fixture_count=1844, values_calls_cw_match=false |
| [original-eventair-comparison.json](_reference/sadx/original-eventair-comparison.json) | fixture_count=1844, values_calls_cw_match=false |
| [original-eventground-comparison.json](_reference/sadx/original-eventground-comparison.json) | fixture_count=1844, values_calls_cw_match=false |
| [original-floor-angles-comparison.json](_reference/sadx/original-floor-angles-comparison.json) | floor_output_and_boundary_match=false, asin_extended_register_match=false, emulator_exception_flags_match=false, bypass_floor_output_control_and_boundary_match=true, bypass_asin_extended_register_match=false |
| [original-game-frame-control-comparison.json](_reference/sadx/original-game-frame-control-comparison.json) | cases=2672, stores_callback_identity_and_boundaries_match=true |
| [original-game-logic-comparison.json](_reference/sadx/original-game-logic-comparison.json) | cases=170, orchestration_stores_press_union_calls_match=true |
| [original-game-logic-support-comparison.json](_reference/sadx/original-game-logic-support-comparison.json) | passed=true |
| [original-ground-acceleration-comparison.json](_reference/sadx/original-ground-acceleration-comparison.json) | local_values_and_call_selections_match=true |
| [original-input-mod-axes-comparison.json](_reference/sadx/original-input-mod-axes-comparison.json) | passed=false |
| [original-input-producer-comparison.json](_reference/sadx/original-input-producer-comparison.json) | fixture_count=5050, value_outputs_passed=true, exception_flags_match=false, passed=false |
| [original-math-comparison.json](_reference/sadx/original-math-comparison.json) | tested_profile_matches=true |
| [original-ninja-sprite-queue-comparison.json](_reference/sadx/original-ninja-sprite-queue-comparison.json) | state_buckets_trace_control_match=true, strict_emulator_match=false |
| [original-ninja-texture-comparison.json](_reference/sadx/original-ninja-texture-comparison.json) | packed_and_metadata_match=true, rgba_primary_policy_match=true, passed=true |
| [original-ninja-texture-selection-comparison.json](_reference/sadx/original-ninja-texture-selection-comparison.json) | cases=683, passed=true |
| [original-player-animation-blend-comparison.json](_reference/sadx/original-player-animation-blend-comparison.json) | cases=7944, stored_mismatches=2808, status_mismatches=1032, stored_state_matches=false, passed=false |
| [original-player-animation-blend-repaired-comparison.json](_reference/sadx/original-player-animation-blend-repaired-comparison.json) | cases=7944, stored_mismatches=0, status_mismatches=1668, stored_state_matches=true, passed=false |
| [original-player-animation-blend-repaired-v2-comparison.json](_reference/sadx/original-player-animation-blend-repaired-v2-comparison.json) | cases=7944, stored_mismatches=0, status_mismatches=1668, stored_state_matches=true, passed=false |
| [original-player-animation-comparison.json](_reference/sadx/original-player-animation-comparison.json) | cases=16440, stored_mismatches=0, status_mismatches=4140, boundary_mismatches=0, stored_state_matches=true, passed=false |
| [original-player-auxiliary-effect-comparison.json](_reference/sadx/original-player-auxiliary-effect-comparison.json) | supported_state_boundaries_callbacks_control_match=true, strict_emulator_match=false |
| [original-player-contact-attributes-comparison.json](_reference/sadx/original-player-contact-attributes-comparison.json) | state_boundaries_control_match=true, full_emulator_match=false |
| [original-player-contact-initializer-comparison.json](_reference/sadx/original-player-contact-initializer-comparison.json) | fixture_count=4800, values_cw_match=true |
| [original-player-coordinates-comparison.json](_reference/sadx/original-player-coordinates-comparison.json) | stored_values_and_stack_match=true, emulator_exception_flags_match=false |
| [original-player-direction-reset-comparison.json](_reference/sadx/original-player-direction-reset-comparison.json) | state_work_vectors_and_control_match=true |
| [original-player-heading-comparison.json](_reference/sadx/original-player-heading-comparison.json) | angle_cases=4960, angle_and_stored_heading_matrix_match=true, emulator_exception_flags_match=false |
| [original-player-hold-comparison.json](_reference/sadx/original-player-hold-comparison.json) | cases=133410, fault_cases=16, passed=true |
| [original-player-input-comparison.json](_reference/sadx/original-player-input-comparison.json) | fixture_count=17848, forced_angle_case_count=625, passed=false, value_outputs_passed=true, exception_flags_match=false |
| [original-player-mesh-math-comparison.json](_reference/sadx/original-player-mesh-math-comparison.json) | values_returns_cw_match=true |
| [original-player-mesh-query-comparison.json](_reference/sadx/original-player-mesh-query-comparison.json) | fixtures=331, values_match=true |
| [original-player-mesh-registry-comparison.json](_reference/sadx/original-player-mesh-registry-comparison.json) | values_returns_cw_match=true, strict_match=false |
| [original-player-movement-comparison.json](_reference/sadx/original-player-movement-comparison.json) | fixture_count=4115, crt_matherr_policy=1, stored_values_calls_and_matrix_match=true, emulator_exception_flags_match=false |
| [original-player-path-comparison.json](_reference/sadx/original-player-path-comparison.json) | returned_values_work_and_control_match=true |
| [original-player-plane-comparison.json](_reference/sadx/original-player-plane-comparison.json) | cases=1487, state_returns_control_match=true, full_emulator_match=false |
| [original-player-position-comparison.json](_reference/sadx/original-player-position-comparison.json) | stored_values_match=true, passed=false |
| [original-player-reaction-comparison.json](_reference/sadx/original-player-reaction-comparison.json) | bypass_state_boundaries_control_match=true, bypass_and_native_owner_boundary_evaluations=10152, state_boundaries_control_match=false, full_emulator_match=false |
| [original-player-rotation-comparison.json](_reference/sadx/original-player-rotation-comparison.json) | stored_values_and_matrix_match=true |
| [original-player-scene-comparison.json](_reference/sadx/original-player-scene-comparison.json) | fixture_count=480, values_match=true |
| [original-player-scene-helpers-comparison.json](_reference/sadx/original-player-scene-helpers-comparison.json) | values_cw_frames_match=true, strict_match=false |
| [original-player-scene-position-comparison.json](_reference/sadx/original-player-scene-position-comparison.json) | values_cw_frames_match=true, strict_match=false |
| [original-player-shadow-comparison.json](_reference/sadx/original-player-shadow-comparison.json) | supported_state_boundaries_callbacks_control_match=true, strict_emulator_match=false |
| [original-player-speed-comparison.json](_reference/sadx/original-player-speed-comparison.json) | tested_values_and_calls_match=true, emulator_exception_flags_match=false |
| [original-player-startup-comparison.json](_reference/sadx/original-player-startup-comparison.json) | prefix_state_resources_boundaries_control_match=true, original_return_observed=false, full_emulator_match=false |
| [original-projection-angle-comparison.json](_reference/sadx/original-projection-angle-comparison.json) | return_angle_control_and_cross_stores_length_match=true |
| [original-scene-mesh-bounds-comparison.json](_reference/sadx/original-scene-mesh-bounds-comparison.json) | fixture_count=249, values_match=true |
| [original-sonic-actions-comparison.json](_reference/sadx/original-sonic-actions-comparison.json) | cases=30192, stored_mismatches=0, boundary_mismatches=0, status_mismatches=2172, stored_state_matches=true, passed=false |
| [original-sonic-animation-table-comparison.json](_reference/sadx/original-sonic-animation-table-comparison.json) | records=147, export_slots=149, cases=103, byte_mismatches=0, fp_mismatches=0, passed=true |
| [original-sonic-player-comparison.json](_reference/sadx/original-sonic-player-comparison.json) | cases=28800, stored_mismatches=0, status_mismatches=2840, ground_prefix_cases=4032, ground_prefix_stored_mismatches=0, ground_prefix_status_mismatches=1632, stored_state_matches=true, passed=false |
| [original-task-lifecycle-comparison.json](_reference/sadx/original-task-lifecycle-comparison.json) | values_links_and_bytes_match=true |
| [original-task-work-comparison.json](_reference/sadx/original-task-work-comparison.json) | values_links_and_bytes_match=true |
| [original-x87-memory-comparison.json](_reference/sadx/original-x87-memory-comparison.json) | stored_ext80_and_comparison_conditions_match=false, non_signaling_register_load_profile_matches=true |
| [production-integration-20261012/original-game-logic-comparison.json](_reference/sadx/production-integration-20261012/original-game-logic-comparison.json) | cases=170, orchestration_stores_press_union_calls_match=true |
| [production-integration-20261012/original-game-logic-support-comparison.json](_reference/sadx/production-integration-20261012/original-game-logic-support-comparison.json) | passed=true |
| [production-integration-20261012/original-ninja-render-comparison.json](_reference/sadx/production-integration-20261012/original-ninja-render-comparison.json) | cases=6160, packed_commands_boundaries_match=true |
| [production-integration-20261012/original-ninja-sprite-comparison.json](_reference/sadx/production-integration-20261012/original-ninja-sprite-comparison.json) | supported_slice_values_vertices_boundaries_control_match=false, strict_emulator_match=false |
| [production-integration-20261012/original-ninja-sprite-queue-comparison.json](_reference/sadx/production-integration-20261012/original-ninja-sprite-queue-comparison.json) | state_buckets_trace_control_match=true, strict_emulator_match=false |
| [production-integration-20261012/original-ninja-texture-comparison.json](_reference/sadx/production-integration-20261012/original-ninja-texture-comparison.json) | packed_and_metadata_match=true, rgba_primary_policy_match=true, passed=true |
| [production-integration-20261012/original-player-animation-blend-comparison.json](_reference/sadx/production-integration-20261012/original-player-animation-blend-comparison.json) | cases=7944, stored_mismatches=2808, status_mismatches=1032, stored_state_matches=false, passed=false |
| [production-integration-20261012/original-player-animation-comparison.json](_reference/sadx/production-integration-20261012/original-player-animation-comparison.json) | cases=16440, stored_mismatches=0, status_mismatches=4140, boundary_mismatches=0, stored_state_matches=true, passed=false |
| [production-integration-20261012/original-player-auxiliary-effect-comparison.json](_reference/sadx/production-integration-20261012/original-player-auxiliary-effect-comparison.json) | supported_state_boundaries_callbacks_control_match=true, strict_emulator_match=false |
| [production-integration-20261012/original-player-position-comparison.json](_reference/sadx/production-integration-20261012/original-player-position-comparison.json) | stored_values_match=true, passed=false |
| [production-integration-20261012/original-player-startup-comparison.json](_reference/sadx/production-integration-20261012/original-player-startup-comparison.json) | prefix_state_resources_boundaries_control_match=true, original_return_observed=false, full_emulator_match=false |
| [production-integration-20261012/original-sonic-actions-comparison.json](_reference/sadx/production-integration-20261012/original-sonic-actions-comparison.json) | cases=30192, stored_mismatches=0, boundary_mismatches=0, status_mismatches=2172, stored_state_matches=true, passed=false |
| [production-integration-20261012/original-sonic-animation-table-comparison.json](_reference/sadx/production-integration-20261012/original-sonic-animation-table-comparison.json) | records=147, export_slots=149, cases=103, byte_mismatches=0, fp_mismatches=0, passed=true |
| [production-integration-20261012/original-sonic-player-comparison.json](_reference/sadx/production-integration-20261012/original-sonic-player-comparison.json) | cases=28800, stored_mismatches=0, status_mismatches=2840, ground_prefix_cases=4032, ground_prefix_stored_mismatches=0, ground_prefix_status_mismatches=1632, stored_state_matches=true, passed=false |
| [production-integration-20261012/original-task-lifecycle-comparison.json](_reference/sadx/production-integration-20261012/original-task-lifecycle-comparison.json) | values_links_and_bytes_match=true |
| [production-integration-20261012/original-task-work-comparison.json](_reference/sadx/production-integration-20261012/original-task-work-comparison.json) | values_links_and_bytes_match=true |
| [sse-math-comparison.json](_reference/sadx/sse-math-comparison.json) | passed=false |

Each comparison keeps its full JSON and often sibling `.cases.bin`. Private before/repaired reports are historical versions. The production reports index may predate later source mutations; compare hashes again.

## Appendix D. Evidence-folder and script inventory

Original source/byte dossiers, experimental fixtures, manifests and exported complete Evidence remain in these folders. The embedded Java/Python tool installations and full prepared game copy are excluded from the short folder inventory, not deleted.

- [acos-dependencies/](_reference/sadx/acos-dependencies/)
- [collision-normal/](_reference/sadx/collision-normal/)
- [collision-primitives/](_reference/sadx/collision-primitives/)
- [coordinate-dependencies/](_reference/sadx/coordinate-dependencies/)
- [coordinates/](_reference/sadx/coordinates/)
- [crt-dependencies/](_reference/sadx/crt-dependencies/)
- [floor-dependencies/](_reference/sadx/floor-dependencies/)
- [gameplay/](_reference/sadx/gameplay/)
- [ghidra-action-fp-recovery/](_reference/sadx/ghidra-action-fp-recovery/)
- [ghidra-engine-recovery/](_reference/sadx/ghidra-engine-recovery/)
- [ghidra-engine-recovery-unseeded/](_reference/sadx/ghidra-engine-recovery-unseeded/)
- [ghidra-game-loop-recovery/](_reference/sadx/ghidra-game-loop-recovery/)
- [ghidra-gameplay-live-recovery/](_reference/sadx/ghidra-gameplay-live-recovery/)
- [ghidra-live-recovery/](_reference/sadx/ghidra-live-recovery/)
- [ghidra-retrospective-recovery/](_reference/sadx/ghidra-retrospective-recovery/)
- [ghidra-scene-helper-recovery/](_reference/sadx/ghidra-scene-helper-recovery/)
- [ghidra-session/](_reference/sadx/ghidra-session/)
- [ghidra-session-client-timeout/](_reference/sadx/ghidra-session-client-timeout/)
- [ghidra-session-sandbox-failure/](_reference/sadx/ghidra-session-sandbox-failure/)
- [ghidra-shadow-recovery/](_reference/sadx/ghidra-shadow-recovery/)
- [goal-obligations/](_reference/sadx/goal-obligations/)
- [ground-dependencies/](_reference/sadx/ground-dependencies/)
- [handoff-checkpoints/](_reference/sadx/handoff-checkpoints/)
- [heading-dependencies/](_reference/sadx/heading-dependencies/)
- [mesh-math-dependencies/](_reference/sadx/mesh-math-dependencies/)
- [mesh-producers/](_reference/sadx/mesh-producers/)
- [mesh-registry-byte-evidence/](_reference/sadx/mesh-registry-byte-evidence/)
- [mesh-resource-dependencies/](_reference/sadx/mesh-resource-dependencies/)
- [mesh-transform-dependencies/](_reference/sadx/mesh-transform-dependencies/)
- [metal-texture-readback/](_reference/sadx/metal-texture-readback/)
- [movement/](_reference/sadx/movement/)
- [object-cleanup-dependencies/](_reference/sadx/object-cleanup-dependencies/)
- [path-dependencies/](_reference/sadx/path-dependencies/)
- [physics-dependencies/](_reference/sadx/physics-dependencies/)
- [plane-dependencies/](_reference/sadx/plane-dependencies/)
- [position-dependencies/](_reference/sadx/position-dependencies/)
- [production-integration-20261012/](_reference/sadx/production-integration-20261012/)
- [rea-capability-audit/](_reference/sadx/rea-capability-audit/)
- [scene-dependencies/](_reference/sadx/scene-dependencies/)
- [scene-mesh-dependencies/](_reference/sadx/scene-mesh-dependencies/)
- [scene-producers/](_reference/sadx/scene-producers/)
- [task-cleanup-dependencies/](_reference/sadx/task-cleanup-dependencies/)
- [task-lifecycle/](_reference/sadx/task-lifecycle/)
- [texture-decoded/](_reference/sadx/texture-decoded/)
- [texture-primary/](_reference/sadx/texture-primary/)
- [windows-runtime-captures/](_reference/sadx/windows-runtime-captures/)
- [windows-transfer/](_reference/sadx/windows-transfer/)

Top-level orchestration scripts and source profiles:

- [audit-rea-capabilities.mjs](_reference/sadx/audit-rea-capabilities.mjs)
- [build-goal-obligations.mjs](_reference/sadx/build-goal-obligations.mjs)
- [ghidra-action-fp-config.json](_reference/sadx/ghidra-action-fp-config.json)
- [ghidra-action-fp-seeds.tsv](_reference/sadx/ghidra-action-fp-seeds.tsv)
- [ghidra-callback-seeds.tsv](_reference/sadx/ghidra-callback-seeds.tsv)
- [ghidra-game-loop-config.json](_reference/sadx/ghidra-game-loop-config.json)
- [ghidra-game-loop-seeds.tsv](_reference/sadx/ghidra-game-loop-seeds.tsv)
- [ghidra-gameplay-callback-seeds.tsv](_reference/sadx/ghidra-gameplay-callback-seeds.tsv)
- [ghidra-gameplay-live-config.json](_reference/sadx/ghidra-gameplay-live-config.json)
- [ghidra-live.mjs](_reference/sadx/ghidra-live.mjs)
- [ghidra-session.mjs](_reference/sadx/ghidra-session.mjs)
- [ghidra-shadow-callback-seeds.tsv](_reference/sadx/ghidra-shadow-callback-seeds.tsv)
- [list-rea-tools.mjs](_reference/sadx/list-rea-tools.mjs)
- [recover-gameplay.mjs](_reference/sadx/recover-gameplay.mjs)
- [refresh-audit-reports.mjs](_reference/sadx/refresh-audit-reports.mjs)
- [sonic-ghidra-action-fp-snapshot.json](_reference/sadx/sonic-ghidra-action-fp-snapshot.json)
- [sonic-ghidra-gameplay-snapshot.json](_reference/sadx/sonic-ghidra-gameplay-snapshot.json)
- [sonic-ghidra-seeded-snapshot.json](_reference/sadx/sonic-ghidra-seeded-snapshot.json)
- [sonic-ghidra-shadow-snapshot.json](_reference/sadx/sonic-ghidra-shadow-snapshot.json)
- [sonic-ghidra-snapshot.failed-start.json](_reference/sadx/sonic-ghidra-snapshot.failed-start.json)
- [sonic-ghidra-snapshot.json](_reference/sadx/sonic-ghidra-snapshot.json)

Toolkit metadata lives under `_reference/sadx/tools/ghidra-*.json`; actual portable Ghidra and existing Python venv remain there. Read retained download/native-build identities rather than guessing versions.

## Appendix E. Handoff-time live resources and worker checkpoints

- Root serialized Ghidra runner47648 was confirmed live at documentation capture. Its private Program is ephemeral and its source target is immutable. Revalidate the handle in the resumed environment.
- No live worker oracle/provider/tester handles remain. Historical animation handles21361/58137/80287/73064 are terminal; stop polling them. Their returned reports are authoritative.
- Scene private compile handles52451/8923 are terminal failures; attempted scene oracle also terminal missing-library failure. They must not be described as running or passing.
- Shared controller refactor was expressly held for handoff consistency; four independent controller/input draft files compile only. Display/consumer/Metal source is likewise checkpointed WIP.
- Durable worker checkpoint: [queue-consumer-checkpoint.md](_reference/sadx/handoff-checkpoints/queue-consumer-checkpoint.md); retained [Metal sprite inspection report](_reference/sadx/handoff-checkpoints/metal-sprite-inspection.json); [controller input source listing](_reference/sadx/handoff-checkpoints/sadx-scene-input-producer.asm); [engine pad-key listing](_reference/sadx/handoff-checkpoints/sadx-engine-pad-keys.asm).
- Canonical Claude memory directories: cross-project `/Users/burbujamc/.claude/projects/-Users-burbujamc-Developer/memory`; project `/Users/burbujamc/.claude/projects/-Users-burbujamc-Developer-rea/memory` was absent. Follow reader/permissions rather than creating a second memory store. This requested file is the primary future-agent entry point.

---

## 2026-10-12 (late) — Repository move and the hybrid-runtime decision

  - **Repository move and the hybrid-runtime decision (gate `repo-move-99-gate.json`):** The user measured the pace (a late-2027 estimate from the per-function method) and asked for a review, a tested roadmap, a modular codebase, agent documentation, something visible, a public repo, and better use of REA. Measured facts: `.text` is 4,038,656 bytes with ~9,268 direct call targets; the 89 oracle reports pin 6,022 verified bytes (0.15 %) across 92 functions, produced in one day of gates. Decision (user-confirmed, ADR-0001): a hybrid runtime — every unported function runs from original bytes in-process (Unicorn, already the oracle engine) behind native Win32/D3D8→Metal/DirectInput/DirectSound layers; ported modules become native overrides; the guest is the runtime oracle. ADR-0002: analysis through REA's Ghidra runner only (Hopper hangs; it was one function per query); REA offers `list_procedures`, `batch_decompile`, callers/callees, snapshots and a seed file that were never used. ADR-0003: this repository (`sadx-macos`, public, MIT), `src/<area>/`, manifests, generated `build/*.mk`, generated `docs/STATUS.md`, hygiene checker, mod loader as a GPL submodule.
    - Moved `rea/sadx-native` → `~/Developer/sadx-macos` with includes rewritten (`"ninja/ninja_screen.hpp"` form), tests split into unit/probes/fixtures/python, oracles under `tools/oracles`, the old Makefile's 103 test invocations and 89 oracle targets parsed into `tests/unit/manifest.tsv` and `tools/oracles/manifest.tsv` (`tools/gen_make.py`), per-object `.d` dependencies. The game install, the pristine download (`game/pristine/sonic.exe`, same hash) and the retail disc images (`game/disc/`) live under `game/` (ignored); the 6 GB reference tree is `reference/` with its large parts ignored. The full gate passes unchanged in the new layout: 103 native programs, 4 Python tests, owner checker 0 duplicates, profile patches 0 UNHANDLED, hygiene 0 violations.
    - New docs: `AGENTS.md` (+`CLAUDE.md` import), `docs/ROADMAP.md` (M0–M5, each with its test table), `docs/REVIEW-2026-10-12.md`, `docs/ARCHITECTURE.md`, `docs/METHOD.md`, `docs/SOURCES.md`, `docs/decisions/ADR-0001..0003`, `docs/NEXT-AGENT-PROMPT.md`, generated `docs/STATUS.md`. The old guide is `docs/AGENT_GUIDE_v1.md` (history; its method section is superseded by METHOD.md).
    - Code since gate 98: `d3dx_math` gained 7716df (perspective, FSINCOS) and 7757e0 (the SSE cofactor inverse, slot 18 of the D3DX table) — **no oracles yet**; `x87_transcendental` gained Bochs FSINCOS/FPTAN wrappers (`fsincos.cc` vendored at the pinned revision, `upstream.json` updated) — **no oracle yet**; `runtime/game_session` (shared boot config + report text for the CLI and the app); `platform/mac/sadx_app.mm` (`build/sadx-app`: AppKit + MetalKit window drawing exactly the D3D8 stream the native boot issued — the Clear and BeginScene's polygon — with the boot report as an overlay; `--exit-after N --screenshot out.ppm` for the M2 `app-smoke` test, not yet in the gate); `src/guest/x86_guest.*` (Unicorn-backed guest: image mapping, calls with a stop address, overrides via code hooks + TB flush) with `tests/unit/guest_smoke.cpp`: **7899a0 executed from the original bytes inside the process equals the native port on 4 cases, and an override replaces it** — the first M1 evidence. `guest-smoke` is in the unit manifest (needs `UNICORN_DIR`, defaulting to the venv wheel's `libunicorn.a`).
    - Found by the new status generator: 19 comparison reports under `post-repair-comparisons/` are mutant leftovers (`tools/mutate.py` used to leave the last mutant's report in `build/`; it now reruns the oracle after restoring the source). They are being rerun and recopied; `docs/STATUS.md` lists any still failing.
    - WinMain 402c92..402d7c analysis is done but unported: `reference/ghidra-game-loop-recovery/wm-*.json` (5850f0, 556e40, 556d60, 568d20, 4f68e0, 5adc40, 600700, 781570, 781500, 781250, 40e2e0, 7716df, 770d10) and `proj-791170.json`; 781250's listing and FixFOV's patches (inactive under the profile) are in the previous session's notes above. 7757e0's listing came from Hopper (`ev_f344ba3f…`).
    - Next: `docs/NEXT-AGENT-PROMPT.md`.

## Second layout iteration: layers, one slug, discovered oracles (2026-10-13)

Evidence: gate `reference/production-integration-20261012/layout2-100-gate.json` (rc 0, 104
native programs, 4 Python tests, 0 layer violations with 3 listed exceptions, 0 duplicate
owners, 0 UNHANDLED patches, 0 hygiene violations); every oracle rerun under the new layout
(`make -j6 -k oracles`, logs in the session scratch) and tracked as summaries in
`reference/oracle-reports/` (96 reports; `make oracles-verdict`: 46 pass, 41 partial, 9 fail).
Decision record: `docs/decisions/ADR-0004-layered-source-and-one-slug.md`.

What changed
* `src/` is now a layer stack `cpu loader math engine input player game runtime guest hle
  platform apps`; files moved, includes rewritten (151 files), no code edited. The old
  cycles (`ninja`↔`game`, `game`↔`player`, `ninja`→`player`, `platform`↔`runtime`) are gone
  except the three dependencies listed in `tools/gate/layer_exceptions.tsv`
  (`ninja_sprite`/`ninja_projection_range` → `player_coordinates` matrix stack, `task.cpp` →
  `player_task`). `tools/gate/check_layers.py` runs in `make check`.
* `tools/` grouped: `lib/` (pe_image, extract), `gate/` (gen_make, gen_status, record_gate,
  checkers, mutate, track_reports, profile_patches.json), `oracles/`, `analysis/`, `emit/`.
  Executables moved to `src/apps/` (sadx_boot, sadx_app, metal_stage, diagnostics/).
* Oracles are discovered from `tools/oracles/verify_original_<slug>.py` (target
  `oracle-<slug>`, report `build/oracles/<slug>.json`); `tools/oracles/manifest.tsv` keeps 16
  override rows. Nine scripts that were never wired (game_logic, game_logic_support,
  ninja_render, ninja_sprite, ninja_sprite_queue, ninja_texture, player_animation,
  player_animation_blend, sonic_animation_table) now run. `verify_sse_math.py` renamed to the
  `verify_original_` scheme.
* 20 oracle scripts still computed the repository root one level short after the move and 8
  listed native sources by pre-area paths (`src/<file>`); both fixed (`native_source()` in the
  harness resolves legacy names to `src/<layer>/<file>`). Nothing had detected this because
  the gate does not run oracles.
* `tools/oracles/verdict.py` is the single reader of report shapes (pass / partial / fail);
  `docs/STATUS.md` now lists all three buckets and an oracle map. `tools/gate/track_reports.py`
  writes size-bounded summaries (lists cut to 10, full sha256 kept) instead of raw copies —
  five raw reports were 3–69 MB.
* Makefile: layers drive the archives; `.mm` and `mac-*`/`metal-*` unit tests link the
  platform archive by rule; `make status`, `make oracles-verdict`, `make app-smoke`.

Oracle state, honestly
* 46 pass. 41 partial: every compared value/state/byte matches, only x87 status / MXCSR /
  exception flags differ, as each report's `limits` say (Unicorn 2.1.4 raises no SSE flags,
  applies the x87 NaN rule to SSE operands; FLD of an SNaN is not the hardware spec).
* 9 fail: `crt_asin_binary64`, `crt_atan2`, `crt_fabs`, `crt_floor`, `floor_angles`,
  `player_reaction`, `x87_memory` report value-class disagreements whose `limits` attribute
  them to Unicorn's host-double FPATAN and SNaN register loads (the native side uses Bochs
  P5/P6); `sse_math` (5,076 value mismatches of 329k evaluations, SSE scalar opcodes vs
  Unicorn) and `ninja_sprite` (`supported_slice_values_vertices_boundaries_control_match`
  false) need a look. None of these verdicts changed with the layout: the pre-move baseline
  run of the same scripts gave the same non-pass set (plus the path breakages fixed above).
  Settling the FPATAN/SNaN cases needs an Intel capture (roadmap M4), not a port change.
* Earlier status pages reported "60/60 passing" from a tracked subset of 60 reports that
  excluded the 37 older oracles; that subset is gone.
* Retired: `verify_original_ninja_sprite.py` + `native_ninja_sprite_probe.cpp`. It was never
  wired into the build, failed on its own value criterion, and tested the original
  77e390..77e3d0 bytes that the loader replaces with `njDrawSprite3D_DrawNow_r` (patch check:
  1 UNHANDLED overlap, now 0); the 77e390 path under the profile is covered by
  `ninja_sprite_display` (handoff step 9). The unit test `tests/unit/ninja_sprite.cpp` stays.
* Not produced: `game_reset_composed` (the native probe rejects the composed fixture:
  `RuntimeError: Native rejected`, identical before the move) — listed as "missing" in STATUS.
  `alternate_acceleration` wrote to a pre-move path; fixed in the manifest, now partial.

Limits
* The gate still does not run the oracles (minutes each); `make oracles` is the separate step.
* Historical names (`reference-<x>-test`, `original-<x>-comparison.json`) remain in older
  handoff entries and gate records.

## Oracle health: every fail and partial explained (2026-10-12, step 1 of NEXT-AGENT-PROMPT)

Evidence: gate `reference/production-integration-20261012/oracle-health-101-gate.json` (rc 0, 104
native programs, 4 Python tests); 19 reports rerun and tracked in `reference/oracle-reports/`.

* The seven named fails (`crt_asin_binary64`, `crt_atan2`, `crt_fabs`, `crt_floor`, `floor_angles`,
  `player_reaction`, `x87_memory`) were checked against their mismatch samples, not just their
  `limits`: asin/atan2/floor_angles/player_reaction values are Unicorn's 53-bit host-double FPATAN
  (e.g. 0x860a91e2cccde000 vs native 0x860a91e2cccddd7c); fabs/floor stop at a different boundary
  only on SNaN inputs (0x7ff0000000000123 class); x87_memory's 1440 value mismatches are exactly its
  1440 SNaN register loads (non-signaling loads all match). No port change. Each script now appends
  a limit naming the Unicorn behaviour and that settling it needs the Intel capture (M4); they stay
  `fail`.
* `sse_math` (the real look): all 5,076 value mismatches fall into three documented Unicorn
  deviations — x87 NaN priority on SSE operands (SSE returns the first source quietened), MXCSR.FZ
  ignored (masked underflow not flushed), MXCSR.DAZ ignored in MINSS. The classifier now also
  checks that the native value equals the Intel SDM rule for its class (before, "classified" only
  meant "different"); `unclassified_value_mismatches` = 0. Verdict stays `fail` (pass criteria
  unchanged); limits added (the report had none).
* `ninja_sprite`: already retired by the layout session (its script tested bytes the loader
  replaces); nothing to do.
* Partial reports whose `limits` did not name the flag difference: collision_shapes, crt_sqrt,
  input_mod_axes, ninja_queue_consumer, player_animation, player_input, player_input_producer,
  player_movement, player_position, player_startup, sonic_actions (11). Each now names it from its
  own examples (x87 PE 0x20 / DE 0x02 / IE 0x01 set natively where Unicorn 2.1.4 raises none; MXCSR
  PE 0x20 for the Input mod axes). Every partial now names its flag difference.
* Verdicts after the rerun: unchanged (46 pass, 42 partial, 8 fail, `game_reset_composed` still
  missing — its native probe rejects the composed fixture, as before).

Limits: the 19 reports were produced by one `make -k` run whose tree already held step 2's
matrix/task refactor (no behaviour change; the full-oracle verdicts before and after it are
identical). Oracles remain outside the gate.

# Roadmap: Sonic Adventure DX, native on Apple Silicon

Goal: play *Sonic Adventure DX* (2004 PC release, `sonic.exe`) on macOS arm64 as a
native application, with the pinned speedrun mod profile (SADX Mod Loader eb4cdcca /
DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0, 63 FPS frame limit), with gameplay
that matches the Windows reference closely enough for speedrun practice — and prove
it with automated tests instead of manual play.

This file is the plan. `docs/STATUS.md` says where we are. `AGENTS.md` says how to
work. Milestones are ordered by dependency; each has a definition of done that is a
set of automated tests, so "done" is never a judgement call.

## Why the plan changed on 2026-10-12

The first strategy ported `sonic.exe` function by function into C++, each function
verified by a Unicorn oracle against the original bytes. It produced an excellent
verification toolchain and 23k lines of exact ports, but the measured throughput was
**~6 KB of verified code per day against 3.85 MB of code (~9,300 functions)**. That
is a multi-year path to a first playable build. See `docs/REVIEW-2026-10-12.md`.

The plan now is a **hybrid runtime**: every function that is not natively ported runs
from the original x86 bytes inside the process (QEMU-based Unicorn, already a project
dependency), behind native implementations of the operating-system and graphics
boundary (Win32, Direct3D 8 → Metal, DirectInput → GameController/keyboard, DirectSound
→ CoreAudio). The hand-ported modules become *native overrides* of guest functions,
and the guest itself becomes a runtime oracle: any override can be checked against the
original code on the same inputs, at any time, automatically.

Fidelity stays the top requirement: the guest executes the game's own instructions;
the known places where emulation is not bit-exact (x87 transcendentals, SSE
reciprocal estimates) are shimmed with the project's exact models, which already
exist (`src/cpu`).

## Milestones

### M0 — Foundation (complete)

* Modular repo, one-command gate (`make gate`): 103 native test programs, 4 Python
  tests, 89 Unicorn oracles, owner checker, profile-patch checker.
* Exact models: x87 (SoftFloat ext80), SSE, Bochs FPATAN/FSINCOS/FPTAN, CRT math,
  D3DX8 SSE path.
* ~100 ported game functions (boot, frame loop, render queue, player physics,
  collision, animation, input, audio bookkeeping) — see `docs/STATUS.md`.

### M1 — The whole game runs headless (hybrid runtime)

Build `src/guest` (x86 guest), `src/hle` (Win32 + D3D8 + DirectInput + DirectSound
HLE), and the override table that routes guest calls into the ported modules.

Definition of done (all automated, `make m1`):

| Test | What it proves |
|---|---|
| `guest-boot` | `sonic.exe` + mod loader + mods load, WinMain runs to the first `Present`, no unresolved import, no unported stop. |
| `guest-frames` | 600 frames headless (SEGA logo → title) complete; the D3D8 command stream per frame is non-empty and hashed. |
| `override-equivalence` | For every ported module with an oracle, the in-process guest runs the original function on the oracle corpus and the native override produces identical outputs. Mismatch = the override is wrong (or the shim is). |
| `determinism` | Two headless runs with the same input script give identical per-frame state hashes (player/mode/RNG regions + command stream). |
| `profile-applied` | Every active patch in `tools/gate/profile_patches.json` is observed as written in guest memory after mod init (the inventory becomes runtime-verified). |
| `fpu-shims` | Every FPATAN/FSINCOS/FPTAN/F2XM1/FYL2X/RCPSS/RCPPS/RSQRT site in `.text` is covered by a shim; the shim results match the Bochs/own models (unit test over the site list). |
| `guest-fps` | Headless frames per second is measured and recorded in the gate (no threshold yet; this is the performance baseline M5 works against). |

Risks: Unicorn speed (QEMU TCG without block chaining); threads (the 40ff50 stream
thread and timers need a cooperative scheduler at blocking Win32 calls); SEH. The
per-frame budget is measured in `guest-fps` before M2 begins; if it is under ~30 FPS
the mitigation is moving hot functions to native overrides first (M5 work pulled in).

### M2 — Something on screen

AppKit window + Metal backend that consumes the D3D8 command stream (clear,
`DrawPrimitiveUP`/`DrawPrimitive` with the FVF layouts the game uses, textures via
`metal_texture`, the render-state subset the game sets: blend, depth, cull, fog
table, texture stage ops).

Definition of done (`make m2`):

| Test | What it proves |
|---|---|
| `golden-frames` | Offscreen renders of fixed frames (SEGA logo, title screen, first Emerald Coast frame) match committed golden PNGs (perceptual diff under threshold; goldens are reviewed once, then frozen). |
| `app-smoke` | `sadx-app` launches, renders 300 frames in a window, exits 0, no Metal validation errors. |
| `state-dump-ui` | The in-app overlay (frame, mode, FPS, native-vs-guest ratio) matches the headless report for the same frame. |

### M3 — Playable

Input (macOS keyboard → DIK + the Input mod's keyboard path; GameController →
the Input mod's SDL pad semantics), audio (DirectSound HLE → CoreAudio; BGM/voice are
WMA on PC — decode via a bundled decoder or a one-time asset conversion; this is an
open decision, see `docs/decisions/`), frame pacing at the profile's 63 FPS.

Definition of done (`make m3`):

| Test | What it proves |
|---|---|
| `replay-emerald-coast` | An input script (recorded once) drives title → Sonic → Emerald Coast 1 → goal; assertions on game state read from guest memory at checkpoints (level id, mode, player position within tolerance 0). |
| `replay-boss-egg-hornet` | Same through Egg Hornet including the post-cutscene (the EHCF mod path). |
| `speedrun-profile` | Frame limiter cadence = 63 FPS; Speedrun mod quick save/load round-trips; CCEF camera behaviour matches reference traces. |
| `audio-events` | The sequence of sound/BGM start/stop events per frame equals a reference trace (no sample comparison). |
| `input-mapping` | Keyboard and controller mapping unit tests against the Input mod's own tables (already partly present). |

### M4 — Faithful to the Windows reference

The reference is the real game under the same profile on Windows. The capture
collector in `reference/windows-transfer` records per-frame traces (player position,
mode, RNG, timers) and screenshots.

Definition of done (`make m4`):

| Test | What it proves |
|---|---|
| `trace-parity` | Same input script on reference and native → bit-identical per-frame traces for the replay suite; a divergence reports the first frame and, via shadow execution (native override vs guest on that frame), the first function. |
| `frame-parity` | Golden frames come from reference screenshots (perceptual diff). |
| `hardware-tables` | The Bochs transcendental models and the RCPSS/RCPPS/RSQRT models are compared against an Intel CPU's actual results (needs an Intel host once; recorded as a dataset). |
| `technique-suite` | Input scripts for the speedrun techniques that matter (spindash chains, Emerald Coast skips, …) reach the same outcomes as the reference. |

### M5 — Native all the way (progressive)

Replace guest execution with native code by hot-path priority, each replacement
proven by `override-equivalence` on corpora recorded from real play, until frame time
spent in the emulator is ~0. Static recompilation (Ghidra CFG → C) is an acceptable
tool here, verified the same way. Performance target: 63 FPS with headroom on an M-series
Mac without the emulator on the hot path.

## How progress is measured

`docs/STATUS.md` is regenerated from the gate: tests passing, oracles passing, number of
functions with native overrides, % of frame time native vs guest, FPS headless, FPS
windowed, replay suite pass/fail. Every gate record (`reference/production-integration-*/`)
is the audit trail. A milestone is complete only when its table is green in a recorded
gate.

## Explicitly out of scope

Other SADX releases (Steam 2010/2011) and other platforms. Pirated assets: the user
supplies their own `sonic.exe`, game data and mods (`docs/SOURCES.md`).

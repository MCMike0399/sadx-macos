# Roadmap: Sonic Adventure DX, native on Apple Silicon

Goal: play *Sonic Adventure DX* (2004 PC release, `sonic.exe`) on macOS arm64 as a
native application, with the pinned speedrun mod profile (SADX Mod Loader eb4cdcca /
DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0, 63 FPS frame limit), with gameplay
that matches the Windows reference closely enough for speedrun practice — and prove
it with automated tests instead of manual play. The finished game builds from readable,
maintained source and runs from imported assets without `sonic.exe`, original Windows/mod
DLL execution or an x86 runtime (ADR-0005). The pinned binaries remain development
references and may supply embedded data during one-time import from the user's own media.

This file is the plan. `docs/STATUS.md` says where we are. `AGENTS.md` says how to
work. Milestones are ordered by dependency; each has a definition of done that is a
set of automated tests, so "done" is never a judgement call.

## Why the plan changed on 2026-10-12

The first strategy ported `sonic.exe` function by function into C++, each function
verified by a Unicorn oracle against the original bytes. It produced an excellent
verification toolchain and 23k lines of exact ports, but the measured throughput was
**~6 KB of verified code per day against 3.85 MB of code (~9,300 functions)**. That
is a multi-year path to a first playable build. See `docs/REVIEW-2026-10-12.md`.

The intermediate plan is a **hybrid runtime**: every function that is not natively ported runs
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

On 2026-10-15 the user clarified executable independence and maintainable source as
required outcomes. [ADR-0005](decisions/ADR-0005-native-source-and-executable-independence.md)
retains this reference runtime and strengthens M5 beyond its original hot-path performance
target. Native-system recovery starts alongside M1–M4; their existing tests are unchanged.

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

### M5 — Native source and executable independence

Recover complete game systems into named, maintained C++ modules and compose them
through native state, resources and callbacks. Share those implementations with the
reference adapters used by `override-equivalence`. Use measured hot paths to prioritize
work, while accounting for initialization, menus, cold paths and pinned mod behavior.
The final dependency graph must account for all shipped game behavior, not only the
Sonic route or functions reached in a title-screen recording.

Separate original code from content currently embedded in `sonic.exe` and model DLLs.
A versioned importer reads the user's supported disc/install once and emits data-only
resources with hashes, schemas and resolved resource references. Identify source variants
explicitly; disc import and installation-folder import need their own fixtures. Imported
packs stay local/ignored. Source builds compile the maintained code without regenerating
it from a game binary. Pure numeric compatibility helpers may remain where required for
fidelity; whole-game guest execution and original-code fallback may not.

Static recompilation is an optional recovery aid, with separate compiler correctness
and maintenance requirements. REA's bounded analysis graph and pseudocode export alone
are insufficient. See [the source investigation](RESEARCH-native-recompilation.md) and
`docs/GOAL.md` §4.5 for the recovery workflow.

Definition of done (planned `make m5`; these targets are not implemented yet):

| Test/check | What it proves |
|---|---|
| `native-source-build` | A clean build from maintained source plus declared native dependencies succeeds with game binaries, Ghidra/REA and Unicorn unavailable. No build-time lifting/decompilation step or generated file from an undeclared local executable is required. |
| `asset-import` | Supported owned disc/install fixtures produce complete versioned data packs, including embedded models, tables, animation and placement data. Hash/round-trip checks and resource-reference validation catch omissions; malformed or unsupported variants fail explicitly. The existing partial geometry extraction is insufficient. |
| `native-runtime-independent` | In an isolated environment containing only the built app, native dependencies and imported pack, boot/title, load/save, stage and boss replays pass with original EXE/DLLs and reference directories unavailable. Dependency/link inspection excludes guest/HLE PE execution and Unicorn; no original code image is hidden in a pack. |
| `native-behavior-closure` | A reviewed inventory accounts for all required game modes, characters, stages, tasks/callbacks and profile hooks, including cold paths; each has a native owner and tests or an explicit remaining gap. Completion requires zero unresolved required behavior, zero guest fallback, and a replay suite covering those systems beyond the initial Sonic route. Test coverage alone does not prove that every indirect target was recovered. |
| `native-profile-parity` | Input 2.6.1 and Speedrun 2.0 behaviors, the 63 FPS limiter, CCEF/EHCF/Skipma and quick save/load run natively and pass M3/M4 traces and technique tests. Binary patch sites serve as provenance; original mod DLLs are unnecessary. |
| `native-source-ownership` | Shipping game interfaces, mutable state and callback dispatch use semantic names and native identities. Source-address mappings are maintained separately with one-owner/provenance checks. A reviewed module map and representative maintenance change demonstrate that gameplay changes do not require editing an address switch, guest registers or PE memory offsets. |
| `reference-corpus-frozen` | Each recovered system's oracle fixtures, replay end states and trace digests are recorded under `reference/` (numbers and hashes only, hygiene green) and its tests run from them with `sonic.exe` absent, so verification outlives the reference runtime (OpenRCT2 TestPaint rotted without this). |
| `native-reference-parity` | Existing module oracles, mutants, full native replays, Windows trace parity and reference-frame comparisons retain their stated fidelity scopes. Numeric/flag differences remain reported; a partial oracle is not promoted to pass by removing the guest. |
| `native-performance` | The native replay suite sustains the profile's 63 FPS cadence on the recorded M-series test configuration, with frame-time distributions and headroom reported. Native execution and executable independence are also enforced structurally; a near-zero guest-time measurement cannot substitute for them. |

First pull forward a bounded frame-control slice (GOAL §6 item 1), then native boot/title
and a stage. Preserve the working reference while these grow; retire a production binary
dependency only after its replacement is proven. M5 remains incomplete until every row passes.

## How progress is measured

`docs/STATUS.md` is regenerated from the gate: tests passing, oracles passing, number of
functions with native overrides, % of frame time native vs guest, FPS headless, FPS
windowed, replay suite pass/fail. For M5 (ADR-0006) it adds: procedures with a symbol-database
name (of 8,875), address-named owners remaining (`*_at_<hex>`, to reach 0, the OpenLoco
measure), `FUNCTION` vs `STUB` annotations, imported-pack coverage of the sa_tools mappings,
and reference corpora frozen per system. Every gate record (`reference/production-integration-*/`)
is the audit trail. A milestone is complete only when its table is green in a recorded
gate.

## Explicitly out of scope

Other SADX releases (Steam 2010/2011) and other platforms. Pirated assets: the user
supplies their own game data from supported media/installations (`docs/SOURCES.md`).
Development/reference tests additionally require the pinned executable and mods; the
finished native game uses the independent imported data pack.

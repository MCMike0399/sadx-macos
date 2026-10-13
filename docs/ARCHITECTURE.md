# Architecture

## Layout

```
src/
  cpu/          exact arithmetic: x87 (ext80, control word), SSE (MXCSR), Bochs transcendentals, CRT math, SourceBytes
  d3dx/         D3DX8 math as linked into sonic.exe (SSE dispatch path)
  ninja/        the Ninja engine layer of the game: render states, sprites, texture lists, render queue, fog, screen, device/system init, frame thunks, keyboard, vibration
  game/         game logic: WinMain, library init, game loop, modes and their leaves, resets, views, tasks, heap, sound bookkeeping, model DLL bindings, mod loader frame, speedrun limiter
  player/       player physics, collision, animation, Sonic
  input/        the Input mod's pad/keyboard semantics and the scene input producer
  runtime/      NativeGame composition (game_runtime) and the sadx-boot CLI
  platform/mac/ Metal texture/sprite/stage, GameController input, virtual 10 MHz clock, AppKit app
  guest/        (M1) in-process x86 guest: PE/DLL mapping, Unicorn execution, override table, FPU shims
  hle/          (M1) Win32 / D3D8 / DirectInput / DirectSound implementations the guest calls
tests/unit      one test program per module (+ manifest.tsv with the arguments it takes)
tests/probes    native_<x>_probe.cpp: flat C ABI around a module for the Unicorn oracles
tests/fixtures  headers shared by tests
tests/python    pytest-free unittest modules (extraction, hygiene)
tools/          build and analysis tools; tools/oracles = verify_original_*.py + manifest.tsv
third_party/    SoftFloat (x87, SSE), Bochs FPATAN/FSINCOS/FPTAN — each with sources.txt, upstream.json
reference/      provenance docs, ledgers, gate records, comparison reports (tracked);
                Ghidra outputs, pinned profile install, Windows collector, python venv (ignored)
game/           the user's game install (ignored)
docs/           ROADMAP, STATUS (generated), METHOD, SOURCES, decisions/, HANDOFF journal
```

Dependencies flow downward: `cpu` ← `d3dx`/`ninja`/`player`/`input`/`game` ← `runtime`
← `platform`. `guest` depends on `cpu` and `runtime` (for overrides); `hle` depends on
`platform` for the real device, input and audio. Nothing in `cpu`…`game` may include
platform headers.

## Ownership rules (enforced)

* Every source global the native code models is a value member named
  `<meaning>_at_<hex>` in exactly one struct (`tools/check_global_owners.py`). Other
  code borrows it by reference/pointer. Arrays count their first element; `SourceBytes<base,size>`
  owns a whole block.
* Every ported code range is listed in its oracle report (`original_ranges_sha256`) and
  checked against the profile patch inventory (`tools/check_profile_patches.py`).
* Native identities (device handles, heap blocks, texture memory) are never guest
  pointer casts. Guest addresses stay `std::uint32_t`.

## Result and failure vocabulary

`*State` (owned facts, `std::optional` for unknowns), `*Runtime` (borrowed owners + host
callbacks), `*Result` with a failure enum, `source_boundary` (the instruction that could
not proceed) and `required_fact` (the address that was unknown). Nested owners are
invoked through `std::function` callbacks and may return `GameLogicOwnerState::unavailable`
(unported) or `failed`.

## The runtime today and tomorrow

`NativeGame` (`src/runtime/game_runtime.*`) composes every ported owner and runs the boot
and the frame loop, stopping at the first unported owner (or surveying past it as a
no-op). D3D8 calls go to `D3D8StateOwner` (recording) unless a Metal backend is set.

With M1, `NativeGame` becomes the override provider for the guest: the guest executes
`sonic.exe` and, at every address in the override table, calls the native owner instead.
Each HLE import logs its call for trace diffs. The headless runner and the AppKit app
share one `GameSession` object: the app only adds a window, a Metal backend and input.

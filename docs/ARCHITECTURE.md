# Architecture

## Layers (`src/`)

`src/` is a strict stack. A file may include headers from its own layer or any layer
**below** it, never above. `tools/gate/check_layers.py` enforces this in every gate; the
three known violations are listed in `tools/gate/layer_exceptions.tsv` and must be deleted
from that file when fixed (a stale exception fails the check).

| Layer (lowest first) | What lives there | May include |
|---|---|---|
| `cpu` | exact arithmetic: x87 (ext80, control word), SSE (MXCSR), Bochs transcendentals, CRT math, `SourceBytes` | — |
| `loader` | what the OS loader gives a program: `ExecutableImage` (PE32 as mapped), `GuestHeap` (Win32 heap identities) | cpu |
| `math` | `NinjaMath` (sine table, angles) and the D3DX8 math as linked into sonic.exe | cpu |
| `engine` | the Ninja engine: task scheduler and work, the matrix stack and primitives (`ninja_matrix`), the camera/view/projection (`ninja_view`: 781570, 781250, 791170), controller records, game-logic timing/edge state and frame control, render states, sprites, texture lists, render queue, fog, screen, device/system init, frame thunks, keyboard, vibration, njMalloc, the speedrun limiter state | cpu loader math |
| `input` | the Input mod DLL's pad/keyboard semantics (axes, pad records) | … engine |
| `player` | player physics, collision, animation, Sonic, the scene input producer | … input |
| `game` | WinMain, library init, startup, game loop, modes and leaves, resets, views, window, sound bookkeeping, model DLL bindings, mod loader frame, Win32 profile | … player |
| `runtime` | `NativeGame` (composition of every ported owner, boot + frame loop) and `GameSession` (shared by the CLI and the app) | … game |
| `guest` | (M1) in-process x86 guest on Unicorn: image mapping, calls, continuations (`redirect`), pause/resume, the override table (`override_table`), FPU shims (`fpu_shims`) | … runtime |
| `hle` | (M1) the guest's Windows: `hle_registry` (imports → traps, call log), `hle_modules` (LoadLibrary & co., DLL linking), `hle_process` (one guest process, TEB/PEB, overlay, assumptions), `hle_kernel32`, `hle_crt` (+`hle_printf`, `hle_text`), `hle_user32` (headless window system), `hle_com`, `hle_dinput`, `hle_d3d8`, `hle_xinput` (synthetic pad state), `hle_raw_input` (HID identity/reports and WM_INPUT), `hle_device_bridge` (device-backed native overrides through verified HLE COM handlers). An unmodelled import stops the guest with its name; every chosen fact is a reported assumption | … guest |
| `platform/mac` | Metal texture/sprite backends, `metal_guest_frame` (draws a recorded guest D3D8 frame: clears, DrawPrimitiveUP with guest textures, blending, depth), GameController input, the virtual 10 MHz clock | … hle |
| `apps` | executables only: `sadx_boot.cpp` (headless CLI), `sadx_app.mm` (AppKit window), `metal_stage.mm` (asset viewer), `diagnostics/` (one-off HID/GameController probes, not built by default) | everything |

Archives: `libsadx-core.a` = cpu…runtime (portable, no frameworks), `libsadx-macos.a` =
platform/mac, `libsadx-guest.a` = guest + hle (+ Unicorn). Programs link what they need.

Known debt (the exceptions file): none since 2026-10-13. The matrix stack and primitives live in
`engine/ninja_matrix` (`NinjaMatrixPrimitives`; `player/player_coordinates`' `PlayerCoordinates`
derives from it and adds the player transforms), and `engine/task.cpp` builds a child's live
`PlayerTaskState` through a factory the player layer installs (`install_player_child_state`,
`player/player_task`); with no factory installed that path fails explicitly
(`TaskFailure::child_state_factory_missing`).

## One name per module

Everything about a module shares one slug, so an agent can find all of it with one `ls`:

| Artefact | Path | Notes |
|---|---|---|
| code | `src/<layer>/<slug>.hpp`, `.cpp` | |
| unit test | `tests/unit/<slug>.cpp` → `build/<slug-with-dashes>-test` | arguments in `tests/unit/manifest.tsv`; `.mm` tests and `mac-*`/`metal-*` tests link the platform archive; `guest_*` tests link the guest |
| oracle probe | `tests/probes/native_<slug>_probe.cpp` → `build/native-<slug-with-dashes>-probe.dylib` | flat C ABI around the module, loaded by the oracle with ctypes |
| oracle | `tools/oracles/verify_original_<slug>.py` → target `oracle-<slug-with-dashes>` | discovered by `tools/gate/gen_make.py`; `tools/oracles/manifest.tsv` only lists non-default inputs |
| oracle report | `build/oracles/<slug>.json` (+ `.cases.bin` side files) | the tracked copy is `reference/oracle-reports/<slug>.json`; `tools/oracles/verdict.py` is the only reader of its pass/fail shape |
| fixtures | `tests/fixtures/*.hpp` | headers shared by several tests |

Oracle slugs predate this rule and do not all equal a module name (e.g. `air_acceleration`
tests `player_acceleration_air`); new ones must.

## Tools (`tools/`)

| Directory | Contents |
|---|---|
| `lib/` | importable helpers: `pe_image.py` (PE reader), `extract.py` (asset extraction, source identity) — on `PYTHONPATH` via the Makefile's `$(PY)` / `$(REFERENCE_PYTHON)` |
| `gate/` | what the gate runs: `gen_make.py`, `check_layers.py`, `check_global_owners.py`, `check_profile_patches.py` (+ `profile_patches.json`), `check_repo_hygiene.py`, `record_gate.py`, `gen_status.py`, `mutate.py` |
| `oracles/` | `verify_original_<slug>.py`, the shared harness (in `verify_original_math.py`: STOP/STACK/CODE/DATA, `load_image`, …), `verdict.py`, `manifest.tsv` |
| `analysis/` | reverse-engineering helpers: `ghidra_asm.py`, `xscan.py`, `iso_list.py`, HID inspectors |
| `emit/` | generators that write source tables from the binary (`emit_*.py`) |

## Ownership rules (enforced)

* Every source global the native code models is a value member named
  `<meaning>_at_<hex>` in exactly one struct (`tools/gate/check_global_owners.py`). Other
  code borrows it by reference/pointer. Arrays count their first element; `SourceBytes<base,size>`
  owns a whole block.
* Every ported code range is listed in its oracle report (`original_ranges_sha256` or
  `source_ranges`) and checked against the profile patch inventory
  (`tools/gate/check_profile_patches.py`, reading `build/oracles/*.json`).
* Native identities (device handles, heap blocks, texture memory) are never guest
  pointer casts. Guest addresses stay `std::uint32_t`.

## Result and failure vocabulary

`*State` (owned facts, `std::optional` for unknowns), `*Runtime` (borrowed owners + host
callbacks), `*Result` with a failure enum, `source_boundary` (the instruction that could
not proceed) and `required_fact` (the address that was unknown). Nested owners are
invoked through `std::function` callbacks and may return `GameLogicOwnerState::unavailable`
(unported) or `failed`. These types live in `engine/game_logic.hpp` so every layer above
`engine` shares them.

## The runtime today and tomorrow

`NativeGame` (`src/runtime/game_runtime.*`) composes every ported owner and runs the boot
and the frame loop, stopping at the first unported owner (or surveying past it as a
no-op). D3D8 calls go to `D3D8StateOwner` (recording) unless a Metal backend is set.

With M1, `NativeGame` becomes the override provider for the guest: the guest executes
`sonic.exe` and, at every address in the override table, calls the native owner instead.
Each HLE import logs its call for trace diffs. The headless runner and the AppKit app
share one `GameSession` object: the app only adds a window, a Metal backend and input.

## Other top-level directories

`tests/{unit,probes,fixtures,python}`, `third_party/` (SoftFloat x87/SSE, Bochs
FPATAN/FSINCOS/FPTAN, each with `sources.txt` + `upstream.json`), `reference/` (tracked:
gate records, `oracle-reports/`, ledgers, provenance; ignored: Ghidra outputs, the pinned
profile install, Windows collector, python venv), `game/` (the user's install, ignored),
`docs/` (ROADMAP, STATUS generated, METHOD, SOURCES, decisions/, HANDOFF journal).

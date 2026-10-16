# Architecture

The layer table describes the current implementation. The shipping destination is
[ADR-0005](decisions/ADR-0005-native-source-and-executable-independence.md): native game
systems built from maintained source and running from imported assets. `guest`/`hle`
and PE image loading are reference/development facilities in that destination.

## Destination: system modules in two tiers (ADR-0005, ADR-0006, ADR-0007)

The finished repository is a normal game codebase that anyone can clone, build, and run
with assets imported from their own disc or install. Code is grouped by **game system**, and
it is split into a **game tier** that ships and a **reference tier** that proves the game
tier matches the original. Coding rules for both tiers are in [`STYLE.md`](STYLE.md).

```
src/
  cpu/          exact arithmetic models and sadx::fidelity helpers (x87/SSE, Bochs, CRT math)
  math/         Ninja math, D3DX math as linked into sonic.exe
  assets/       imported-pack reader: resource ids, typed records (models, landtables,
                texlists, motions, SET/CAM, physics, sound lists); never reads sonic.exe
  engine/       the Ninja library and core services, by system:
    ninja/        matrix stack, view/projection, draw, sprite, texture, motion, light, fog,
                  render queue, memory
    task/         task scheduler, TaskWork and friends, task callbacks as native functions
    device/       the D3D8-shaped graphics device interface the engine draws through, plus
                  the audio, input and clock interfaces (backends live in platform/)
  input/        pad and keyboard semantics, including the Input mod's behaviour
  player/       common physics, collision response, input, animation; then one module per
                character: sonic/ tails/ knuckles/ amy/ big/ gamma/
  game/         flow/ (njUserMain modes, game loop, resets, startup), camera/, collision/,
                stage/, object/, enemy/, boss/, event/, adventure/, chao/, minigame/, ui/,
                sound/, save/, profile/ (Speedrun 2.0, CCEF, EHCF, Skipma, quick save/load;
                PATCH annotations)
  enhancements/ opt-in features and the hook/settings registry (STYLE.md §9); off by default
  session/      native composition: owns every system's state, boot → title → stage, the
                frame loop, settings (the legacy PE-backed composition is now in guest/)
  platform/mac/ Metal, CoreAudio, GameController, AppKit backends of the engine interfaces
  reference/    (today guest/ and hle/) Unicorn guest, Win32/D3D8 HLE, per-system reference
                adapters, the override table, PE loading; development only
apps/           sadx (production: session + platform + assets), sadx-reference (guest app),
                developer CLIs
tools/import/   the one-time asset importer (stage E); reads the user's disc or install
```

Rules:

* **Game tier** = everything except `reference/` (today: `guest`, `hle`, and the adapter
  code in them). It contains no guest addresses outside provenance annotations, no register
  or stack marshalling, and no callee-address dispatch. Unrecovered dependencies are named
  `STUB` seams (STYLE.md §5).
* **Reference tier** depends on the game tier, never the other way round. Each recovered
  system gets one reference adapter that marshals guest memory to the system's state types,
  fills its service seams from the guest, and exposes it to oracles and to override
  registration. Address tables live only there (gta-reversed's single hook list, OpenLoco's
  path to native builds).
* **Platform** backends implement interfaces declared by the engine. The same Metal device
  backend serves the native session and the reference app, so both draw through one tested
  path.
* **Enhancements** depend on game systems and attach at hook points. Game systems never
  depend on enhancements. Fidelity tests run with every enhancement registered and off.
* **Production and reference build graphs are separate** (M5 `native-source-build`). The
  production app links the game tier, `assets`, `session` and `platform` only.

Migration is per system, together with its tests. A module moves to its destination
directory when it is refactored into game-tier form (GOAL §3 track R). Nested paths are
`src/<layer>/<system>/<slug>.{hpp,cpp}`. The slug stays globally unique and keeps naming its
unit test, probe, oracle and report (`tests/unit/<slug>.cpp`, `oracle-<slug>`), so moving a
file into a system directory renames nothing else. The layer is still the top-level
directory that `check_layers.py` ranks. The layer table below
and `tools/gate/check_layers.py` stay authoritative for the current tree. When a destination
directory is created, its layer rank is added to the checker in the same step.

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
| `player` | native ground acceleration and shared friction, player physics, collision, animation, Sonic, the scene input producer | … input |
| `game` | semantic game-mode flow, WinMain, library init, startup, game loop, resets, views, window, sound bookkeeping, model DLL bindings, mod loader frame, Win32 profile; mode provenance and legacy leaves live in guest | … player |
| `runtime` | reserved for maintained native composition; the former PE-backed composition moved to guest | … game |
| `guest` | ground/friction legacy-call adapters and constant decoding, mode provenance adapter and legacy leaf interpreter (`game_mode_reference`, `game_mode_leaves_reference`), PE-backed `NativeGame` survey and boot configuration (`native_game_reference`, `native_game_session`); in-process x86 guest on Unicorn, image mapping, continuations, pause/resume, override table and FPU shims | … runtime |
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

* Migrated source globals use semantic value members with a preceding `GLOBAL`
  annotation and explicit source-byte extent (`tools/gate/check_provenance.py`).
  Legacy `<meaning>_at_<hex>` owners remain checked by `check_global_owners.py`;
  arrays count their first element and `SourceBytes<base,size>` owns a whole block.
  The new checker also rejects overlaps between annotations and those legacy
  extents. Other code borrows owners by reference/pointer. Both checkers run;
  the new checker has not replaced the legacy checker.
* Every ported code range is listed in its oracle report (`original_ranges_sha256` or
  `source_ranges`) and checked against the profile patch inventory
  (`tools/gate/check_profile_patches.py`, reading `build/oracles/*.json`).
* Native identities (device handles, heap blocks, texture memory) are never guest
  pointer casts. Guest addresses stay `std::uint32_t`.

## Result and failure vocabulary

Current ports (the legacy shape): `*State` (owned facts, `std::optional` for unknowns),
`*Runtime` (borrowed owners + host callbacks), `*Result` with a failure enum,
`source_boundary` (the instruction that could not proceed) and `required_fact` (the address
that was unknown). Nested owners are invoked through `std::function` callbacks and may return
`GameLogicOwnerState::unavailable` (unported) or `failed`. These types live in
`engine/game_logic.hpp` so every layer above `engine` shares them.

Destination (ADR-0007): this vocabulary moves to the reference adapters. Game-tier functions
return their real result types, state holds recovered values, and the remaining unknowns
appear at three boundaries: the adapter (a fixture or guest fact is missing), a `STUB` seam
(the behaviour is not recovered), and the importer (an asset is missing or unsupported).
Each boundary still reports the source boundary and the missing fact.

## The runtime today and tomorrow

`NativeGame` (`src/guest/native_game_reference.*`) is a reference composition of ported
owners and runs the boot
and the frame loop, stopping at the first unported owner (or surveying past it as a
no-op). D3D8 calls go to `D3D8StateOwner` (recording) unless a Metal backend is set.

Its PE-backed boot configuration, mode provenance adapter and legacy leaf interpreter belong
to the reference tier. They live beside it in `src/guest`, and are excluded from `libsadx-core.a`.
`game/game_mode` owns the structured mode decisions. Its private typed state and callee seams
have two link-time bindings: plain native state with explicit missing-callee stops, and the
reference adapter with guest call-site metadata and lazy missing-fact checks. The adapter
calls the same core; it contains no fallback mode interpreter.
The headless survey and the AppKit native survey branch share
`guest/native_game_session.hpp`; the shipping native session remains future work.

`player/ground_acceleration.hpp` exposes plain borrowed motion fields and native dependency
owners. Its shared friction tail lives in `ground_friction.cpp`; both ordinary and alternate
acceleration use that one implementation. The legacy-call adapters in
`guest/player_acceleration_{ground,friction}.cpp` retain missing fixture facts, constant-word
decoding and source diagnostics. Native constants are literals; reference constant words
remain injectable. Native input is supplied by its owner, and uncomposed mutation/yaw owners
stop explicitly. The native-only test needs no PE or guest runtime.

`engine/game_frame_control` owns plain native frame timing, reset input and the limiter.
Its private state seams bind either to native fields or to `guest/frame_control_reference`,
which owns fixture optionals, split counter words and numeric failure boundaries. Both use
the same core. Legacy logic, startup, device, screen, queue and frame-call CPP interpreters
are in `guest/`; their public compatibility headers retain forward declarations only for
the moved timing types. These relocations do not complete semantic rewrites of those systems.
The native host and portable frozen-corpus builds have separate isolated source closures.

`runtime/engine_code_tables` supplies maintained, typed engine code tables: blend and depth
modes, keyboard translations, the unit matrix, and sprite/view constants. The normal native
boot configuration supplies these explicitly. The reference composition converts them at its
boundary; callers omitting that configuration still receive injectable PE-decoded facts,
including their previous missing-fact behavior. Native table source builds without a PE or
reference runtime and is checked against every source encoding. Reset state, content pointers,
the sine table and other initialization families still need migration; this is not a native
session or asset importer.

The hybrid guest executes `sonic.exe` with `process_overrides` supplying native owners.
Each HLE import logs its call for trace diffs. These reference paths do not establish
native boot/title or executable independence. The separate native frame-control and game-mode pilots
exercise their maintained cores without a guest or PE loader.

Naming and provenance follow [ADR-0006](decisions/ADR-0006-symbol-names-and-provenance-annotations.md):
identifiers are semantic, the source address sits in a `// FUNCTION:`/`// GLOBAL:`/`// DATA:`/
`// PATCH:`/`// STUB:`/`// LIBRARY:` annotation, and a generated symbol database
(`tools/symbols/`, from the mod loader's symbol-based headers) supplies the names. Legacy
`*_at_<hex>` owners stay checked by `check_global_owners.py` until migrated.

For M5, sharing moves to a native session and semantic game interfaces. A separate
reference adapter marshals original addresses/registers into the same implementations
for equivalence tests. Native callers use state types and native callbacks directly;
shipping initialization reads declared constants and imported resources rather than
`ExecutableImage::u32(va)`. Model/DLL data references become validated resource identities
at import. Native platform services call Metal, input and audio backends directly.

The current `libsadx-core.a` includes `loader` and EXE-backed runtime initialization,
and the app links `libsadx-guest.a`; its name does not establish independence. Separate
production and reference build graphs as systems migrate. Existing source-global
ownership rules remain enforced until mappings and replacement checks migrate together;
preserve tagged failures and exact observable arithmetic/order throughout.

## Other top-level directories

`tests/{unit,probes,fixtures,python}`, `third_party/` (SoftFloat x87/SSE, Bochs
FPATAN/FSINCOS/FPTAN, each with `sources.txt` + `upstream.json`), `reference/` (tracked:
gate records, `oracle-reports/`, ledgers, provenance; ignored: Ghidra outputs, the pinned
profile install, Windows collector, python venv), `game/` (the user's install, ignored),
`docs/` (ROADMAP, STATUS generated, METHOD, SOURCES, decisions/, HANDOFF journal).

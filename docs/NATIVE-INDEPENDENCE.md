# Native independence dependency inventory

Observed 2026-10-15, after gate 138; prerequisite to GOAL §6 item 2 (stage B).
This is a static source/build-graph inventory, not an independence test or a
claim that every dependency has been found. ADR-0005 defines the destination.
The existing hybrid app remains the reference. Line numbers below identify the
inspected source revision and may move as migration proceeds.

“Destination” and “verification” describe required migration work, unless a
current check is explicitly identified. Categories are **code**, **assets**,
**constants/initial state**, **callbacks**, and **platform**. A container can
supply several categories; reading DLL data does not imply running DLL code.

## Build graph: compilation, reference verification and regeneration

| Observed edge and source | Category and current requirement | Native destination | Verification needed |
|---|---|---|---|
| `all` → `sadx-app`, guest/core/mac archives (`Makefile:29–35,52,77–89`) | Code/platform: app links Unicorn and guest/HLE; portable core also includes the PE loader. The app's link recipe itself does not open an EXE/DLL. Unicorn headers/archive come from the reference Python wheel. | Separate production and optional reference targets; production links only recovered systems and declared native libraries. | Clean production build with EXE/DLLs, reference venv and guest/HLE unavailable; inspect linked objects/dependencies and run it. Not performed here. |
| C++ objects and FP libraries (`Makefile:47–75`) | Code: source compilation plus checked-in SoftFloat/Bochs sources. No original executable is a prerequisite of these recipes. | Retain justified native arithmetic dependencies; no executable-derived code generation in ordinary builds. | Clean isolated build and numerical/reference tests. This inspection does not audit third-party licences or FP fidelity. |
| Included `build/tests.mk` / `build/oracles.mk` (`Makefile:134–139`, `tools/gate/gen_make.py`) | Build metadata: mandatory included-makefile regeneration from manifests and verifier filenames, not game bytes. | Reference/test graph stays separate from production graph. | Regenerate without EXE/DLLs and compare recipes; test target separation. |
| `assets` → `tools/lib/extract.py "$(GAME_EXE)"`; `run-unit-tests: assets …`; `gate: all test … app-smoke` (`Makefile:99–102,145–166`) | Assets/constants: gate and tests currently regenerate packets from the EXE even though `all` has no `assets` edge. `render` also imports every invocation. | One-time typed asset importer; tests use declared imported resources or source fixtures as appropriate. | Asset import tests and production rebuild/run after removing original containers. Existing gate is intentionally reference-dependent. |
| `OVERRIDE_CORPORA` → `oracle-*`; `m1` and `app-smoke` (`Makefile:94–97,141–144,175–189`) | Code/constants/platform: differential verification executes pinned originals through Unicorn and guest HLE. EXE, mod/loader DLL and Windows runtime paths are declared at lines 13–27. | Keep as optional reference verification, outside the shipped game/build. | Existing corpus replay/live digests continue to judge migration; they cannot prove independent production execution. |
| `check-symbols: symbols` (`Makefile:155–160`, `tools/symbols/build_symbol_db.py`) | Metadata: gate generation reads pinned mod-loader headers, sa_tools labels and tracked inventory, not EXE machine code. Source checkouts are additional gate inputs. | Development provenance tooling; production builds should consume maintained source. | Current symbol fixtures/checks establish metadata consistency only; clean production graph must exclude this gate prerequisite. |

No `tools/emit/` script is invoked by the inspected Makefile or its recipe
generator. Their emitted `.cpp`/`.hpp` files are tracked source consumed by the
normal compiler. Thus their original/evidence inputs are **manual regeneration
dependencies**, not mandatory ordinary compilation dependencies. This distinction
does not establish that generated source meets every maintainability criterion.

| Regenerator and source | Category/input and output | Native destination | Verification needed/current check |
|---|---|---|---|
| `tools/emit/emit_mesh_matrix_math.py:8–9,54–102` | Code/constants: retained determinant/inverse assembly under `ROOT.parent/_reference/sadx/{mesh-math-dependencies,scene-mesh-dependencies}`; writes `src/player/player_mesh_matrix.cpp`. Current paths point outside this repo. Checks line counts and records evidence hashes; does not open EXE. | Maintained named matrix operations; evidence remains an optional reproducibility input with an explicit location. | Existing `player_mesh_math` oracle; repair/validate regeneration separately. Not regenerated here; counts/hashes alone do not prove semantic correctness. |
| `tools/emit/emit_player_plane_math.py:10–11,65–80` | Code/constants: retained REA JSON at `ROOT.parent/reference/mesh-transform-dependencies/plane-intersection.json`; checks embedded target SHA; emits `src/player/player_plane.cpp`. Current path also resolves outside this repo. | Maintained plane operations, optional pinned evidence regeneration. | `player_plane` oracle; regenerate/compare independently. Not run here. |
| `tools/emit/emit_sonic_animation_table.py:14–40` | Constants/callback-resource references: explicit EXE and retained pseudocode arguments; validates EXE SHA, complete 147 × six-field assignments, hashes 7,674 source bytes at `0x7cec90`; emits recipe header plus metadata. Consumers include `player_animation_decode.cpp:40`. | Typed animation descriptors with imported action/motion resource IDs. Preserve source initialization behavior where observable. | Existing `sonic_animation_table` producer oracle; typed import must resolve each referenced action and round-trip data. Current generator does not resolve DLL action identities. |
| `tools/emit/emit_sonic_animation_operations.py:13–55` | Code/constants/resource references: explicit EXE plus byte-located assembly; checks each decoded instruction's bytes against PE, interprets MOV/XOR and requires 882 writes; emits operation header/metadata. `sonic_animation_table.cpp:12` iterates these operations at runtime. | Named descriptor initialization with typed resource reads; original-address operation records remain reference metadata rather than shipping dispatch/storage identities. | Producer differential oracle including read/write order and aliasing; independent native composition test. Generation is optional but this operation table is currently a runtime representation. |

## App and boot configuration

| Observed dependency and source | Category/current use | Native destination | Verification needed |
|---|---|---|---|
| `sadx-app --guest …` (`src/apps/sadx_app.mm:163–180`) | Code: opens/parses `GAME_DIR/sonic.exe`, creates x86 process, maps main image, installs FPU sites and native overrides, calls entry `0x643f96`. `reference/inventory/fpu-sites.json` is a runtime input. | Native startup/frame orchestration calling recovered systems directly. Retain this mode as separate reference tool. | Production link/runtime audit with Unicorn, guest/HLE, PE loader and originals unavailable; native replay versus frozen reference corpus. |
| Guest DLL discovery (`src/hle/hle_modules.cpp:73–120`); app `use_real_cpp_runtime` (`sadx_app.mm:167`, `hle_process.cpp:65–66`) | Code/assets/platform: imports and dynamic LoadLibrary map real install files; selected real system DLLs are `msvcp140.dll`, `vcruntime140.dll`; other recognised system DLLs can become HLE modules. Under the pinned profile this mechanism runs loader, Input/Speedrun mods and SDL2. It is transitive/runtime discovery, not a closed DLL list in the app. | Recover pinned profile behavior into native input/game/runtime systems; native platform backends. Import model-container data once. | Full reachable and cold-code accounting, native profile replay, explicit module/file-open tracing. A known system module token is not evidence that a Windows DLL was read or executed. |
| Guest drawing/input (`sadx_app.mm:67–70,137–158,183–190`) | Platform: Metal consumes guest D3D8 commands; GameController samples synthetic XInput user 0, keyboard sends Windows keys. | Native render/input interfaces fed by same recovered game systems. | Rendering goldens, input traces and physical-controller parity; existing assumptions remain explicit. No physical-controller result is asserted here. |
| Native app branch (`sadx_app.mm:204–210`) → `load_boot_config` (`src/guest/native_game_session.cpp:7–20`) | Constants/initial state: both supplied EXE and Input mod DLL must open and parse before native boot runs. This branch does not call their original entry instructions. The loader helper validates PE parsing, not pinned SHA. | Named boot/profile defaults and injected platform settings; no PE-backed NativeGame configuration in production. | Missing-container native boot test, exact defaults/initial-state comparison against pinned reference. |
| Seven optional model DLLs (`game_session.cpp:21–24`) | Assets/code-export identities: probes adjacent `system/` for `CHRMODELS_orig.dll`, `ADV00MODELS.DLL`, `ADV01MODELS.DLL`, `ADV01CMODELS.DLL`, `ADV02MODELS.DLL`, `ADV03MODELS.DLL`, `BOSSCHAOS0MODELS.DLL`. Missing files/parse errors leave slots absent. | Import typed model/action/motion/landtable resources; recover callable export behavior separately. | Per-container hash/version checks, pointer/reference resolution, data round trips and export behavior tests. Optional loading is not evidence that assets are unnecessary. |
| `sonicDX.ini` (`game_session.cpp:26–27`) | Platform/configuration: optional file adjacent to EXE; absent file uses source profile fallbacks later. Not executable code. | Native settings schema preserving profile semantics. | Present/absent/malformed setting tests against reference; source defaults must be distinguished from imported user choices. |
| Input constants (`game_session.cpp:29–32`) | Constants/initial state: seven float bit patterns at DLL addresses `1000bb6c/bbb0/bba0/bba4/bb7c/bba8/bb78`; profile pad settings are already explicit source values. Helper dereferences `u32` without a missing-read branch. | Named source constants with provenance and validated profile configuration. | Bit-exact input producer/pad tests and independent startup; no guessed fallback if source evidence is missing. |

## NativeGame load-image facts and later reads

`src/guest/native_game_reference.cpp:17–216` reads PE-backed optionals using byte/word
helpers. BSS/zero-fill is part of that loader abstraction, so a value read as zero
is still currently obtained from an executable image. The following groups cover
the inspected initializer's owners; they are not an independently verified list
of semantic defaults. Native destinations require whole-owner migration with the
provenance/overlap checker, not replacement of missing facts with zero.

| Group and source lines in `game_runtime.cpp` | Category/current dependency | Native destination | Verification needed |
|---|---|---|---|
| Mode, frame timing/logic/loop/save/reset, RNG, scene/mode leaves (`22–36,54,61,67–68,90–95,161–192`) | Constants/initial state from EXE; many values are global load-image words/bytes rather than initialized runtime state. | Named owned mode/frame/save/reset/scene state with explicit startup operations. | Existing frame/logic/mode/reset oracles plus composed deterministic native startup and reference replay. Stage B starts with frame control only. |
| Keyboard edges, controllers and input state (`38–49,103–104,129–132,145–146,157–160`); Input DLL raw records/DreamPads (`55–59`) | Constants/initial state/platform: raw record templates, mappings, pointers and device-known/null distinctions from both images. | Typed input state and named keyboard/controller tables; native device handles, pinned Input behavior. | Input/keyboard/controller oracles, frame replay and hardware mapping tests; preserve unknown non-null pointer behavior. |
| Audio/menu/sound records and volumes (`50–52,69,129–130,179–182`) | Constants/initial state/platform: audio flags, listener identities and menu blocks; no proof of native playback. | Native sound/menu state and CoreAudio ownership; imported media resources. | Sound/stream oracles, audio-event trace and menu replay. |
| Render/fog/projection/queue/screen/view/texture state (`27–29,64–66,70–84,96–114,133–142,147–156,183–188,194–215`) | Constants/initial state: colors, blend/depth tables, fog arrays, dimensions, matrices, pool/queue pointers and null handles. Several `.value_or(0)` defaults already exist (`71,84,186,200`); inventory does not endorse them as known facts. | Named renderer/scene/texture state, native resource handles and source constants; imported data where content-dependent. | Existing Ninja/screen/projection/fog/view oracles, reference command traces and Metal goldens; distinguish recovered values from assumptions and unsupported record types. |
| Sine table `0x88af78` and unit matrix `0x389d650` (`185,193`), numeric constants (`73,198–199`) | Constants: executable tables/float encodings retained only when all required reads succeed. | Provenanced native constants or identified imported data, with explicit ownership choice. | Full byte comparison, arithmetic oracle coverage and build/run without PE reads; no assumption that host libm reproduces the table. |
| Exit callbacks, frame hooks/records, mode callback and input callback (`53,106–108,127–128,177`) | Callbacks: numeric source identities from the load image; input callback is made known-null only for source zero, later set by native frame-control initialization. | Typed direct native callables/resource identities; address lookup only in reference adapters. | Callback order/null/unavailable cases through common core and reference adapter; cold callback target accounting. |
| Recording lambdas/device defaults (`80–89,115–126,204–210`) | Platform: native command recording is already source, but identity transform on early read is a reported assumption; MultiplyTransform uses host arithmetic. | Explicit native graphics interface with verified creation/default state. | Device creation and transform parity, render traces/goldens; recorded calls alone do not prove backend behavior. |

The executable remains live beyond initialization: `game_runtime.cpp:525` reads
128 matrix slots at `0x3a7cf48`; line 563 falls back to arbitrary EXE config-word
reads; line 596 reads 27 table words at `0x389d888`. Their destinations are named
matrix initialization, boot settings and system tables, checked against source
bytes and composed startup behavior. These must migrate with the initializer.

`game_runtime.cpp:500–512` maps/rebases optional model images, resolves exports and
hands the bindings/images to mode leaves. Slot 0 follows the loader proxy's
`CHRMODELS_orig.dll` convention; the code explicitly reports that the loader's
instructions were not executed to prove this native claim. The destination is a
typed imported resource graph plus native implementations for callable exports;
the model-DLL oracle alone does not prove complete asset import.

## Bring-up extractor

`tools/lib/extract.py:212–223` requires the pinned EXE SHA-256 and uses `PEImage`;
it executes no game instructions. `extract_stage` (`108–209`) follows the Emerald
Coast 1 landtable at `0xe99cb8`, COLs, object graph, model points/indices/materials
and vertex colors, transforms geometry with the sine table, and emits
`emerald-coast.mesh`. The destination is GOAL stage E's versioned typed pack with
resolved references (sa_tools mappings as locators), verified by record round
trips and pointer-site resolution. This inspection mesh omits textures/UVs,
animation, object placement and gameplay/collision visibility semantics; it uses
reconstruction rounding and inspection lighting (`1–5,59,186–194,201–202`).

The same mandatory `assets` recipe also emits these reference/test packets
(`extract.py:221–299`):

| Category | Observed packet groups | Native destination and verification |
|---|---|---|
| Constants/initial state | `ninja-trig`, eight `player-parameters` records, `ninja-unit`; input-producer, ground/event acceleration constants; rotation, projection angle (and extra), position, contact attribute/response/initialization, animation, Sonic player, scene position, reaction, mesh query, floor angle, CRT floor, scene/path/direction reset and coordinate-axis packets | Named provenanced constants and typed player configuration; classify content tables before choosing source versus imported pack. Byte-exact comparison and each module's differential oracle remain necessary. |
| Constants/assets | Ninja blend/depth tables, texture return words, queue-consumer/sprite constants and sprite record, player auxiliary-effect table | Typed tables/records or imported resources as established by evidence; verify every field/bit and typed references, then associated render/player oracle and replay. |
| Callbacks/code metadata | `sonic-action-dispatch` reads 352 bytes at `0x49bfbc`; contact-attribute/mesh-query packets also include words from code-range addresses | Named native dispatch and source constants; addresses/embedded instruction bytes must stay reference evidence, not become production callback IDs. Enumerate targets/meaning before migration and test dispatch/cold cases. |
| Code evidence | `source.json` includes baseline CCEF/EHCF patch-site bytes (`256–260`), PE description, source offsets and hashes | Optional reference provenance/profile verification; do not require these executable bytes to build/play. Profile behavior is native code, checked by profile-specific replay. |

`source.json` explicitly labels observations as offline file bytes, not REA or
runtime verification (`225`). Hashes establish identity/integrity, not fidelity.
The packet list is the extractor's output list, not proof that every packet is
needed by the shipping game or that it supplies all constants consumers need.

## Unexamined scope and completion evidence

This step reviewed the named entry points, all four current `tools/emit/*.py`
scripts and the top-level Makefile/recipe generator. It did not run a native
independence experiment, regenerate emitters, change gameplay or run the gate.
No new REA evidence was needed for these static dependency edges.

Still unexamined: the complete recursively loaded DLL/export set on every path;
all dynamic filesystem opens (including textures, SET/CAM, saves, INIs, FMV and
audio); all downstream model/animation decoders and executable reads outside
these entry points; transitive callback targets; unreachable/cold game/mod code;
build/install/package scripts outside the inspected graph; full provenance and
licensing of generated data; every supported disc/install variant. Platform
assumptions in native/HLE runtime reports remain unresolved evidence, including
skipped startup owners and media/device initialization.

M5 completion still requires its full ROADMAP acceptance tests: clean source
build, typed one-time import, native runtime with original containers and x86
runtime unavailable, recovered-system maintenance interfaces, profile/fidelity
and cold-path accounting. A passing frame-control pilot will prove only that
slice. This inventory establishes work to remove dependencies; it removes none.

## Frame-control pilot after the inventory

`make native-frame-control` now verifies a bounded independent module. It copies
six maintained source files (the frame-control and Speedrun core headers/bodies,
a native host journey and the existing flat probe) plus a frozen numeric corpus
into a temporary directory, compiles directly, checks compiler dependencies,
symbols and linked libraries, and runs with a minimal environment. No core archive,
PE loader, guest/HLE, Unicorn or original binary is used by this check. The tool
records hashes and scope in `build/native-frame-control.json`; it is an isolated
source/link test, not an OS filesystem-access sandbox.

The native caller supplies mode state, reset input/save facts and clock callbacks.
It invokes the installed native input callable directly. `FrameRateModeState`
holds the three frame-mode owners; the existing logic-timing state extends it.
`CounterWords` keeps low/high availability independent, preserving the original
failure and store ordering. Original-address/register transport stays in the
reference adapters; source instruction addresses in failure diagnostics remain
provenance rather than dispatch identities. Eleven annotated globals cover 50
source bytes; the pre-existing callback pointer's source width is now explicitly
four bytes instead of the old checker's unknown-type first-byte extent.

`reference/frame-control-corpus.tsv` freezes all 2,672 original frame-control
fixtures and outputs as numbers, with source/range/generator hashes in its JSON
metadata. It is generated by `tools/oracles/freeze_frame_control.py` from the
unchanged original-instruction oracle, never from native results. The isolated
check replays those rows through the same probe/core without the executable.
The native host journey additionally covers the 63 FPS limiter, reset/save input
and mode transitions. The limiter's full development oracle corpus is not frozen
here: frozen differential scope is the four game entries, while its native host
journey and separate pinned-mod oracle provide their stated additional coverage.

This removes no dependency from `sadx-app`, `NativeGame` initialization, asset
extraction or the production build graph. It proves frame control only. Native
boot/title, resources, other owners, hardware parity and all remaining M5 rows
still require their own recovery and verification.

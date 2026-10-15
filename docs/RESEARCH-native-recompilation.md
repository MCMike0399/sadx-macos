# Native recompilation references (2026-10-15)

Question: what can SADX learn from LASRecomp and pokeheartgold when the desired
product is a readable, maintainable Apple Silicon application that builds and runs
without `sonic.exe`, with assets supplied separately?

This is a source review, not an independent build or playthrough of either project.
The commits below were resolved with `git ls-remote ... HEAD` on 2026-10-15. Links
pin the inspected revision so later changes cannot silently change the evidence.

| Project | Inspected commit | What it produces |
|---|---|---|
| LASRecomp | `6244eb9d4d3497094e21cac7dabe998ded1f2800` | Native application plus a locally generated game library |
| ctrl-cheeb-del/pokeheartgold | `77565981bfdef611f982a87a4116702e0b8fe266` | Matching Nintendo DS ROMs |

## LASRecomp: ahead-of-time native execution with a guest model

LASRecomp describes static recompilation of Switch Link's Awakening, with Metal on
Apple Silicon and Vulkan on other platforms. Its releases exclude the game's code;
first launch generates and compiles that code from the user's own v1.0.0 extracted
game. Development builds can perform the same generation with `make gen`. A shipped
compiler and generator make that process local to the player's machine.
[Source: README](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/README.md).

The build separates the handwritten runtime from generated `main/*.c` and `sdk/*.c`.
`make gen` consumes `extracted/exefs/main` and `extracted/exefs/sdk` at fixed module
bases. On macOS the generated objects become `lasgame.dylib`; the runtime links SDL3,
FFmpeg and Apple frameworks, including Metal. The generator, runtime headers and
flags contribute to a library identity used to invalidate stale generated code.
[Source: Makefile](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/Makefile#L35-L62).

This is instruction lifting, not recovery of human-level game source. The lifter
turns integer instructions, memory access and control flow into C. On AArch64 it
can re-emit floating-point/SIMD instructions as inline assembly; other hosts use
portable lowering. Its `Unsupported` exception makes unsupported instruction
decoding visible. That same-ISA SIMD path does not directly transfer to SADX's x86
instructions and x87 arithmetic on Apple Silicon.
[Source: lifter](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/tools/lifter.py#L1-L6).

The generator emits functions named from their addresses (`f_<hex>`), taking
`Cpu *`. Direct calls can become direct native calls; branches become labels such
as `L_<hex>`, and indirect branches update the guest PC and dispatch. It discovers
boundaries and jump-table targets, including using Unicorn during generation.
These are inspectable C files, but their structure follows machine execution;
they do not establish semantic names, ordinary typed game objects or maintainable
domain APIs automatically.
[Source: generator](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/tools/gen.py#L335-L378).

The runtime retains guest registers, flags, SIMD state and a PC in `Cpu`, and guest
addresses are host addresses. Its dispatcher maps guest addresses to recompiled
functions and supports overrides by address. Thus native machine instructions and
an address-oriented runtime can coexist. Removing an interpreter alone would not
meet the user's readability objective.
[Sources: CPU model](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/runtime/cpu.h#L7-L35),
[dispatcher](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/runtime/dispatch.c#L11-L24).

Crucially, original executable modules remain a runtime dependency. `boot_game`
constructs the ExeFS path and calls `loader_load(exefs)`. The loader opens `main`
and `sdk`, checks their NSO headers, loads all three segments, parses module
metadata and applies relocations. RomFS assets have a separate path. The originals
are therefore needed beyond the initial compilation, even though their machine
instructions are executed through compiled replacements.
[Sources: boot](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/runtime/main.c#L126-L135),
[loader](https://github.com/peiikai/LASRecomp/blob/6244eb9d4d3497094e21cac7dabe998ded1f2800/runtime/loader.c#L40-L95).

Useful lessons: automate broad recovery, separate platform code and game code,
check generated artifact identities, and replace platform APIs at a clear boundary.
Copying this architecture would preserve several dependencies the user wants SADX
eventually to remove. This conclusion follows from the build and loader above.

## pokeheartgold: source reconstruction targeting original hardware

The repository calls itself an independent matching decompilation and says both
US ROMs reproduce the listed SHA-1 hashes. Those are project claims; this research
did not rebuild them. Its product is an `.nds` ROM, so the fact that its build runs
on macOS does not make the resulting game a native macOS application.
[Source: README](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/README.md).

The build selects an ARM946E/ARMv5TE target and Metrowerks compiler, links ARM9/ARM7
components, packages the Nintendo filesystem into the ROM and, when comparison is
enabled, checks the output against a recorded SHA-1. This validates matching output
for the original architecture. A macOS ARM64 port would need different platform and
hardware implementations; it cannot use that byte-for-byte ROM criterion as its
own whole-program success test.
[Source: Makefile](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/Makefile#L118-L123).

The documented setup requires user-supplied compiler and Nitro SDK archives; on
Apple Silicon it uses Rosetta and Wine for the toolchain. It describes building
from source and checksum manifests, without a base-ROM input step in that documented
workflow. This is a narrower observation than proving every optional analysis tool
works without reference ROMs.
[Source: installation guide](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/INSTALL.md).

There are good examples of semantic source: `bag.c` operates on `Bag`, `ItemSlot`,
named pockets and item IDs through operations such as registration, insertion and
removal. `main.c` exposes initialization, save creation, input and overlay lifecycle
with domain-oriented names. Those are useful models for SADX's gameplay code,
even though the underlying Nintendo services need replacement for a native port.
[Sources: bag](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/src/bag.c#L11-L58),
[main loop](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/src/main.c#L48-L100).

The README's all-C/C++ language claim does not imply portable C everywhere:
`lib/NitroSDK/src/crt0.c` contains `asm void _start(void)` with ARM instructions,
hardware addresses and CP15 accesses. The repository's own progress documentation
also distinguishes C files containing assembly from C-only objects. File extensions
or a decompilation percentage alone do not demonstrate the desired portability.
[Sources: startup](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/lib/NitroSDK/src/crt0.c#L34-L110),
[progress definitions](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/INSTALL.md).

Its filesystem build enumerates packaged game resources and rebuilds archives from
the `files/` tree; that tree also contains tracked image and binary resources.
This differs from SADX's policy of keeping game assets external and ignored. The
useful architectural idea is separating resources from game logic; this comparison
does not recommend copying their asset-distribution arrangement or draw legal
conclusions about it.
[Sources: filesystem rules](https://github.com/ctrl-cheeb-del/pokeheartgold/blob/77565981bfdef611f982a87a4116702e0b8fe266/filesystem.mk#L560-L564),
[resource tree](https://github.com/ctrl-cheeb-del/pokeheartgold/tree/77565981bfdef611f982a87a4116702e0b8fe266/files).

## Consequences for the SADX goal

These are requirements adopted in ADR-0005 from the user's request, rather than
claims that either reference already meets them:

1. Distinguish **native execution**, **independence from the original executable**
   and **maintainable recovered source**. They require separate acceptance tests.
2. Make the final game a normal Apple Silicon source build. Ordinary development
   builds and user launches must not read, regenerate from, load or execute
   `sonic.exe` or profile DLLs. Keep pinned originals as optional analysis/oracle
   inputs in a separate reference workflow. An explicit one-time importer may read
   executable-resident resources from the user's owned media, producing documented
   data-only resources; that does not authorize execution of the imported binaries.
3. Use recovered semantic functions, types and owned state as production APIs.
   Preserve original addresses in evidence maps, oracle adapters and diagnostics;
   address provenance remains useful without being the gameplay programming model.
4. Preserve the existing faithful arithmetic and behavioral evidence. Extract
   complete subsystems with their transitive dependencies and test representative
   journeys, rather than measuring progress only by renamed functions or recovered
   bytes. Static lifting can help recovery, but generated register-level C alone
   does not meet the source-quality requirement.
5. Audit executable-resident non-code data explicitly: initial globals, lookup
   tables, models, strings, relocations and function-pointer tables cannot simply
   disappear when the PE loader is removed. Classify each item into recovered
   source-owned data, typed references, or documented externally supplied resources.
   A renamed executable memory dump would preserve the dependency in another form.
6. Import only documented resource formats from the user's disc/install folder.
   Keep asset import independent from production compilation, with clear provenance
   for required executable/DLL-resident resources. The final acceptance fixture must
   contain supported assets while excluding all original executable modules.
7. Express the pinned speedrun profile as native behavior with tested configuration
   and timing semantics. An address patch list remains reference evidence; it is not
   the final mechanism by which the native application acquires those features.

Neither reference establishes the SADX-specific cost of recovering every remaining
subsystem or proves correctness of this repository's current implementation.

## REA source audit: evidence for recovery, not a whole-program compiler

Inspected the clean local REA source checkout at
`23d50ea3bec186b4459d36c04854a92276accdf1` (package version 6.4.0), and checked the
installed 6.4.0 bridge used by SADX. Their Java files are not byte-identical; version
labels alone do not establish equal implementations. The bounds and annotation
lifecycle below also appear in the installed bridge/capability declarations. This
was source inspection; no new Ghidra analysis or provider conformance run was performed.

* `ReaGhidraBridge.java` calls Ghidra's `decompileFunction` and obtains C text through
  `getDecompiledFunction().getC()`. Its high-p-code extraction selects at most 3,000
  operations, 64 inputs per operation and 12,000 def-use edges and reports omissions.
  This is a bounded analysis representation, unsuitable as an assumed complete input
  for compiling arbitrary game functions.
  [Bridge source](https://github.com/morluto/rea/blob/23d50ea3bec186b4459d36c04854a92276accdf1/bridge/ghidra/ReaGhidraBridge.java#L105-L107).
* `EnhancedTools.#batchDecompile` resolves each requested procedure and retrieves its
  pseudocode with per-item status. `GhidraRequestQueue` serializes requests to a Program.
  Batching reduces orchestration work; it does not recover omitted call targets or
  produce a linked game.
  [Batch workflow](https://github.com/morluto/rea/blob/23d50ea3bec186b4459d36c04854a92276accdf1/src/application/EnhancedTools.ts#L250-L300),
  [request queue](https://github.com/morluto/rea/blob/23d50ea3bec186b4459d36c04854a92276accdf1/src/ghidra/GhidraRequestQueue.ts#L37-L52).
* Provider capabilities qualify caller/callee lists as resolved references only;
  targetless computed flows remain unknown. Dossier value flow does not establish
  memory aliasing or whole-program side effects. Use reference execution and assembly
  to investigate these gaps rather than declaring the inventory complete.
  [Provider semantics](https://github.com/morluto/rea/blob/23d50ea3bec186b4459d36c04854a92276accdf1/docs/ghidra-provider.md#L48-L56).
* Names and entry comments can be edited in a macOS analysis session. They are ephemeral,
  invalidate immutable snapshot reuse/saving after mutation, and require evidence export
  followed by closing without a snapshot. `export_evidence_bundle` writes deterministic
  Evidence, not buildable game source. Durable recovered names, layouts and ownership
  therefore need repository-owned source/provenance records.
  [Capability declarations](https://github.com/morluto/rea/blob/23d50ea3bec186b4459d36c04854a92276accdf1/src/ghidra/GhidraProviderCapabilities.ts#L59-L78),
  [session/export contracts](https://github.com/morluto/rea/blob/23d50ea3bec186b4459d36c04854a92276accdf1/src/contracts/sessionToolContracts.ts#L75-L95).

The owned SADX runner currently admits analysis tools and always tries to save a snapshot
on close; it does not admit naming/annotation tools. Adding them would require a deliberate
lifecycle change. The normal recovery path can use versioned source maps without that
change. [Local runner](../reference/ghidra-live.mjs).

## Current SADX dependencies and migration implications

These observations are an initial audit of specific load paths, not a complete program
inventory. Source references are relative to this repository so future work can verify them.

| Observed dependency | Source | Required destination |
|---|---|---|
| App guest mode opens `GAME/sonic.exe`, parses a PE image and constructs the guest process | [sadx_app.mm](../src/apps/sadx_app.mm) | Native session consuming an imported pack; reference app remains separate |
| Even native boot loads the EXE, Input DLL and available model DLLs; reads input constants at original VAs | [game_session.cpp](../src/runtime/game_session.cpp) | Declared native profile/configuration and typed imported resources |
| Native initialization reads many original globals, tables and callbacks from `ExecutableImage` | [game_runtime.cpp](../src/runtime/game_runtime.cpp) | Explicit initial state, owned data and native callback identities, classified and verified per system |
| Overrides bind original addresses and transport registers/stack/memory | [override_table.hpp](../src/guest/override_table.hpp), [hle_device_bridge.cpp](../src/hle/hle_device_bridge.cpp) | Reference adapters to the same maintained native core used by native callers |
| Asset extraction reads the sine table, player parameters and Emerald Coast land geometry from EXE VAs; the tool explicitly excludes full scene/content semantics | [extract.py](../tools/lib/extract.py) | Complete versioned asset importer, with resource references and source-variant validation |
| The core archive includes `loader` and EXE-backed runtime code; `sadx-app` links guest/Unicorn, and emitters consume original data | [Makefile](../Makefile), [emit tools](../tools/emit/) | Distinct native production and reference build graphs; ordinary builds independent of original binaries |
| The global-owner checker discovers `*_at_<hex>` members and source-byte blocks | [check_global_owners.py](../tools/gate/check_global_owners.py) | Preserve this enforcement while migrating each owner and mapping; add equivalent validation before removing the representation |

The existing [game_frame_control](../src/engine/game_frame_control.hpp) already has named
operations, a real native callback type, explicit failure results and an injectable limiter
clock. This makes it a bounded first pilot: recover semantic state and a native caller,
retain an adapter to the existing oracle, and test it with original binaries and Unicorn
unavailable. It is a frame-control proof only. Boot/title, full content import, gameplay
systems and profile-wide independence remain subsequent work.

This supports incremental source reconstruction using the existing verification investment.
It does not establish that bulk pseudocode compilation will recover SADX, that a rename
removes a dependency, or that the full migration has a measured completion date.

## Second pass (2026-10-15): more projects, SADX resources, agent decompilation

This pass widened the comparison to projects that went from hooking an original x86 PC
executable to standalone source, to decomp-based native ports, and to published LLM-agent
decompilation runs. Counts marked *counted* were taken from shallow clones at HEAD on
2026-10-15; items marked *unverified* rest on a secondary source. The conclusions feed
[ADR-0006](decisions/ADR-0006-symbol-names-and-provenance-annotations.md) and `docs/GOAL.md` §4.3–4.5.

### Hook-then-replace projects (the path SADX's hybrid runtime is on)

* **re3/reVC** (GTA III/VC): started in 2018 "by replacing single functions of the game with
  their reversed counterparts using a dll"; standalone exe in April 2020 (~180k lines); assets
  still from the user. Original repository is DMCA'd; read through the archived mirror
  [Reverse-Engineered-III](https://github.com/Huga22118/Reverse-Engineered-III). Exact hook
  syntax *unverified*.
* **gta-reversed** (GTA SA, [repo](https://github.com/gta-reversed/gta-reversed)): an injected
  DLL with 6,066 `RH_ScopedInstall(Name, 0xADDR)` registrations (*counted*), globals as
  `StaticRef<T>(0xADDR)`, a per-function reversed flag (`docs/hooks.json`), and an AI-agent
  notes file (`docs/ai/`). Registration is one table; the game code itself is named C++.
* **OpenRCT2 / OpenLoco**: hooks into the original exe until every function was native; then
  OpenLoco moved globals off fixed addresses (`loco_global<T, addr>` 342 → 157 between
  releases, [v25.11 notes](https://openloco.io/news/2025/11/openloco-v25.11.html)) before
  "native, independent 64-bit builds" (v25.12 CHANGELOG). Address provenance survives only as
  comments: 3,047 `// 0x00…` function comments at HEAD (*counted*). OpenRCT2's differential
  paint harness (`test/testpaint`) was removed in 2022 because "it hasn't compiled for years"
  once the original was out of the loop.
* **isledecomp + reccmp** ([isle](https://github.com/isledecomp/isle),
  [reccmp](https://github.com/isledecomp/reccmp)): provenance is a comment, never an
  identifier (`// FUNCTION: LEGO1 0x100b12c0`, plus `GLOBAL`, `VTABLE`, `STUB`, `LIBRARY`,
  `SYNTHETIC`, `TEMPLATE`), linted by `decomplint`; reccmp scores recompiled MSVC functions
  against the original. [isle-portable](https://github.com/isledecomp/isle-portable) builds the
  same source for macOS arm64 and keeps 2,379 `// FUNCTION: LEGO1` annotations (*counted*).
* **devilution → DevilutionX** ([repo](https://github.com/diasurgical/devilution)): a faithful
  reference tree kept for documentation and a separate modernised portable fork.

Lessons: the hybrid phase is a proven route, not a dead end, provided (1) game code is named
from the start and address registration is confined to one table, (2) state migration off the
original layout is planned and measured, and (3) the differential evidence is frozen before the
original leaves the loop.

### Decomp-based native ports and static recompilers

* **sm64 → sm64-port/sm64ex**, **OoT decomp → Ship of Harkinian**: C source; assets described
  by offset, name and type (OoT: 763 `assets/xml` files, *counted*) and extracted once from the
  user's ROM, in SoH into an `.o2r` archive behind a platform layer
  ([Shipwright](https://github.com/HarbourMasters/Shipwright)).
* **N64Recomp/Zelda64Recomp**, **XenonRecomp/UnleashedRecompiled**: static recompilation with
  hand-written C patches/overrides (Zelda64Recomp writes patches against the MM decomp headers;
  Xenon's weak `PPC_FUNC` symbols let overrides win). Both need the original binary to build or
  run; generated code stays `sub_XXXXXXXX`/symbol-file names.
  ([N64Recomp](https://github.com/N64Recomp/N64Recomp),
  [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)).

Lesson: none of the static recompilers reached binary-free readable logic; every port that did
was decompilation with names. SADX already has "the whole game running" through Unicorn, so a
recompiler would buy speed, not the goal.

### SADX-specific resources

* **Mod loader headers** (already the `reference/sadx-mod-loader` submodule, eb4cdcca):
  `SADXFunctionsNew.h`/`SADXVariablesNew.h`/`SADXStructsNew.h`/`SADXEnumsNew.h`, headed
  "based on symbols", plus the classic `SADXFunctions.h` etc. Measured locally: ~3,800 distinct
  function addresses, ~3,150 global addresses, 468 struct definitions. Examples: 411e30
  `dsInitInt`/`SetFrameRateMode`, 7899a0 `stHWSetVSync`, 3b11188 `FrameMultiplier`, 40c090
  `GameModeHandler`, 421180 `texLoadTexturePvmFile`/`LoadPVM`, struct `task` with `exec`/`disp`.
  The repository did not use them before this pass. Their symbol origin is reported as an Xbox 360
  prototype PDB ([build list](https://dcmods.unreliable.network/index.php/2021/09/03/list-of-available-sonic-adventure-builds/)) — *unverified*.
* **X-Hax sa_tools** ([repo](https://github.com/X-Hax/sa_tools), inspected at
  `b72d52f3`): `GameConfig/PC_SADX/` (140 files) maps data embedded in `sonic.exe` and the
  model DLLs. The combined `sadxpc` mapping has 13,160 entries (*counted*: 9,416 basic models,
  1,680 animations, 480 chunk models, 456 texlists, 180 cutscene texts, 123 sound lists,
  62 landtables, 60 level texlists, 57 death zones …), each with `address=`, `type=`, output
  file and `pointer=` (referencing code sites); `sonic_labels.txt` adds data labels. This is the
  schema the importer needs. Its libraries are C#; reimplement formats, check the licence
  before reusing any code.
* **Related decompilation efforts**: [SADX-cutscene-decompilation](https://github.com/X-Hax/SADX-cutscene-decompilation)
  (event scripts as C++); [doldecomp/sadx](https://github.com/doldecomp/sadx) (GameCube, early,
  dtk/objdiff); [FraGag/sadx_decomp](https://github.com/FraGag/sadx_decomp) (2013, dormant).

### LLM-agent decompilation

* [LLM4Decompile](https://arxiv.org/abs/2403.05286): Ghidra pseudocode re-executes correctly in
  ~20% of its benchmark; LLM refinement of that pseudocode improves it substantially. Pseudocode
  plus refinement beats end-to-end generation.
* Published agent runs (Rock Band 3 [agent swarms](https://freeqaz.com/blog/how-agent-swarms-decompile-games),
  Melee [planner/worker/judge](https://stephenjayakar.com/posts/magic-decomp/),
  Klonoa GBA [project notes](https://gambiconf.substack.com/p/starting-a-decompilation-project),
  Snowboard Kids 2 [one-shot loop](https://blog.chrislewis.au/the-unexpected-effectiveness-of-one-shot-decompilation-with-claude/))
  report the same success factors: a fast, mechanical, unforgeable equivalence check; names
  and types from prior work in context; correct function boundaries; one function per task with
  parallel workers; independent judge/review agents; caps on dead ends. They also report agents
  gaming loose metrics (tolerance or compiler tweaks) — figures from these blog posts are the
  authors' claims, *not reproduced*.

For SADX the target is not a byte-matching MSVC build, so the equivalence check is
differential execution against the Unicorn reference (oracles, override-equivalence, replay)
— already this repository's strongest asset. The missing pieces were the names (the headers
above), an address-as-annotation convention, a data importer schema (sa_tools), and the
agent-loop rules now in `AGENTS.md`.

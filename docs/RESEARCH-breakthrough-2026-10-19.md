# Moving from verified functions to a native game

Investigated 2026-10-19 against project sources and first-party project reports.
External release and progress statements below are their authors' claims; none was
independently built or played for this note. This supplements, rather than replaces,
[the earlier comparison](RESEARCH-native-recompilation.md).

## Diagnosis supported by this repository

The immediate constraint is **composition**, not lack of decompiler access. The last
recorded [status](STATUS.md) has 115 probes and 116 oracles, yet no native journey.
The semantic [game-mode implementation](../src/game/game_mode.cpp) exists, but its
[native adapter](../src/game/game_mode_native.cpp) throws at initialization, logo,
title, frame-rate and other seams. Native boot still uses a
[PE-backed session](../src/guest/native_game_session.cpp). Recovering another
independent leaf cannot make that composition display a frame.

The journal reinforces this: gates 169–175 preserve or prepare fog and scene-position
judges while R1–R4 remain unchanged. This work preserves valuable evidence, but
does not close a native journey. The last recorded stage rate, 13.64 FPS at gate171,
also leaves a separate reference-runtime throughput problem. These are observed
outcomes, not an estimate of time wasted or a claim those checks were unnecessary.
[Evidence: HANDOFF](HANDOFF.md).

Readability work has the same composition problem. The already migrated fog and
frame-control modules are useful models; merely moving address-oriented behavior
into `guest/` cannot create an ordinary native caller. The next module should be
judged through its public native interface and unchanged reference adapter together.
[Evidence: STYLE §§11–12](STYLE.md).

The current session's exploratory Station Square measurements also narrow the
performance question: the same 120-frame workload measured 10.97 FPS before,
13.99 FPS with indexed graph synchronization, and 14.85 FPS in a temporary
original-guest-dispatch experiment. State and screenshot hashes matched; the last
experiment was not retained. Concurrent workloads make these directional evidence,
not a controlled benchmark or a 63 FPS result. An intro sample spent approximately
90% in the guest, with `helper_uc_tracecode` accounting for 893 of 3,473 samples.
The next performance investigation should isolate interpreter/JIT tracing cost or
remove it through a closed native slice. More renderer fog coverage does not
address that measured hotspot. Final session evidence belongs in the accompanying
handoff and gate record; these numbers do not establish a general speedup.

## What the apparent breakthroughs actually establish

| Project / first-party evidence | Demonstrated or claimed result | Transfer to this project |
|---|---|---|
| [SARecomp README](https://github.com/sonicfreak1337/SARecomp), [development setup](https://github.com/sonicfreak1337/SARecomp/blob/main/docs/DEVELOPMENT.md) | Playable Dreamcast beta on Windows/Linux/Deck, native rendering. Development consumes a pinned Katana runtime and retained compiled game partitions; a complete source bootstrap is explicitly unprovided. macOS is a later possibility. | Strong evidence that broad native execution can deliver visible coverage rapidly. It does not establish a clean, readable, executable-independent source build meeting ADR-0005. Examine its connected-group qualification and capture methods, rather than importing generated register code into the game tier. |
| [OpenLoco v25.12](https://openloco.io/news/2025/12/openloco-v25.12.html) | C++ reimplementation completed in September 2025; native 64-bit Windows/Linux/macOS followed in December after fixed-address owners fell from 342 in October to zero. Interop was then removed. | A close architectural comparator: code recovery and owned state migration are separate deliverables. Complete the native data/callback closure of a system instead of counting translated functions. |
| [Maurice Heumann, October 9](https://momo5502.com/posts/2026-10-09-game-decompilation/) | Reports 99% reconstructed functions and 83% byte exact after months; source stays private. An initially playable, readable implementation had semantic errors; a protected compiler/relocation/type judge changed the outcome. Portability had been deferred. | Supports objective differential feedback and fixed judges, already present here. It provides no public reproducible portable source build and no basis for treating parallel token throughput as a replacement for integration. |
| [Snowboard Kids 2 author, February 16](https://blog.chrislewis.au/the-long-tail-of-llm-assisted-decompilation/) | Reports rapid matching progress followed by a long tail. Similar recovered functions, domain graphics tools and cleanup helped; awkward permuter output harmed later attempts. | Supply one clean recovered neighbor and domain decoding per attempt. Existing clean fog/frame-control source is better context than another unrestricted pseudocode dump. Similarity scheduling can help recovery, but journey dependency rank must choose the system. |
| [Dreamcast Sonic Adventure decomp](https://github.com/consolesplayingconsoles/sonic-adventure-decomp) | An in-progress kickoff with named functions, assembly reconstruction and an equivalence checker; rebuilding from assembly is distinct from recovered C coverage. | Useful sibling evidence for Ninja names, assets and behavior. Dreamcast SH-4 behavior is a hypothesis for this PC speedrun profile, not its fidelity judge. |

The practical inference is to retain the current differential investment and change
the **unit of delivery**: a native executable completing one visible journey, with
semantic source and a closed resource/state/callback graph. A source-quality goal
does not require replacing the useful guest reference with another recompiler.

## Readable source patterns to copy, with actual code references

These source trees were inspected at the pinned revisions linked below on October
19. They are examples of particular interfaces, not blanket endorsements of every
file or evidence that SADX should inherit their compatibility policy.

* **Named initialization in OpenLoco.** `OpenLoco::initialise()` visibly orders entity
  reset, language loading, startup checks, shortcuts, map allocation, graphics,
  UI and scenario/object indexes. The sequencing remains ordinary calls to the
  systems that own the work. A maintainer can find the language or graphics change
  without resolving a numeric dispatch table. Its `Config` separately groups typed
  display, audio and network settings. This is the immediate model for our library
  initializer: named state and operations, with source sites mapped privately by
  the reference adapter.
  [Initializer](https://github.com/OpenLoco/OpenLoco/blob/6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e/src/OpenLoco/src/OpenLoco.cpp#L155),
  [typed configuration](https://github.com/OpenLoco/OpenLoco/blob/6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e/src/OpenLoco/include/OpenLoco/Config.h#L146).
* **Creation policy and owned lifetime in isle-portable.** `MxOmniCreateParam`
  carries media path, window, video parameters and creation flags. `MxOmni::Create`
  asks named flag questions, builds named managers and handles each manager's
  failure explicitly; `Destroy` refers to those same owners. Function addresses
  stay in provenance comments. The transferable idea is a recognizable lifecycle
  over typed fields. Copying its singleton/global choices or adding generic
  manager factories here is unnecessary.
  [Creation parameters](https://github.com/isledecomp/isle-portable/blob/d0bf04fe605d7ec2a291e65df08bf26712452379/LEGO1/omni/include/mxomnicreateparam.h),
  [creation/destruction](https://github.com/isledecomp/isle-portable/blob/d0bf04fe605d7ec2a291e65df08bf26712452379/LEGO1/omni/src/main/mxmain.cpp#L94).
* **Typed game/session flow and content in TRX.** The executable entry point works
  with `SHELL_ARGS`, named mod selection and a session restart, while the session
  owns and releases its arguments. The game-flow schema names content operations
  such as pictures, loading screens and level statistics. Community content can
  therefore describe a sequence without knowing an executable address. The
  migration guide explicitly separates player settings from game-flow content;
  that distinction prevents an expanding game-flow file from becoming the owner
  of every feature switch.
  [Entry point](https://github.com/LostArtefacts/TRX/blob/5a3f76f546e4f650464eac865394092e849fdf4d/src/trx/game/shell/main.c),
  [session ownership](https://github.com/LostArtefacts/TRX/blob/5a3f76f546e4f650464eac865394092e849fdf4d/src/trx/game/shell/session.c),
  [content schema](https://github.com/LostArtefacts/TRX/blob/5a3f76f546e4f650464eac865394092e849fdf4d/docs/gameflow.schema.json),
  [settings migration](https://lostartefacts.dev/trx/docs/stable/migrating).

For SADX, the user-facing programming model should consequently be a typed native
configuration, system-owned state, named initialization/cleanup calls and resource
identities. A reference adapter may translate a small private operation enum into
original calls and retain missing-fact diagnostics. Its instruction-by-instruction
read and call bookkeeping must not become the public interface another developer
has to learn to initialize the game.

In particular, replacing `config(0x28, site)` with `read(Field::PolyBufferQuad, site)`
inside an otherwise generic dispatcher is only a cosmetic step. The final game
code should express the operation over `polygon_buffers.quad`, and its caller
should not supply register words, source addresses, or fixture-availability flags.
The reference backend preserves the old read/store/callback order; the unchanged
oracle judges that preservation. A single eager configuration snapshot is unsafe
where callbacks can change values or an early failure leaves only some stores.

There is concrete existing type evidence, and a warning against trusting it blindly:
the mod loader declares `NES_LIB_PARAM` at `0x892944`, four bytes after this port's
startup-block base, and supplies names for the polygon-buffer counts and settings.
Exact PE bytes and the REA dossier for its string-copy callee establish **four**
initial strings: company, application, system root and `BUP` root. The header's
three-string layout is incomplete/shifted for that block; its first field cannot
simply become a host instance handle. This is why semantic recovery needs the
original header, decoded calls and actual data together, rather than indiscriminately
renaming hex offsets. The existing generic-window-rectangle comment is also wrong.
[Local declaration](../reference/sadx-mod-loader/SADXModLoader/include/SADXVariablesNew.h),
[structure](../reference/sadx-mod-loader/SADXModLoader/include/SADXStructsNew.h).

For future features, expose events or specific vanilla decisions at the owning
system, keep configuration typed and default faithful, and name resources in the
imported pack. A practice overlay should observe frame/player events; a gameplay
change should attach at a documented decision and mark its session appropriately.
Neither should be implemented by teaching every algorithm a generic address-based
`invoke` interface. The existing [STYLE §9](STYLE.md) already specifies that policy;
the missing step is applying it to production module interfaces.

## A harness that makes composition measurable

Build a **native journey capsule** for each N row. A capsule is a small executable
using the production module interface, the imported assets it really needs, and the
same graphics-device command interface as the guest reference. Start with N1.
It should produce five reviewable artifacts from one run:

1. A native screenshot and the equivalent guest checkpoint, plus an explicit image
   comparison result. Keep temporal/state checks too: a screenshot alone can pass
   while transition or input behavior is wrong.
2. A frame trace of semantic state, input edges and ordered resource/draw events.
   Compare the fields actually observed at the seam; retain source failure sites
   only in the reference adapter.
3. A **closure ledger** listing every consumed resource, state owner and callback,
   with either a native owner or a named unfinished seam. A successful native
   journey must reach no unfinished seam. Derive it from runtime calls and REA
   dossiers; static direct-call inventory alone misses callbacks.
4. A build/link dependency report showing no PE loader, guest interpreter or HLE
   reference code, followed by a run in an assets-only directory. Explicit one-time
   import and reference-oracle commands remain separate workflows.
5. The unchanged function-oracle verdicts for recovered code, and targeted deliberate
   faults for draw suppression, transition timing and wrong texture selection.

These checks answer different questions. Binary exclusion proves independence;
an oracle checks a sampled behavioral seam; a journey checks composition. None
alone establishes all three. Existing `sadx_driver.py`, offscreen screenshots,
state JSON, CPU texture decoder and shared Metal device supply much of the machinery.
Avoid building a second generic task scheduler merely to orchestrate them.

## Immediate execution order

1. Finish and verify the in-flight scene-position rewrite without enlarging it.
2. Close N1 as a real module: logo state/update/display, actual `SEGALOGO_E.PVM`
   loading, native sprite/device setup, frame control and an assets-only caller.
   Recover the complete observed logo closure in one REA session before coding.
   Initializer save/task work must be implemented or explicitly reported, never
   silently replaced by a no-op to obtain a logo screenshot.
3. Keep N1's callable interface small: native owned logo state, input/time values,
   texture resources and a draw/task adapter. Keep the mutable logo owners local;
   avoid a public interface exposing the original global-memory map. Existing
   optional-heavy Ninja sprite interfaces may require a native adapter; that is
   not permission to duplicate their arithmetic.
4. Reuse the capsule for N2/title. Recover menus and stage import in dependency
   order. Allocate later function attempts by the first unfinished seam on the next
   journey, with instruction similarity only as a secondary ordering signal.
5. Track native journey completion, unresolved seams reached, executable-backed
   bytes consumed and measured update/render cost beside the existing oracle counts.
   Give each completed system a native caller before starting another independent
   recovery campaign.

No milestone, fidelity policy or license change is proposed. This is an execution
strategy within ADR-0005–0008, not a claim that the entire port is now estimated.

## Bounded recovery completed during this investigation

The new [logo module](../src/game/logo.hpp) recovers `RunSegaSonicTeamLogos`
behind an owned-state interface and three explicit calls. Its native test runs the
entire 387-update logo sequence. The new differential oracle compares four state
words and ordered callback-entry snapshots: 5,320 development cases and a holdout
of 10,000 new random inputs plus 1,320 repeated boundary inputs match the original
x86 instructions, including integer wrap,
callback changes and failure prefixes. Three deliberate faults were detected:
early transition (8 mismatches), wrong second-logo texture (374), and clearing the
logic-entry flag before its release callback (390). All were restored.

REA dossier `ev_11d4646288706920f09891c29e3a1829f45042ce58a3e2ae9c0c4ebfcbbcfb03`
and its decoded instructions establish the source ordering. The report declares
the exact source range. Initialization, rendering, object execution and texture release remain
explicitly outside this oracle's claim. This is a recovered update module, not N1.

One concrete tooling trap emerged: `DisplayLogoScreen` was initially truncated by
Ghidra's false no-return inference after `njSetTexture`; its first dossier contains
only three instructions. A successful `analyze_function` response therefore does
not establish a complete body. Also, the active pinned loader wraps that display
in background scaling (`profile_patches.json`, `bgscale.cpp:38`), so an unpatched
EXE-only pixel oracle would be insufficient for N1.

The original display body's exact bytes were subsequently decoded independently
with `PEImage` and Capstone. The new display module emits its setup calls, six tile
draws and backdrop call through a small typed seam. Its oracle compares their exact
values, control word and failure prefixes in all twelve masked x87 profiles:
3,936 development cases and 8,544 holdout cases have zero value mismatches. Low-six
exception flags differ in 1,902 and 5,139 cases respectively, so this is explicitly
**partial**. Alpha, row-stride and scale-input faults produced 192, 2,070 and 1,013
value mismatches; all were restored. The bounded profile adapter admits only the
640×480/unit-stretch case where the pinned background wrapper is an identity.

An assets-only [native visual fixture](../tests/unit/mac_logo_demo.mm) now sends
those source-derived tile coordinates and decoded `SEGALOGO_E.PVM` textures through
the shared Metal device renderer. Its screenshot was inspected: the real
PRESENTED BY SEGA screen is visible. Its device defaults and painter order are
explicit fixtures; it differs from the current guest screenshot in 36,751 of
921,600 RGB channels. This establishes an original-derived native visual, not
pixel parity, production draw/queue composition, full native initialization or N1.
The [bounded evidence summary](../reference/logo-native-recovery-20261019.json)
records source hashes, profile evidence, judgments and these limits.

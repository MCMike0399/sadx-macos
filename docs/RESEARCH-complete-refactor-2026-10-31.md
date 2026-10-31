# Complete readability and modularity refactor: source audit

Research date: 2026-10-31. Inspected checkout: `93fa912bfc652056d17106017d20df7496613e60`.
This is a source audit and proposed acceptance contract, not implementation or verification.
The audit worker wrote this note; the root prepared the companion starting prompt.
The investigation used source reads and lexical measurements, without builds, game runs,
gates or decompiler providers. Documentation publication is separate from verification.
Canonical memory and repository instructions were read; source below takes precedence over dated notes.
References use repository-relative paths with source line numbers at this commit.

## Finding and scope

**A complete refactor must cover all existing maintained game behavior, including legacy behavior
already under `guest/`, and modern native composition—not just files in the shipping archive or
the worst lexical scores.** Keep genuine reference infrastructure, CPU arithmetic models and
import tools; recover no additional gameplay merely to call the refactor complete. Existing
unrecovered behavior remains explicitly named, bounded `STUB` seams. Completion means every
existing module has a reviewed disposition and the same implemented behavior remains observable.
This follows STYLE's two tiers and ARCHITECTURE's system destination; neither promises that every
original game function is recovered. [STYLE.md:13–21,80–123](STYLE.md#1-two-tiers-what-may-be-address-shaped),
[ARCHITECTURE.md:8–67](ARCHITECTURE.md#destination-system-modules-in-two-tiers-adr-0005-adr-0006-adr-0007).

The proposed **refactor-before-further-gameplay** ordering explicitly supersedes GOAL's current
route-only readability priority; the root prompt should state that change, preserve fidelity and
milestone definitions, and record any decision requiring an ADR. This research does not resume
the paused implementation goal. [GOAL.md:207–229](GOAL.md#33-tracks),
[AGENTS.md:218–220](../AGENTS.md), [HANDOFF.md, final entry](HANDOFF.md).

## Census: allowances are not live debt, and debt is not a semantic verdict

Read-only Python imported `check_readability.py` with bytecode writing disabled and called its
pure `measure()` function; it did **not** run the checker's `main`, compare regressions, refresh
baselines or execute a gate. Scope was its eight `LAYERS` and six `SOURCE_SUFFIXES`, recursively.
There are 495 live scoped files versus 458 entries in the schema-2 baseline; 636 files have these
suffixes across all `src`. Count tokens/lines as defined by the checker, not unique addresses,
functions or proven violations. [check_readability.py:27–56,106–161](../tools/gate/check_readability.py),
[readability-baseline.json:1](../reference/readability-baseline.json).

| Metric | Tracked allowance total | Live measurement |
|---|---:|---:|
| Hex literals outside named constexpr declarations | 3,244 | 3,244 |
| `*_at_<hex>` occurrences | 1,454 | 1,454 |
| Address dispatch with hex **first** argument | 61 | 61 |
| `goto` / address labels | 0 / 0 | 0 / 0 |
| Lines over 120 columns | 1,183 | 1,183 |
| Multiple statements per line | 1,818 | 1,818 |
| Large decimal integer literals | 233 | 233 |
| Emulator arithmetic indicators | 3,322 | 3,297 |
| Address-comment lines | 1,059 | 1,059 |

The only per-file metric difference from stored allowances is `cpu/fidelity_float.hpp`:
12 live emulator indicators versus 37 allowed. This is **not** evidence of 25 removed arithmetic
operations: that file aliases `X87Context` as `Context`, and the checker does not resolve types.
[fidelity_float.hpp:11](../src/cpu/fidelity_float.hpp),
[check_readability.py:106–119](../tools/gate/check_readability.py).

| Layer | Live files | Hex | Address identifiers | Address calls | Emulator indicators |
|---|---:|---:|---:|---:|---:|
| cpu | 27 | 221 | 9 | 0 | 305 |
| math | 8 | 43 | 0 | 0 | 121 |
| engine | 137 | 653 | 469 | 13 | 679 |
| input | 10 | 54 | 36 | 0 | 32 |
| player | 116 | 1,828 | 542 | 0 | 2,005 |
| game | 141 | 445 | 398 | 48 | 143 |
| runtime | 56 | 0 | 0 | 0 | 12 |

Additional comment/string-masked scans over the same 495 files found **1,019 `std::optional<`
occurrences in 175 files**, **97 `struct`/`class` declarations whose names end in `Calls` or
`Services` in 56 files**, and **715 `virtual` tokens in 43 files** (including destructors).
These are triage counts: optionals may mean genuine absence; interfaces may represent real
platform boundaries. They are not counts of bad declarations. `native_session.hpp` has exactly
**23 `void attach_*` declarations**. The census is reproducible with the named lexical patterns
and the checker masking function; no AST/type resolution was used.

## Checker coverage: what a green result cannot establish

1. **GOAL's checker criticism is partly stale.** It says decimal constants, emulator calls and
   address comments are uncounted. All three are now metrics; `player_mesh_matrix.cpp` has
   **508 emulator indicators**, rather than being nearly clean. The older approximate totals
   and service counts in GOAL are historical, not this census.
   [GOAL.md:270–325](GOAL.md), [check_readability.py:35,106–139](../tools/gate/check_readability.py),
   [player_mesh_matrix.cpp:7–39](../src/player/player_mesh_matrix.cpp).
2. **Dispatch coverage is narrow.** `sonic_player.cpp` scores zero address calls because its
   helper receives `(rt, f, fp, callee, site, ...)`. A separate same-line candidate pattern
   `\b(?:call|ret|invoke)(?:_\w+)?\s*\([^;\n]*0[xX][0-9a-fA-F]+` finds **112 occurrences
   in 11 scoped files**, including **40 in Sonic**. This is an overlapping heuristic, not
   112 unique dispatch sites or a replacement gate. Actual Sonic dispatch is plainly visible.
   [sonic_player.cpp:30–49,1294,1425](../src/player/sonic_player.cpp),
   [check_readability.py:134–136](../tools/gate/check_readability.py).
3. **Named constexpr exempts entire declarations, including tables containing PCs.** The raw
   masked-code hex-token counts are **6,162** in `sonic_animation_operations.hpp` and **882**
   in `sonic_animation_table_data.hpp`; both report zero hex literals after constexpr masking.
   The former contains 1,027 instruction-shaped operations, and the initializer interprets
   `op.address`, numeric field offsets and partial availability. A typed recipe is not the
   same thing as a table of source instructions. Separate provenance/diagnostic metadata from
   game initialization while preserving interleaved reads and stores.
   [check_readability.py:59–84,125](../tools/gate/check_readability.py),
   [sonic_animation_operations.hpp:5–14](../src/player/sonic_animation_operations.hpp),
   [sonic_animation_table_data.hpp:5–12](../src/player/sonic_animation_table_data.hpp),
   [sonic_animation_table.cpp:6–29](../src/player/sonic_animation_table.cpp).
4. **Provenance validates annotations it sees; it does not require every recovered function or
   owner to have one, or prove a `FUNCTION` body is complete.** Its legacy-name regex only
   recognizes `_at_<hex>`, leaving names such as `run_game_reset_40bf40` outside that naming
   ratchet. `OptionsMenu_Display` is annotated `FUNCTION`, but its body only reports
   `NeedsDrawing`/`Complete`. Correct scope declarations are a human-review obligation.
   [check_provenance.py:22,134–221](../tools/gate/check_provenance.py),
   [game_reset.cpp:5](../src/game/game_reset.cpp),
   [title_menu.hpp:545–546](../src/game/title_menu.hpp),
   [title_menu.cpp:50–52](../src/game/title_menu.cpp).
5. **Ownership checks are deliberately incomplete models.** Legacy arrays/vectors count one
   byte; scalar widths and `SourceBytes` extents are modeled, not arbitrary layouts/aliases.
   Annotated `GLOBAL` ranges are checked against legacy claims, and retained baseline extents
   cannot disappear silently. But unannotated semantic fields are invisible: Session still
   labels many source-backed owners with ordinary comments. Do not infer that these are
   duplicates; audit each against canonical owners before migrating it.
   [check_global_owners.py:1–49](../tools/gate/check_global_owners.py),
   [check_provenance.py:86–130,197–239](../tools/gate/check_provenance.py),
   [native_session.hpp:197–213,276–284,301–305](../src/runtime/native_session.hpp).
6. **Layers check includes, not calls or links.** Sonic calls `get_player_acceleration_ground`,
   defined under `guest`. Native closure now force-loads the selected archive and checks
   project symbols/Unicorn imports, but Sonic is absent from `native-sources.txt`, so that
   check cannot certify all player code. Its source validator also requires exactly three
   path components; system subdirectories will need an evidenced tooling change first.
   [check_layers.py:9–29](../tools/gate/check_layers.py),
   [sonic_player.cpp:124](../src/player/sonic_player.cpp),
   [player_acceleration_ground.cpp:112](../src/guest/player_acceleration_ground.cpp),
   [Makefile:39–45,112–120](../Makefile), [native-sources.txt](../native-sources.txt),
   [check_native_closure.py:31–49](../tools/gate/check_native_closure.py).

The metric excludes `guest`, `hle`, `loader`, `platform` and `apps`. Those layers still require
readability review appropriate to their purpose. No checker above proves semantic naming,
small cohesive APIs, missing-fact containment, complete recovery, absence of duplicate game
algorithms or source independence by itself. ADR-0008 explicitly rejects counting relocation
as a rewrite. [ADR-0008:67–70](decisions/ADR-0008-visible-journeys-and-proportionate-verification.md).

## Representative work, and patterns worth preserving

| Concrete source | Diagnosis and refactor boundary |
|---|---|
| `player/sonic_player.cpp:30–49,124,449–457`; `sonic_player.hpp:59–65,80–82` | Gameplay dispatch, call ABI, fixture availability and address-named state are mixed. It has 547 measured hex literals and 171 address-identifier occurrences. Give Sonic semantic action/state interfaces; adapters translate source calls and facts. Preserve ordering and common-owner borrowing. |
| `game/game_reset.cpp:5–30` | The game reset sequence is a compact list of source call sites with partial-failure behavior. Rewrite reset policy in the game system; move only diagnostic/ABI translation. Do not merely put this sequence in `guest`. |
| `player/player_mesh_matrix.cpp:7–105`; `engine/ninja_matrix.cpp` | Generated register temporaries/spills and emulator calls obscure math. The matrix files have 508 and 347 emulator indicators respectively. Choose arithmetic level per expression under unchanged judges, not a wholesale float substitution. |
| `engine/ninja_queue_consumer.cpp:66–95` | Rendering policy, record offsets, fixture facts, callbacks and failure PCs coexist. Native typed records must retain the read of NEXT **after** callbacks and the explicit unlifted-draw stop. Queue flags/depth/blend decisions remain engine behavior. |
| `player/player_scene_decode.cpp:7–43,45–69` | Actual MZ/PE32 parsing and VA translation live in a player module. Move executable decoding to importer/reference facilities, publish typed imported collision data to game code, preserve strict invalid/missing-resource outcomes. Relocating this parser is legitimate; relocating collision response is not. |
| `guest/game_logic.cpp:40–89`; `guest/ninja_queue_lifecycle.cpp:8–27` | Already-excluded files still contain input accumulation/tick policy and queue reset/render behavior. Include their game algorithms in the campaign inventory. A reference directory is not proof that a file contains only marshalling. |
| `runtime/native_session.hpp:45–177,185–351`; `game/title_menu.hpp:71–87` | Session combines save/audio/menu/resource/player ownership and attachment sequencing; title has 138 `virtual` tokens. Consolidate by existing systems and lifetimes, exposing borrowed state and narrow named operations. Do not replace this with a generic service registry or move every field into another monolith. |

Existing migrations are **patterns to audit and reuse**, not declarations that those systems
need no further work:

- `game/game_mode.cpp:15–38,65–96` keeps decisions in ordinary named game functions;
  `guest/game_mode_reference.cpp` supplies reference binding. Some numeric character/stage
  meanings still need naming (`game_mode.cpp:50–55`).
- `engine/ninja_fog.cpp:8–10,56–72,82–118` retains exact expressions and explains why;
  `guest/ninja_fog_reference.cpp:12–42` maps logical calculations to source diagnostics.
  Keep one fog algorithm rather than reimplementing it in the adapter.
- `game/game_library_init.cpp:29–77` expresses initialization, retry and cleanup directly;
  lines 17–26 explain why unsigned conversion cannot be casually simplified. Callback reads
  and partial stores remain live and ordered.
- `engine/game_frame_control.cpp:29–41,45–81` has plain reset/limiter decisions; optional
  clock results represent OS failure, which is legitimate game/platform absence.
- `player/actor_current.hpp:8–35` provides canonical annotated owners and observes every
  publication, including null/same-value stores. `engine/polybuff_initialization.hpp:11–29,47–69`
  owns all nine records and shares one algorithm through compile-time native/reference seams.
- `player/player_scene_position.cpp:53–99` retains named collision behavior in the player
  layer, but its wide private Context still merits API review; it has 97 `virtual` tokens in
  `player_scene_position_internal.hpp`. `ground_acceleration_internal.hpp:8–10` still names
  arithmetic identities `zero`/`half`. A prior migration is not an exemption from STYLE.

## Legitimate addresses and exact arithmetic

Preserve provenance annotations; x86 guest memory maps, ABI/register/stack marshalling, override
registration and diagnostic instruction PCs; PE loader/importer VA/RVA/relocation logic; oracle
fixture addresses, raw source encodings and disassembly evidence. Preserve genuine format masks,
colors, IEEE representations and CPU control words, named where that clarifies meaning. These
are not all executable addresses. The loader's MZ/PE constants and VA accessors are appropriate
in `loader/executable_image.cpp:8–24,43–46`; they are inappropriate dependencies for gameplay.

CPU implementations may need instruction-level x87/SSE operations. Game expressions should use
plain arithmetic where **all** STYLE proofs hold; otherwise named fidelity helpers or exact
value expressions with evidenced precision, operand order, rounding and explicit stores.
Do not move entire game algorithms into `cpu` to evade a metric. `cpu/source_bytes.hpp:7–28`
is explicitly a missing-source-fact container, not an appropriate native game state model.
[STYLE.md:125–189](STYLE.md#7-arithmetic-exact-and-readable),
[cpu/fidelity_float.hpp:13–36](../src/cpu/fidelity_float.hpp).

## Proposed order and completion criteria for the root prompt

1. **Freeze scope and evidence before implementation.** Inventory every maintained module,
   including excluded reference/platform/app sources, every recovered entry and shared owner.
   Record destination system, canonical ownership, callers, native/reference bindings, oracle
   and mutant coverage, missing callees, and present verdict/counts. Separate genuine reference
   machinery, game behavior, imported content and generated diagnostic tables. No unreviewed
   file may disappear from the denominator through deletion, renaming or exclusion.
2. **Make verification fit that scope in separate judge work.** Extend closure/build discovery
   for nested system paths and cover every migrated module, including currently unlinked ones.
   Keep lexical measurements plus a reviewed inventory of semantic violations and justified
   exceptions; address constexpr tables, non-first-argument dispatch and unannotated owners.
   Never rewrite baselines or harnesses merely to admit the refactor. Existing judge changes
   require the evidenced separate-commit procedure. [JUDGE.md:11–42,47–66](JUDGE.md).
3. **Fix ownership and composition before expanding module APIs.** Split Session into existing
   save, audio, menu/resource and player/task systems; establish lifetimes and borrowed handles.
   Migrate whole owner mappings and checks together, retaining legacy compatibility only at
   adapters. Demonstrate same-owner identity, aliasing, callback mutation and ordered partial
   failure; copying optional state into fresh plain records is not generally equivalent.
4. **Refactor by dependency, in coherent systems.** Start with common state/types, import seams
   and reusable arithmetic; then engine matrix/view/queue/sprite/texture/task services and
   input; then player geometry/contact/movement/animation and Sonic actions; then remaining
   game flow/reset/startup/sound/menu behavior and legacy algorithms in `guest`. Integrate
   each completed system into both bindings before moving on. Sequence around actual owner
   dependencies, not filenames or ease of reducing counts. Keep stable test/oracle slugs.
5. **Close the campaign explicitly.** Every inventory row must be a reviewed semantic module,
   legitimate reference/tool facility, or a specifically evidenced missing-behavior seam.
   There must be no address-shaped gameplay, fixture-availability state, PE parsing, register
   marshalling, guest-defined game dependency or duplicate reference implementation of migrated
   game behavior. Every residual exact-arithmetic use and large interface needs a reason.
   Private internals may remain; public APIs should express a system rather than each source
   instruction. No blanket ban on optionals, virtual methods, hex or CPU emulation substitutes
   for this review. Directory organization and public surfaces must match the system map.
6. **Prove preservation, not new gameplay.** For each rewrite retain oracle verdicts **and
   compared counts**, outputs/boundaries/store order and existing mutation detection; use
   frozen holdouts where available and independent STYLE review at coherent system closure.
   Extend coverage separately where the existing judge is inadequate. At final integration,
   run the complete gate, all affected oracles/mutants, native force-load and clean source-only
   builds, and existing native/reference journeys with inspected visual evidence. Verify
   native production needs no EXE/DLL/guest archive and reference adapters exercise the same
   algorithms. Retain loud missing-callee boundaries; do not skip them to pass a journey.
   Preserve existing enhancement-off behavior without inventing new hooks/frameworks.
   [GOAL.md:493–553](GOAL.md#7-the-loop-for-one-step), [STYLE.md:367–387](STYLE.md#12-review-checklist-author-and-independent-reviewer).

## Evidence limits

This audit measured the entire lexical scope but inspected representative implementation
bodies, not every recovered function or source dossier. Candidate counts overlap and do not
establish bugs. No C++ parser, original executable, provider, build or test was run; no current
fidelity verdict is inferred. The latest recorded green gate is at `9fa9966`, older than this
checkout; it records 274 native programs and 400 Python tests, with one existing filesystem
skip. STATUS separately lists 123 passing, 79 partial, 10 failing and 15 missing oracle reports.
Those are recorded inventory facts, not refreshed results. The last HANDOFF retains an
interrupted subsequent gate and pending weld/cursor source-only/full-gate work.
[STATUS.md:3–17,42–46](STATUS.md), [HANDOFF.md, final entry](HANDOFF.md).

Do not make “all oracles pass” a silent acceptance change: unchanged flag partials/failures must
remain explicit, while any worsening blocks the affected refactor. Nor does `make gate` alone
rerun every function oracle or mutant: inspect its actual dependencies and run the additional
required judges. Historical TESTING counts are dated; its coverage warnings remain relevant.
[Makefile:350–362](../Makefile), [TESTING.md:10–65](TESTING.md#1-the-layers).

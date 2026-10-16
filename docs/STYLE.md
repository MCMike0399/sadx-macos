# Readable source standard

How recovered SADX code is written. The decision is
[ADR-0007](decisions/ADR-0007-readable-game-code-and-extensibility.md), which builds on
ADR-0005 (native, executable-independent) and ADR-0006 (names in code, addresses in
annotations). Every recovered function and every reviewer works from this page.

**The test for any game-tier change:** a developer who has never seen `sonic.exe` can find
the code, understand what it does, change a gameplay value or add a feature, and run the tests
that say whether they broke fidelity, without reading assembly, guest addresses, or register
names.

## 1. Two tiers: what may be address-shaped

| Tier | Where | May contain |
|---|---|---|
| **Game tier** (ships) | `src/cpu` helpers, `src/math`, `src/engine`, `src/input`, `src/player`, `src/game`, `src/runtime` (destination: system modules, `docs/ARCHITECTURE.md`) | semantic names, enums, structured control flow, plain values, literals, direct calls, service seams, provenance annotations |
| **Reference tier** (development only) | `src/guest`, `src/hle`, reference adapters, override table, oracle probes, `tools/oracles` | guest addresses, register/stack marshalling, callee-address dispatch, `source_boundary`/`required_fact`, `std::optional` for facts a fixture may not supply, `static_assert`ed original layouts |

If game-tier code needs something from the reference tier, the design is wrong. Add a seam
(§5) and let the adapter supply it.

## 2. Names

* Look up every address before naming it (`docs/METHOD.md` §1 "symbol lookup"). Prefer the
  `*New.h` (original-symbol) name as the annotation symbol, and choose a semantic identifier
  in project style. Names are hypotheses until the behaviour you recovered agrees. Record
  decisions and conflicts in `reference/symbols/overrides.tsv` with the evidence id.
* Style: `snake_case` functions, variables and fields; `CamelCase` types; `CamelCase`
  enumerators in `enum class`; `kCamelCase` for named `constexpr` constants; namespaces
  `sadx::<system>`.
* Recovered record types follow the original layout's meaning: `taskwk` → `TaskWork`,
  `playerwk` → `PlayerWork`, `motionwk` → `MotionWork`. Field names come from the struct
  headers. Rename them when the original name is opaque (`flag` → `status_flags`), and keep
  the original in a trailing comment.
* Unknown meaning: describe the role, not the address (`logo_result`, `title_return_code`),
  and say so in a comment (`// meaning of values other than 1 and 3 not established`). Never
  `word_at_3b2c45c`, `byte_3abdf74`, `sub_42cea0`, `FUN_`, `DAT_`, `local_`, `param_`, `uVar3`.
* Enums come from `SADXEnumsNew.h`/`SADXEnums.h` when they exist (`MD_GameMode` →
  `enum class GameMode`), with the enum's original name in a comment on the declaration.

## 3. Provenance annotations (ADR-0006)

On the line above the declaration: `// FUNCTION: SONIC 0x40c090 GameModeHandler`,
`// GLOBAL: SONIC 0x3abdc7c ulGlobalMode size=4`, `// DATA:`, `// PATCH: SPEEDRUN +0x10f0 ->
0x7899e0`, `// STUB:`, `// LIBRARY:`. The annotation is the only place in game-tier code where
an address appears. `tools/gate/check_provenance.py` validates the annotations. The exact
grammar is in `docs/METHOD.md` §3.

## 4. Control flow and functions

* One recovered source function becomes one named function. A shared tail that the compiler
  merged (the old `goto L3bc` labels) becomes a named helper or a `return`. Inline a small
  helper only when the original clearly inlined it.
* Structured code only: `if`/`else`, loops, `switch` over an `enum class`. No `goto`, and no
  labels named after addresses. Keep `[[fallthrough]]` where the original falls through.
* **Order is behaviour.** Keep the order of observable side effects: stores to shared state,
  calls, and RNG draws. Where the order looks odd but matters, say so: `// stores mode before
  calling the loader: the loader reads it`.
* Keep the original's quirks and bugs when they are observable, and label them:
  `// Original quirk: unsigned comparison, so negative modes fall to the default branch.`
  Speedrun techniques depend on quirks. Never "fix" one in the game tier; an enhancement may
  (§9).
* Functions longer than about 80 lines, or nested more than 3 levels, get split into named
  steps unless the oracle shows that splitting changes behaviour (it should not).
* Do not over-abstract. A block used in fewer than three places stays inline unless it is a
  named step of a long function. Add no class hierarchies, registries or "managers" that the
  original did not have. (HarbourMasters' policy: "AI models have a tendency to over abstract
  and over explain".) Changing the original's architecture, for example replacing global
  state with a hash map, is a design change and needs an ADR.
* A helper that does not exist in the original is tagged `// NOTSA: <why>` on its
  declaration (gta-reversed's convention), so readers can tell recovered structure from
  structure we added.
* **Invent nothing.** Every field, callee, guard and branch must trace to evidence: the
  dossier, the pseudocode or the oracle. Introduced structure is the strongest predictor of
  divergence that the tests miss (arXiv 2609.05370, RESEARCH third pass). Readable but wrong is
  the default failure of agent decompilation, so readability is always judged by the same
  oracle.

## 5. Seams for behaviour that is not native yet

The game tier never calls by address. A system that depends on unrecovered behaviour declares
each missing callee as an ordinary, named function in a seams header. The `STUB` annotation
sits directly on the declaration:

```cpp
// game/flow/game_mode_seams.hpp (game tier)
namespace sadx::flow {
// Not recovered yet. The reference build defines it by calling the guest.
// STUB: SONIC 0x42cc70 RunSegaSonicTeamLogos
int run_logos();
// STUB: SONIC 0x42c7b0 InitializeSegaLogo
void initialize_sega_logo();
// … one function per callee, named from the symbol database
}
```

These are link-time seams. The reference build links an adapter that defines them by calling
the guest at the original address, so the address lives only in the adapter. The native build
links the recovered implementation, or a definition that fails loudly
(`unimplemented("run_logos")`) so nothing is skipped. When the callee is recovered, its real
definition replaces the stub, and its annotation becomes `FUNCTION`.

`check_provenance.py` accepts `// STUB: MODULE 0xADDR name` with optional reason text after
the symbol, for example `// STUB: SONIC 0x42cc70 RunSegaSonicTeamLogos awaiting recovery`.
Ordinary member function declarations inside structs are accepted by the same declaration
grammar. Callable fields (`std::function`, function pointers) are values and cannot carry
function-entry annotations. Prefer the plain link-time seams above; member syntax alone is
not a reason to introduce a service class. STUBs retain symbol validation and the unique
source-entry rule: annotate the seam declaration once, not its adapter definition too.

## 6. State

* Each system owns one state struct (`GameFlowState`, `FrameRateModeState`) with plain values
  and recovered initial values: `.data` initialisers as annotated literals, `.bss` as zero
  (`= {}`). The session owns all system state. No C++ globals, so tests can build isolated
  worlds.
* One owner per source address (`GLOBAL` annotation). Others borrow by reference.
* Native types: native pointers and handles, `std::array`, `std::span`-like views. Original
  32-bit layouts appear only in reference adapters, with `static_assert(offsetof(...))`.
* `std::optional` in the game tier means "absent in the game", for example "no target
  object". It never means "the oracle fixture did not supply this word". That case belongs in
  the adapter.

## 7. Arithmetic (exact, and readable)

Fidelity does not change. Use the most readable form that the module's oracle accepts, in this
order:

1. **Plain `float`/`int` expressions.** The build sets `-ffp-contract=off -fno-fast-math`.
   With x87 at 24-bit precision and round to nearest, `+ - * / sqrt` on floats round like
   IEEE single except in the cases of the next item. The reference census now records
   `0x023f` (53-bit precision) at every reached intro/controller entry; ground acceleration
   was unreached. The earlier Windows observation in HANDOFF does not establish a universal
   24-bit premise. All four of these preconditions must hold, because fixtures
   may never reach the divergent paths:
   * the assembly (`ghidra_asm.py`) of the expression shows only single-precision (`m32`)
     memory operands, with no `fld qword`, `fistp` or transcendental in it;
   * the census shows only `0x007f` at the function's call sites;
   * the oracle verdict and counts are unchanged;
   * the mutants are still caught.
2. **Named fidelity helpers** (`src/cpu`, namespace `sadx::fidelity`) for the known
   divergences: operations with a `double` constant (`fld qword`: the exact product is
   rounded once, not twice), float→int conversion (`_ftol` truncation and `fistp` under the
   current rounding control), x87 transcendentals (Bochs models), unordered comparisons used
   through `fnstsw`/parity, and results that leave the single exponent range while still in a
   register.
3. **An exact value type with operators** (`src/cpu/fidelity_float.hpp`,
   `sadx::fidelity::ExactFloat`) over `X87Context`, for modules where (1) fails.
   `ExactFloat(fp, speed.x) - max_x_speed` retains the selected precision and exponent range;
   `value.rounded()` explicitly stores binary32. A `float` right operand represents memory,
   while another `ExactFloat` represents an already loaded value; this distinction preserves
   signaling-NaN selection. Memory-first subtraction/division use reverse operations.
   Doubles are rejected: use the appropriate m64 fidelity helper. All values in an expression
   must share a live context. Sequence independent operand calculations in named temporaries
   when exception order matters; C++17 does not order the two sides of a binary expression.
   Ground acceleration and its shared friction tail use this arithmetic tier under their
   unchanged differential oracles; existing flag-only discrepancies remain partial.
4. **Explicit `X87Context` sequences**, only inside `src/cpu` helpers, or where an oracle shows
   that instruction-level ordering matters. Add a comment saying which oracle case shows it.

Integer code mirrors the source's widths and signedness explicitly (`std::int16_t` for a
`MOVSX` short, unsigned comparisons where the source used `JA`), with a comment where it
matters.

## 8. Constants and tables

* Compiler constant-pool values are source literals: write `0.1f`, `1.16f`, `5.0f`. Use the
  shortest decimal that round-trips exactly, or a hex-float when no short decimal does.
  Give a constant a `constexpr` name when the name explains it (`kSlopeSlideThreshold`). A
  unit test or the oracle confirms the value. Fetching a constant from the executable at run
  time (`c(0x7e0178)`, `image.u32(...)`) is reference-tier only.
* Content tables (physics parameters, models, animations, texlists, object, sound and SET
  lists) come from the imported pack (ADR-0005, GOAL stage E), typed and named.
* Code-structure tables (state machines, action dispatch, jump tables) become named enums and
  arrays of named functions.

## 9. Enhancements and future features

The faithful game, the pinned profile, is the default and the only thing the fidelity tests
measure.

* An enhancement (practice tools, overlays, widescreen and UI scaling, input display, free
  camera, debug views, new content) has an id, a typed setting that defaults to off, and a
  class:
  * **replay-neutral:** it must not change game state. The replay and trace digests prove
    this with the enhancement on and off.
  * **gameplay-affecting:** it marks the session as non-faithful and is excluded from
    fidelity runs.
* It attaches at named hook points that a system exposes. There are two kinds:
  * **Events** that observe and never change the result: `on_frame_end`,
    `on_player_physics_end(const PlayerWork&)`, `on_stage_loaded(StageId)`.
  * **Vanilla-behaviour decisions**, the Ship of Harkinian pattern: a decision point that
    enhancements may need to change is written once as
    `if (hooks.should(VanillaBehavior::RingLossOnHit, /*vanilla=*/true, player)) { … }`. The
    `VanillaBehavior` enum documents each point's vanilla result and arguments, and points at
    the `FUNCTION` it lives in.

  With no handler registered, `should` returns the vanilla value. A gate test proves that the
  replay digests are identical with every enhancement registered and off. It never adds an
  ad hoc `if` inside a faithful algorithm.
* Original bugs that someone wants fixed are enhancements too: a named entry
  `{id, description, evidence, default=off}` in the fix registry (gta-reversed's
  `ReversibleBugFix`, but off by default here, because the speedrun profile depends on the
  bugs).
* Settings are typed fields in one settings struct, loaded from the user's config. Do not
  hard-code game-specific options in algorithms; TRX spent years undoing that.
* Enhancement code lives in its own module (`src/enhancements/<id>`). It may depend on game
  systems, and game systems never depend on it.
* Content mods replace imported resources by resource id. A native mod API needs its own ADR.

## 10. Formatting and files

* `.clang-format` (LLVM base, 4-space indent, 100 columns) applies to every new
  or migrated file. Write one statement per line, and do not chain lambdas on one line. Do not
  mass-reformat unmigrated files, because mutant specs and blame depend on them.
* `make check` runs `tools/gate/check_readability.py`. It measures code hex literals,
  address identifiers, `call`/`ret`/`invoke` dispatch helpers with a hex first argument,
  `goto`, address labels, lines over 120 columns and lines with multiple semicolons outside
  parentheses. Comments and strings are masked for code counts; line length includes them.
  These are lexical debt indicators, not a C++ parser or a fidelity verdict. Scope is
  `src/{cpu,math,engine,input,player,game,runtime,enhancements}` recursively; reference,
  loader, platform and app code are outside this metric.
  Per-file allowances in `reference/readability-baseline.json` may only decrease against
  HEAD; unlisted files have zero allowance. After reducing debt, run
  `python3 tools/gate/check_readability.py --write-baseline` to lower the tracked allowances.
  The checker refuses to refresh over an increase. Source archives enforce the supplied
  baseline; comparing baseline edits against history requires a Git checkout.
* One public header per system. Keep internal helpers in the `.cpp` or in a `detail`
  namespace. Slug rules are in `docs/ARCHITECTURE.md`.
* Comments explain why, the original's quirks, and fidelity-relevant ordering. They do not
  restate the code or carry addresses (addresses belong in annotations).

## 11. Before and after (from this repository)

The game-mode, ground-acceleration and frame-control examples are implemented (GOAL §6).

**Game mode handler** (`src/game/game_mode.cpp`, `update_logo`; source `GameModeHandler`).
The adapter in `src/guest/game_mode_reference.cpp` holds call-site addresses, guest arguments,
and lazy missing-fact diagnostics. The native backend reads plain state and stops explicitly
at uncomposed callees. `FlowPhase` names logical steps; it carries no source address or game
branch in the adapter.

```cpp
// before: transliterated assembly
case 0x01:
    r=ret(0x40c124,0x42cc70);
    if(r==1){call(0x40c130,0x411e30,1,1);goto L3bc;}
    if(r!=3)goto tail;
    call(0x40c14a,0x411e30,1,1);st.word_at_3b2c45c=0;mode=8;goto tail;
```

```cpp
// after: actual game-tier helper
void update_logo(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Logo);
    const auto result = run_logos(flow);
    if (result == 1) {
        enter_phase(flow, FlowPhase::LogoTitle);
        set_frame_rate_mode(flow, 1, 1);
        enter_title(flow);
    } else if (result == 3) {
        enter_phase(flow, FlowPhase::LogoMovie);
        set_frame_rate_mode(flow, 1, 1);
        write_movie_number(flow, 0);
        write_mode(flow, GameMode::Movie);
    }
}
```

State accessors publish every source store, including same-value stores, and validate facts
only on reads. This preserves owner callbacks that change or invalidate state during a
frame, without putting fixture optionals in the game tier. The unchanged oracle covers
24,000 cases/252 paths; three preserved mutations and a separate 2,048-case holdout judge
the rewrite. Native-only linkage proves the dispatcher builds without a PE or guest runtime;
it does not claim the menu, logo, audio, or other missing callee implementations are complete.

**Ground acceleration** (`src/player/ground_acceleration.cpp`, `adjust_slope`; source
`PGetAcceleration`). Native `GroundConstants` holds literal defaults, with all 24 binary32
encodings checked against PE bytes by the reference unit. Its fields name the shared source
constant roles. The census does not justify plain floats: the core uses `ExactFloat` and
explicit binary32 stores, including a retained lateral value until drag consumes it.

```cpp
// before: emulator calls and constants fetched by address
if(less(cf(player.dotp,c(0x7dd568))) && greater(cf(fabs32(cross.y),c(0x7dd584))) &&
   greater(cf(player.motion.speed.x,c(0x7e0178)))) {
    gravity.x=0;gravity.y=fp.to_float(load(p.weight).negated());retained_z=load(zero);
} else if(less(cf(player.dotp,c(0x7e00e4))) && greater(cf(player.motion.speed.x,c(0x7e0178)))) {
    update(gravity.y,fp.reverse_subtract_float_memory(product(p.weight,c(0x7dccfc)),gravity.y));
}
```

```cpp
// after: actual game-tier slope cases; exact() constructs the retained value
if (less(compare(state.alignment, tuning.cross_slope_alignment)) &&
    greater(compare(fidelity::absolute_float(cross.y), tuning.cross_slope_threshold)) &&
    greater(compare(state.player.speed.x, tuning.slide_speed))) {
    gravity.x = 0;
    gravity.y = (-exact(p.weight)).rounded();
    retained_z = exact(zero);
} else if (less(compare(state.alignment, tuning.steep_negative_alignment)) &&
           greater(compare(state.player.speed.x, tuning.slide_speed))) {
    gravity.y =
        (gravity.y - (exact(p.weight) * tuning.steep_slope_weight_multiplier)).rounded();
}
```

The adapter validates gravity, contact and input scratch only when reached; no fixture
optionals enter the core. Borrowed references preserve callback changes. The common friction
implementation computes final Y, stores X, spills Y, then stores Y and Z; unmasked-exception
tests distinguish arithmetic failure from store failure in the native immediate-failure
model; deferred hardware x87 fault delivery remains unverified. Native-only testing completes a
controlled inactive step; missing native mutation/yaw composition remains explicit.

**Frame control** (`src/engine/game_frame_control`, source `dsInitInt`). The core owns
plain `ModeState`, `RendererState` and borrowed input values. Zero-filled source fields start
at zero; the two file-backed settings start at 1 and 3. In particular, the initial frame
multiplier is zero, rather than the illustrative value 1 in the earlier sketch.

```cpp
// actual game-tier reset decision; fixture facts are checked in the adapter
void update_soft_reset_flags(InputState &state) {
    const auto flags = soft_reset_flags(state);
    if (flags != 0) {
        if (save_phase(state) == 0)
            store_soft_reset_flags(state, flags | 1U);
        return;
    }
    constexpr std::uint8_t kResetInputMask = 68;
    if (reset_key_edge(state) != 0 && (reset_input_mask(state) & kResetInputMask) != 0) {
        store_soft_reset_flags(state, flags | 4U);
        return;
    }
    store_soft_reset_flags(state, flags & 251U);
}
```

`guest/frame_control_reference` retains optional counter halves, mapped global identities,
and source failure boundaries, with no duplicate algorithm. Private typed state seams let
the native backend read plain fields and the reference backend check facts at each consumed
read. Frequency validation precedes the multiplier store; all three mode stores precede the
selected renderer branch; callback registration follows successful interval setup. The limiter
reads the previous counter after each clock callback and commits the sampled time without
accumulating a deadline. Its optional query results describe actual OS failure.

The isolated checker builds two distinct closures: a plain native host journey with no
reference header or fixture, and a portable fact adapter that preserves all missing-fact
assertions and the unchanged 2,672-row frozen numeric corpus. Neither closure needs the PE
loader, guest runtime or original binary. This proves the named frame-control slice, not
native boot/title, complete game independence or physical timing fidelity.

## 12. Review checklist (author and independent reviewer)

1. No address outside annotations in game-tier files. No `*_at_<hex>`, `goto`, address labels,
   or callee-address calls. `check_readability.py` passes once it exists.
2. Every name traced to the symbol database or justified in `overrides.tsv`.
3. Control flow structured; side-effect order unchanged; quirks commented.
4. Arithmetic at the most readable level the oracle accepts, and the level chosen is stated
   in the handoff.
5. Constants are literals or named `constexpr`; content comes from the pack, not the EXE.
6. Unrecovered callees are `STUB` seams, not silent no-ops.
7. Oracle, scripts, fixtures, tolerances and mutants unchanged. Mutants are still caught
   after the refactor.
8. Could a feature be added here through a hook, without editing this algorithm?
9. Nothing invented: every field, callee and guard has an evidence id. Helpers we added are
   tagged `NOTSA`. No abstraction the original lacks.
10. Platform-replacement sites (a Win32/D3D8/DirectSound behaviour re-expressed on macOS) are
    tagged `// [platform:<area>]` (isle-portable's `[library:…]` convention), so they can be
    found and reviewed together.

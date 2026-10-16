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
3. **An exact value type with operators** (to be built over `X87Context`), so that
   `(speed.x - max_x_speed) * air_resist` reads as math in modules where (1) fails.
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

The "after" versions are sketches showing the shape. Each becomes real only when its oracle
passes unchanged (GOAL §6).

**Game mode handler** (legacy reference interpreter now in
`src/guest/game_mode_reference.cpp`, 40c090 `GameModeHandler`; the game-tier rewrite is pending):

```cpp
// before: transliterated assembly
case 0x01:
    r=ret(0x40c124,0x42cc70);
    if(r==1){call(0x40c130,0x411e30,1,1);goto L3bc;}
    if(r!=3)goto tail;
    call(0x40c14a,0x411e30,1,1);st.word_at_3b2c45c=0;mode=8;goto tail;
```

```cpp
// after: game tier
case GameMode::Logo: {                                     // MD_LOGO
    const int logo_result = run_logos();                   // RunSegaSonicTeamLogos (seam)
    if (logo_result == 1) {
        frame_control.set_frame_rate_mode(1, 1);           // dsInitInt
        return enter_title(state);                         // was the shared tail L3bc
    }
    if (logo_result == 3) {
        frame_control.set_frame_rate_mode(1, 1);
        movie.number = 0;                                  // movie_num
        mode = GameMode::Movie;
    }
    break;
}
```

**Ground acceleration** (`src/player/player_acceleration_ground.cpp`, 44c270
`PGetAcceleration`). The constants below are the actual values at 7dd568, 7dd584, 7e0178,
7e00e4 and 7dccfc:

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
// after: plain float, if the ground_acceleration oracle agrees under CW 0x007f
if (pw.dotp < 0.1f && std::fabs(cross.y) > 0.6f && pw.speed.x > 1.16f) {
    gravity = {0.0f, -params.weight, 0.0f};    // steep slope at speed: weight only
} else if (pw.dotp < -0.4f && pw.speed.x > 1.16f) {
    gravity.y -= params.weight * 5.0f;
}
```

**Frame control** (`src/engine/game_frame_control.hpp`, the ADR-0006 pilot). The names and
annotations are done, but the shape is still reference-tier: every field is `std::optional`,
`CounterWords` splits a 64-bit counter into optional halves to mirror fixtures, and results
carry `source_boundary`. The game-tier version holds `std::int64_t counter_frequency` and
`int frame_multiplier = 1`. The adapter keeps the optional halves and reports which word a
fixture lacked.

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

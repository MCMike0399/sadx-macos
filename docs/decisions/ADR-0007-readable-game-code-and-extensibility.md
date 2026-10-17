# ADR-0007: Readable game code, a separate reference tier, and room for enhancements

Date: 2026-10-15. Status: accepted direction (user request, same day as ADR-0005/0006).
Refines ADR-0006 (names) and the method in `docs/METHOD.md` §3. It changes neither the
fidelity policy, the licence, nor what is committed. Coding rules are in
[`docs/STYLE.md`](../STYLE.md).

## Context

The user reviewed the source and found much of it "straight up unreadable": address calls,
address-named state, and code that reads like a disassembly. ADR-0006 moved addresses from
identifiers to annotations, but names alone do not fix the shape of the code. A measurement
of the game layers (`engine input player game runtime`, about 21,300 lines, 2026-10-15)
found roughly 7,700 hex literals with five or more digits, about 1,950 `*_at_<hex>`
identifier uses, 249 calls dispatched by callee address (`call(0x40c0b4,0x415000)`), 73
`goto`s to labels named after instruction addresses, and 16 provenance annotations. There
are five causes, and the project's own documents caused most of them:

1. **The method asked for it.** `METHOD.md` §3 told agents to write "state struct with
   `<meaning>_at_<hex>` members … every store in instruction order; every branch with its
   exact x87/flag condition". Agents followed it literally and produced transliterated
   assembly (`src/game/game_mode.cpp`).
2. **The hybrid runtime leaked into game code.** Because any callee might still be guest code,
   ports call their callees through `invoke(GameModeCall{site, callee, args})` and receive
   `source_boundary` results. Game code therefore names every call by two addresses.
3. **Exact arithmetic is written as emulator calls.** Physics reads
   `fp.add_float_memory(product(runtime.world_gravity->x,p.weight),world.motion.acceleration.x)`
   instead of `gravity.x * weight + acceleration.x`.
4. **Constants are fetched by address.** `c(0x7e0178)` looks up a value that the original
   compiler emitted from a source literal.
5. **Unknowns are threaded through every read.** Every field is `std::optional`, and every
   read cites the instruction that would stop (`need(mode, 0x40c091)`). That precision belongs
   in the reference harness. In game logic it buries the behaviour.

The user's destination is source that anyone can download, build, and play with assets from
their own disc or install, and that developers can later extend with new features. That
requires code organised by game system, with readable behaviour and an explicit place for
additions that cannot disturb the faithful game.

## Decision

1. **Two tiers.** The *game tier* is the shipping code: game systems, the Ninja engine, the
   pinned profile's behaviour, and the session. It uses semantic types, structured control
   flow, named constants, direct named calls, and plain values with recovered initial
   state. The *reference tier* (`src/guest`, `src/hle`, the per-system reference adapters and
   the override table) holds everything that is address-shaped: guest addresses, register and
   stack marshalling, callee-address dispatch, `source_boundary`/`required_fact` reporting,
   and optional "unknown in this fixture" state. Game-tier code never names a guest address
   except in a provenance annotation.
2. **Seams instead of address calls.** When game-tier code depends on behaviour that is not
   yet native, it calls a named function declared in the system's seams header, for example
   `int run_logos();`, annotated `// STUB: SONIC 0x42cc70 RunSegaSonicTeamLogos`. The
   reference build links an adapter definition that calls the guest, which keeps the address
   in the adapter. The native build links the recovered implementation or a definition that
   fails loudly, and it never skips the call silently (STYLE.md §5).
3. **Explicit unknowns move to the boundary.** "Explicit unknowns" (AGENTS.md) still holds.
   An unknown is reported where it arises: in the adapter, when guest memory or a fixture does
   not supply a fact; at a `STUB` seam; or in the importer, when an asset is missing. Game-tier
   state holds values with recovered initial state (`.data` initialisers as annotated
   literals, `.bss` as zero, content from the imported pack). New game-tier state does not use
   `std::optional` to model "the fixture did not map this word".
4. **Arithmetic is readable, and fidelity is proved per module.** Exactness is still
   non-negotiable. It is now expressed at the most readable level that the module's oracle
   accepts. In order of preference: (a) plain `float`/`int` expressions (`-ffp-contract=off`
   is already set), allowed when the module runs under the measured game-thread x87 control
   word (24-bit precision, round to nearest: Windows capture in `HANDOFF.md`, "Floating-point
   observations") and its oracle and mutants pass in that form; (b) named fidelity helpers
   from `src/cpu` for the known divergences (mixed float/double operands, float-to-integer
   conversion, transcendentals, unordered comparisons); (c) an operator-overloaded exact type
   over `X87Context`, so that expressions still read as math; (d) explicit `X87Context` call
   sequences, only inside `src/cpu` helpers or where an oracle shows that instruction-level
   sequencing matters, with a comment saying why. The control-word premise behind (a) is a
   hypothesis. It is checked per call site in the guest before a module relies on it (GOAL
   §6).
5. **Literal constants are literals.** Values that the original compiler emitted from source
   literals into its constant pool are written as literals in the code. The value must round
   trip exactly (hex-float if needed), and named `constexpr` values are used where a name adds
   meaning. Tables of content (physics parameters, models, animations, object and sound
   lists) come from the imported pack. Tables of code structure (dispatch, jump and state
   tables) become named enums and arrays of named functions.
6. **Organised by game system.** New and migrated game-tier code lives in system modules (the
   destination map is in `docs/ARCHITECTURE.md`): Ninja engine, tasks, collision, player
   (common plus one module per character), camera, objects and enemies, stages, bosses,
   events, adventure fields, Chao, minigames, UI, sound, save, game flow, and the pinned
   profile. Each system has one public header. The layer checker keeps enforcing the
   dependency order.
7. **A readability ratchet in the gate.** A checker (`tools/gate/check_readability.py`, in
   the gate since 2026-10-15) counts per file the address-shaped constructs outside the reference
   tier: hex literals not in annotations, `*_at_<hex>` identifiers, callee-address calls,
   address-named labels and `goto`, and over-long or multi-statement lines. A tracked baseline
   may only go down. New files must be clean. `.clang-format` applies to every new or migrated
   file.
8. **The faithful game is the default, and enhancements sit on top.** Additions beyond the
   pinned profile, such as practice tools, overlays, widescreen, debug views and future
   features, are *enhancements*. Each enhancement has an id, a typed setting that defaults to
   off, and a declared class: *replay-neutral* (it may not change game state, which the
   replay digests prove) or *gameplay-affecting* (it disables the "faithful" label of a
   session). Enhancements attach through named hook points that systems expose, or a single
   documented settings check at a hook site. They never edit faithful code paths ad hoc. All
   fidelity tests run with every enhancement registered and off, and replay-neutral
   enhancements are also tested on. Content mods overlay imported resources by resource id.
   A native mod API is out of scope until an ADR adds it.

## Consequences

* METHOD §3 is rewritten: a recovered function is committed in game-tier form. An
  assembly-shaped draft may be used to get an oracle passing inside one step, but it is
  refactored under the same oracle before the commit, which is how LLM4Decompile-style
  "pseudocode, then refine" work is meant to run.
* The existing exact ports are verified, so they can be refactored under their oracles
  ("refactor under the judge"). This is the cheapest readability work in the project and
  becomes a standing track (GOAL §3, §6).
* Oracles keep judging native code through probes. A probe calls the game-tier function
  through its reference adapter, so oracle scripts, fixtures, and tolerances do not change
  during a refactor. Where a refactor needs a probe ABI change, the change is a separate
  reviewed commit, as AGENTS.md already requires.
* STATUS gains readability metrics, and M5 `native-source-ownership` is checked partly by
  the ratchet reaching zero outside the reference tier.
* The two-tier split is the same move OpenLoco made (fixed-address `loco_global`s 342 → 157 →
  0 before native 64-bit builds) and the one gta-reversed makes (named C++ plus one hook
  table). The enhancement rules follow the HarbourMasters ports' separation of vanilla
  behaviour from opt-in settings and hooks (`docs/RESEARCH-native-recompilation.md`, third
  pass).

## Alternatives rejected

* **Keep transliterated exact code and only rename** (ADR-0006 alone): the shape of the code
  stays unreadable, and future features would have to be written against goto/address
  dispatch.
* **Rewrite readable code without the oracle as judge:** this fails the fidelity
  requirement, and agents would drift into plausible but wrong physics.
* **Mass reformat or mass rename:** it breaks blame and mutant specs and decouples names from
  evidence. Migration goes one system at a time, with its tests.

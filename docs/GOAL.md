# The goal, for `/goal`

This is the standing brief an agent loads for `/goal` in `~/Developer/sadx-macos` (Codex:
`/goal implement @docs/GOAL.md`; Claude Code: `/goal`, from `.claude/commands/goal.md`).

It was rewritten on 2026-10-26, when the project moved to DiveMac. Three sources fed it:

* the user's 2026-10-20 request for a connected native Sonic story route;
* the 2026-10-21 review ([REVIEW-astra-goal-2026-10-21](REVIEW-astra-goal-2026-10-21.md));
* a fresh read of the code, the docs and the history.

The 2026-10-16 version is in [history/GOAL-2026-10-16.md](history/GOAL-2026-10-16.md).
ADR-0005 to ADR-0008 still hold:

* ADR-0005: native, and independent of the executable;
* ADR-0006: names in code, addresses in annotations;
* ADR-0007: a readable game tier and an address-shaped reference tier;
* ADR-0008: visible journeys and proportionate verification.

This file does not change a milestone definition, the fidelity policy or the licence.
`docs/STATUS.md` holds the numbers.

## 0. The prompt (what `/goal` injects)

```
You are continuing sadx-macos: a native Apple Silicon port of Sonic Adventure DX (2004 PC,
sonic.exe 6b0937ca…24cd74) under the pinned "updated speedrun" mod profile (SADX Mod Loader
eb4cdcca / DLL 3293cf35, Input mod 2.6.1, Speedrun mod 2.0 with CCEF/EHCF/Skipma/63 FPS).
The product is readable, maintainable C++ organised by game system, which anyone can clone,
build and play with assets imported from their own disc or install. It runs no sonic.exe,
no original DLL and no x86 runtime (ADR-0005). The code reads like a game codebase, not a
disassembly (ADR-0006/0007, docs/STYLE.md). The hybrid guest is the development judge, not
the product.

The acceptance target is one connected, normal-input NATIVE Sonic story route (GOAL §1.2):
start → create or load a save → character select → Chaos 0 defeated → Emerald Coast
cleared → train to Mystic Ruins → Egg Hornet defeated → Wind Stone placed → Windy Valley
cleared, at a measured 63 FPS, with audio, keyboard and Switch 2 Pro input, and the pinned
Input/Speedrun behaviour. Progress is the next native route checkpoint (GOAL §3), shown on
screen and proved by an automated check.

Read AGENTS.md, docs/GOAL.md, docs/STYLE.md, the top of docs/STATUS.md and the last
docs/HANDOFF.md entry. Then take the first open item in GOAL §6 (or the item the user
named), and work through the GOAL §7 loop. Integrate before you recover: the next
checkpoint's first missing call decides the work. Game code must stay readable. No address
calls, `*_at_<hex>` names, emulator-style x87 calls or constants fetched from the EXE in
the game tier. If the route needs a legacy module, migrate it under its unchanged oracle;
never link it through a guest adapter.

Keep working after each checkpoint, gate and commit until the route is complete or you are
truly blocked. Never edit an existing judge to pass. Never call a "partial" oracle a pass.
Record a gate only with its real exit status. Keep reviewed work in git, never only in
/tmp. Commit with Conventional Commits, no attribution lines, no push unless asked.
```

## 1. The goal, exactly

### 1.1 The product (unchanged since ADR-0005/0007)

The product is a native arm64 macOS game, built from readable, maintained source. It plays
*Sonic Adventure DX* with the pinned speedrun profile faithfully enough for speedrun
practice, which automated tests prove. The community can maintain and extend it. The user
supplies the assets from their own disc or install through a one-time importer.

* **Native execution.** All shipped game and profile code compiles for arm64. Unicorn, x86
  execution and fallbacks to original code exist only in the development reference.
* **Executable independence.** Building uses maintained source and playing uses imported
  assets. Neither opens `sonic.exe` or runs an original DLL. Only the one-time importer may
  read the user's own EXE/DLL/PVM/PRS files.
* **Readable, maintainable source** (`docs/STYLE.md`). This is the user's standing steer.
  Early agents filled game code with calls to function addresses, `op.at(0x…)` markers,
  `fp.multiply_float_memory(…)` emulator calls and `*_at_<hex>` fields, which nobody can
  read. That style is not acceptable in the game tier, in new code or in code the route
  touches. A developer who has never seen `sonic.exe` must be able to find Sonic's jump,
  change it, and run the tests that say whether fidelity broke.
* **Extensible without losing fidelity** (STYLE §9, ROADMAP M6). Enhancements are opt-in,
  attach at named hook points, and are proved not to change the game when they are off.

### 1.2 The acceptance route (the user's request of 2026-10-20)

The route uses one normal-input native Sonic story run, with no state injection, teleports,
save edits or stitched stage fixtures:

1. Startup, then create a new save or load one.
2. Character select, then Sonic's real story entry.
3. Chaos 0 in Station Square: Sonic is controllable and the boss is active. Chaos 0 is
   defeated and the story returns naturally.
4. Emerald Coast is cleared, followed by the score screen and the return to Station Square.
5. The train goes to Mystic Ruins.
6. Egg Hornet is defeated, with correct progression, including the EHCF path.
7. The Wind Stone is carried and placed.
8. Windy Valley is cleared.

These requirements must hold along the route:

* correct physics, collision, camera, animation, objects, events and save progression;
* delivered music, sound effects and the voices the route reaches;
* keyboard and the physical Switch 2 Pro controller over USB;
* gameplay measured at the pinned 63 FPS cadence;
* the pinned Input/Speedrun behaviour (CCEF, EHCF, Skipma, the limiter) bound at the real
  callers that run.

The overall goal is complete only when the whole route and these requirements have current
evidence. The route is not a reason to rebuild unrelated parts of the game first. Recover
what the route actually reaches, including the real menus and actor callbacks before it.
Never swap in an easier stand-in, such as a static pose for an actor or a fixture for a
transition.

## 2. How we got here, and what it taught

| Days | What happened | What the user could see |
|---|---|---|
| 10-10/11 | Function-by-function Unicorn oracles (Hopper) | nothing |
| 10-12 | Throughput review; ADR-0001 hybrid runtime, ADR-0002 Ghidra via REA | nothing |
| 10-13 | Guest boots sonic.exe with loader, mods and SDL2; Metal app; overrides | reference: logo → title → file select |
| 10-14/16 | Symbol DB, ADR-0005..0008, provenance/readability/judge checkers, 11 constant-migration gates | no change (the user's 10-16 complaint) |
| 10-16/17 | D3D FPU fix (24-bit), Metal fixed-function renderer, fog/scene-position rewrites | reference: Station Square drawn |
| 10-19 | Composition diagnosis ([RESEARCH-breakthrough](RESEARCH-breakthrough-2026-10-19.md)); logo module; Chaos 0 controllable in the reference | reference: Chaos 0 |
| 10-20 | Codex "Astra" goal: native Start/logo/movie/title connected; Switch 2 Pro over USB; native audio | **native**: logos, intro movie, title, audible Start |
| 10-21 | Native FileSelect, dialog, MainMenu, fresh save; CharacterSelect caller started; reference replay through the train to Mystic Ruins | **native**: menus and saves; CharacterSelect stops at its first actor draw |

Lessons. Keep them all; the first five are new since 10-16.

* **Composition is the bottleneck, not recovery.** Exact leaf functions with no native
  caller do not move the game. The jump on 10-20/21 came from integrating real caller
  chains through the real app, and full-frame comparison found defects that isolated
  oracles missed.
* **Correlated mistakes slip past local oracles.** In the quick-open dialog, both the
  implementation and its probe used the wrong field (fade instead of the animation counter).
  Only composed native/reference frames caught it. Check owner mappings at the adapter
  boundary with distinct non-zero values, callback order, and the composed frame.
* **Weak checkers pass broken output.** The MainMenu journey accepted ten solid-black frames
  until 5511f66. Every checkpoint checker must check drawn content: source-backed regions
  or golden hashes, plus semantic state. A draw-suppression mutant must make it fail.
* **Reviewed work kept only in `/private/tmp` was lost.** On 10-21 about ten reviewed
  "packets" (object draw, texture selection, Sonic entry/current state, collision component,
  Gamma entry, CharacterSelect runtime and assets) lived only under `/private/tmp/sadx-*`.
  A reboot wiped them. §8 makes packets durable.
* **Policy drift.** A worker invented a "physics parameters must be compiled literals" rule
  and cited STYLE §5. STYLE §8 and ADR-0007 §5 allow typed physics content in the imported
  pack. Read the documents and do not invent rules.
* **Ceremony without a visible target stalls quietly** (10-16: 22 green gates, nothing
  visible, game-tier source 1.7 % of added lines).
* **Relocating is not rewriting.** Moving code into `src/guest` lowers the ratchet without
  making anything readable.
* **A reference that models the platform wrong blocks readable code.** The guest's D3D8
  device did not set 24-bit x87 precision. Fixing that (ADR-0008 §4) made plain `float`
  the default for post-device game math.
* **Agents edit judges when they can.** The judge is tamper-evident (`docs/JUDGE.md`). Edits
  to existing judges go in separate, evidenced `fix(judge)` commits.

## 3. Strategy: the route ladder

### 3.1 Native checkpoints (the product; the spine)

Each row is something the user can watch with `make play-native` (or a dedicated `make`
target), plus an automated check in `make native-journeys`. That check:

* drives `build/sadx-native --offscreen` with normal key events or a script;
* asserts semantic state, input, story and save transitions;
* checks drawn content against the reference app at aligned checkpoints, and reports raw
  timing offsets instead of hiding them;
* checks delivered audio events, and resource lifetimes where they matter.

A checkpoint is done when that check passes in a full gate, and a source-only build (no
EXE/DLL, no prebuilt game objects) runs it from imported assets.

| Row | Checkpoint | State at HEAD `0881b5e` |
|---|---|---|
| N1 | SEGA/Sonic Team logos | done (16 states + images vs reference) |
| N2 | Intro movie (play and Skipma), title, audible Start | done |
| N3a | FileSelect, dialog, MainMenu, fresh save/load/restart, empty profile | done (`native-file-select`, `native-main-menu`, `native-fresh-save`) |
| N3b | **CharacterSelect**: all preview actors initialised, updated and drawn; cursor/cancel/reentry; music/voice; Sonic confirmed → story entry | **in progress**: init, palette, seven actor tasks and camera run; stops at `native_basic_object_renderer`, frame 1382 |
| N4 | Story entry: the opening event (played or skipped as the profile does) → Station Square drawn from the imported pack with the real camera | open |
| N5 | Sonic controllable at Chaos 0: run, jump, spin/homing attack, hurt, rings, with the boss active; first frames match the reference trace | open |
| N6 | Chaos 0 defeated → natural story return → save progression | open |
| N7 | Emerald Coast act 1 cleared → score → Station Square | open |
| N8 | Train → Mystic Ruins | open |
| N9 | Egg Hornet defeated, EHCF path, progression | open |
| N10 | Wind Stone carried and placed → Windy Valley cleared | open |
| P | Cross-cutting: measured 63 FPS gameplay (frame-time distribution), gameplay audio (pitch, position, listener, queued voice), CCEF/Skipma/EHCF at their gameplay callers, Switch 2 Pro over the gameplay path | open |

`tools/gate/gen_status.py` still uses the 10-16 wording ("N4–N5"). Update it from the
machine-readable checkpoint names when you add the N4+ checks.

### 3.2 The reference lane (the judge)

The reference app (`sadx-app --guest`) runs the original code and draws it with our Metal
backend. It supplies every native checkpoint's judge: state traces, images, audio events
and replays.

* R1–R4 (logo, title, character select, 3D world) are required gate journeys.
* The story route replay is a clean, normal-input recording through the train's arrival at
  Mystic Ruins:
  * `reference/story-route-20261020/sonic-story-through-mystic-ruins-train-arrival.recorded.json`
    (2,467 commands, SHA-256 `6a88fd5d…`, with a `.recorded.evidence.json`);
  * the verdict, `reference/story-route-20261020/clean-replay-20261021/final-verdict.json`,
    has 410/410 state and image checkpoints, and the final save matches byte for byte.

  These files are ignored and local, copied to DiveMac with the tree. Replay them with
  `tools/journey/replay_story_route.py`.
* Still open in the reference: Egg Hornet → Wind Stone → Windy Valley. Extend the replay
  with normal input only, and keep one input owner per observer process.
* Reference stage throughput is about 14 FPS (guest tracing cost). That is a reference
  limit, not a product number, so do not optimise the guest unless it blocks a judge.

### 3.3 Tracks

| Track | What | Judge |
|---|---|---|
| **J: route** | the next N row: caller chains, renderer, stage import, actors, audio, input, session | that row's native checkpoint check, plus unit tests |
| **R: readability** | rewrite legacy modules into game-tier form *when the route needs them*, and the worst offenders between checkpoints | same oracle verdict/counts, mutants still caught, ratchet down |
| **S: recovery** | recover a missing callee in game-tier form (`METHOD.md` §3) | new oracle with mutants and holdout; independent review for a new system |
| **V: verification** | stronger checkers, replays, reference-lane extension, M4 Windows parity | ROADMAP tables |
| **E: enhancements** | later (M6) | enhancements-off parity |

The rule: **every step advances the next N row, or removes what blocks it.** Readability
work is part of the route, not a separate campaign. When the native caller reaches a legacy
module (most of `src/player`), migrating that module is the step.

### 3.4 Step size and proportionate verification

* A step is a coherent connected slice: a caller chain to the next visible boundary, a
  whole system, or a whole module rewrite. It is never one constant family.
* Run focused checks while you iterate, and one full gate at each coherent checkpoint. A
  gate takes about 10–20 minutes on DiveMac.
* Verify each kind of change at the level the §7 table gives.
* Commit small evidence: the gate record plus a short JSON or Markdown summary. Reusable
  harnesses go in `tools/` with a test. One-off scripts are not committed.

## 4. Readable code: what it means here

This is the user's steer. STYLE.md has the full rules; these are the ones agents broke
before.

* **Two tiers.** The game tier (`src/{cpu,math,engine,input,player,game,runtime}`) may
  contain:
  * semantic names and `enum class`;
  * structured control flow;
  * literal constants;
  * plain `float`/`int` where the oracle accepts it;
  * named `sadx::fidelity` helpers only where the oracle shows x87 differs;
  * `STUB` seams for callees not recovered yet.

  Addresses appear only in `// FUNCTION:`/`// GLOBAL:`/`// STUB:` annotations. Every
  address, register, marshalling step and fixture unknown lives in the reference adapters
  (`src/guest`, `src/hle`).
* **Banned in the game tier:**
  * `call(0x…, 0x…)` or `ret(0x…)` dispatch;
  * `op.at(0x…)` markers and `fp.xxx_memory(…)` emulator calls;
  * `*_at_<hex>`, `sub_`, `FUN_`, `DAT_`, `local_` and `param_` names;
  * `c(0x…)`, `image.u32(…)` and any other constant fetched from the EXE;
  * `goto`;
  * `std::optional` meaning "the fixture did not supply this".
* **Magic numbers become names** once their meaning is known. Menu choices, character ids,
  actor modes, stage/act ids and flags become `enum class` or `constexpr` with the
  original's name in a comment. Unknown meanings say so in a comment.
  * The 2026-10-26 review found new code that gets past the hex ratchet by writing the same
    constants in **decimal**: `interpolate_colors(16777215U, 4294967295U, …)` in
    `title_menu.cpp`, and `anchor = up ? 1191182336U : 16777216U` in
    `file_select_display.cpp`. Colours, packed flags and float bit patterns are named
    `constexpr` values, in hex where hex is how the format is read (`kOpaqueWhite =
    0xFFFFFFFF`).
  * `CharacterSelect` and `MainMenu` still dispatch on raw `submode` 0..10, dialog ids
    7..10, music 3/4, button masks 64/128 and return values 101..104. Name them as the
    callers are extended.
* **Plain `float` is the default for post-device code** (STYLE §7 level 1, census `0x007f`
  after the D3D fix).
  * New menu code still threads `X87Context`/`ExactFloat` through UI arithmetic:
    115 uses in `file_select_display.cpp`, 98 in `title_menu.cpp`, 90 in
    `menu_draw_primitives.cpp`.
  * `docs/TIPS.md` lines 31–35 still describe the pre-fix census (`0x023f`, 53-bit) and
    mislead agents into choosing level 3. That TIPS entry is stale; STYLE §7 and
    `reference/fp-control-census.json` win.
  * Use `ExactFloat` or helpers only where the oracle proves a difference.
* **System-owned state and link-time seams, not service sprawl.** STYLE §5 prefers plain
  link-time `STUB` seams. Today there are 89 `*Calls`/`*Services` structs with about 580
  `virtual`s, and `runtime/native_session.hpp` is a god object with about 100 fields and 23
  `attach_*` methods.
  * Before gameplay adds hundreds of fields, split the session into system owners (save,
    sound, menu resources, stage/task state) that game code takes by reference.
  * Interface methods that remain need named parameters and a `STUB` line naming the
    original callee.
  * Every new screen currently costs an importer, an asset header, an `attach_*` and a
    `Calls` adapter. That will not scale to stage objects.
* **Annotate what you recovered, not more.** A body that implements only a prefix of an
  original function must say so in its annotation (`// STUB: … prefix only, <boundary>`), not
  carry a plain `// FUNCTION:`. One example is `OptionsMenu_Display` in `title_menu.hpp`.
* **No executable parsing in the game tier.** `src/player/player_scene_decode.cpp` parses
  the PE header to read collision data (ADR-0005 violation). Collision comes from the
  imported pack.
* **No upward links.** `sonic_player.cpp` calls `get_player_acceleration_ground`, which
  `src/guest/player_acceleration_ground.cpp` defines. `check_layers.py` only checks
  includes, so it does not see this. The native closure check (§6 item 2) must.
* **Arithmetic identities are literals.** `GroundConstants::zero`/`half`
  (`player/ground_acceleration_internal.hpp`) violate STYLE §8.
* **The readability metric is necessary, not sufficient.** `check_readability.py` does not
  count:
  * decimal magic numbers;
  * emulator-style calls (`fp.multiply_float_memory`, `X87Context`);
  * per-line address comments.

  So `player_mesh_matrix.cpp` (167 `multiply_float_memory`, `// 0x79500e` on every line)
  scores almost clean. §6 item 2 makes the metric honest. Until then, judge readability by
  STYLE §12, not by the number.
* **Functions read like the game.** Split anything over about 80 lines or nested more than
  three levels into named steps. Add no managers, registries or class hierarchies the
  original lacks. A helper the original does not have gets a `// NOTSA:` tag.
* **Comments explain why** (quirks, ordering that matters). They are not evidence dumps;
  evidence ids go in HANDOFF and in the commit.
* **The ratchet only goes down** (`tools/gate/check_readability.py`,
  `reference/readability-baseline.json`). On 2026-10-26 the game tier still had:

  | Debt | Count |
  |---|---|
  | hex literals | about 10,700 (7,000 of them in two generated Sonic animation tables) |
  | address identifiers | 1,494 |
  | address calls | 62 |
  | emulator-style `fp.`/`op.at(` uses | about 1,250, across 31 files in `src/player` and `src/game` |

  The worst files are:
  * `player/sonic_player.cpp` (1,606 lines; address dispatch; calls ground acceleration
    through the guest adapter);
  * `player/player_scene_position.cpp`;
  * `player/player_mesh_registry.cpp`;
  * `engine/ninja_queue_consumer.cpp`;
  * `engine/ninja_sprite_queue.cpp`;
  * `player/player_animation.cpp`;
  * `player/player_contact_response.cpp`;
  * `player/player_acceleration_alternate.cpp`.

  These are exactly the systems N5 needs. Migrating them under their unchanged oracles is
  route work.
* **Readable but wrong is the default failure** of agent decompilation. Readability is
  judged by the same oracle and mutants, never by eye.

## 5. Where things stand (2026-10-26, HEAD `0881b5e`)

* **Native app** (`build/sadx-native --assets game/imported`). It runs normal input from
  start through the logos, movie (Skipma), title, FileSelect, dialog and MainMenu, and
  creates or loads saves.
  * Adventure → CharacterSelect initialisation and draw → palette 2 → seven real actor tasks
    and the camera factory → the Question actor's yaw/matrix/specular/texture prefix.
  * It stops explicitly at `native_basic_object_renderer` (frame 1382). The first UI frame
    is still the transparent fade over the outgoing logo.
  * Evidence:
    `reference/production-integration-20261021/native-character-select-caller-frontier.json`.
* **Last green full gate:** `native-fresh-save-integration-gate.json` at `f8c2875` (exit 0;
  247 native programs, 268 Python tests).
  * The CharacterSelect integration commits after it (`5311f40`, `0600979`, `0881b5e`) have
    no green gate.
  * The first attempt exited 2: the legacy `player-auxiliary-effect-test` needed
    reference-only linkage. Judge commit `0881b5e` addressed that.
  * The final attempt was stopped by the user (exit −15).
  * **HEAD is not gated green.**
* **Reference:** R1–R4 pass. The clean replay runs through the Mystic Ruins train arrival
  (§3.2).
* **Oracles** (STATUS): 209 in total, with 119 pass, 78 partial (values exact, FP flags
  differ under Unicorn), 10 fail and 2 missing.
  * Fails that matter for N5: `floor_angles` and `player_reaction`. Triage them before you
    claim Sonic gameplay fidelity.
  * The CRT/x87/SSE/D3DX fails are Unicorn transcendental or SNaN limits. M4 settles them.
* **Production closure.** `NATIVE_SOURCES` in the Makefile (about 125 files) plus the
  `*_native.cpp` wildcards build `libsadx-native.a`, with no loader, guest, HLE or Unicorn.
  It includes only 6 of the 107 `src/player` files.
* **Lost work.** The reviewed `/private/tmp/sadx-*` packets named in the last HANDOFF
  entries no longer exist. The Codex worker transcripts that wrote them are on DiveMac in
  `~/Developer/sadx-agent-history/codex-rollouts/` (71 sessions from 10-20/21, about
  650 MB, JSONL). Their `exec` tool calls contain the heredocs and patches that created
  each packet file.

## 6. What is next, in order

Take the first open item. Items 0–3 are setup; from item 4 on, each item is one or more
steps.

0. **DiveMac orientation (once).** Run the environment check in §9:
   * `rea doctor`;
   * `make -j10 all build/sadx-native`;
   * `make native-journeys`.

   The docs added on 2026-10-21/26 were committed and pushed on 2026-10-26:
   * `docs/GOAL.md`;
   * `docs/history/GOAL-2026-10-16.md`;
   * `docs/history/README.md` and `AGENTS.md`;
   * the REVIEW, RESEARCH and PROMPT files;
   * the `.claude/commands/goal.md` update.

   The two untracked `reference/ghidra-*requests.json` files are leftover request sets: commit
   them if a recovery still needs them, otherwise delete them.

   Run the full gate (`make -j10 gate`) once from DiveMac's GUI login session.
1. **Green gate at HEAD.** Make `make -j10 gate` pass at the CharacterSelect frontier.
   * Fix real failures in code.
   * Judge defects go in separate `fix(judge)` commits with evidence.
   * Machine-move artifacts (absolute paths, fingerprints, host identity) are judge
     defects too: fix them so the gate is portable, never by re-recording expected values
     to pass.
   * Record the gate and regenerate STATUS.
2. **Tooling that pays for itself before gameplay.** This is one step, as separate
   `fix(judge)` commits with evidence. It strengthens the judges; it does not loosen them.
   * **Native closure link check.** Add an `nm`-based check that `libsadx-native.a` plus
     the platform objects resolve every symbol without the guest, HLE, loader or Unicorn
     objects. Mutant: reintroduce the ground-acceleration guest link, and the check fails.
   * **`NATIVE_SOURCES` out of the protected Makefile.** Derive it, or move it to a file
     that is not judge-protected, guarded by the closure check above. Today 49 of the 75
     `fix(judge)` commits touch the Makefile, often just to add one `.cpp` to the product.
   * **Honest readability metric.** `check_readability.py` gets three new counts:
     * decimal literals of 2^16 or more outside named `constexpr`;
     * emulator-style arithmetic call sites (`fp.`, `X87Context`, `multiply_float_memory`);
     * per-line address comments.

     Hex inside a named `constexpr` declaration is allowed. Baseline the new counts at
     HEAD; they may only go down.
   * **Content-checking checkers.** Every native checker checks content on every capture
     it takes:
     * full-frame not blank;
     * a committed coarse signature, or a source-backed region compared with the
       reference.

     `check_native_file_select.py` and `check_native_fresh_save.py` today check several
     captures only for a valid PPM header. Do the same for reference R2/R3 in
     `run_journeys.py`, which pass on a colour count. Each strengthened checker gets a
     draw-suppression mutant.
   * **A state-trace diff for gameplay.** Put `tools/journey/compare_native_startup.py` in
     the gate. Generalise it into a per-frame native-versus-reference state-trace diff on
     the same input script, covering mode, player, camera and the task list. This is the
     N5 judge, and it scales better than per-function oracles.
   * **Fix stale docs.**
     * `docs/TIPS.md` lines 31–35 (census): cite `reference/fp-control-census.json`.
     * STATUS's milestone line, its 10-13 boot survey and its "not built yet" sections:
       generate them from source, never hand-edit.
     * The duplicated R1 row.
     * Archive HANDOFF entries before 2026-10-19 to `docs/history/HANDOFF-2026-10-10-to-18.md`.
3. **Recover the lost packets, time-boxed.** For each packet the next checkpoint needs, in
   the order of item 4:
   * Try to rebuild its files from the rollouts. Write one reusable extractor in
     `tools/agents/` that replays a session's file writes for one `/private/tmp/sadx-<name>`
     prefix into `build/salvage/<name>/`.
   * Rerun its oracle, mutants and holdout before you trust it.
   * If a packet does not rebuild cleanly within one attempt, recover it fresh. The
     transcripts still give the evidence ids, the decisions and the traps they recorded.
   * Never integrate salvaged code that its unchanged oracle does not re-verify.
4. **Close N3b, CharacterSelect.** Write the checkpoint's dependency table first (§8), then
   integrate in the order the native app reaches each call:
   * `403070`: queued and current texture-list selection and cache binding. Leaving the
     queued list NULL would falsely skip models through `403250`.
   * The shared, mutable object graph and its traversal (`dsDrawObject 408530` →
     `DrawObject 4034b0`), then the model draw `77ef70` on the shared Metal device.
   * Every preview actor CharacterSelect creates: Question, Gamma (mode 0/ID 2), Sonic
     `49a9b0` and the others. Each needs its real init (`PInitialize`, animation tables,
     CCL collision init `41caf0`, welds, common tail), update and display. No pose-only or
     seeded-tail stand-ins.
   * Cursor, cancel and reentry; music 3/4 and character voices; source cue 2047 stays an
     explicit missing asset.
   * Sonic confirmation, which reaches the real story-entry path and its first unrecovered
     call.

   The check has normal input. It compares images of CharacterSelect with actors drawn
   against reference R3 at aligned frames, and includes a draw-suppression mutant.
5. **N4, story entry and Station Square drawn.**
   * The event/cutscene flow, played or skipped as the pinned profile does.
   * The stage loader reads the imported pack: landtable, models, texlists, SET/CAM. Write
     sa_tools-schema descriptors and typed, hashed records that re-serialise to their
     source bytes; unknown hashes fail with a tagged error.
   * The R4 renderer path draws geometry from native resources.
   * The real gameplay camera owner replaces the startup "camera absent" placeholder.
6. **N5, controllable Sonic at Chaos 0.**
   * Migrate the player systems into the production closure: Sonic's actions, acceleration
     and friction, slopes, collision/contact, the camera-relative input producer, animation
     and the mesh registry.
   * Use semantic interfaces. `sonic_player.cpp` must stop reaching
     `get_player_acceleration_ground` in `src/guest`.
   * Triage the `floor_angles` and `player_reaction` fails.
   * The judge is the first N frames of the reference Chaos 0 segment of the story replay,
     state trace plus images.
   * Start the 63 FPS gameplay measurement here (P), separating build/test load.
7. **N6, Chaos 0 defeat**, then **N7–N10** along the route. Bind CCEF, Skipma and EHCF at
   their real gameplay callers as they are reached, along with the gameplay audio paths
   (variable pitch, positional sound, listener, queued voice).
8. **Reference lane, in parallel when capacity allows.** Extend the normal-input replay
   through Egg Hornet → Wind Stone → Windy Valley, and verify each extension from a clean
   start. Its role is to judge the native rows that follow.
9. **Readability between checkpoints.** When a checkpoint closes, take the worst remaining
   offender that the next row will touch (§4 list) and rewrite it under its unchanged
   oracle.
10. **Later:**
   * M4: Windows trace collector, Intel transcendental tables for the Unicorn-limited fails.
   * M5: freeze the reference corpora, separate the build graphs, ratchet at zero outside
     the reference tier.
   * M6: hook registry, settings, practice overlay.

## 7. The loop for one step

1. **Name the item, its N row and the outcome** in one line: the boundary that moves (from
   which call or frame to which), or the debt removed (file and counts). Read the last
   HANDOFF entry and reread `docs/STYLE.md`. Instructions decay over long sessions.
2. **Find the frontier and build context.**
   * Run the native app to the current stop and read the reported boundary.
   * For game logic, build the bundle (METHOD §0, §3):
     * names, signatures and structs from the symbol database (`tools/symbols/lookup.py`,
       `context_bundle.py`);
     * callers and callees, and which are native;
     * named pseudocode (`batch_decompile`/`analyze_function` through
       `reference/ghidra-live.mjs`);
     * xrefs for globals;
     * exact bytes (`pe_image`);
     * profile status (0 UNHANDLED);
     * the nearest game-tier module as a style model.

     Pseudocode is evidence, not source.
   * Platform work starts from the D3D8/Win32 contract and the guest's call stream.
3. **Implement in two tiers** (§4, STYLE). Invent nothing: every field, callee and guard has
   evidence. A transliterated draft may exist inside a step, but it is refactored under the
   same oracle before the commit. Map each borrowed owner once at the adapter boundary, and
   check it with distinct non-zero values.
4. **Verify at the level the change needs. No more, no less:**

   | Change | Required |
   |---|---|
   | Integration of existing native modules into the app | the checkpoint's journey check, extended; images with content checks; a draw-suppression or wrong-owner mutant fails it; source-only build runs it |
   | Constants or tables moved to native source | one unit test comparing every value with the PE bytes, one deliberate break |
   | Platform work (renderer, input, audio, importer) | unit tests for the decoding/state logic, the journey check, one deliberate break |
   | Readability rewrite of a ported module | same oracle verdict and counts, existing mutants still caught, ratchet down |
   | New exact port or recovered system | oracle + `verdict.py` + 3–7 mutants, dev cases while iterating, holdout once at the end; independent STYLE §12 review |
   | Judge fix | a separate `fix(judge)` commit with evidence (`docs/JUDGE.md`) |

   * Break the code once and watch the check fail.
   * Never change an existing tolerance, expected value, fixture, mutant or harness to
     pass.
   * After three failed approaches on one function, stop. Record the unknown with its
     evidence in HANDOFF and `docs/TIPS.md`, then move on.
5. **Gate at the checkpoint:** `make -j10 gate 2>&1 | tee build/gate.log; echo rc=$?`. It
   needs the GUI login session (windowed app checks).
   * On rc 0 only, record and regenerate STATUS:
     ```
     python3 tools/gate/record_gate.py --log build/gate.log --command "make -j10 gate" \
       --exit-status 0 --limit "…" --output reference/production-integration-<date>/<name>-gate.json
     python3 tools/gate/gen_status.py
     ```
   * If ported code changed, rerun its oracles:
     `make oracle-<slug>`, then `make oracles-verdict`, then
     `python3 tools/gate/track_reports.py <slug>…`.
   * Register new tests and probes with `check_judge.py --update --additions-only`.
6. **Journal and commit.**
   * Write one HANDOFF entry of at most 15 lines, **in plain sentences with spaces between
     words and numbers.** Recent entries ("Actualintegrationfoundwrongquick-open…") are
     unreadable; do not copy that style.
   * Cover the row, what is now visible, the new boundary, verdicts with counts, what was
     not run, and the next call.
   * Commit with a Conventional Commit subject, no attribution lines, no push.
7. **Report and continue.** Tell the user, in plain words:
   * what the native app now does, and the command to see it;
   * the gate name and rc;
   * what is unverified;
   * the next missing call.

   Then continue with the next item. Do not stop after one step unless the user asked for
   one step, or you are blocked on something only the user can provide.

## 8. Working with workers and packets

* **The root integrates; workers remove independent dependencies.**
  * Keep one dependency table per checkpoint in the HANDOFF entry, with these columns:
    actual caller/callback, canonical owner, missing behaviour or resource, packet, owner,
    integration check.
  * Consume finished packets before you start new recovery.
  * Never assign two agents to the same owner.
* **Packets are durable.**
  * A worker works in its own git worktree on a branch named `packet/<name>` (for example
    `git worktree add ../sadx-wt-<name> -b packet/<name>`).
  * Freezing a packet means committing to that branch, with its evidence summary and a
    `PACKET.md` stating scope, limits, evidence ids and the integration check.
  * Large raw evidence goes under the ignored `reference/` or `build/` of that worktree,
    never `/private/tmp`.
  * The root merges or cherry-picks surgically, not by copying old whole headers.
* **Builds.**
  * One owner runs shared builds and the gate.
  * Workers build in their own worktree.
  * Before a full gate, preflight the affected build: target registration in
    `tests/unit/manifest.tsv`, Make generator prerequisites, probe freshness (`.dylib`
    newer than its source), and profile-patch tracking.
* **Capacity on DiveMac** (M5, 10 cores, 16 GB).
  * A gate at `-j10` uses most of the machine.
  * Ghidra headless takes `GHIDRA_HEADLESS_MAXMEM=2G`, and a reference observer about
    1.8 GB.
  * Run at most two or three concurrent workers. Do not run a full gate while a worker
    compiles.
* **Independent review.** Use a fresh reviewer, given the evidence, oracle results and
  diff, for each recovered system and before closing each N row. Review caught real
  defects on 10-20/21.

## 9. This machine: DiveMac

The project lives at `~/Developer/sadx-macos` on DiveMac (macOS 26.6, Xcode 27.1, Apple M5,
16 GB). It was copied from Macintosh on 2026-10-26 with every ignored input, keeping file
times. The paths are the same as on Macintosh. On 2026-10-26 on DiveMac:

* `make -j10 all build/sadx-native` built in 36 s;
* `make native-journeys` passed all three checkpoints, with exact reference-region pixel
  matches, so the goldens survived the GPU/OS change;
* `rea doctor --provider ghidra` was healthy.

Setup facts:

* **Use a login shell.** A non-login SSH shell resolves `python3` to `/usr/bin/python3`
  3.9, which breaks the gate scripts. `command -v python3` must print
  `/opt/homebrew/bin/python3` (3.14.7).
* **Power.** Keep DiveMac on AC with the lid open, and run long work under `caffeinate
  -dims`. Its battguard daemon switches to Low Power Mode when the lid is closed on
  battery. Do not run Ghidra (2 GB heap) during a gate.
* **Ghidra environment files.** Only `reference/tools/ghidra-game-loop-environment.json`
  (used by `ghidra-inventory-config.json`) and `ghidra-sdl-environment.json` resolve. The
  `shadow`, `session`, `gameplay` and `action-fp` environment files point at a stale
  `~/Developer/rea/_reference/sadx/…` that is empty on both Macs. Use
  `reference/ghidra-inventory-config.json`, or fix the others in a small commit.
* **REA snapshots.** None of the committed or local snapshots matched the current seed
  digest (`e580b0…`), on Macintosh as well. A fresh seeded analysis through the owned runner
  succeeded on DiveMac in about 6 minutes (evidence `ev_cc468d35…`) and saved a new
  snapshot matching the current seeds. The old stale snapshot is
  `reference/inventory-raw/sonic-ghidra-inventory-snapshot.before-divemac.json`. After a seed change
  (`make symbols` with new overrides), expect one fresh import of several minutes.
* **The venv's `bin/pip`** points at a deleted interpreter; use
  `reference/python/bin/python -m pip`.
* **SDL3/libusb.** `make -n build/native-controller-deps/include/SDL3/SDL.h` must say it is
  up to date. If it is ever rebuilt, run
  `rm -rf build/native-controller-sdl3 build/native-controller-libusb` first; the old CMake
  cache names a deleted `/private/tmp` cmake.

| Path | What |
|---|---|
| `game/Sonic Adventure DX/` | the working install (sonic.exe `6b0937ca…`, system/, mods/) |
| `game/pristine/sonic.exe`, `game/disc/…` | untouched download; retail disc images |
| `game/imported/` | native imported assets (`tools/import/native_assets.py`) |
| `reference/updated-pc-reference/game/` | the pinned profile install: the guest's install directory |
| `reference/windows-system/` | 32-bit msvcp140/vcruntime140 the guest runs |
| `reference/sadx-mod-loader/` (submodule), `reference/sa-tools/` | symbol and data-mapping sources |
| `reference/python/` | venv: Unicorn 2.1.4, capstone 5.0.9, clang-format 18.1.8 (Homebrew python@3.14) |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra; JDK = Homebrew `openjdk` 27 |
| `reference/tools/rea/node_modules/rea-agents/` | REA 6.4.0; `~/Developer/rea/` is the source checkout |
| `reference/story-route-20261020/` | reference story replays and verdicts (§3.2) |
| `build/native-controller-*` | pinned SDL3 3.4.2 / libusb 1.0.29 build (needs `cmake`, installed) |
| `~/Developer/sadx-agent-history/codex-rollouts/` | Codex transcripts of 10-20/21 (salvage source, §6 item 3) |

Environment check (also METHOD §0):

```sh
cd ~/Developer/sadx-macos
export GHIDRA_INSTALL_DIR="$PWD/reference/tools/ghidra_12.1.4_PUBLIC"
export JAVA_HOME=/opt/homebrew/Cellar/openjdk/27/libexec/openjdk.jdk/Contents/Home
export REA_GHIDRA_STARTUP_TIMEOUT_MS=600000 REA_GHIDRA_SEED_FILE="$PWD/build/symbols/rea-seeds.tsv"
REA="$PWD/reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs"
make symbols && node "$REA" doctor --provider ghidra --json | tail -5
make -j10 all build/sadx-native && make native-journeys
```

The physical Switch 2 Pro controller qualification was done on Macintosh. Requalify it on
DiveMac only when a hardware-dependent path changes, and ask the user to connect it. The
windowed checks (`app-smoke`, the R journeys) need the GUI console session. Run the gate
from Terminal on DiveMac or through Screen Sharing, not from a detached SSH shell.

## 10. Tools (unchanged; details in METHOD §0)

* **Cookbook.** `docs/METHOD.md` §0 maps each question to an exact command: symbol lookup,
  structs and enums, callees with names, pseudocode, clusters, xrefs, assembly, bytes, profile
  patches, sa_tools mappings, runtime facts and oracles. `docs/TIPS.md` holds SADX/MSVC
  idioms; add to it when you learn something.
* **REA.** The owned runner `reference/ghidra-live.mjs <config>` replays a JSON request
  file through `rea mcp` with Ghidra and saves a snapshot. The CLI (`rea.mjs
  function|decompile|xrefs|instructions|doctor`) answers single or cached questions from
  the snapshot.
  * Names enter Ghidra only through the seed file. Live renames do not persist.
  * Types cannot be pushed into Ghidra. Struct layouts live in project headers, checked by
    oracles and `static_assert`.
  * There is no emulation, struct inference or compile-to-source. Runtime facts come from
    the guest.
  * Cite every `evidence_id`.
* **Symbol database** (`make symbols`). Sources, in order of preference: the mod loader's
  `*New.h`, the classic headers, sa_tools labels, Ghidra boundaries, then
  `reference/symbols/overrides.tsv`. A name is a hypothesis until the recovered behaviour
  agrees. The loader headers are GPL: use their facts, and write our own declarations.
* **Comparable projects** ([RESEARCH-native-recompilation](RESEARCH-native-recompilation.md),
  [RESEARCH-breakthrough](RESEARCH-breakthrough-2026-10-19.md)):
  * re3/gta-reversed: hook table, `NOTSA`;
  * isledecomp: annotations, macOS port;
  * OpenLoco: state migration peaks before it falls;
  * Ship of Harkinian: `should(VanillaBehavior…)`;
  * TRX: typed settings, no hard-coded options;
  * momo5502 and the LLM-decompilation papers: a tamper-proof judge, and readable-but-wrong
    as the default failure.

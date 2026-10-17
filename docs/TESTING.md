# Testing: what the suite proves, what it does not, and the complete suite to build

This is the map of the automated tests for agents and maintainers. It was written on
2026-10-16 from a review of the whole suite (see "Evidence" at the end). "Faithful" in this
project means: same game state, same frames and same sounds as the Windows game under the
pinned profile, for the same inputs. No single test proves that. The suite is a stack of
layers, each catching a different kind of mistake. Today the lower layers are strong and the
upper ones are missing.

## 1. The layers

| Layer | Question it answers | Today | In `make gate`? |
|---|---|---|---|
| L0 static checks | Is the code structured as agreed? (layers, owners, provenance, readability ratchet, judge manifest, profile patches, hygiene) | complete | yes (`make check`) |
| L1 unit tests | Does each module do what its author intended? (see STATUS for counts) | all unit programs registered, including a guest-free graphics CPU/GPU slice and all four guest draw entry points | yes |
| L2 function oracles | Does a native function equal the original bytes on many inputs? (116 Unicorn oracles; see STATUS for verdict counts) | broad but synthetic; 54 of the reports in `build/oracles` date from 10-12 | **no** (`make oracles`, ~10 min); only the 11 initial-state data oracles and the 20 override corpora are rebuilt by the gate |
| L3 mutants and holdouts | Would the oracle notice a realistic porting bug or an overfitted port? | 1–3 hand mutants per module; holdouts for three modules | **no** (`tools/gate/mutate.py`, `tools/analysis/verify_*_holdout.py`, run by hand) |
| L4 override-equivalence | Does the native function behave the same inside the running game? (21 overrides + 5 HLE bridges, 71,115 corpus comparisons) | bound owners only; by frame 600 the collision, task and D3DX overrides have not been called yet | yes |
| L5 whole-run digests | Is the running game deterministic and unchanged? (7,228-frame two-boot determinism, first 600 digests with overrides, 900-frame controller parity, title cadence) | intro/title/menu only | yes |
| L6 journeys | Does the app show the right screens to a player? (`tools/gate/run_journeys.py`) | SEGA → title → character select → Station Square (3D) | yes (`make journeys` after app-smoke) |
| L7 gameplay replays | Does a recorded input reach the same game states? (Emerald Coast, Egg Hornet, speedrun techniques) | **missing** | — |
| L8 Windows parity | Does our run match the real game on Windows, frame by frame? (traces, screenshots, FPU tables) | **missing** (collector exists in `reference/windows-transfer`) | — |
| L9 native-only | Do the native journeys and replays pass with no `sonic.exe`, guest or Unicorn present? | frame control only | partly |

## 2. What the current suite does not catch

These are the weaknesses the review found, most important first. Each has an addition in §3.

1. **No gameplay ground truth.**
   * Every physics or collision test is one function with synthetic inputs. Nothing runs Sonic
     for more than one frame.
   * No fixture comes from a real game state.
   * The 7,228-frame digests cover the logo, movie and title only.
   * So composition, state carried between frames, and call order between systems are
     unverified.
2. **Precision coverage still has limits.**
   * Direct3D now selects 24-bit/nearest unless PRESERVE is requested. The census retains
     pre-device 53-bit startup samples and observes post-device 24-bit game entries.
   * The oracles still sweep 0x007f, 0x027f and 0x037f. Fog, ground acceleration and frame
     control reproduce their full pre-fix reports; this does not settle hardware parity.
   * The corrected census includes Station Square player entries; zero calls elsewhere
     still mean unobserved. It is not a complete gameplay or Windows trace.
3. **Circular floating-point evidence.**
   * Guest FPU shims replace every FPATAN/FSINCOS/FYL2X with the same Bochs models the native
     code uses, and RCPSS with an assumed estimate model. The guest and native agree on these
     by construction.
   * The nine failing oracles and the "native is right" claims rest on those models until Intel
     hardware tables exist (M4).
4. **The flag channel is saturated.**
   * Unicorn raises no SSE flags and differs on x87 flags, so 46 oracles are permanently
     "partial".
   * A real flag bug in native code would be invisible: in ground acceleration all 2,496 cases
     already mismatch on flags.
   * Flags are masked in-game (0x7f), so they rarely matter to gameplay. But the channel
     carries no signal either way.
5. **Stubbed callees and excluded cases hollow out some oracles.**
   * Callees stubbed identically on both sides are not tested. 92 % of the ground-acceleration
     cases hit a stub.
   * "Explicit boundary" cases that are dropped before comparison are not tested either.
6. **Mutation testing is sparse and manual.**
   * Each module has one to three mutants, and the mutant runner is outside the gate.
   * A mutant is "caught" only if it worsens the verdict, so a flag-only mutant can never be
     caught on a partial baseline.
7. **Override-equivalence is not independent.**
   * It replays the same corpora as the oracles.
   * It compares only declared write ranges, so a stray write elsewhere is invisible.
8. **Two-run comparisons hide deliberate changes.** The determinism and frames tests compare
   one run with another. A deterministic behaviour change passes as long as the mode sequence
   holds. Add committed expected digests per journey and replay (§3.2).
9. **Systems with no test at all:**
   * camera (except the CCEF flag);
   * objects and enemies;
   * stage loading as a system;
   * menus and UI logic;
   * save/load and the quick save/load round trip;
   * audio output: `DirectSoundCreate8` returns `DSERR_NODRIVER`, so the game runs silent;
   * FMV drawing, Chao, bosses, events and cutscenes, EHCF, Skipma.
10. **App coverage is incomplete.**
   * `app-smoke` checks one frame (the SEGA logo); offscreen journeys add title and character select.
   * Station Square now draws through the shared device backend. Broader scene/state coverage,
     Windows GPU parity and playable stage performance remain open.
   * Only local optional golden signatures; no Windows screenshot parity, audio checks or performance thresholds.

## 3. The complete suite to build

Each addition names its GOAL §6 item. Build them as part of the item that needs them, not as
separate bookkeeping steps.

### 3.0 The harness: how agents and tests drive the game

Three tools, from lowest to highest level:

* **`sadx-app --guest … --script FILE | --control - [--offscreen]`** (`src/apps/app_harness.hpp`).
  * Commands, optionally scheduled with `@FRAME`:
    * `press KEY [FRAMES]`, `down`/`up KEY`;
    * `pad BUTTONS [FRAMES]`, `stick X Y [FRAMES]`, `rstick`, `triggers`, `release`;
    * `screenshot PATH.png|.ppm` at any frame, any number per run;
    * `state [PATH]`: JSON with game mode, game state, stage, act, rings, timers, and the
      first player's action, status, rotation, position and speed;
    * `pause`, `run`, `step N`, `quit`.
  * `--offscreen` renders into a texture in a plain loop. There is no window, no 63 Hz cap,
    and no stall when the screen is covered or locked. Use it for every automated run.
  * Frame numbers equal the guest's Present count, the same as in `--keys` and `--exit-after`.
    Runs are deterministic.
* **`tools/journey/sadx_driver.py`**: a Python `Session` for step-by-step play. It starts paused,
  and provides `advance(n)`, `press`, `stick`, `pad`, `screenshot`, `state` and
  `run_until(predicate)`.
  * `sadx_driver.py demo` walks title → file select → menus → character select → Station
    Square, with a PNG and a state JSON per screen. On 2026-10-16 it took 178 s; at frame 2,157
    the game was in mode 5, stage 26, with a player.
  * `sadx_driver.py shell` reads the same commands from stdin.
  * SADX takes no mouse input, so the harness has no clicks.
* **`tools/gate/run_journeys.py`**: the pass/fail journey checks, described next.

Agents check visible claims with these: take a screenshot, read the PNG, and read the state.

### 3.1 Journeys (L6): what the player sees (GOAL items 1, 3, 7, 8)

`tools/gate/run_journeys.py` drives `sadx-app` with scripted keys and checks the frame. It
checks the size, colour statistics and textured draw counts. It requires that no draw is
skipped (`not drawn`) and that the guest never stops. It also compares the frame with an
optional local golden signature (`--bless`, kept under `build/journeys/`, never committed).
Journeys are `required` (a failure is a regression) or `open` (a ladder row not reached yet).

* `make journeys` runs one `--offscreen --script` route, capturing required frames 120, 700,
  1,700 and 2,000 as PPM and game-state JSON. R4 also requires mode 5 and stage 26. The full gate runs it after app-smoke, so the two
  app processes cannot race on their shared save overlay. No visible window is needed for
  journeys. `make app-smoke` retains the separate windowed R1 check.
* R3 uses the user's local completed save, `savedata/SonicDX01.snc`, in the pinned reference
  install. Supply your own completed adventure save in its first slot. The report records
  save-file hashes; no save or game image is committed. Wrong/missing saves may stop the
  route at another menu and must be diagnosed from the captured frame/state.
* `--only title,character-select` limits checkpoints, `--bless` refreshes local golden
  signatures, and `--game-dir` / `--system-dir` select equivalent local input directories.
  Thresholds are unchanged from the original windowed runner. Frame properties and a
  locally blessed signature are not Windows GPU parity.
* Add one journey per new ladder row:
  * `world-3d` is required for Station Square after the first renderer slice (item 3);
  * then `emerald-coast-start`;
  * then the native rows `native-sega-logo`, `native-title` and `native-menus`. Each native
    journey compares its frame signature with the reference journey's frame.
* Golden frames (ROADMAP M2): for each journey, a perceptual comparison with a reference
  image. First the image is generated locally from the reference app; later it comes from a
  Windows screenshot (M4).
* App health: no Metal validation errors, frame-time distribution reported. Once R5 exists,
  add a minimum frame rate in a stage.

### 3.2 Gameplay replays and state traces (L7): the core of "faithful" (GOAL items 5, 10)

* **State trace format.** One record per frame:
  * game mode, level/act, timer;
  * RNG state;
  * player position, speed, angles, action and status flags;
  * camera position and target;
  * rings, lives and score;
  * the count of active tasks by type.

  Read it from guest memory in the reference app, and from native state in the native app.
  Write it as a compact binary file with a per-frame hash. Report the first divergent frame
  and field.
* **Replays.** Input scripts recorded once from the app (keyboard and pad), stored as text
  under `tests/replays/`, with checkpoint assertions:
  * `replay-emerald-coast` (title → Sonic → Emerald Coast 1 → goal);
  * `replay-egg-hornet` (including the EHCF path);
  * `replay-station-square` (adventure field).

  Each replay runs twice for determinism and stores its trace digest. A change in a digest
  is a regression unless a deliberate judge change explains it.
* **Technique scripts** (ROADMAP M4 `technique-suite`): spindash chains, light-speed dash,
  Emerald Coast skips and the quick save/load round trip. Each asserts the outcome state.
* **Differential player tests.** Start from states captured during a replay and run native
  Sonic and guest Sonic for N frames under the same input. Compare the trace every frame. This
  replaces synthetic one-frame physics fixtures as the main judge for player systems (N5).
* **Frozen corpora.** Before a system's guest dependency is removed (ROADMAP M5
  `reference-corpus-frozen`), freeze its replay end states and trace digests under `reference/`.
  Store numbers and hashes only.

### 3.3 Make the existing judge stronger (L2–L5)

* **Guest FPU precision** (GOAL item 2): corrected; the census includes Station Square
  and separates pre-device startup from post-device calls. Paired before/after digest
  hashes and unchanged clock traces are recorded in `d3d8-fpu-digest-delta.json` under
  `reference/production-integration-20261016/`. Windows stage control words remain unverified.
* **Real fixtures.** Add to each player oracle a holdout corpus of states captured from the
  replays (L7), alongside the synthetic cases. Report each oracle's branch coverage of the
  original function (Unicorn block coverage), and list the uncovered branches.
* **Stubs.** Report per oracle the fraction of cases that reach a stubbed callee. When the
  callee is ported, run the oracle again with the real callee on both sides.
* **Shadow execution in replays.** During a replay, snapshot before each bound override, run
  the original and the native code, and diff every dirty page, not only the declared ranges.
  Turn mismatches into new oracle cases.
* **Mutation testing in a nightly target** (`make nightly`): automated operator, constant,
  operand-order, precision, rounding and spill mutants per ported module, with a mutation
  score tracked in STATUS. Separate the value channel from the flag channel, so a flag-only
  mutant can be judged on its own.
* **Holdouts in the gate** for every module that has one (the three `verify_*_holdout.py`).
* **One verdict schema.** `verdict.py` classifies by key-name patterns, and each verifier
  defines its own "values match" boolean. Give every report the same fields: values, boundary,
  calls, control word, flags. Then pass/partial/fail means the same thing everywhere.

### 3.4 Windows parity (L8) (GOAL item 13)

* Run the replay input scripts on the Windows machine with the collector, which records the
  same state trace and screenshots at the journey frames.
* Report the first divergent frame. Then shadow-execute that frame to find the function
  responsible.
* Hardware tables: on an Intel CPU, measure FPATAN/FSINCOS/FPTAN/F2XM1/FYL2X and RCPSS/RSQRTSS
  results and flags on the oracle corpora. This settles the nine failing oracles and replaces
  the circular shim evidence.
* Record the FPU control word at stage entries on Windows. This confirms or refutes the 0x007f
  premise directly.

### 3.5 Audio, input and timing

* **Audio events:** a per-frame trace of sound and BGM start/stop/volume calls through the
  DirectSound HLE, compared between runs and later with Windows (M3 `audio-events`). Do not
  compare samples.
* **Input:** the mapping tests against the Input mod tables already exist. Add end-to-end tests:
  a host key or pad event goes in, and the expected game input state comes out on the same
  frame. Cover keyboard and both pad families. Add deadzone tests.
* **Timing:** the 63 FPS limiter cadence exists. Add stage cadence (attract and gameplay
  multipliers) and the Speedrun mod's timer behaviour on quick load.

### 3.6 Native-only (L9) (GOAL items 7–10, M5)

* Every native journey and replay also runs in an environment without `sonic.exe`, the guest
  library or Unicorn. A link check proves they are absent from the native binary.
* Each native result is compared with the reference result from the same script (frame
  signature, state trace), not only with fixed numbers.

### 3.7 Quick fixes (fold into GOAL item 1)

Implemented in GOAL item 1: all four formerly unregistered unit programs run in the gate;
`game_reset_composed` has a tracked 300-case verdict; corpus files depend on native probes,
which depend on the native archives; each unit invocation uses `/usr/bin/time -p` and writes
its wall time immediately after the result. Python unittest and the journey report also
record durations. These checks retain their existing tolerances and expected values.

## 4. Rules for agents

* A step adds the tests its change needs (GOAL §7 step 4). A journey step adds or promotes a
  journey. A system step adds its oracle *and* registers its states in the replay trace.
* Prefer tests that exercise the game as it runs (journeys, replays, state traces) over more
  single-function synthetic fixtures. They are what tells us the game is faithful.
* A test that only checks "it ran without stopping" is a smoke test. Name it so, and never
  report it as proof of behaviour.
* Never weaken an existing test to pass (AGENTS.md "Never game the judge"). A new test may start
  as `open` (expected to fail) when it describes a ladder row not reached yet. It is promoted
  to required when it passes, in the same step.
* Game imagery and captured game data stay local. Commit hashes, signatures, scripts and
  numbers only.

## 5. Running the suite

```sh
make -j10 gate                              # L0, L1, L4, L5, app-smoke and L6 journeys
make journeys                              # L6 journeys alone, one offscreen route
python3 tools/gate/run_journeys.py --list
make -j6 oracles && make oracles-verdict    # L2 (about 10 min)
make m1                                     # the M1 table
```

## Evidence

* The test inventory and per-milestone coverage come from a review of the Makefile,
  `tests/unit/manifest.tsv`, `tests/probes`, `tools/oracles` and `tools/gate`.
* The oracle quality findings come from a sample review of 11 judges: pass (`speedrun_limiter`,
  `player_hold`, `game_logic`), partial (`ground_acceleration`, `fog`, `player_movement`,
  `sonic_player`), fail (`floor_angles`, `player_reaction`), and override entries 419030 and
  7815c0.
* The journey results come from the runs recorded in `docs/REVIEW-2026-10-16.md`.

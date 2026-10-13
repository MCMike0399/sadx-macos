# ADR-0004: Layered `src/` with an enforced dependency order, one slug per module, discovered oracles

Date: 2026-10-12. Status: accepted.

## Context

After the repository move (ADR-0003) the areas `cpu d3dx ninja game player input runtime
platform guest` contained include cycles (`ninja`↔`game`, `game`↔`player`, `ninja`→`player`,
`platform`↔`runtime`), so "where does this go" had no answer an agent could check. Oracles
carried three different names each (make target `reference-producer-test`, script
`verify_original_player_input_producer.py`, report `original-input-producer-comparison.json`),
nine oracle scripts were not wired into the build at all, and the pass/fail shape of the 90
reports was read ad hoc in two places. Twenty oracle scripts also still pointed at pre-move
paths, which nothing detected because the gate does not run oracles.

## Decision

1. `src/` is a stack of layers, lowest first: `cpu loader math engine input player game
   runtime guest hle platform apps`. A file includes only its own layer or lower.
   `tools/gate/check_layers.py` runs in `make check`; known debt is listed in
   `tools/gate/layer_exceptions.tsv` and a stale exception is an error.
2. The shared vocabulary that caused the cycles moved down: PE image and heap to `loader`;
   `ninja_math`/`d3dx_math` to `math`; the task scheduler, controller records, game-logic
   state/result types, frame control and the speedrun limiter state to `engine` with the
   rest of the Ninja layer; the scene input producer up to `player`; executables to `apps`.
3. One slug per module across code, unit test, probe, oracle script, make target and report
   (`docs/ARCHITECTURE.md` "One name per module"). Oracle targets are `oracle-<slug>` and
   reports `build/oracles/<slug>.json`; the tracked copies live in
   `reference/oracle-reports/<slug>.json`.
4. Oracles are discovered from `tools/oracles/verify_original_*.py` by
   `tools/gate/gen_make.py`; `tools/oracles/manifest.tsv` lists only recipes that need other
   inputs. A manifest row without a script is an error.
5. `tools/oracles/verdict.py` is the only reader of report shapes and yields `pass`,
   `partial` (values exact, FP status/exception flags differ, documented in `limits`) or
   `fail`. `docs/STATUS.md` reports the three buckets separately; partial is not pass.
6. `tools/` is grouped by purpose: `lib/` (importable), `gate/`, `oracles/`, `analysis/`,
   `emit/`. Scripts are run through the Makefile's `$(PY)` / `$(REFERENCE_PYTHON)` or add
   `tools/lib` to `sys.path` themselves.

## Consequences

* Adding a module or an oracle needs no registration beyond the files themselves (plus a
  manifest line when a unit test takes arguments or an oracle needs extra inputs).
* Three upward dependencies remain as listed debt (matrix stack owned by `PlayerCoordinates`,
  child `PlayerTaskState` created in `task.cpp`). Fixing them changes ported code and needs
  the affected oracles rerun with mutants.
* Historical names in `docs/HANDOFF.md`, gate records and old reports are left as written.
* The gate still does not run the oracles (minutes each); `make oracles` + `make
  oracles-verdict` is the separate step, and the next agent prompt makes it part of every
  session's close.

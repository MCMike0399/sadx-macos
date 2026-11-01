# Independent bounded judge review

Candidate `8c45eed`, diff `82961ec..8c45eed`, including observer commits
`63afe54`/`c6eec3e`; immutable cohort `0026c8a`/`8ce3f10`.
Verdict: **request changes before reserved continuation**. No Source rewrite requested.

1. **P1 — Continuation does not seal all consumed dependencies.**
   `tools/oracles/animation_judges/consumer_continuation.py:59–92` verifies the
   runner definitions and libraries, but omits `freeze['inputs']` and the extracted
   Python helpers loaded by `run.modules()`. The frozen runner's
   `modules()` imports from `build/animation-final-cohorts/current/tools/`;
   its freeze definitions cover only the nine runner files, not those helpers.
   In particular `verify_original_sonic_player.py`, `verify_original_player_startup.py`,
   `verify_original_math.py`, `pe_image.py` and `extract.py` are unchecked.
   Gamma reads three imported content files whose frozen hashes are also unchecked.
   Reproducer: enumerate the paths validated by preflight; none includes these
   helpers or imported assets. Changed bytes could therefore reach observation
   despite unchanged checked definitions/libraries. Current helper bytes match
   `38e1c69` and current assets match the original freeze; no present drift alleged.
   Bind and verify the complete imported helper/content closure before importing
   or executing it, either in the runner or a required sealed authorization.

2. **P2 — Instruction-budget exhaustion is counted as original return.**
   `consumer_fault_observer.py:73–76` labels any zero boundary `returned`.
   Frozen `verify_original_sonic_player.py:48–59` uses
   `emu_start(..., count=5000)` and returns boundary zero without checking EIP
   against `STOP`. Unicorn count exhaustion need not raise an exception.
   Pure fake-helper reproduction returning `({}, 0)` without reaching STOP yields
   `original_outcome='returned'`; continuation then records a source-returned ID.
   Require actual STOP-PC evidence for return, validate established stop PCs,
   and fail closed for instruction-budget/unclassified stops. Add a pure regression.

Bounded evidence accepted: seven focused Python tests pass. Recomputed comparison
of retained `OBSERVER-DIAGNOSTIC.json` against normalized `NATIVE-BOUNDARY.json`
confirms read-unmapped PC `0x49aa50`, address 6, size 2, EBP 0; mapped actor-current,
INFO/mode projection; actual written-null publication; matching prefix, all 40
native words and return; no FP difference. No serializer address-6 read, zero-page
mapping or continuation appears in the observer.

All revised definition hashes, 26 retained positive artifacts, nine final runner
definitions, ten numeric libraries and eight custom libraries match. All 660
Source files remain unchanged (`d82d0ac:src == 8c45eed:src`). Positive tests exercise
real Sonic updater/startup routing, native precedence, missing runtime/timer,
canonical attachment and earlier failure; real Gamma Context/PInitialize owners
share one imported table with isolated state and failed-binding preservation.
Four compiled faults remain retained and caught. Existing `native_gamma_binding`
calls `gamma::run`; ROOT-PRODUCT-CLOSURE references its receipt. New binding coverage
is bounded ownership evidence.

Original exit 1 and prior records `[0 completed, 1 exception]` remain retained.
No reserved execution, emulator, build, mutation, holdout, gate, journey or push
was performed. IDs 1–1023 still require fresh acceptance and distinct committed
Root authorization. PSetMotion, playback, destruction, N3b, protection union,
registered build and final integration gate remain open.

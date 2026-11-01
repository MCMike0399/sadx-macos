# Independent origin-seal correction recheck

**Verdict: the three reported P2 corrections are closed within this bounded
tool review. No remaining actionable defect found.** This is not execution
admission or Task, Source, campaign or integration approval.

Reviewed candidate `3f2e1cb` at metadata HEAD
`ee5e46ac8011392988fd8fbd18825b1c9c32a250`. Read the complete 453-line
`tools/oracles/task_judges/reserved_run.py` and complete 366-line
`tests/python/test_task_reserved_run.py`, the prior
`RECHECK-EXECUTION-GUARDS.md` report (`d26096a`), and the complete
`IMPORT-TOOLCHAIN-CORRECTIONS.json`. The two reviewed files are unchanged
between candidate and metadata HEAD; their SHA-256 identities match that JSON:

* Runner: `d3c51ca3580fec36ca9e56da195dad3ba8ba6f14238b47cd3506a7e89d5b1877`.
* Tests: `1c7ad602d80b57749f0f9dea8672ef29a8709eb12745f03cb6163fd02ffe38d2`.

## Correction dispositions

1. **Compiler symlink round trip: closed.** At runner lines 133–137,
   `root_inputs()` records lexical absolute filenames with their own digest,
   then separately records resolved target bytes for symlinks. Thus the
   `clang++` key carries its link-text identity while the `clang` key carries
   binary identity. `verify_freeze()` hashes each corresponding filename using
   the same rule. The complete fake round trip exercises the real pin collector
   and verifier, including a `clang++ -> clang` link, fake package, PE,
   toolchain and committed-freeze retrieval. It passes unchanged. The actual
   build retains the C++ driver spelling. This directly crosses the boundary
   the earlier driver-spelling-only test missed.

2. **Selected toolchain versus pins: closed.** Preparation now obtains one
   selection and passes that same object into `root_inputs()` and the freeze
   record (lines 161–167). Verification rediscovers the selection once,
   requires complete dictionary equality, checks every stored digest, then
   requires equality with the entire recomputed input map (177–184).
   The build consumes the checked frozen selection. Compiler, linker, LTO,
   SDK, resource, C++ and ordered include-search metadata substitutions are
   refused by the focused regression. Existing tree tests cover changed
   headers, library content and link targets; extra dependency refusal is
   retained. Explicit build arguments and sanitized build environment remain.
   These findings do not certify the complete actual compiler/runtime closure;
   that frozen closure remains Root's separate responsibility.

3. **Startup and ordinary import origins: closed for the reported paths.**
   Entry requires all three flags, and generated child/comparator argv includes
   `-I -S -B` (408–416, 430–432). The actual temporary-venv `.pth` regression
   demonstrates poisoning without `-S` and sealed fake-module import with it.
   The child checks cached project/transitive and Unicorn origins before
   importing helpers, exposes the world, Root helper and pinned package search
   roots explicitly, imports each original, and checks origins before invoking
   generation or observation (359–389). The observer's subsequent ordinary
   import therefore reuses the checked original. Archived project caches are
   refused as extra files; Root source helpers execute checked source bytes.

   Unicorn import temporarily guards its actual `ctypes.cdll.LoadLibrary`
   path, permits only the resolved pinned package dylib, and checks the loaded
   handle's `_name` and package version `2.1.4`. Restoration is in `finally`.
   Read-only inspection of the installed pinned package confirms this is the
   loader API its binding uses. Its version string is binding metadata, not an
   independently queried native version; binary identity comes from the
   frozen package bytes. No actual Unicorn import or dylib load was performed.
   The reference venv configuration identifies Python 3.14.7, so the older
   Python `-S`/venv-prefix compatibility issue is not this pinned setup.

## Complete-file Standards / purpose-specific Spec

The axes were assessed separately using AGENTS, GOAL, STYLE (including §12),
JUDGE, the task brief and ROOT-RESERVED-PROTOCOL. No further agents were used.

| File | Standards disposition | Purpose-specific Spec disposition |
|---|---|---|
| Complete `reserved_run.py` | Accept reference orchestration: semantic helpers, structured control flow, explicit refusals and retained evidence. Game-tier address restrictions do not prohibit this reference tooling. | Accept the three bounded corrections. Freeze verification, immutable archives, terminal/single-use markers and parent/stage/pipe checks remain. Actual frozen closure and execution qualification are outstanding. |
| Complete `test_task_reserved_run.py` | Accept maintained isolated fakes and bounded Python subprocess tests. No game source or expected oracle result was changed. | Adequate regressions for the reported failures. Complete fake freeze now crosses collection/verification; startup test executes real Python startup; loader tests prevent foreign loading. Negative identity/restoration cases received additional independent fake checks below. |
| `reserved_observer.py` and its test module | Prior complete-file disposition retained; Git comparison against `d26096a` confirms both unchanged. No new complete-file observer review claimed here. | Accepted constructor/direct correction scope retained. Its ordinary original import is now covered by the runner's pre-behavior seal. |

## Verification and retained limits

Executed `python3 -B -m unittest discover -s tests/python -p 'test_task_*' -v`:
**67 tests, exit 0, no skips**. Existing constructor/direct fault attribution,
recorder adequacy, stage refusal and tiny Python EOF-exit-125 tests remain passing.

An additional inline fake-only check executed the real origin and loader guard
helpers: **11 checks passed**. It refused foreign original, PE helper,
missing-origin extract, Unicorn package and nested binding origins; accepted
three legitimate origins; rejected wrong binding version and wrong loaded path;
and verified loader restoration on identity and import failures. All native
loader calls were mocked. No protected files were edited for these checks.

No actual prepare, freeze, generation, reserved child command, admitted run,
compiler, build, original/native/emulator/mutation/provider/network execution,
manifest/inventory refresh, Main/Source edit, gate, child agent or push occurred.
Temporary Python startup/EOF subprocesses exercised fakes only. Root's separate
input-free archive/eight-probe preflight was not replayed or promoted to runtime
evidence. Source acceptance `0e92356` remains separate with inherited partials.
Reserved seeds 20261231, 20270101, 20270102 and 20270103 remain UNGENERATED.

Only this report is committed. Root retains the actual full closure freeze,
distinct admission, actual qualification, Main integration, source-only build,
journeys and coherent gate. No final-execution approval follows from this report.

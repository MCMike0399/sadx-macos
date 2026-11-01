# Independent final Task runner review

**Verdict: changes required before execution freeze/admission.** Reviewed candidate
`dc4367f4fda0d29843842e8ceb2adf6ef14e9ee7` at metadata HEAD
`d43263c21ed01d2dcc189a8e51e897148139fd61`; the four complete requested files
are unchanged between them. Source review `0e92356` supplies no tool approval.

## Prioritized findings

**P1 — Frozen Python source does not bind executed cached code.**
`reserved_run.py:42–57,183–184,217–220` hashes Root `*.py`, then imports
`reserved_observer` and `reserved_inputs` normally. Root bytecode is neither
hashed nor refused. `-B` prevents cache writes, but still reads timestamp-valid
bytecode. Consequently an unchanged, correctly hashed helper can execute stale
code. Archive checking excludes extra caches inside each world, but does not
check Root's import directories.

Executed temporary-file reproducer: write `VALUE = 'stale'`, compile its cache
with `py_compile.compile`, replace source with equal-length `VALUE = 'fresh'`,
restore its original nanosecond mtime, then import with bytecode writes disabled.
The source SHA remains correct; imported VALUE is `stale`. No project helper was
modified. Load sealed helpers from verified source without accepting unverified
caches, or bind/refuse every executable cache. Add a stale-cache refusal test and
check actual module origin/identity, rather than relying solely on search order.

**P2 — Compiler/SDK closure is not frozen or isolated.**
`reserved_run.py:52–54,99–103,142–154` pins the clang executable and
`SDKSettings.json`, but neither SDK headers/libraries nor compiler resource
headers/linker. Builds use a new `xcrun clang++` resolution without an explicit
frozen SDK; the inherited environment drops only PYTHONPATH. CPATH,
CPLUS_INCLUDE_PATH, SDKROOT, LIBRARY_PATH and related settings can change the
compiled world outside its checked archive. Generated dependency/link maps are
never checked against an allowed closure.

Executed fake-only reproducer: supply temporary compiler/SDK/package/PE files
and fake xcrun answers to `root_inputs`; change an SDK header, leaving settings
unchanged. Both pin dictionaries are identical and omit the changed header.
Mock subprocess.run in `command` with CPATH and SDKROOT set: both reach the
build environment unchanged. No compiler ran. Freeze and enforce the actual
compiler/SDK/dependency closure and sanitized environment; add refusal tests.
The direct two-CPP recipes avoid Make/downloads, but do not themselves prove
that external build inputs match the freeze.

**P2 — Internal entry points can continue a terminated run.**
`reserved_run.py:157–180,231–236,250–254` checks the freeze, permanent marker,
library hashes and input hashes, but never requires a live admitted parent,
the expected next stage, or absence of terminal RUN.json. The marker contains
only a freeze hash. CLI `_generate`/`_observe` do not require `--admitted`.

Static reproducer, deliberately NOT executed: after generation succeeds and an
old observation fails, the parent writes RUN.json and exits. Invoke `_observe`
for a previously unstarted current world/slug. Its original marker, BUILD and
INPUTS files still satisfy every guard; its started filename is unused, so it
can consume reserved inputs after termination. The terminal receipt prevents no
continuation. This does not turn the failed parent receipt into success, but
breaks execution admission and failure finality. Bind each child to the live
parent's exact stage/identity and reject terminal runs; test with fake dispatch
only. Existing exclusive filenames correctly prevent same-stage retries.

**P2 — Constructor failures corrupt source-attempt attribution.**
`reserved_observer.py:107–121,215–220` creates the event only after the original
constructor returns. A first constructor failure records no setup attempt; a
later failure overwrites the preceding event with its error and previous
machine's PC. The failing sequence remains classified as unattempted without
an explicit setup-error record.

Executed fake reproducer: inside `audit_helpers('task_work', ..., two_cases)`
call a constructor that returns a fake machine once, then raises
`RuntimeError('synthetic constructor setup failure')`. Only case 0 remains,
marked unexpected_error at its old PC; no case 1 setup attempt exists. Constructor
restoration succeeds and the error propagates. Record setup attempts separately
before construction, with unavailable PC explicitly absent; preserve the earlier
event. Add first/subsequent constructor and hook-installation failure tests.

## Prior P2 and complete-file dispositions

**Prior direct lifecycle P2: closed within its bounded purpose.** The hook is
attached after Original construction and remains through helper/direct execution.
Both direct phases retain entry, requested stop, actual/error PC and exact invalid
access signatures; faults return false and propagate. Call/constructor/emu_start
and installed fault hooks restore. `0x401000` remains an established stop, not a
source return. This does not establish precise constructor/setup accounting above.

Standards use STYLE's reference-tier allowance, structured control flow, semantic
helpers and explicit unknowns; Spec uses the request and ROOT-RESERVED-PROTOCOL.

| Complete file | Standards | Purpose-specific Spec |
|---|---|---|
| `reserved_observer.py` (456 lines) | Accept reference-tier structure; setup attribution needs explicit unknowns. | Direct correction accepted; constructor accounting requires correction. |
| `reserved_run.py` (257 lines) | Accept bounded orchestration and separate immutable worlds; no unrelated abstraction. | Not adequate for final admission: findings above. |
| `test_task_reserved_observer.py` (406 lines) | Accept readable fakes and real-recorder integration. | Meaningful bypass/input/extent/fault regressions; missing constructor/setup refusals. |
| `test_task_reserved_run.py` (103 lines) | Accept isolated, fast refusal tests. | Seven tests cover top-level refusals, source hash/set changes, archive edits/extras and exclusive writes; none exercises child-stage/terminal guards, stale imports or toolchain closure. |

Retained generator/comparator contracts remain accepted for their bounded purposes:
owned releases, child allocation indices, fixed-row labels, complete ordered
N/2N+2/N/N-minus-exclusions ABI binding and exhaustive byte comparison. Frozen
definitions imply 8/8/556/570 input cases; lifecycle adds its repeated control,
and delete completions depend on exact observed exclusions. Source returns,
established stops and excluded projections remain distinct. No cardinality alone
proves fidelity. Missing/changed archive members, library/input hash changes and
nonzero child exits fail closed along the normal parent path; no automatic retry
is present. Separate archived scripts/probes and both engine CPPs are selected
statically; actual compilation and dependency closure remain unexecuted.

## Checks and limits

All **52 fast tests pass**, exit 0, using
`PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests/python -p 'test_task_*' -v`.
Additional checks above used only fakes and temporary files. Read-only inspection
of Main's retained direct-development evidence matches both declared SHA256s:
normal observation `fdb8ef22…c2b17`, synthetic faults `7316a3a9…62cd7e`.
Normal receipt is validated/exit 0; synthetic signatures retain each direct PC,
access 19, address 6 and width 2. These are development observations, not pristine
final holdouts or independently replayed proof.

No prepare/run/child command, build, original/native/emulator execution, reserved
generation, mutation, provider/network action, child agent, protected edit,
manifest/inventory refresh, gate or push occurred. Only this report is committed.
Inherited Source72 FP partials/exclusions stay unchanged. Root owns final freeze,
distinct admission, reserved qualification, Main integration, source-only closure,
journeys and gate. No Source, gameplay or campaign approval follows.

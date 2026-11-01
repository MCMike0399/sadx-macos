# Independent reserved Task helper review

Pinned Main: `7924190c4368a4978c307bb022f1a1aadabaa9dd`.
**Verdict: bounded generator/fixture-adapter acceptance; complete qualification blocked.**
All six named files were read completely. Neither Source approval nor the
`1372ef2` gate establishes their adequacy. Development receipt hashes match.

## Prioritized findings and reproducers

**P1 — expected execution is not bound to captured records.**
`reserved_observer.py:75–98,119–132` validates report counts, then returns the
oracle exit without inspecting the capture. `compare_streams.py:17–47` validates
only each recorder-created ledger. Neither proves the independent expected call
count, ordered case-to-symbol mapping, input bytes or required buffer extents.
The completeness docstring at observer line 76 consequently overstates its check.

Verified fake-only reproducer: inject a fake display module through `sys.modules`
and replace `ctypes.CDLL` with a Python library. Its main consumes fixtures once,
processes `[[1],[2]]`, writes `cases=2, source_outcomes={'0':2}`, and returns zero.
Invoke the wrapped Python function for case 0 and its unwrapped callable for
case 1, simulating a recorder bypass. Call `observe()` twice in fresh directories
with seed 7, two fresh labels and dummy executable/library paths. The **real
freeze.py** writes both ledgers/streams: each has one call and an 8,192-byte
output. Both observer exits are **0**; comparator exit is **0**, records **1**,
no mismatches. No stream or receipt was rewritten. Thus a plausible common
capture omission passes independently consistent evidence; this is stronger than
the accepted jointly-rewritten-stream trust limitation. It does not demonstrate
an omission in the unchanged original judges. Initial fake setup raised TypeError;
its constructor was corrected before this successful reproduction.

Before qualification, bind captured records to independently derived expectations:
work N calls; lifecycle 2N+2, including callback traces and the retained inactive
experiment; display N; delete N minus exact exclusions. Verify ordered symbols,
input identities, scalar lengths and complete arena/trace/output extents. Add
fake regressions for omitted/bypassed calls and missing/shrunken buffers.

**P2 — observer tests miss the acceptance boundary.**
`test_task_reserved_observer.py` never calls `observe()` or combines it with the
real recorder/comparator. Its eight tests therefore cannot detect P1. Preserve
the current tests and add the fake integration reproducer before use.

## Complete-file Standards / Spec dispositions

Paths below are relative to `tools/oracles/task_judges/` and `tests/python/`.
Standards were assessed directly against STYLE's reference-tier allowance,
semantic naming, structured control flow, maintained scope and truthful limits;
game-tier address/provenance rules do not require rewriting these reference tools.

| Complete file | Standards / Spec disposition |
|---|---|
| `reserved_inputs.py` | Accept bounded pure generation: local RNG, explicit bounds, owned frees, child allocation indices and top-level destruction bookkeeping; no framework or import-time generation. |
| `test_task_reserved_inputs.py` | Accept bounded domain/reproducibility checks; does not prove execution or source outcomes. |
| `reserved_observer.py` | Structure acceptable; correct completeness wording. Fixture substitution only accepted; qualification blocked by P1. Deep copies and finally-restoration are appropriate. |
| `test_task_reserved_observer.py` | Readable focused fakes; insufficient qualification coverage, P2. |
| `compare_streams.py` | Accept existing bounded ledger/equality purpose; exhaustion, strict Base64, hashes, extents and whole-record comparison remain intact. P1 requires an independent expected contract. |
| `test_task_stream_completeness.py` | Accept all 15 regressions, including six asymmetric additions `1d0e0e0`; retains accepted review `791883c`. |

## Verification and limits

Pinned Main Python, `PYTHONPATH=tools/lib:tools/oracles`: all **27 tests pass**.
Additional fake checks preserve exit 1/SystemExit 2, reject double fixture supply,
and restore identities. Static dependency inspection confirms display's trailing
11×4 fixed rows, delete's leading 9×2×3+4 rows, truthful delete CLI seed/count,
and unchanged exclusion reasons. Reversed all-excluded IDs with wrong seed and
no boundary evidence pass `validate_report`; text/counts alone attest no outcome.

Required, already-declared future work remains: actual per-case attempts,
STOP/return/fault/exclusion evidence and development characterization. Arena/lifecycle
shared calls check stack restoration, not STOP; display/delete check normal STOP
but lack emitted per-case boundary evidence. This is a pre-execution requirement,
not a newly introduced parent-runner bug. Runner, compiled recipes, committed
closure seal and single-use admission remain pending. Only synthetic permitted
seeds were used; reserved inputs remain ungenerated. No builds, original/native/
emulator/runtime execution, network, delegates, protected edits, refresh or push.
No Task, inventory or campaign closure; no reserved execution authorization.

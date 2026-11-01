# Bounded post-failure stage review

Verdict: ACCEPT a8e3b0f's stage-identity correction only. No blocking finding
against this two-protected-file patch. No continuation admission, execution
approval, Task qualification, campaign qualification or full-run qualification.
Reviewed at metadata HEAD 1dab2ed; no children or protected edits.

## Standards and spec

The complete protected diff is the three-line `name` → `library_name` loop
rename in `reserved_run.py` and one 41-line regression in
`test_task_reserved_run.py`. The manifest changes exactly these two protected
hashes. Source, observer, generator and comparator are unchanged since 5487937;
no values, expectations, tolerances or exclusions change. The semantic name
clarifies the loop's role without adding policy. Both review axes pass this
bounded correction.

Complete dispatch review: exact stage admission precedes freeze verification,
permanent-marker binding, complete eight-library set/hash validation, and
(for observation) four-cohort input set/hash validation. Exclusive stage-marker
creation precedes archive verification, module-origin checks, sealed search
paths/Unicorn loading, checked-source helper loading, and world-specific imports.
Generation then writes inputs; observation then reads the selected bundle and
calls `observe()`. The rename preserves this order and all guards. Comprehension
variables do not leak in Python 3; the ordinary library loop did.

## Failure and accounting

Read-only evidence came from `/Users/burbujamc/Developer/sadx-macos/build/`.
`task-final-reserved-run.exit` is 1. RUN has no completed slug; all four remain
uncompleted. The first old-task_work traceback fails at exclusive marker
creation, before observer imports/call. BUILD's final library key is the absolute
current task_work library path. Generation therefore wrote
`current/libraries/task_work.dylib.started.json` with mode `generate`; observation
collided with it. An absolute right operand also discards OUT during path joining.

All four input hashes match both INPUTS.json and RESERVED-ATTEMPT-1-FAILURE.json:
8 work, 8 lifecycle, 556 display, 570 delete = 1,142 generated rows. Labels retain
1,040 fresh and 102 repeated-fixed rows (44 display, 58 delete). All eight compiled
library hashes match BUILD. Permanent marker, failed RUN, first-child log and
wrong-marker hashes match the failure receipt; neither world has a captures
directory. These facts plus serial fail-fast dispatch establish zero reserved
Source case attempts and zero native case calls, not zero setup/build activity.
The original freeze/admission bytes remain identical to a5e14af/5487937; the raw
pin manifest hash also matches.

## Tests and limits

The changed test constructs eight fake libraries, four single-row fake input
bundles and real hash manifests; calls real `child('observe','old','task_work',...)`;
raises a sentinel from the fake observer; asserts the exact old-task_work marker
and absence of library-suffixed markers. Admission, freeze/archive/origin checks,
Unicorn, helper loading and original imports are mocked. This is dispatch
coverage, not independent validation of those guards.

Reproducer: load the current test module with bytecode disabled and invoke
`test_real_child_dispatch_keeps_stage_name_after_library_validation`. It passes.
Bind its RUN to the committed pre-fix runner (`git show a8e3b0f^:tools/oracles/task_judges/reserved_run.py`)
in memory and repeat: exactly one assertion failure, missing exact stage marker;
zero errors. No protected file was altered. Six selected fake tests pass: this
regression, terminal/missing-parent refusal, parent/stage/pipe identity, no-admission
refusal, existing-output refusal, exclusive receipt writing.

The preserved hash-matching log reports 69 tests/OK: inputs 4, observer 28,
runner 22, stream completeness 15. I did not rerun that suite. Coverage concern:
the added test does not exercise generation followed by observation, other
worlds/slugs, repeated dispatch, marker payload, or observer argument fidelity.
Nonblocking for this exact rename; insufficient evidence for a continuation.

## NEXT contract, not admission

Existing RUN terminal refusal and original output must stay intact. The corrected
runner differs from the original frozen dependency pin, so the original seal
cannot authorize it. Root must separately freeze and review any continuation
mechanism before explicit admission: bind the failed receipt and exact ordered
input bytes/labels/IDs, unchanged models/archives and dependency closure; permit
only proven unattempted work, with no generation, rebuild, erasure or historical
re-execution. Use separate exclusive continuation receipts/captures, preserve
parent-lifetime and terminal guards, and retain complete attempt/outcome accounting.
Unexpected failure stops permanently. Original finalrun96598 remains TERMINAL
EXIT1 forever; a later receipt cannot convert it to pass.

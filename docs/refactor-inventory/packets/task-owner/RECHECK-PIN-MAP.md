# Bounded pin-map storage recheck

**Verdict: accept the storage-format change; no actionable Standards or
purpose-specific Spec finding. No final-execution approval.**

Reviewed exact two-point diff `3f2e1cb..ee47cacca38e83c86866f3a0de68102e0b97af5d`
at metadata HEAD `225880834fa36ac03f7617e84d2967cb22144a9a`.
Both executable files are byte-identical between candidate and metadata HEAD.
Read the complete 473-line `tools/oracles/task_judges/reserved_run.py` and
391-line `tests/python/test_task_reserved_run.py`, prior
`RECHECK-ORIGIN-SEAL.md`, protocol and `PIN-MANIFEST-STORAGE.json`.

## Complete changed-function and test scope

Runner: new `write_pin_manifest` and `frozen_inputs`; complete affected
`prepare` and `verify_freeze`; new fixed `PIN_DATA` constant. Traced unchanged
`write_new`, `digest`, `root_inputs`, toolchain selection and admission/child
consumers through the complete file.

Tests: complete changed
`test_complete_unchanged_symlink_freeze_round_trip_and_selection_refusal`,
`test_changed_dependency_refused_before_imports`,
`test_changed_dependency_set_refused`, and new
`test_raw_pin_map_hash_count_and_exclusive_creation`. All 20 prior runner test
methods remain; one is added. The first test exercises the real raw-map writer,
reader, collector and verifier with the fake compiler symlink. The two focused
dependency tests mock only the new storage boundary and retain their assertions.

Also inspected every remaining diff: prior review document, storage evidence,
inventory metadata and judge manifest. Manifest changes bind exactly the two
changed executable files and the storage evidence; all three hashes reproduce.
Metadata does not supply execution admission or source acceptance.

## Standards

Accept bounded reference tooling under AGENTS, GOAL, STYLE and JUDGE: small named
helpers, explicit failures, exclusive creation and unchanged source/behavior
judges. No game implementation, tolerance, expected result or mutation policy
changed. The fixed raw-map location is ignored by Git. This review makes no
broader inventory-conformance claim.

## Purpose-specific Spec

The writer serializes the entire dictionary without filtering, aggregation or
class substitution; the committed small freeze binds its path, entry count and
exact raw-file SHA-256. Existing output cannot be overwritten. The reader checks
the fixed location, refuses a symlink at that location, checks raw bytes before
parsing, and requires a dictionary of the recorded cardinality.

Verification still checks every stored dependency digest and then complete
`root_inputs(selected)` dictionary equality. Thus a stale dependency fails even
with unchanged raw bytes; omissions, extras and substitutions fail even with
self-consistent manifest hashes/counts. Verification never rewrites pins from
new outputs. Raw proof is outside the collected input trees. Committed-freeze,
selected-toolchain, distinct-admission, single-use and child guards remain intact.

## Verification and limits

`python3 -B -m unittest discover -s tests/python -p 'test_task_*' -q`:
**68 tests, exit 0, no skips**. Additional inline temporary-fake checks:
**19 passed**, covering lossless round trips, exclusive creation, relocated and
traversal paths, same-content symlink, corrupt/substituted bytes, missing map,
wrong count, non-map payload, stale dependency, omission, same-cardinality
substitution, extra pin and newly collected dependency.

Read Main's actual `build/task-pin-closure-preflight.json`: it records 42,329
entries and unchanged reproduction of every hash. Its count, toolchain and map
identity agree with committed storage evidence. This review did not repeat that
preflight or independently hash its production dependencies. Actual freeze,
production raw-map and reserved-output paths were absent on read-only inspection.

No actual prepare/run/generation/observation/build, original/native/emulator,
mutation/provider/network execution, reserved seed generation, protected/Main/
Source edit, global refresh, children, gate or push. Only this report is committed.
Prior origin-seal acceptance and separate Source72 reviews retain their limits.
Root owns the actual committed freeze, distinct admission, execution, Task/Main
integration, source-only qualification, journeys and coherent gate.

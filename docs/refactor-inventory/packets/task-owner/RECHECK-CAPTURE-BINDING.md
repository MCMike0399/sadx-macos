# Independent capture-binding recheck

Reviewed candidate `4b73978` on Main snapshot `b7fbbec`, base `7924190`,
against review `5967027` imported by `d7fef10` and
`CAPTURE-BINDING-CORRECTION.json`. Both correction file hashes match.
**Verdict: P1 and P2 closed for the bounded helper purpose; no new concrete
blocking defect found. No final execution or Source fidelity approval.**

## Findings and reproductions

**P1 — closed.** The observer derives work N, lifecycle 2N+2, display N,
and delete N minus individually validated exclusions independently of the
recorder. It checks ordered symbols, integer returns, scalar arguments, exact
before/after buffer keysets and byte extents, and both phases' input-word hashes.
The unchanged `validated_records` still verifies the compressed digest, complete
ordered ledger, strict Base64 and every recorded byte/hash; the iterator is
exhausted before acceptance. No equality field or failure status was masked.

Independent fake-only reproduction used the maintained
`TaskReservedObserver.observe_fake_display` with the prior observer loaded
in memory from `git show d7fef10:tools/oracles/task_judges/reserved_observer.py`
and its repository `__file__` retained for real recorder resolution.
The complete mode and all four adversarial modes returned 0 and passed the real
comparator. With the current observer, bypassing call 2 rejects with count 1/2;
2,047-word output and pointer-hidden output reject on extents; wrong second
input rejects on input identity. Neither capture nor ledger was rewritten.

**P2 — closed.** All sixteen test bodies and their helper were inspected.
The five integration bodies actually call `observe()` with real `freeze.py`,
a fake original module and a Python ctypes library replacement. Positive
evidence reaches the real comparator; the four rejection assertions fail with
the prior observer. Contract tests cover work extents, lifecycle callback/control
ordering and precise delete omission. Existing restoration/report tests remain.

## ABI and complete-file dispositions

Static checks against all four unchanged judge callers and probe exports confirm:
arena 16,777,248 bytes; work trace 3N words; lifecycle trace 3N+11;
callback buffer 4C+16 words, returning 4C; repeated inactive command
`[1,2,7,0,3,0,0,0]`, 17-word trace, then four-word boundary output;
display/delete 2,048-word outputs. Primary success returns remain 0/0/1/1.
Delete skips only the report's exact validated case IDs.

Read-only tracked historical ledgers retain compatible counts 16/40/236/244,
symbols, returns, buffer extents and matching before/after input hashes.
Raw case blobs/streams are absent here: no fresh historical byte replay or
independent regeneration was performed. Scalar/order expectations were checked
against caller code, not inferred from ledger summaries.

| Complete file | Standards | Spec |
|---|---|---|
| `reserved_observer.py` | Accept reference-tier structure, semantic names, bounded changes and truthful limits. | Accept fixture adaptation and expected native capture binding only. |
| `test_task_reserved_observer.py` | Accept readable, isolated fakes and restoration. | Accept bounded acceptance-boundary regression coverage. |

The previously accepted generator/test and comparator/test pairs are byte-unchanged
from `d7fef10`; their prior bounded complete-file dispositions stand. Recorder,
original judges and probes are also unchanged.

## Verification and limits

System Python 3.14.7: `PYTHONPATH=tools/lib:tools/oracles python3 -m unittest
discover -s tests/python -p 'test_task_*' -v`: **35 pass**. Pinned checkout
Python is absent. Additional real-recorder fakes preserve exit 1 and
`SystemExit(2)`, restoring fixture/main/argv identities.

Per-case actual source attempts, STOP/return/fault/exclusion instrumentation and
development characterization remain pending. Exact final source-specific runner,
compiled recipes, committed closure seal and single-use admission remain
unimplemented and unauthorized. Their declared absence is not a new helper bug.
No reserved seeds used/generated, original/native/emulator/build/provider/network
workloads, children, protected edits, refresh, gate or push. No Task/campaign closure.

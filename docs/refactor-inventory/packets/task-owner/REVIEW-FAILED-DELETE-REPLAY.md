# Failed deletion replay: independent bounded review

Candidate `d48f80e6062cc78b23a5c77400e1676ebb811d2e`, before `b235538`.
Reviewed every maintained body in the four files below, applicable shared
instructions/memory, and STYLE §12. No delegates. **Adequacy acceptance withheld:**
one blocking Spec/judge finding; no additional blocking Standards finding.
This report grants no execution admission.

## Finding

**P2 — A committed proposal passes the admission boundary.** In
`reference/packets/task-delete-native-replay/replay.py:23–32,132–154`,
`admitted()` proves only that a selected JSON matches a selected commit. Neither
it nor `main()` requires an affirmative admission discriminator/status. The
reviewed proposal explicitly says “no execution admission yet” but already has
all fields and exact pins needed by `main()`. Selecting that proposal and its
commit would reach output creation and library loading without a separately
committed Root admission. This violates the requested proposal/admission
separation; commit integrity alone does not establish that separation.

Reproduced without execution: the real `admitted()` accepted the proposal at the
exact candidate commit; all eleven pins and the real contract/accounting checks
passed read-only. Independently, a pure fake `main()` check with proposal status,
fake pins/contracts, intercepted `Path.mkdir`, and forbidden `ctypes.CDLL`
reached `OUTPUT_CREATION_REACHED` (one intercepted mkdir, zero library loads).
No output directory was created. Require an explicit committed admission kind
and affirmative state before output creation, reject the proposal state, and
cover that boundary with a fake `main()` regression. Root must still issue the
separate admission after review; this recommendation does not issue it.

## Accepted comparison properties and remaining limits

- Normal `validated_records()` and comparator CLI still require exit 0. The
  added failed-delete reader is separate, requires exactly five pinned files
  and exit 1, and retains full ordered ledger, digest, extent and byte checks.
  Existing default failures, fixtures and exclusions are unchanged by this diff.
- Read-only parsing reproduced 570 ordered original attempts: 314 returns and
  256 established stops. The 563 produced records comprise 313 returns and 250
  stops. Exclusions remain exactly 85, 222, 238, 242, 361, 502, 562, including
  their input/reason identities. Every input vector equals the retained original
  input bundle; its SHA-256 is
  `28fe354ea0a9c872b98301fe6eda54b5bd8eaf54f037a7ee0865c5530f116e27`.
- Calls are mapped through produced cases rather than zipped with all cases:
  native record 206 maps to original case 207. Its complete original projection
  retains all eleven differing words. Changed failure identities with unchanged
  counts are rejected. Library/Python/helper/artifact hashes are checked before
  output creation; the fixed ABI checks return, both input phases, scalar counts,
  output initialization and all 8,192 output bytes, including padding.
- `replay()` stops at the first inequality and exclusive output creation blocks
  reuse of an existing output directory. Full native equality can establish
  preservation only: it retains case 207 and emits `source_fidelity: FAIL`.
  An earlier unequal/incomplete replay can emit `INCOMPLETE`, but still records
  the retained original mismatch and never emits source PASS. Both original
  attempts remain terminal EXIT 1 in their unchanged receipts. The seven
  historical exclusions do not absorb case 207. This preservation-only scope
  does not repair source fidelity or qualify Task integration.
- The 20 new fake tests exercise comparison/contracts but do not exercise
  `main()` admission/output orchestration; the finding above escaped them.

## Verification and exact reviewed hashes

Main's exact `reference/python/bin/python` ran
`-B -m unittest discover -s tests/python -p 'test_task_*.py' -q` from this worktree:
**94 tests passed, exit 0**, including 20 new fake-only tests. Worktree Python is
absent. Read-only real pin/contract parsing and the additional fake admission
boundary reproduction passed as described. `git diff --check b235538 d48f80e`
passed. The proposed output directory was absent after inspection.

| Complete reviewed file | SHA-256 |
| --- | --- |
| `reference/packets/task-delete-native-replay/replay.py` | `e6ebe63b92281600ab4d633cfd5445d03fb9b11e4ed0ead823418ba177cc76dc` |
| `tests/python/test_task_delete_native_replay.py` | `eda6702bbc0ad7db93669af351727d4b4a3ab4561826de8e2834c6c0406045e8` |
| `tools/oracles/task_judges/compare_streams.py` | `38d882f2fad2711584f37ea9b129e52f4b6ddaee9d14c758aecb6652be4485f1` |
| `docs/refactor-inventory/packets/task-owner/DELETE-NATIVE-REPLAY-PROPOSAL.json` | `b99dddd685f3202c8cd8263e54d31e4a712135b737a60f2b472ff21056ae9a20` |

Candidate dylib hash verified read-only:
`2915ae1b9b198b30aa06852ecda822df721eabb4e99d8ab286557dfe403fa577`.
No game/original/native execution, game-library load, generation, input change,
build, mutation, gate, provider/network action, Source edit or artifact write.
Source72 was not re-reviewed. Main and retained artifacts were preserved.
Only this report is committed; no push. Root retains freeze/admission/execution,
manifest union, Task merge, source-only closure, journeys and full gate.

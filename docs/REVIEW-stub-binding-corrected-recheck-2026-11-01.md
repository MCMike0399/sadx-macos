# Corrected STUB binding inventory: independent bounded recheck

Candidate: `92dbd0710ed1bda85807dfea2b55ff63c53311d2`.
Reviewed in the provided isolated checkout, on
`packet/recheck-stub-binding-projection-20261101`. Main and the candidate's other
checkouts were untouched. Only this report is committed.

Disposition: bounded approval. No actionable findings in the requested scope.
The two P2 defects in review `4ed48c2` are corrected by `76352d7`, integrated by
`15e6519`, with semantic-label validation added by `0617cf5`.

## Standards

No findings. The change remains inventory validation and authored advisory
evidence. It does not alter game behavior, oracle expectations, tolerances or
fixtures, and does not award file/module conformity from binding review.
The explicit unknown behavior remains visible in every projected STUB row.

## Spec and corrected validation

No findings. Inspected `tools/inventory/refactor_inventory.py:104–175` and
`:425–450`, the focused Python tests, the authored warning-binding fragment,
the prior review/resolution, source borrowers and coverage receipts.

- Owner, native and reference sections require nonempty lists of path/SHA
  records. Coverage requires concrete fingerprinted records. Recursive validation
  checks tracked membership, file existence and actual bytes for every nested
  evidence path. Omissions cannot leave a truthy but evidence-free section.
- Owner/reference resolutions and native kind/method must be nonblank strings;
  an explicit native failure must name its failure identifier. Limits must be a
  nonempty list of nonblank strings. These are structural requirements; the
  correctness of an authored semantic description still needs human/agent review.
- Exact annotation, following declaration and declaration-file fingerprint are
  required. Source/evidence changes cannot silently inherit binding approval.
- Duplicate-aware parsing rejects duplicate object keys before JSON discards
  them. The separate identity guard still rejects repeats across fragments.
  Unknown/moved identities fail reconciliation.

## Owner revalidation and honest projection

Independently reproduced the six comparisons in
`reference/production-integration-20261101/save-warning-binding-owner-revalidation.json`
using Git objects at its base `e5cf19d`, recorded source `d160717`, and candidate
`92dbd07`. The complete bodies of `TitleScene::update_save_completion`,
`create_save_warning` and `close_save_warning` are identical, with the recorded
body SHA values matching. The annotated `SaveWarningTask` member declaration is
present exactly once in all three versions. Both complete save-completion source
files match their recorded hashes. The Session diff extracts the separate regular
resource block; the title-task diff changes its font texture resource borrower.

Compared both authored entries with the original `aaf759f` fragment: only the
`native_session.hpp` and `native_title_tasks.cpp` evidence fingerprints change.
All other per-binding content is identical. The top-level revalidation proof's
fingerprint also matches; this proof was manually checked here, not treated as a
new automated behavior judge.

The actual methods still throw `native_save_warning_message` and
`native_save_warning_close`. `SaveCompletionState` borrows the live Session task;
publication follows the creation callback and close does not clear the owner.
The original-machine fixture hooks and native probe provide ordered suffix/call
observations, not recovered warning allocation/closure or a live guest binding.
The authored limits and projected labels preserve that distinction.

Compared projected identities with `e5cf19d`: all 84 annotations and 11 other
mentions retain their IDs, paths, lines, kinds, annotation text and following
source. All 95 retain `unrecovered_open_debt`; exactly two receive
`bindings_reviewed_behavior_unrecovered`. Neither is recovered behavior.

## Verification and limits

- `python3 -B tests/python/test_refactor_inventory_review.py`: exit 0, 18 tests.
- `python3 -B tools/inventory/refactor_inventory.py --check`: exit 0 before this
  report was added. No generated inventory was rewritten.
- Additional read-only in-memory probes: 92 rejection checks across both entries
  for missing/empty/unlinked evidence, absent hashes, blank or wrong-type semantic
  labels, and live-byte changes to each owner/binding/coverage evidence file.
  All reject. Four further full-render probes reject duplicate keys and changed
  coverage fingerprints. The focused suite also covers cross-fragment duplicates,
  declaration/source changes, stale identities and attempted behavior retirement.
- Direct SHA audit of the 938-entry protected manifest: all 885 locally present
  inputs match; 53 ignored `.bin` corpus inputs are absent. No full local
  938-input checker pass is claimed. Root's supplied Main pass was not rerun or
  independently verified against Main; no binary copies or transfers occurred.

No builds, oracles, compiled mutations, providers, network/storage operations,
full gates or delegates ran. Root's running frozen-source gate was untouched and
has no verdict from this review. No system/campaign conformity, warning recovery,
new runtime qualification or closure is claimed. Shared memory, HANDOFF, STATUS,
candidate source, manifests and generated inventory were not modified.

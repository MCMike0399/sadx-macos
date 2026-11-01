# Independent final bounded preservation review

**Accept preservation for the captured domain. Source fidelity remains FAIL.**
No introduced preservation defect or blocking accounting finding was found.
The strict behavior-preserving readability refactor can be accepted with this
inherited source failure because every produced candidate native contract is
identical to immutable old, including the failing contract. This does not waive
fidelity requirements, qualify all Task behavior, or approve integration.

Reviewed Main `0a5a2cc`, immutable old `bd22ee423ea015f00152a97fa8fc2e1b4034b411`,
Source `72d0e83ead1255aa658ef6b78220c04b1b737b93`, private packet
`f8ab5e408427bb799aa244e46274648862b0ec05`, and result `0f58ca3`.
Read AGENTS, STYLE including §12, and the candidate's complete
`task-ownership-source/SPEC-FIDELITY-REVIEW.md` (`0e92356`), without repeating
the Source review. All four candidate engine members match Source72 Git bytes.
Standards recheck `e7c2062` and Spec acceptance remain bounded to their scopes.

## Actual independent checks

Used inline standard-library Python with bytecode disabled and read-only Git
inspection. No production helper was imported or invoked. Assertions checked
every retained deletion record, rather than trusting aggregate receipt counts:

* All 570 ordered source/input IDs are present. Exactly 563 produced IDs map to
  563 old ABI calls and 563 candidate calls, with no duplicate, extra or
  unattempted produced ID. Call 206 maps to source case 207.
* Every old call has the expected symbol, ordinal, scalar counts and integer
  return 1. Candidate returns match. Packed source inputs equal both input
  phases in both streams. Initialized output bytes and all 8,192 final output
  bytes, including padding, match old/current for every case. Candidate scalars
  and symbol follow the fixed ABI in the reviewed, hash-pinned replay helper;
  its candidate gzip does not independently record those fields.
* Recomputed every original expected-word comparison: 562 exact projections,
  one failure, case 207. All per-case result entries, output hashes, mismatch
  identities and the committed result agree with these raw comparisons.
* Case 207's diagnostic output is byte-identical to replay case 207. It is the
  same unique generated case, not another case of coverage.
* Work/lifecycle/display complete old/current JSON records are byte-identical,
  respectively 8/18/556. Independently decoded every buffer and checked both
  identity ledgers' sizes/hashes, returns and ordering; all match their
  comparison receipts. Full source-observation lists and input captures match
  old/current. Display retains 416 returns and 140 established stops.

The three recomputed uncompressed stream SHA-256 values are:

| Cohort | SHA-256 |
| --- | --- |
| Work | `507c4102a41e45ce7c900fd5aa277465f43019c3b6cb1b968a066c5a2112abd3` |
| Lifecycle | `60989cbf1f4fe5acaa8e1e86c4d85b3083d69e2688185d86f1ab4ca56491835d` |
| Display | `88de4194bec2713a3b89174442f1f771999484ab95873636c779772c778babf8` |

Raw base: Main `build/task-final-reserved-continuation/{old,current}/captures/`;
candidate deletion: `build/task-delete-native-replay/native-calls.jsonl.gz`.
The four frozen bundles remain byte-identical across original attempt and
continuation: 8 + 8 + 556 + 570 = 1,142 generated inputs. ABI call counts
8/18/556/563 are a separate measure. Lifecycle's additional callbacks/control
(eight helper observations plus one inactive established stop) add no generated
cases. An established stop is not a returned original function.

## Inherited failure and exact exclusions

**Open inherited fidelity defect:** source case 207 returns at STOP with no
memory fault, failure or source boundary; old/current native output reports
`CallbackMutationInvalidatedDestructor`. Static original assembly saves links
at `40b59a`/`40b59e`, calls FreeChildren at `40b5ac`, then writes through those
saved links at `40b5bc`/`40b5bf`. The saved-slot-6/child-2 deletion explanation
is consistent with this assembly and the retained fixture. Immutable old
`task.cpp:548–552` already has the post-child saved-link resolution guard;
Source72 retains it. This is an inherited discrepancy, not a candidate fix or
a new exclusion. Assembly evidence: `reference/task-lifecycle/task-lifetime.asm`.

All eleven differences remain exactly `[word, original, native]`:
`[0,0,116], [4,3,4094], [5,1,2], [17,2,0], [25,1,0], [26,6,0],
[27,1,0], [33,2,0], [61,1,0], [63,2,0], [69,6,0]`.
Output SHA-256: `fd8dad973e31bcd71b9ffaee59dab47baf486bcbc0bbc66ca7a21798f39c9359`.

Recounted source outcomes: 570 = 314 returned + 256 established stops;
produced 563 = 313 returned + 250 stops; excluded seven = one returned
free-list-cycle (502) + six reentrant stops (85, 222, 238, 242, 361, 562).
Every exclusion's input and reason matches its source event. Case 207 is
produced and compared. No tolerance, expected value or exclusion was relaxed.

## Receipt integrity and terminal history

Verified all eleven Root admission pins, including the **unloaded** library
SHA-256 `2915ae1b9b198b30aa06852ecda822df721eabb4e99d8ab286557dfe403fa577`,
helper, comparator, interpreter and source capture files. Admission bytes match
commit `92bdda125f395f2dc13a860bda44bc00f8899da9`; STARTED binds that admission.
All three committed replay artifact sizes/hashes match; RESULT SHA-256 is
`442c0208b37788a46ca1febc8e2249828b40e8bfc5dd83ec576c8595f2bdb590`.
All 68 terminal continuation artifacts match their manifest sizes/hashes.

Read actual exit files, RUN receipts and failure logs: initial reserved attempt
exit 1 before observation; distinct corrected continuation exit 1 at old
deletion source comparison; wrong-admission-commit native launch exit 1 in
`admitted()` before output creation/library load; corrected native replay exit
0. The command correction matches `d3dbeab` and the failed-log hash. Neither
earlier original exit 1 is promoted. Native replay exit 0 proves preservation;
RESULT and committed receipt explicitly retain source FAIL.

Prior helper finding `3a7bee1` P2 is closed by `1c15c5d`/`fea2289`; the pinned
helper still requires affirmative committed admission before contracts/output/
loading. Ordinary comparator exit-zero requirements remain separate from the
pinned failed-delete integrity reader. The prior 95 fake tests are accepted
evidence, not newly executed tests in this review.

Independently hashed all 1,333 current manifest files: zero violations.
`87135d8` retains every one of 1,331 parent paths, changes only the reviewed
comparator hash, and adds the replay helper and its fake tests. This verifies
that finite manifest snapshot, not Root's future merged-source evidence union.

## Remaining limits and ownership

The private packet's `task-ownership-source/SPEC-FIDELITY-REVIEW.md`
and `FINAL-STREAMS.json` retain development streams 16/40/236/244,
14 scheduler plus three bridge valid faults, 23 units and five consumers.
Those checks were not rerun. Their inherited collision exit 2/6,462 FP partials,
startup unobserved original return, auxiliary limits and arbitrary-callback
limits remain. Helper originals retain their existing scalar/PC/arena-hash
observation limits; native byte equality does not expand original coverage.

`INACTIVE-TASK-INVENTORY-PROPOSAL.json` projects only four core members/two
modules, retains the source failure and pending integration, and is inactive.
Its older “native-only remaining preservation is pending” wording is superseded
by this bounded result; Root should reconcile that wording before activation.
Prior Task actor, PlayerSystem and other helper debt stays open.

Root owns exact Source merge, parent manifest/evidence union, current table/
semantic/provenance bindings, actual native/reference clean source-only closure,
journeys and coherent full gate. No native/original/game execution, ctypes
game-library load, generation, input change, rebuild, mutation, gate, provider,
network, storage operation or Source edit occurred. Only this report is written
and committed; no Main edit, push, execution admission, product-fidelity pass
or campaign-complete claim.

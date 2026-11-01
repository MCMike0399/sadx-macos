# Continuation anchor recheck

**HOLD candidate 062839e at metadata HEAD 4a65efe.** Two reproducible Spec
gaps remain. This supersedes neither Source72 acceptance nor original failure;
it gives no Task, campaign, preparation, admission or execution approval.

## Priority findings

**P1 — Copied continuation inputs retain a mutable coherence gap.**
`tools/oracles/task_judges/reserved_run.py:475–480,505–511` checks the new
`OUT/INPUTS.json` against new copied files, but never compares that manifest to
`frozen['continuation']['inputs']`. The earlier proof equality validates the
preserved ORIGINAL_OUT files, not these new observation inputs. After copying,
a coherent replacement of a continuation input and its new manifest therefore
passes child validation and reaches observation with unadmitted cases. A
replacement shared by both worlds also survives old/current input equality.

Verified fake reproducer: create temporary original model placeholders for all
eight required paths and continuation input placeholders for all four slugs.
Keep a complete frozen proof with the hashes of `cases=[[1]]`; replace only
the new work input with `cases=[[999]]` and update new INPUTS accordingly.
Supply that unchanged proof from mocked `verify_freeze` and
`unattempted_previous_run`; mock admission, archive/origin/Unicorn checks and
imports. Invoke the in-process fake observation route for old/task_work.
The observer mock receives `[[999]]` and returns zero. All original inputs,
frozen proof and model identities remain unchanged. No real observer executes.

Required correction: in continuation observation, require the entire new input
manifest to equal the frozen reuse input map before markers/imports/observation,
then validate every copied file against those anchored hashes. Add this coherent
replacement regression through the complete fake child route.

**P2 — Invalid complete-key proofs create permanent output before rejection.**
`reserved_run.py:175–184,520–530` validates keys only. A proof containing all five
required keys with null values passes `validate_mode`. Parent creates OUT and
EXECUTED_ONCE before checking equality with the actual previous-run proof;
rejection then writes terminal RUN. This violates the requested wrong-proof
refusal before output and consumes the exclusive output path.

Verified fake reproducer: use
`dict.fromkeys(('terminal_sha256','freeze_sha256','inputs','libraries','original_output'))`
as continuation proof; mock verification to return that freeze and previous-run
validation to return a different proof. With temporary OUT/FREEZE, fake parent
dispatch raises `Previous unattempted proof changed`, but OUT contains both
EXECUTED_ONCE.json and RUN.json. Archive/build/command mocks remain untouched.
Validate nonempty typed proof contents and previous-proof equality before OUT
creation. Cover complete-key null values and well-shaped unequal proofs.

## Standards and route assessment

**Standards: bounded pass, independently assessed for both complete files.**
Read all 596 runner lines and all 536 test lines, every changed function and
the complete prepare/verify/run/child/CLI route; also read both anchor JSONs,
prior HOLD and preserved failure evidence. Semantic names, structured flow,
exclusive writes and explicit failures fit reference tooling. Source C++, story,
numeric expectations, tolerances and exclusions have no change in this delta.

Tests have concrete independent purposes: mode refusal, continuity selection,
stage-label preservation, toolchain/driver/search pins, sanitized environment,
source-cache/import/native-loader sealing, parent/stage/pipe identity and EOF,
terminal/exclusive refusal, raw-pin integrity and complete archive membership.
Mocked compilation tests inspect argv. The new tests catch null/empty proof
fallback and coherent ORIGINAL input substitution; they omit the two cases above.
Their Standards verdict does not derive from Source acceptance or passing counts.

**Spec: partial correction, blocked by findings.** CLI selection binds mode and
three paths; verify, parent and child check frozen mode. Original rejects reuse;
continuation generation refuses before admission. Both prepare mismatch directions
refuse before writes. Original terminal guard, exclusive new parent/stage markers,
stage/token/inherited-pipe identity and orphan EOF termination remain intact.
Valid continuation copies bytes without rebuilding/regenerating. Future original
proof equality binds models, but does not close P1's copied-input gap.

## Evidence and limits

Fresh read-only checks match admission 5487937's freeze, failure a8e3b0f's
RUN/marker/log/wrong-marker/four input hashes and counts 8/8/556/570, and
2e82cf4's BUILD/eight-model map. HEAD's model anchor matches 2e82cf4; code actually
reads fixed admission/failure commits and the committed HEAD model anchor.
Both preserved archives match all 1,208/1,209 Git blobs and exact membership;
source-tree IDs match freeze and candidate matches Source72. Raw pins match
their hash/cardinality (42,329); reviewed project-helper delta is runner only.
External toolchain/package contents were not exhaustively rehashed.

Original remains terminal exit 1: 1,142 generated, zero Source/native attempts,
no captures. No continuation freeze/pins/output exists. Fresh verification:
23 runner fake tests pass with Popen forbidden, additional mode/path/prepare
fakes pass, and both reproducers confirm findings. Startup/.pth and real EOF
subprocess tests were skipped; reported 72 tests were not rerun. No actual
prepare/run/child/build/generation/replay/emulation/mutation, network/provider,
refresh, protected edit, child agent or push occurred. Only this document is committed.

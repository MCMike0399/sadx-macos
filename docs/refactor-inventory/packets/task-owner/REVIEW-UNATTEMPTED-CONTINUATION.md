# Unattempted continuation review

Verdict: **HOLD ce38b9c; two blocking Spec findings below.** Reviewed at metadata
HEAD 639b2c9 against 95f92a6, including every line of the current 549-line runner
and 478-line test file, the proposal, failure receipt, stage review, original
freeze and admission. Source72 and the accepted observer are reused unchanged;
the observer has no diff from 5487937. This is no Source, Task, campaign or final
execution approval. Root owns a new immutable freeze and distinct admission
after acceptance; neither is requested from the user.

## Priority findings and reproducers

**P1 — Selected continuation can fall back to generation/build.**
`reserved_run.py:166`, `:240`, `:413` and `:480`: CLI selection changes three
paths, but execution mode comes independently from the truthiness of
`frozen.get('continuation')`. Freeze verification never requires a continuation
proof for continuation paths. A committed, otherwise valid continuation freeze
with that field missing, null or empty therefore reaches the ordinary archive,
build and `_generate` branch. Child generation refusal is conditional on the
same truthiness. Distinct filenames alone do not enforce the promised
“NO generation/build path”; a wrongly assembled freeze can consume regenerated
reserved inputs under the continuation output name.

Fake reproducer, executed without commands or builds: import RUN from the test
module; patch ROOT/PACKET into a temporary directory with a build parent;
call `select_continuation()`; create a dummy selected freeze; mock
`verify_freeze()` to return the normal before/candidate/toolchain fields plus
`continuation=None`; mock `archive()` to return a dummy tree and `build()` to
raise a sentinel; make `command()` raise if called. Invoke the fake parent
dispatch. Both archive and build mocks are reached once. This isolates dispatch;
static inspection establishes that `verify_freeze()` accepts the missing mode
field when its other checks pass. No reserved data was involved.

Required correction: bind explicit selected mode to freeze schema/identity in
parent and child before side effects. Continuation must require a complete
nonempty reuse proof; original mode must reject reuse proof. Reject generation
unconditionally in selected continuation mode. Add fake CLI/path-selection,
wrong-freeze/null-proof and child-generation tests, with build/generation mocks
that must remain untouched.

**P2 — Initial continuity proof is self-consistent, not anchored to attempt 1.**
`reserved_run.py:173–208`: the helper accepts any failed, empty-completed RUN,
checks a local marker against a local original freeze, and checks inputs/models
against mutable INPUTS/BUILD manifests. It never checks the original freeze
against its committed admission identity or the RUN/marker/input hashes against
`RESERVED-ATTEMPT-1-FAILURE.json`. The returned proof freezes whatever passes
today. Subsequent equality checks protect that new selection, but cannot prove
it was the exact preserved admitted attempt.

Concrete fake reproducer: the new positive test itself supplies an original
freeze with no before/candidate/protocol/source-tree identity and an arbitrary
error receipt, and succeeds. In an in-memory copy of that test, replace the fake
work input with `{'cases': [[999]]}` and update its INPUTS hash before the first
helper call: the helper again succeeds. The unchanged remainder of the test
passes. Updating only input bytes is rejected, as intended; coherent wrong
selection is not. This is a continuity/selection gap, not a claim that SHA hashes
defend against an attacker rewriting all trusted metadata.

Required correction: anchor the original freeze/admission and failure identities
to committed accepted evidence before producing a new proof. Compare the four
original input hashes to the committed failure receipt, preserving ordered bytes
and their 8/8/556/570 rows. Retain exact eight-model checks and bind the reviewed
BUILD selection in the new immutable proof. Add negative tests for a coherent
substituted input/manifest, different failed receipt and wrong original freeze.

## Standards and Spec purpose verdict

**Standards: bounded pass for both complete files.** Semantic helper names,
structured flow, explicit exceptions and exclusive receipts follow the reference
tooling style. The stage fix retains `library_name`; no game logic, values,
expectations, tolerances or exclusions change. Existing tests retain their
specific purposes: C++ driver spelling, explicit linker/include/SDK closure,
sanitized environment, checked-source imports, sealed Unicorn origins, parent
identity/EOF, terminal refusal, archive content/set, raw-pin integrity and
exclusive writes. Mocked build tests inspect argv only. Observer/comparator
full-call and buffer checks remain dependencies, not newly established fidelity.

**Spec: fail pending both findings.** With a valid reuse proof, parent copies
exact bytes with post-copy hashes, reuses original models/archives, passes
`--continuation` to observation children, and performs no build/generation.
Children recheck prior proof, eight models, all input hashes, archives and import
origins before observing. New stage markers and capture paths are distinct.
Stage/token/pipe and terminal guards remain; EOF kills an orphan and exclusive
OUT prevents retry even when abrupt parent death leaves no final RUN receipt.
Ordinary caught failure writes an error RUN and stops serial dispatch. RUN's
completed list counts fully compared slugs, not individual attempted cases;
observer receipts/streams retain partial source/native accounting. No promotion
of partials or historical failure to pass is implemented.

The added test covers one helper's positive path and changed-file, prior-capture
and completed-subset refusals. It does not cover continuation preparation,
mode/freeze agreement, copied-input dispatch, flag propagation, no-build/no-generate
invariants, or failure finality across that route. Add those fast fakes.

## Evidence and limits

Fresh read-only hashes under `/Users/burbujamc/Developer/sadx-macos` match the
committed failure's RUN, permanent marker, first-child log, wrong marker and four
input hashes; all eight models match BUILD. Original freeze matches admission;
raw pins match freeze. There are no capture directories. Thus 1,142 rows remain
generated, with zero original Source/native case attempts, and original EXIT 1
remains terminal. No continuation output, freeze or raw-pin file exists there.
This review checkout has no build directory; absolute original pins intentionally
require the original root, not a silently rebased worktree.

Fresh verification: 21 runner fake tests pass, plus the two fake reproducers.
Popen was forbidden during the 21-test run. Startup/.pth and real pipe-EOF tests
were skipped because they create subprocesses. The proposal's 70-test result was
not rerun or promoted to execution evidence. No actual preparation, admission,
build, generation, child, original/native/emulator observation, mutation run,
provider/network action, refresh or protected edit occurred. Only this review
document is committed.

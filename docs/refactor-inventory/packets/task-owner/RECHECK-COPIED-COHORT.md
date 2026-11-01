# Copied-cohort correction recheck

**Bounded acceptance: candidate `88f2501`, metadata HEAD `c80d7bd`.** No remaining
priority findings in this correction. P1 and P2 from review `3e52fd7` (imported
by `cadb15b`) are closed. This accepts the reviewed orchestration correction,
not Task qualification, campaign completion, preparation or execution.

## Spec and dataflow

P1 is closed in `reserved_run.py:478–495`. Child validation first checks selected
mode, the committed freeze, and equality with the actual anchored previous-run
proof. The new BUILD library map must equal the frozen reuse map, retain exactly
eight original model paths, and match every model file hash. Observation requires
the entire copied INPUTS map to equal the frozen four-slug input map, then hashes
all four copied files against those values. These checks precede the exclusive
stage marker, archive imports, Unicorn loading and observer invocation. Coherent
replacement of copied case bytes and their mutable manifest cannot supply a new
cohort. Both worlds use this route; old/current equality is additional evidence,
not the admission boundary.

P2 is closed in `reserved_run.py:175–197,532–541`. Validation requires exactly five
proof fields, the exact original output path, exactly four input slugs and eight
original model paths, and lowercase 64-hex string hashes throughout. A null-valued
complete-key proof fails validation. A well-shaped unequal proof fails comparison
with `unattempted_previous_run()` before OUT creation or the permanent marker.
That producer still reads fixed committed admission/failure anchors and the
committed model anchor, checking original freeze, terminal/marker, input selection,
cardinality, BUILD selection and model bytes. Key/type checks alone do not grant
continuation. The second equality check inside the parent remains fail-closed.

Original mode still rejects reuse proofs and retains its build/generate route.
A valid continuation copies exact bytes and uses preserved models without build
or generation. CLI selection binds mode and continuation paths; `verify_freeze`,
parent and child validate mode. Continuation generation refuses before admission.
Parent/stage/token/inherited-pipe identity, terminal refusal, orphan EOF handling,
exclusive output/receipt/stage creation and terminal failure recording retain
their previous contract. The changed validation introduces no observed false
refusal for valid proofs or valid continuation dispatch. Concurrent hostile file
rewrites after validation remain outside this protected, single-writer workflow.

## Standards and test adequacy

Standards: bounded pass, assessed independently of Source72 approval. Read the
exact patch, all 613 runner lines and all 577 test lines, including complete
parent/child, prepare, verification, continuation and CLI routes; read the
correction JSON and prior HOLD. Semantic names, explicit failures, structured
flow and exclusive writes fit reference tooling. No game implementation,
expectation, tolerance, exclusion or native source changed in this delta.

The complete changed test scope includes the proof factory; parent null-proof
and well-shaped unequal-proof subcases; the refactored full fake child fixture;
the retained original-mode stage-label control; and coherent copied-case plus
INPUTS replacement regression. Parent cases assert absent OUT and untouched
archive/build/command mocks. The child case cannot reach its raising observer
and asserts no started marker. Its broad ValueError assertion alone could hide
an unrelated early refusal; independent controls below establish the intended
boundary and valid continuation reachability. Existing tests cover anchored
original substitution, mode fallback, dependency/import sealing, toolchain argv,
pipe/stage identity, terminal/exclusive guards and archive/pin integrity.

Fresh verification: 25 runner tests pass with subprocess Popen forbidden. Skipped
by scope: startup/.pth subprocess and real pipe-EOF subprocess tests. Independently,
25 malformed proof variants refuse and a valid proof passes; fake model-map,
input-map and copied-file alterations fail with their specific diagnostics before
imports/started markers; a valid continuation reaches the fake observer. The
reported 74 focused tests were not rerun in full.

## Evidence and limits

Correction-file and judge-manifest hashes match both reviewed files. Read-only
hashes match committed admission `5487937`, failure `a8e3b0f`, and model anchor
`2e82cf4`: original freeze, RUN, permanent/wrong markers, first child log, four
inputs, BUILD and eight libraries. Source is unchanged since `cadb15b`. Preserved
failure remains exit 1, 1,142 generated and zero attempts; no captures exist.
Continuation freeze, pins and output remain absent. External closure was not
exhaustively rehashed; no actual prepare/run/child, generation, compilation,
original/native/emulator execution, mutation, network/provider, global refresh,
protected edit, child agent or push occurred. Only this document is committed.

Root owns a new immutable continuation closure and distinct explicit admission
after this acceptance. Preserve the old failed run. No Task, campaign or final
execution approval is given.

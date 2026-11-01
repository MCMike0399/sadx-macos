# Fresh independent review of 4bad9c6

Recommendation: **BLOCK** for one remaining P2 proof-classification defect.
The exact original R1–R7 reproducers and last F1–F3 reproducers are corrected;
R3's definition guard still has the narrow qualifier-combination gap below.

Candidate: `4bad9c64c76e435b816b424d5ae330c6be63f542`, branch
`packet/constexpr-table-review`, maintained path
`/Users/burbujamc/Developer/sadx-wt-constexpr-table-review`.
Base: `c55762db5a42ad77b56fbaa8617ccfead4dbf0e1`.
This fresh review inspected the complete changed-path set, both prior reports
(`9c9fb0f`, `da6377a`), current implementation/evidence/fixtures, and particularly
`6cba6f8..4bad9c6`. Standards and spec were examined by this reviewer alone.
The candidate remained at the pinned HEAD with a clean worktree throughout.

## Prioritized finding

### G1 / R3 — P2: Ref-qualified virtual definitions still inherit dispatch proof

Location: candidate `tools/gate/check_address_dispatch.py:111`, with the false
proof emitted at lines 116–120.

The following ordinary C++17 member definition is accepted after an evidenced
file-local `call` helper, instead of failing for explicit contract review:

```cpp
struct Painter {
    virtual bool call(unsigned color, unsigned mask) & final {
        return color == mask;
    }
};
```

For the synthetic registered two-argument bool dispatcher, `census` returns one
**proven dispatch** whose target expression is `unsigned mask`. No target
forwarding occurs in this member. `&& noexcept final`, `& override` and
`&& noexcept override` definitions have the same result (override cases assume
their matching virtual base declaration comes from a header). No compilation was
run; these observations exercise the lexical checker directly.

The guard recognizes `final`/`override` immediately after the parameter list,
and recognizes ref qualifiers followed by optional noexcept/trailing return and
then `{`. It omits virtual specifiers after the ref qualifier. This is a small
combination of suffix tokens already named by the contract, not a request for
general C++ type, scope or overload resolution. Like the original R3 case, a
declaration is being published as an evidenced invocation. Replacing one existing
call expression with this definition keeps the dispatch count at one, and
`increases` returns no errors; a count increase cannot reliably compensate.

Extend bounded definition/ambiguity detection to include these suffix combinations,
or fail them for review before constructing a proven site. Retain the valid
logical/bitwise AND expression fixtures. Add a discriminator that rejects this
definition while continuing to count `call(site,target) && ready` correctly.

Exact reproducer:
`observations.py::ReviewObservations.test_remaining_ref_qualified_virtual_definitions_inherit_dispatch_proof`.
It checks all four suffixes, the incorrectly asserted target, and the unchanged
ratchet count. It passes by reproducing the defect, not by approving that behavior.

## R1–R7 and F1–F3 disposition

- **R1 corrected.** Unresolved callable-name tokens remain descriptive/report-only.
  A new semantic module passes; adding a known-signature dispatch fails. Proven
  counts retain committed/working ratchets. Dirty/untracked migration, forged
  allowances, repeated initialization and lowered-allowance fixtures pass.
- **R2 corrected within the bounded grammar.** Top-level ambiguous `<` expressions
  fail explicitly, including template commas before the target. Parenthesized
  templates preserve the complete target. Parenthesizing comparisons/left shifts
  is a declared conservative restriction, not hidden semantic inference.
- **R3 original examples corrected; G1 remains.** Immediate bodies, const,
  noexcept, trailing returns, simple ref-qualified definitions and new same-name
  lambdas fail. Virtual specifiers after a ref qualifier still escape.
- **R4 / F1 corrected for the supported surface.** Nested parenthesized custom
  braced initializers are balanced and fingerprinted; changing their values
  changes the fingerprint. Independent observations cover 1, 2, 3 and 6 nested
  expression parentheses. Direct parenthesized braced arrays are captured; the
  supported named-parameter function declaration/definition is skipped; ambiguous
  parenthesized arrays fail explicitly. The inherited named-constexpr prefix and
  limited declaration grammar remain lexical limits, not general type discovery.
- **R5 corrected for registered evidence.** Whole-file source/consumer hashes
  invalidate in-range consumer changes. Current evidence validates, including
  external consumer/default-initializer anchors. Hashes preserve selected reviewed
  evidence; they do not discover every transitive dependency or prove semantics.
- **R6 / F2 corrected for the tested lifecycle.** Retirement retains the exact
  original active review and requires real current evidence/verification and a
  hashed named successor or explicit removal proof. Changed successor/evidence
  files fail until reviewed fingerprints are updated. Updates then require an
  append-only copy of the full previous proof, with the original review unchanged.
  Independent temporary observations exercise two updates to both live files,
  rejection without the history append, rejection of altered original proof,
  missing evidence/successor rejection and rejection of dropped retirement history.
  Original historical proof is preserved without requiring old files to stay live.
  Active/historical totals and prior dispositions remain visible. The unresolved
  retirement count concerns outcome tags; invalid known-outcome proof still fails
  via the problem count/exit status. The actual registry has no retirements yet.
- **R7 corrected.** The focused archive fixture enforces its supplied dispatch
  baseline without Git, rejects added known dispatch, and rejects baseline
  writing/migration without Git. Git migration/history checks remain exercised.
- **F3 corrected for its exact false positive.** Ordinary logical AND and bitwise
  AND expressions remain accepted, with the original target expression retained.
  Additional observation `call(site,target) && noexcept(ready())` also passes.

## Standards and semantic scope

Read AGENTS, PROMPT-complete-refactor, STYLE including section 12, JUDGE, the
orientation/current status and handoff context, and the canonical campaign note.
No independent standards blocker beyond the proof-contract defect above was found.
STYLE's behavior/ownership/arithmetic rewrite requirements do not imply new game
oracles for this judge-only candidate. No game behavior is changed in this diff.

The 11 helper contracts retain exact definition/body anchors and evidenced target
forwarding. Sonic's fourth argument is the target, while its fifth is a diagnostic
site; the census is not treating colors, masks, numerical magnitude or unresolved
names as address proof. The candidate's 113 proven sites include the existing Sonic
sites. This is a new measurement of unchanged code, not newly added source debt.

The 49 aggregate review entries reconcile to 19 numeric encodings, 11 named
structures, 4 typed content, 14 open migrations and 1 open review. Reviewed their
purposes/evidence, declaration surface and key consumer distinctions; no new
classification error was established. Instruction recipes and address/PC-indexed
tables remain open. Sonic animation recipes remain open_review because typed
content/encodings do not by themselves resolve the import boundary. Numeric
rounding maps, sine coefficients, matrices, device enums, channel shifts and
input masks are not misclassified solely for containing hex. Named native
function pointers remain distinct from original-PC dispatch. These dispositions
do not certify whole modules or complete the campaign inventory.

Unregistered names, macros/preprocessing, indirect calls, template-id callees,
receiver/type/scope resolution and complex declarations remain outside this
bounded tool's semantic proof. Selected evidence hashes and successor identifier
tokens are review anchors, not compiler-resolved fidelity proofs. G1 is inside
the already-enforced declaration suffix surface; these broader declared limits
are not additional blockers or requests for a general C++ analyzer.

Diff inspection confirms no changes to game source, inherited probes/fixtures,
numerical/route oracle code, mutants/tolerances, the old readability checker or
allowances, or the judge manifest. The new registry/baseline, tools and fixture
paths satisfy `check_judge.protected`. New prototype checker fixtures were changed
to implement the reviewed policy; inherited game judges were not weakened.
Historical candidate prose is explicitly labeled and superseded by review-fixes.

## Executed verification

Against the clean pinned maintained candidate, all with `PYTHONDONTWRITEBYTECODE=1`:

```sh
python3 -m unittest discover -s tests/python -p test_address_dispatch.py -v
python3 -m unittest discover -s tests/python -p test_constexpr_tables.py -v
python3 -m unittest discover -s tests/python -p test_readability.py -v
python3 -m unittest discover -s tests/python -p test_judge.py -v
python3 tools/gate/check_address_dispatch.py
python3 tools/gate/check_constexpr_tables.py
python3 tools/gate/check_readability.py
```

Results: **57 tests pass** (16 dispatch, 12 constexpr, 17 old readability,
12 judge). Each suite exits 0. All three CLIs exit 0: 113 proven dispatch sites,
23,463 report-only unresolved tokens, 49 active/historical aggregates,
14 open migrations plus 1 open review, zero current problems/increases.
The old CLI reports 495 files and zero regressions.

From this separate review packet:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 reference/production-integration-20261101/review-4bad9c6/observations.py
```

**10 observation tests pass**, including the deliberate G1 reproduction. They
import the maintained candidate and only write temporary fixture files. They
exercise the earlier fixes independently; R7 and immutable-HEAD CLI behavior are
covered by the freshly run candidate fixture suites above. No candidate edit,
second candidate checkout or old child-thread reuse was involved.

No agents/providers, source builds, oracles, full gates, journeys, runtime/GPU
checks, manifest refresh, SSH or storage helper calls were run. No HANDOFF,
STATUS or main changes. Final judge-manifest refresh, integrated gate/receipts
and integration-drift checks remain Root-owned after approval. Focused green
checks neither qualify the integrated gate nor establish whole-codebase completion.

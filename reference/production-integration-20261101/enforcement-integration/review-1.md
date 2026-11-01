# Fresh independent review of c306a21

Recommendation: **BLOCK** pending the findings below. This is a formal review of
`c55762d..c306a21`, not advice from the research thread and not a full-gate verdict.
Candidate: `packet/constexpr-table-review`, maintained worktree
`/Users/burbujamc/Developer/sadx-wt-constexpr-table-review` (clean during review).
Review branch: `packet/review-constexpr-dispatch-c306a21`.

The standards and spec axes were both reviewed by this fresh reviewer. No delegates,
providers, implementation changes, builds, oracles or full gates were started.
User restrictions supersede the generic review skill's parallel-agent workflow.

## Prioritized findings

### R1 — P1: Ratcheting every callable token blocks the requested semantic refactor

`tools/gate/check_address_dispatch.py:174` (candidate collection at lines 98–102).
`Makefile:356` makes this blocking for every check/gate.

`census("void update_player() { advance_animation(); }", new_path, [])`, compared
with an absent file baseline, produces two blocking candidate increases. A new
native module cannot even declare its ordinary semantic function without failing.
Existing modules also cannot add/rename a semantic helper or increase a normal
call count. These 23,463 tokens include ordinary calls and declarations, provide
no target proof, and can still hide semantic substitutions within existing counts.
`--write-baseline` refuses increases; changing the registry digest invalidates the
committed baseline; no reviewed expansion/migration path is implemented. The
candidate evidence explicitly confirms this restriction at
`address-dispatch-enforcement.md:42`. This obstructs PROMPT §3's ordinary calls,
module extraction and future hooks without improving *proven* dispatch detection.

Bounded alternative: keep the evidenced known-signature dispatch/evidence ratchet;
report unresolved token additions as a separately labeled review inventory rather
than a blanket gate. If a blocking unknown-helper obligation is needed, review
bounded helper definitions with explicit dispatch/non-dispatch dispositions and
fingerprinted evidence, with an implemented reviewed update path. An unresolved
name or numeric magnitude must never become semantic proof. Add a regression that
a new semantic module passes while new calls to a known dispatch helper still fail.

### R2 — P2: Template commas silently corrupt the asserted target argument

`tools/gate/check_address_dispatch.py:55`, consumed at lines 104–114.

The supplied reproducer uses Sonic-like target position 4 and allowed arity 5–7:
`call(rt,f,fp, choose<int,int>(a,b),site);`. It passes but reports target expression
`choose<int`, not `choose<int,int>(a,b)`. The extra comma makes six apparent
arguments, still inside the allowed range. A template comma in an earlier
argument can instead cause a different argument to be labeled the target.
The stated unsupported-template limitation does not fail closed, and the current
fixture suite has no discriminating template-comma case. Support the bounded form
or reject ambiguity before publishing a proven target; a documentation request to
parenthesize expressions cannot enforce the checker contract.

### R3 — P2: Same-name overloads with qualifiers silently inherit the contract

`tools/gate/check_address_dispatch.py:105`.

Both `void call(int a,int b,int c,int d,int e) noexcept { paint(d); }` and
`struct Painter { void call(int a,int b,int c,int d,int e) const { paint(d); } };`
are accepted and emitted as proven dispatch with target `int d`. The body check
only recognizes an immediate `{`; `const`, `noexcept`, trailing returns and similar
syntax evade it. File-wide name/receiver matching then applies the original
contract to unrelated scopes. A count increase might incidentally catch a net
addition, but it is not duplicate-helper rejection or proof; replacing old sites
can retain the count. Extend definition/ambiguity detection or classify uncertain
same-name forms as unresolved and require review. Add qualifier/overload fixtures.

### R4 — P2: Parenthesized aggregate objects escape declaration review

`tools/gate/check_constexpr_tables.py:38`.

`constexpr std::array<const char*,2> names({"Sonic","Gamma"});` produces no
candidate, while the equivalent brace-only object produces one. This is an
aggregate-valued object initialized through a parenthesized braced temporary,
not a function declaration. New or changed resource strings in this form have
neither fingerprints nor review entries; the old literal checker also has no
string-content coverage. Do not discard every `(` match: discriminate supported
object initialization from functions or require explicit unresolved review.
Add a C++17-compatible parenthesized array/custom-value regression. No compilation
was run; the separate exploratory C++20 raw-array form is not used as the finding
because the Makefile currently selects C++17.

### R5 — P2: Consumer evidence can change without invalidating classification

`tools/gate/check_constexpr_tables.py:96`, especially lines 110–112.

A synthetic table with evidence at line 2 passes before and after replacing that
line's `consume_as_data(modes)` with `dispatch_recipe(modes)`. Declaration bytes
are unchanged, the evidence path/line still exists, and both review checks return
zero problems. Most current registry evidence points to the declaration itself,
although the stated classifications describe consumer semantics. The existing
stale-evidence fixture only checks a line beyond EOF. This is not a demand for
C++ semantic proof: preserve a reviewed consumer snippet/hash (and relevant type
or default-initializer anchor where needed), so changed evidence becomes open
review rather than silently retaining its prior meaning. Add an in-range changed
consumer fixture. Whole-module independent review remains necessary.

### R6 — P2: Retirement accepts fabricated evidence and loses the debt denominator

`tools/gate/check_constexpr_tables.py:130`, and active-only reporting at line 176.

Starting from an `open_migration` entry, remove its declaration/review and add a
retired item with the old hash, reason `Migrated`, evidence
`["does-not-exist.diff"]` and successor `src/engine/missing.cpp::native`.
`review_problems({}, new, old)` and `reference_problems(root,new)` both return no
problems. Retired evidence/successors are not checked at all; original classification,
purpose and verification are not preserved, and the printed denominator becomes
zero candidates/zero open migrations. Moving a recipe out of the scanned surface
can thus be accompanied by an ungrounded retirement receipt. Registry hash
protection makes this a reviewed judge edit, but does not make its lineage true.
Preserve prior review disposition and explicit retirement outcome; validate durable
evidence and a real successor or explicit justified deletion. Report active plus
retired totals and unresolved retirements. Add a nonexistent-successor/evidence
fixture. This is an accounting defect, not a claim that current empty history was
already used to launder a migration.

### R7 — P2: Unconditional Git history requirement regresses archive checks

`tools/gate/check_address_dispatch.py:197`–205; `Makefile:356`.

An isolated source-archive fixture with an exact supplied registry and matching
baseline passes the old readability checker but the new CLI exits 1 with
`ADDRESS DISPATCH: Git HEAD required: ls-tree HEAD -- reference/address-dispatch-baseline-v1.json`.
Therefore the new mandatory check makes `make check` and `make gate` fail outside a
Git checkout even when supplied baseline hashes are intact. STYLE and JUDGE
explicitly preserve archive validation while restricting history comparisons to
Git. The evidence discloses the requirement but the integration does not preserve
the capability. Restore supplied-baseline enforcement for archives, reserving
migration/baseline-writing/history checks for Git; test both paths. Makefile
inspection shows ordinary `all`, `build/sadx-native`, and native link rules do not
depend on this checker. This is a check/gate portability regression, not evidence
of lost native runtime or ordinary source compilation capability. Builds were not run.

## Positive checks and scope assessment

- Read AGENTS, PROMPT-complete-refactor, GOAL orientation, STYLE including §12,
  JUDGE, STATUS, latest HANDOFF and the supplied November 1 baseline/evidence.
  Baseline evidence remains partial/in-progress as recorded; no green gate inferred.
- Compared all 13 changed paths. Source, inherited probes/oracles/fixtures/mutants,
  old readability checker/baseline and manifest are unchanged in this diff. The
  absent final manifest refresh is the explicitly assigned Root integration step,
  not an additional finding against this staged candidate.
- Inspected all 11 captured helper bodies and their target-forwarding records.
  Sonic's fourth parameter `a` is `SonicPlayerCall.address`; `b` is its call site.
  Other registered target indices match the named call records/invoke forwarding.
  The current 113 sites include 41 Sonic sites; these are new measurements, not
  source regressions. There is no numeric address/color/mask heuristic here.
- Existing fixtures discriminate first/later targets, decimals, aliases, variables,
  ternaries, multiline/nested calls, strings/comments, duplicate exact anchors,
  ordinary bodies/lambdas, receivers and lexical bounds. These pass but do not
  cover R2/R3. Macros, template-id callees, indirect calls, scopes/overloads and
  preprocessing remain lexical blind spots; no semantic completeness is approved.
- Inspected the 49 observed declaration spans and their direct consumers/usages.
  Current dispositions reconcile to 19 numeric encodings, 11 named structures,
  4 typed content, 14 open migrations and 1 open review. No present classification
  error was found. The scene_input_producer entry explicitly covers three objects
  in one declaration; 49 counts declarations, not all individual object names.
- Sonic operations are interpreted as ordered reads/stores, PC diagnostics,
  field offsets, resource slots, masks and partial initialization; keeping them
  open_migration is correct. Sonic recipes feed action-export/object-count decoding
  and contain typed content/FP encodings; open_review avoids falsely treating all
  their hex values as PCs or claiming that their import boundary is settled.
- Numeric rounding maps, sine coefficients, matrices, device enums, color shifts,
  button masks and source layout positions have direct numeric consumers. Named
  factory pointers are semantic C++ calls, and save/flag maps are typed mappings.
  Address-indexed constants, PC dispatch and DLL bindings remain explicitly open.
  Classification does not certify their surrounding modules or source fidelity.
- Declaration fingerprints retain exact strings, comments and initializer bytes;
  a newline before a declaration does not change the fingerprint. Existing tests
  catch new/changed/renamed/deleted recognized declarations and duplicate names.
  Parenthesized forms, consumer/type changes and retirement need the fixes above.
- Immutable-HEAD migration tests reject dirty/untracked increases, forged working
  allowances and repeated initialization; lowered committed ratchets cannot grow.
  The old ratchets remain independently enforced. No failure found in those tested
  paths; R1 concerns unusable policy, not successful allowance laundering.
- `check_judge.protected()` returns true for both reference registries/baselines,
  the signature registry, both checker implementations and both fixture files.
  Final hash-manifest generation and full integration remain Root-owned.

## Execution and evidence

Executed on the clean candidate:

- `python3 -m unittest discover -s tests/python -p test_address_dispatch.py -v`: 12 pass.
- Same command for `test_constexpr_tables.py`: 8 pass.
- Same command for `test_readability.py`: 17 pass.
- Same command for `test_judge.py`: 12 pass.
- Both new checker CLIs and old readability CLI: exit 0; 113 dispatch sites,
  23,463 unresolved tokens, 49 aggregate declarations, no current count drift.
- Protection predicate inspection and Git diff checks described above.

Executed in this review-only worktree:

`PYTHONDONTWRITEBYTECODE=1 python3 reference/production-integration-20261101/review-c306a21/reproduce.py`

Seven focused observation tests pass by reproducing R1–R7's unwanted behavior.
They are review evidence, not acceptance tests or fixes. Fixtures are temporary;
the maintained candidate is never edited. No source build, oracle, full gate,
journey, GPU/runtime test or final manifest qualification was run. Those remain
unverified here by explicit task ownership. No HANDOFF, STATUS or main edit.

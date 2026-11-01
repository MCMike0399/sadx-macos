# Fresh independent recheck of 6cba6f8

Recommendation: **BLOCK** for the three P2 findings below. The original seven
reproducers are corrected, but aggregate coverage remains incomplete and two fixes
introduce obstacles to subsequent ordinary refactoring.

Candidate: `6cba6f8599332ec5431a7e57bd1246450889cd06`, read and tested at
`/Users/burbujamc/Developer/sadx-wt-constexpr-table-review`.
Reviewed `c55762d..6cba6f8`, especially `c306a21..6cba6f8`, against the blocking
review committed as `9c9fb0f`. This is a fresh review task on
`packet/review-constexpr-dispatch-6cba6f8`; the prior review thread was not reused.
Standards and spec were examined by this reviewer without additional agents.

## Prioritized findings

### F1 / R4 — P2: Nested parentheses silently omit custom aggregate objects

`tools/gate/check_constexpr_tables.py:38`–41.

`declarations('constexpr Matrix identity(Matrix{{1.0f,0.0f}});')` returns one
fingerprinted candidate. Adding expression parentheses, as in the valid C++17
aggregate expression `constexpr Matrix identity((Matrix{{1.0f,0.0f}}));`, returns
an empty list without an ambiguity error. Changing the initializer also returns
an empty list. The early custom-type `continue` precedes balanced scanning and
never examines the braces inside the parentheses.

The exact prior array/custom examples are fixed, but this still violates the
requested distinction between supported function declarations and unsupported
ambiguous object forms. It needs no type resolution: balance the parenthesized
initializer and capture the braced custom value, or explicitly reject this form
for review. Do not require general C++ semantic completeness. Add a nested
parentheses regression alongside the direct parenthesized aggregate fixture.

Reproducer: `RecheckObservations.test_r4_nested_parenthesized_custom_aggregate_disappears`
in `reproduce.py`.

### F2 / R6 — P2: Immutable retirement receipts permanently freeze live successor files

`tools/gate/check_constexpr_tables.py:139` and 176–178.

A valid retirement records the whole live successor file's hash. After that
retirement is committed, even appending a comment to the successor file produces
`changed/invalid retired aggregate successor`. Updating its hash after a new
review makes reference validation pass but produces `retired review history
changed`, because the entire prior retired object must remain exactly equal.
Neither leaving the receipt alone nor revalidating it can pass. The same conflict
affects mutable files used as retirement evidence. This blocks further maintenance
of the first migrated system, contrary to PROMPT's complete-system campaign and
future maintainability requirement.

Keep the prior review/disposition and original retirement proof immutable, but
separate historical proof from the current successor validation, or provide an
append-only reviewed successor/revalidation chain. Both anti-laundering history
and subsequent legitimate edits need discriminating tests. This is a future
workflow defect demonstrated with a valid synthetic retirement; the candidate's
actual registry has no retired entries yet.

Reproducer: `RecheckObservations.test_r6_retired_successor_cannot_be_revalidated_after_edit`
in `reproduce.py`. It first passes both validators, edits the live successor,
observes the reference error, refreshes only the successor hash, and observes the
history error against the frozen previous registry. No Git-history mutation or
candidate edit is involved; `previous` is the same committed-registry input that
the CLI supplies to `review_problems`.

### F3 / R3 — P2: Ordinary logical AND is rejected as a new overload definition

`tools/gate/check_address_dispatch.py:108`–111.

The new qualifier guard matches any suffix beginning with `&`, including the
ordinary expression `bool ok = call(site,target) && ready;`. A registered bool
dispatcher with `bool ok = call(site,target);` has one accepted dispatch site;
adding the logical condition retains that site/count but raises `unregistered
overload definition`. The real registry includes bool-returning dispatch lambdas
in `mod_loader_frame`, `game_scene_logic` and `ninja_vibration`, so this is a
relevant supported expression shape, not a requested new dispatch allowance.

Discriminate ref-qualified declarations from call-expression operators before
rejecting them. Preserve the new const/noexcept/trailing-return failures and add
an ordinary `&&` call-expression regression. Parenthesizing the whole invocation
can work around this false positive, but the checker should not impose an
undocumented rewrite on ordinary calls to avoid declaring a nonexistent overload.

Reproducer: `RecheckObservations.test_r3_ordinary_logical_and_is_misidentified_as_definition`
in `reproduce.py`.

## R1–R7 verification

- **R1 fixed.** `increases` checks only proven dispatch counts. Existing unresolved
  candidate snapshots remain descriptive, validated data; no candidate-count
  allowance gates ordinary names. The CLI regression proves a new semantic module
  passes and an added known-signature dispatch fails. HEAD/working ratchets and
  immutable migration guards remain exercised.
- **R2 fixed for the stated bounded grammar.** A top-level `<` in registered
  arguments fails closed, including a template comma in an earlier argument.
  Parenthesized templates retain the entire target expression. Comparisons and
  left shifts also need parentheses under this intentionally conservative rule.
- **R3 original reproducers fixed; new false positive F3.** const/noexcept and
  trailing-return same-name definitions are rejected, as are the prior ordinary
  body/lambda cases. This is file/receiver spelling enforcement, not overload or
  scope resolution.
- **R4 partially fixed; F1 remains.** Direct parenthesized braced arrays/custom
  values are fingerprinted. The supported named-parameter function declaration
  is skipped; an ambiguous parenthesized array fails explicitly. Nested custom
  expression parentheses still disappear instead of failing or being reviewed.
- **R5 fixed for registered evidence.** All active evidence has whole-file
  fingerprints. In-range consumer changes invalidate it. Added external anchors
  cover graphics_frame, player_contact_initializer, player_rotation, model_dlls,
  sonic_player and both Sonic animation consumers; ground_acceleration_internal
  records the default initializer definitions. This does not discover arbitrary
  transitive dependencies or certify whole-module semantics. Current evidence
  hashes validate; classification counts and purposes remain unchanged.
- **R6 original forgery/accounting defects addressed, but F2 blocks maintenance.**
  Retirement preserves the exact prior review object; absent/fake evidence,
  missing successors and changed fingerprints fail reference validation. Named
  successor tokens are checked outside masked comments/strings. A justified
  removal requires a removal_proof evidence object. Active/historical totals,
  prior dispositions and outcome counts are printed. Historical retired entries
  cannot disappear or change. The printed unresolved count counts missing/unknown
  outcome tags only: an invalid successor with a known outcome still shows zero
  unresolved, although the CLI's problem count and exit status correctly fail.
- **R7 fixed.** The archive fixture validates its supplied baseline without Git,
  rejects an added known dispatch, and refuses writing/migration without Git.
  Existing Git migration/working-baseline/history fixtures continue to pass.

## Tests and scope

Executed directly against the clean maintained candidate with
`PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests/python -p NAME -v`:

- `test_address_dispatch.py`: 15 pass.
- `test_constexpr_tables.py`: 11 pass.
- `test_readability.py`: 17 pass.
- `test_judge.py`: 12 pass.

Both new checker CLIs and the old readability CLI exit 0: 113 proven dispatch
sites, 23,463 report-only unresolved tokens, 49 active/historical aggregate
declarations, zero current problems/increases. Aggregate dispositions reconcile
to 19 numeric encodings, 11 named structures, 4 typed content, 14 open migrations
and 1 open review. No current classification error was identified in this recheck.

Executed from this review packet:

`PYTHONDONTWRITEBYTECODE=1 python3 reference/production-integration-20261101/review-6cba6f8/reproduce.py`

All three observation tests pass by reproducing F1–F3. These are defect evidence,
not acceptance tests or candidate fixes. They use candidate imports and temporary
fixture files, never a second candidate checkout.

Read AGENTS, PROMPT, GOAL orientation, STYLE including section 12, JUDGE, STATUS,
latest HANDOFF, the prior review and `review-fixes.json`. Inspected the complete
changed-path list, new checker implementations, signature contracts, table review
registry, tests, Make integration and protection changes. Game source, inherited
numerical/route judges, probes, fixtures, tolerances, old readability code and
allowances, and the judge manifest are unchanged in `c55762d..6cba6f8`. Prototype
tests were deliberately corrected for R1's policy; this is not weakening an
inherited game judge.

Lexical limitations remain explicit: no preprocessing, macro expansion, general
C++ scope/type/overload resolution, indirect-call discovery or template-id callee
resolution. Dispatch parsing bounds spans/depth and matches evidenced file-local
contracts. Aggregate detection uses the inherited named-constexpr lexical prefix;
aliases, complex declarators and non-constexpr data still need inventory review.
Hashes make selected evidence changes visible; they do not prove its semantics.
Named successor validation proves a live identifier token and matching file hash,
not a compiler-resolved function definition. No semantic completeness is approved.

No candidate/game source, HANDOFF, STATUS, main or manifest was edited. No agents,
providers, builds, oracles, full gates or journeys were started. Root's separately
reported green main baseline at ee343ef and render evidence at e6c5a49 are context;
they do not qualify this candidate. Manifest refresh and final integrated gate
remain Root-owned after review approval. No candidate integration/full-gate claim.

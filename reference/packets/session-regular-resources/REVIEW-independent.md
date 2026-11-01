# Independent bounded regular resource owner review

Verdict: bounded approval. No actionable correctness or STYLE section 12 findings
in the six source files at 1bcd1b9 relative to e5cf19d.

Reviewed commits: 9d3938f (frozen native context/baseline), 8090d81 (source),
3ffb915 (qualification), and 1bcd1b9 (Root registration). Review performed in the
tool-provided isolated checkout, using Git objects for the candidate. Candidate
and Main source, build products, and Git state were not changed. This is a source
and evidence review; recorded executions below were not rerun by this reviewer.

## Source assessment

* `src/runtime/native_regular_resources.hpp:24` retains exactly the seven former
  Session members, in the same order and with the same initializers. The class
  has a NOTSA explanation and deletes copy/move operations. No second resource
  cache, character owner, GPU owner, or fixture-availability state was introduced.
* `src/runtime/native_regular_resources.cpp:4` retains the original attachment
  body through State allocation. Independent in-memory assertions against
  e5cf19d verify this after only substituting the live resync/language parameters.
  Bounds checks, group/catalog aliasing, zero scratch/cache initialization and
  partial stores remain ordered. In particular, failed pre-State attempts retain
  existing group entries and append again on retry; no clearing or rollback was
  added. Published State blocks retries even after Display/binding failure.
* `src/runtime/native_session.hpp:246` replaces the contiguous original block at
  its original location. Reverse destruction still destroys menu aliases before
  regular State/catalog/groups/lists/assets. State dies before localization
  storage, and loop/language outlive it. The extraction does not establish a new
  callback-after-Session lifetime guarantee.
* `src/runtime/native_session.cpp:99` forwards cleanup to the actual common and
  32 regular pointer slots. Character slots/loaded flags retain their owners.
  Lines 259–319 preserve regular loading, menu attachment, live language lookup,
  Japanese loading and callback wiring. State allocation remains BEFORE the
  Display check. The callback captures Session and reacquires services and State
  when invoked; it does not capture an attachment-time service snapshot.
* `src/runtime/native_title_tasks.cpp:462` retains the individual-load guard and
  `native_font_texture_loader` literal. `src/runtime/native_selection_preview.cpp:121`
  retains the named-load guard, and line 202 passes the same nullable State
  pointer to Gamma. All former seven-field consumers were searched; no stale
  Session member consumer remains. MenuTextureBindings borrows the same vector
  and the unchanged menu/cleanup algorithms consume the same underlying lists.
* Independent literal-multiset comparisons preserve every `native_*` error
  literal in all three changed consumer implementation files. No arithmetic,
  content table, recovered callee, address-shaped game logic, silent stub or
  enhancement policy changed. The new owner is a bounded native storage/binding
  seam; this does not certify the surrounding Session files as wholly conforming.

## Evidence and discrimination

Read PACKET.md, CONTEXT.md, SUMMARY.json, qualification.json, the frozen fixture,
new unit/checker, all four original reports, consumer compile receipts and Root
registration diff. Independent read-only assertions establish:

* Frozen attachment body equals e5cf19d verbatim; extracted owner body and actual
  Session transport suffix match that body under the documented substitutions.
* All nine source/test/fixture/checker hashes in SUMMARY.json match 1bcd1b9.
  The 55-line frozen observation file equals the recorded baseline stdout.
* The qualification receipt records successful strict baseline/current builds
  and executions. The unit compares complete observation vectors and explicitly
  asserts live resync/language references, common/regular slot identity, menu
  aliases, callback-time service changes, localization/cache lifetime and cleanup
  failure prefixes. These are observations of a reduced native host, not 55
  independently executed Session journeys.
* All six mutant builds record exit 0 and executions record assertion failure
  (-6): retry clearing, missing common binding, collapsed regular alias,
  missing duplicate guard, eager validation and nonzero scratch initialization.
  The protected frozen mutant receipt is byte-identical to qualification.json.
* Original regular609/menu1401/texture1416 reports record zero mismatches, with
  identical before/after counts, mismatch lists and source/range fingerprints.
  Their native-source hashes match the candidate. Cleanup2604 also records zero
  mismatches, including the unchanged 300-case reset judge. Cleanup is additional
  post-edit evidence; no pre-edit cleanup execution is inferred. All retain their
  resource/COM/upload/external-owner receipt limitations; no FP partial is hidden.
* Root adds the unit manifest row and protects the fixture/frozen mutation receipt.
  The manifest grows from 936 to 938 inputs; the only changed prior protected hash
  is the appended unit registration. Registration hashes match their Git blobs.
  Existing original judges, probes, tolerances and vectors are unchanged. The
  checker/mutation loop is an explicit local command, not automatic gate coverage;
  protection of the receipt is tamper evidence, not a fresh mutant execution.

## Approval limits and remaining Root work

No builds, gates, mutation loops, providers, network, storage transfers or delegates
were run. Focused checks were Git reads, hash/body/literal/report comparisons and
`git diff --check e5cf19d 1bcd1b9` (exit 0). The 12 consumer compile receipts were
inspected; they do not prove full native linkage or actual runtime behavior.

The synthetic Display/LogoRenderer spy and source-bound transport host prove a
bounded native extraction. Original Windows judges qualify unchanged lower game
and resource behavior; they do not execute this native import attachment or actual
Session wiring. Callback invocation after Session destruction remains outside the
existing contract. No GPU/pixel/Windows attachment equivalence is established.

Root retains full native source closure/force-load checks, actual native journeys,
the reserved post-review cohort, integrated original comparisons and the integrated
gate. This approval does not close all Session/resource ownership, a whole resource
module, the refactor campaign, or any new gameplay milestone.

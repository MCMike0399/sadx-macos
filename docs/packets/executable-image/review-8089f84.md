# Independent complete-module review: ExecutableImage

Reviewed 2026-11-01. Candidate `8089f8404152d79b3037d43f65171b5de65ea84b`
(`packet/executable-image-integration`), against Main
`1300815eae321d5de8e992958b2841f96ca47039`. Source is `fc4be98`, context
`f1de322`, fresh baseline `85def98`, judge registration `06e42d8`, worker
qualification `6373d72`. Review performed independently in the isolated
`t3/executable-image-review` checkout using committed Git objects.

**Verdict: approve the complete two-file ExecutableImage module within the limits
below. No actionable correctness, STYLE, spec or fidelity defect found.** This is
an authored review verdict, not a full gate or inventory update.

## Exact scope and identities

Both whole files, including unchanged public members and all private state, were
read against the base; this was not a changed-lines-only review.

| Member | Reviewed SHA-256 |
| --- | --- |
| `src/loader/executable_image.hpp` | `a445f239d4b4303c54ab321697eeb0bb06ff7c96352576298cfdf8f7a54315b6` |
| `src/loader/executable_image.cpp` | `3f376bb5c78c6a4a1d67162175ab459b144fe1e27849f9fae6acabdad7ac3f99` |

These identities match SUMMARY.json and qualification.json. The source commit
changes only these two files. Accepted evidence comprises PACKET.md, CONTEXT.md,
context.json, baseline.json, SUMMARY.json, qualification.json, pinned-models.json,
judge-registration.md, qualify.py, both complete new unit sources, the registered
mutant receipt and the actual registration/manifest diff. The existing model,
guest-import and guest-DLL tests and their immediate consumers were also inspected.

## Standards

No findings. The rewrite exposes named decoding steps with structured control
flow, narrow private helpers and a single value owner for bytes, section facts,
exports and directory metadata. Public signatures, nested records and return
types are retained. No owner or algorithm is displaced to another module.

The helper annotations explain the introduced boundary structure. PE offsets,
signatures, masks, RVAs and VAs are legitimate format values in this reference
loader, rather than recovered game addresses concealed in game-tier code. This
approval follows inspection of the implementation, not a blanket reference-purpose
exemption or a lexical metric. No speculative manager, general PE abstraction,
new variant support or invented allocation cap is introduced. STYLE sections 4,
7, 10 and 12 were applied with that reference-loader scope.

## Spec and fidelity

No findings. The following boundaries were checked adversarially by inspection:

- `contains` uses subtraction after checking the starting offset. File reads,
  section-table multiplication, RVA table indices, strings, directories and
  relocation targets use wide arithmetic before narrowing. All current private
  little-endian read widths are 1, 2 or 4, so shifts remain defined.
- Optional-header fields are dereferenced only after validating the complete
  declared extent and the required 64-byte prefix. Directory reads require their
  complete declared optional-header records. Raw section ranges and maximum
  raw/virtual extents are checked before access; empty raw sections avoid pointer
  arithmetic on arbitrary offsets. Exact mappings ending at 2^32 are admitted,
  while complete word reads and entry-point VAs cannot wrap through zero.
- Header mapping comes first. Byte lookup retains first-matching-section
  precedence; flattening retains ordered raw copies, so later overlapping
  sections overwrite earlier bytes. Zero tails and flattened gap zeroing remain
  distinct as before. These quirks are not normalized into a different mapping.
- Named exports retain case sensitivity, ordinal-table indexing, first insertion
  for duplicate names, direct VAs and forwarder strings. Shared RVA/string reads
  retain the existing 4,096-byte limit and distinguish absent bytes from zero.
  The code does not invent ordinal-only export support or require direct exports
  to be executable code.
- Import descriptor/thunk order, OriginalFirstThunk fallback, zero-sized-directory
  traversal, ordinal/hint/name fields and FirstThunk publication are preserved.
  IAT slots now require a complete mapped word. The inherited four-byte hint
  availability check is deliberately retained. A successful call does not clear
  the caller's existing error string.
- LB-1 now rejects the directory extending beyond the flat image before any
  optional relocation entry can be dereferenced. Complete directory/block/entry
  bounds, odd or partial entries, unsupported types and full target words are
  checked. Page-plus-offset arithmetic cannot wrap into another mapped word.
  HIGHLOW value-plus-delta arithmetic remains explicitly modulo 2^32, including
  downward rebasing. ABSOLUTE entries remain padding.
- Relocations still read the live mapped bytes in order; the rewrite does not
  predecode all entries or cache target words. The output keeps adjusted direct
  export VAs, unchanged forwarders and import/entry metadata. Relocation metadata
  remains absent from a flattened result, preserving the existing one-shot
  behavior when that result is rebased again.

Stricter rejection of malformed/truncated or overflowing inputs is within the
packet's explicit contract. This does not establish Windows acceptance rules.
No valid tested behavior was narrowed to conceal a mismatch.

## Evidence integrity and qualification limits

Read-only Git/hash inspection independently confirmed:

- baseline.json's old header/body identities match Main1300815, and its contract
  test identity matches `85def98`. That entire 224-line test remains byte-identical
  in the candidate. Its pre-rewrite receipt records 65,630 valid checks at exit 0
  and LB-1 at exit 1. It reports acceptance of malformed input, not a deterministic
  crash from the old optional precondition violation.
- qualification.json binds the exact seven recorded candidate inputs. Its later
  comparison reconstructs both the old header and implementation, avoiding a
  new-header/old-body hybrid. The 11 additional valid edge checks and the actual
  model-DLL comparisons are post-rewrite comparisons against immutable old code,
  not additional fresh pre-rewrite observations.
- The candidate receipts record 65,674 contract checks and 8,221 edge checks before
  the final CLI assertion. These are assertion totals, not distinct fixtures or
  paths. Both ASan/UBSan executions record exit 0 and no sanitizer diagnostics.
- All six fault variants record compile exit 0 and assertion-run exit 1:
  ignored directory bound, missing delta, wrong direct export, broken thunk
  fallback, wrong zero tail and wrapping relocation target. Their replacement
  anchors and failure messages match the inspected source/tests. Compile failure
  is not counted as mutation detection. The protected mutant receipt is
  byte-identical to qualification.json.
- The unchanged model_dlls.cpp comparison records exit 0 for old and candidate
  code over six actual DLLs with hashes/sizes, covering 85 bindings and HIGHLOW
  mapping. This is one test over six inputs, not six independent test programs or
  guest DLL execution. guest_imports.cpp and guest_dlls.cpp are also unchanged.
- Main's 943 protected paths are all retained in the candidate's 944-entry
  manifest. The sole new protected path is the frozen mutation receipt; the only
  changed inherited hashes are Makefile and the unit manifest. Both file changes
  preserve the complete old prefix and append the two explicit compiler recipes
  or test rows. There is no tolerance, existing expectation, fixture, corpus,
  harness or old mutant edit in the reviewed diff.

I did not execute tests, builds, sanitizers, mutation loops, oracles, gates,
providers, network/storage operations or delegates. The execution results above
are reviewed worker receipts, not newly reproduced results. All 53 inherited raw
`.bin` corpora are absent from this isolated checkout. The manifest comparison
proves retention of their entries, not a local filesystem/full-manifest pass or
independent verification of Root's restoration operation.

The approved owner is ExecutableImage only. Inspected consumers borrow its
immutable value facts: X86Guest builds a separate mutable mapping; HleRegistry
consumes imports/IAT addresses; HleModules parses/rebases and resolves exports;
ModelDllSet consumes mapped exports/data. Actual guest-imports, guest-dlls and
composite integration execution, source-bound inventory reconciliation and the
full gate remain Root-owned. No Main/candidate source, Git state or builds were
modified by this review.

This approval establishes neither Windows/general-PE equivalence nor resource
exhaustion handling, new runtime/journey qualification, whole Loader closure or
complete refactor campaign closure. Root's frozen Main130 gate is outside this
review and has no verdict here.

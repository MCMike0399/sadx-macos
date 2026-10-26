# Independent object/texture binding review — round 1

**APPROVE the bounded composition at `940dca3` against `3878f33`; no blocking
findings under STYLE §12 or the supplied composition requirements.** This is
approval to proceed to Root's combined verification, not approval of N3b,
Question/model pixels, GPU rendering, gameplay or frame-rate fidelity.

Review branch: `packet/review-object-texture-binding`, separate worktree
`t3-a6a8d508`. No prior review round. Read the actual runtime/header,
CharacterPack, engine, registration and test changes, not just the three
component approvals in `reference/production-integration-20261026/`.

## Composition and STYLE §12

`Display::texture_selection_state()` borrows current, requested, cached GBIX,
active memory and the existing palette state in the correct order. Requested
memory can change while equal GBIX preserves the previous active identity and
device image. `LogoRenderer::select_texture_list()` uses the queued wrapper;
Sprite's direct selection leaves queued list/index untouched. There is no new
pool, cache, palette owner or asset pack.

The weak bound-Surface observation does not own manager source bytes. Device
state independently retains the immutable image; refresh observes replacement
images on in-place upload without changing prior submitted Draw snapshots.
The explicit unbind resets both the weak observation and device image. Display
release/clear and colored-polygon cache unbinding use that path, preventing
refresh from resurrecting an explicitly cleared binding. Null palette binding
really clears the device; a present surface without an image throws before
cache/active stores. The binding unit exercises equal-ID aliasing, queued/direct
separation, in-place upload, old Draw retention and failure-prefix publication.
The manager-byte ownership conclusion comes from the weak/shared ownership
structure and existing lifetime contract, not a new runtime assertion here.

`ObjectState` publishes one stable mutable object per Object record in the
single imported CharacterPack namespace. Its optional entries represent actual
resource kinds. Session constructs it once; draw/cancel/reentry paths do not
reconstruct it. Traversal returns references, without per-draw object copies or
a second pack. Missing/non-object IDs fail at the resource boundary. Tests check
stable identity, live mutation, immutable imported payloads and invalid IDs;
actual post-constructor cancel/reentry remains for the combined journey run.

The native object Context borrows the actual Display matrix/FP owners, fog
flags/active byte, queued depth, pending logic ticks and signed lighting palette.
The simple-draw byte defaults to 1; I independently read `01` at `0x90a0a0`
from the local pinned PE. The unit compares that byte with Display's initial
value; the semantic role is recorded in `overrides.tsv`. The `408530` annotation
was removed from Question and appears once on the engine wrapper. Structured
traversal and reference state preserve callback-sensitive stores and aliasing;
no invented successful callee, rollback or guard was found.

Production model/late implementations live in a separate TU and throw named
unrecovered boundaries. `object_draw_binding` supplies link-time lower-callee
receipts only in its test executable. The native source list/Makefile and
force-loaded closure log include the production TU, not those test definitions.
Default/alternate object rotation remain loud seams. Question's imported
`0x17` flags skip the object's transform block; reaching model draw proves no
numeric Euler binding. Sprite selection remains the closed UI subset: full
`78d140` material/sampler behavior is explicitly next renderer recovery, not
certified by this review. Named arithmetic operations reuse existing matrix
implementations; no newly recovered numeric rotation body is being approved.

## Evidence audited and limits

All 16 composition source hashes match `native-object-texture-binding.json`.
All present judge-manifest file hashes and its evidence hash match. Protected
judge changes are registrations/additions, with no existing tolerance or input
narrowing. Read source/component evidence and Root logs: 683 legacy texture
cases; 3,669 texture dev/holdout; 4,256 gate dev/4,096 holdout; 2,918 object dev
and 5,918 holdout. Object dev/holdout raw reports have zero mismatches and match
the current probe library and recorded native source hashes. Object mutation
receipts distinguish 18 oracle detections from the sibling-alias unit detection.
All four runtime mutant edited-source hashes match, and their raw assertion
logs match the recorded compile-success/assertion-failure receipts.

Root's `build/runtime-bindings-closure.log` reports `Native closure: PASS`
(SHA-256 `33ec951850416a1a5bc9e757103c70f1357d028c36f1f66cf8d595bc41130038`).
This is an inspected execution receipt, not an independently rerun closure or
source-only export. The three native menu journey receipts report exit 0 but
predate the later ObjectState constructor/binding changes.

The final normal-input command, exit receipt and application log under Root's
`build/character-object-binding-final-frontier` agree with the evidence:
exit 1 at `native_basic_model_renderer`, frame 1382, 762 resident slots. The
1382 state and PPM hashes match. I did not independently display the PPM; the
author's recorded inspection says outgoing title, with no Question pixels.
Full combined gate, source-only export and final integrated journey checks
remain pending. Downstream FP partials remain partial, including the separately
pending Root late-sprite rerun; none is promoted to a strict pass here.

This reviewer ran only reads, hashes and a PE-byte read, plus the requested
review branch/artifact commit. No code edits, compilers, Ghidra, applications,
unit/oracle/mutant executions, full gates or workers ran. Approval does not
certify any unexecuted check or extend the existing component approvals into
rendering or N3b completion.

# Texture selection core — independent review, round 1

Approved for component integration: `c54386aefd835d41b0513f3cb9b162716ca6a6ce`
against `ca144825d81605947a6078e0f842e49c0c126ff3` on
`packet/texture-selection-core`, worktree `t3-6cfb337a`. No blocking findings.
This does not approve N3b completion or certify Root's runtime composition.

## Standards

Read AGENTS, GOAL, JUDGE and STYLE including §12; inspected the complete diff.
The selection, cache, palette-prefix and render-state branches remain in Engine.
Guest contains lazy fact validation, source diagnostics and device receipt adaptation;
its path discriminator selects diagnostics rather than moving source behavior there.
The private typed access adapters justify sharing one algorithm with the unchanged
missing-fact facade. No service hierarchy, registry or new resource owner was introduced.
State borrows canonical owners, including the existing palette owner. Named constexpr
hex values are formats/sentinels, not disguised addresses; integer wrapping is explicit.
The retained legacy DTO/header is unchanged, rather than claimed as newly readable code.
No masked mismatch, invented successful upload, or blocking over-abstraction found.

## Source fidelity and judge evidence

Independently matched all five dossier file hashes, evidence IDs, inclusive body ranges
and pseudocode to source-evidence.json: ev_0083aa55… (403070), ev_57670efc…
(77f3d0), ev_3c2c1114… (78cf20), ev_e23d9686… (77f440), ev_2cc132c0…
(78cdc0). Read original assembly, including the fresh palette/index dossiers.
Queued list then zero index precede direct selection; current then requested precede
cache. Advertised count is not a guard. Wrapped stride-12 aliases select entries;
cross-field reads fail explicitly. Equal GBIX preserves active identity. Null direct
surfaces skip binding; palette binding permits null. Callback completion precedes GBIX
then active-memory stores (78ce21/78ce27 and 78cf54/78cf5a), despite reversed palette
pseudocode. Captured IDs survive callback mutation; return format is reread afterwards.
Dirty palette revision precedes colors load, surface dereference and explicit unsupported
lock at 78ce5a. Known-null colors are not invented or faulted before their source use.

Verified legacy probe/verifier/header/unit bytes against base and their recorded SHA-256s.
Checked actual baseline/migrated 683-case reports and identical case blobs; checked raw
3,669-case dev and 3,669-case holdout reports, library hashes and implementation hashes.
Regenerated both seeded input hashes without executing an oracle, and matched all five
range hashes plus the four-result literal table directly against the pinned PE.
All 20 mutant edited sources and library hashes match their raw failing reports and
compile/judge receipts: 13 legacy, seven native. Witnesses include version_store case 307,
index_stride 649, queued_stores_after_call 237, native_cache_before_callback 21 and
native_wrapped_index 432. The verifier discards outcome only on incomplete executions;
it still compares boundary, owner stores, versions, calls and callback-time state.
All manifest content hashes match; only the allowed appended unit registration changes
an existing protected hash. Profile receipt covers five unique ranges with zero overlaps.

## Composition and separate preparation

The API fits Display's current/queued list, GBIX cache, active memory and palette owners;
Root must add requested-memory and queued-index storage once and borrow it. Bind the
actual Surface image to device_state_.texture, preserve null unbinding, and fail explicitly
for unavailable images. The inspected unit covers independent submitted Draw image retention
and callback mutation, but is not proof of actual app delivery. Root must exercise Question,
selection/cancel/reentry and texture lifetime, then downstream oracles and the full gate.

Reviewed judge-prep.patch separately: exactly three reference-link target registrations,
matching the pre-port ca14482 Makefile; no changed recipe, tolerance or expected input.
It is valid preparation before the port, in a separately evidenced fix(judge) commit.

No Ghidra, app, compiler, worker, unit executable, oracle, mutant or full gate was run by
this reviewer. Reported executions were audited through raw reports, binaries' hashes,
source comparisons and metadata, not independently reexecuted. Shared packet/root artifacts
were read only. Review artifact frozen on packet/review-texture-selection.

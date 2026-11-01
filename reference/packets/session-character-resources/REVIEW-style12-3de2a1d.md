# Independent STYLE §12 review: native character texture owner

Reviewed 2026-11-01. Candidate `3de2a1d`, base `92dbd07`, in isolated branch
`packet/review-character-resources-3de2a1d`. No actionable findings. Bounded approval.

## Complete new-owner module disposition

The complete `src/runtime/native_character_resources.hpp` and `.cpp` module conforms
to STYLE §12 for the native imported character texture storage/binding responsibility.
Both files were read in full, together with the three changed Session/preview files
against the base and their actual cleanup, telemetry, preview and Gamma consumers.
This disposition covers the entire new two-file owner, not merely its changed lines.
It does not close Session, the wider resource system, character actors, or the campaign.
Root still owns the final cohort, composite integration, manifest union and full gate.

The five fields are exactly the former contiguous Session block: copied archive
metadata, nine vectors of actual texture lists, cleanup pointer vectors, their
published nullable aliases, and nine signed `int8_t` loaded bytes. No second owner,
readiness/cache state, GPU allocation, actor state or new source global is introduced.
The native host structure is explicitly NOTSA; existing semantic field names and
types are retained. There are no address-shaped operations, emulator arithmetic,
invented source constants, reference dependencies or silent missing-callee paths in
the new module. Its arithmetic is ordinary container indexing; no FP operation moves.
Existing loader/release services remain the behavior seams; no speculative hook is
needed for a storage extraction.

Metadata copying remains at the original member-initialization point after LogoScene.
The list allocation/binding loop executes only through `initialize()` at the original
Session constructor-body point, after all later members exist and before frame-rate
writes. It is not executed by the owner constructor. The exact old loop preserves
character/declaration order, both error literals, list resize before entry validation,
entry resize before cleanup-pointer append, and publication after each complete
character. Later invalid declarations retain the same earlier mutation prefix.
The owner is explicitly noncopyable and nonmovable, including assignment. Member
order preserves destruction of published/cleanup views before lists and metadata.
Initialization remains a documented once-only constructor-body operation; no new
retry/rollback behavior is claimed or required.

Session cleanup and public telemetry borrow the same binding array and signed bytes.
PreviewServices remains local to each existing invocation, builds descriptors from
the retained strings and actual lists, retains its mismatch failure, and borrows the
loaded array. Gamma's CharacterTextureViews remains invocation-local and its catalog
borrows the same strings, lists and `loaded.data()`. The actual native Gamma binding
still calls `regular_textures::load_character`; cleanup still reads callback-mutated
counts and preserves failed-release prefixes. No lifetime or callback-after-Session
safety beyond the existing contract is asserted.

## Evidence independently checked

Read CONTEXT.md, context.json, baseline-source.json, native-baseline.txt, SUMMARY.json,
PACKET.md, qualification.json, gamma-consumer-identities.json, native-journeys.json,
the four original judge reports, frozen fixture, unit, checker and additional Gamma
observer. All six context source hashes match Git blobs at the stated base.

Read-only Python execution of `source_binding()` succeeds. Its exact frozen loop,
five source hashes and other binding fields equal the qualification receipt. The
fixture and 25-row baseline remain byte-identical to pre-source commit `c078948`;
both recorded baseline/current stdout streams equal that complete baseline. The
unit checks copied metadata, actual list/string/count alias identities, signed
extremes, six invalid declaration positions, callback mutations and failed release.
Its manifest row is present; the existing native-source wildcard includes the new cpp.

The six recorded mutants each compile with exit 0 and fail execution with exit -6.
Their diagnostics distinguish collapsed aliases, zero declarations, premature
publication, invented loaded state, omitted last character, and eager construction.
The eager mutation overrides the actual private header with an include path before
`-Isrc`, compiles both consumers against it, and fails the initial null-binding check.
These are recorded runs inspected here, not mutation runs performed by this reviewer.

The first source comparator repeated the implementation's overlapping substring
rewrite bug; it was not independent evidence that the original preview source was
valid. Commit `4d223fa` fixes both malformed `lists()loaded_` consumers. The corrected
checker uses identifier boundaries. In a read-only in-memory substitution, its
`source_binding()` rejects the historical `befe066` preview source and accepts the
current source. This specifically discriminates the earlier failure. The full build
log confirms the initial compile errors and exit 2; the corrected log/exit receipt
shows archive, app, force-load app link and native closure PASS, exit 0. This was an
incremental same-worktree rebuild with unchanged objects and pinned SDL reused, not
a cold build or proof supplied by the source comparator alone.

Recorded original comparisons are character textures 5,297 exact, preview 1,608 exact
and cleanup 2,604 exact. Original Gamma has 1,422 cases, zero value failures and 498 FP
partials, exit 2; the aggregate oracle make also exits 2. Gamma is not a pass.
The additional protected observer uses the unchanged fixtures/evaluator and compares
every returned word, all four metadata words and status before/current. Its recorded
full-output digests match, all 1,422 outputs match and all 498 distinct partial case
identities match. The evaluator hash agrees with the receipt, and the evaluator/probe
are unchanged against base. The receipt identifies the copied pre-character Main
probe; its baseline/current library hashes are identical. This lower-consumer check
does not itself exercise Session construction or qualify actor/render behavior.

The actual native journey receipt records exit 0 for file select, main menu and fresh
save, including restart, on corrected source `4d223fa`. The corresponding source files
are unchanged through `3de2a1d`. These are inspected execution receipts, not newly run
journeys or a new visual/gameplay assessment by this reviewer.

The candidate manifest contains 941 inputs versus base/Main's 938: three additions,
no removed entries, and only the unit manifest hash changes among prior entries.
All 888 locally present protected files hash correctly. The 53 ignored oracle-report
shards are absent in this isolated checkout. No local full-manifest hash pass is
claimed, and no protected expectation, tolerance or original judge was modified.

## Limits and handback

Only read-only source/evidence/Python checks were executed for this review. No builds,
mutation loops, oracles, gates, providers, network, storage operations or delegates
were run. Candidate/Main state and active artifacts were not altered. Recorded
readability evidence reports zero regressions; no new lexical or integration gate
was run here. The initial baseline command failures and first full-build failure
remain explicit in the packet and were not treated as successes.

This is approval of a bounded native ownership extraction from a frozen native
baseline. It is not Windows constructor parity, whole resource/Session conformance,
actor/GPU/N3b/runtime completion, new controller qualification, gameplay or campaign
completion. Root must perform the remaining final cohort/integration/union/gate work.

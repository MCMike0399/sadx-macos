# Independent music owner integration review

Candidate `5a2fe5dfd79d3469b9cb6c9b02fb0b7f1dd7e022`, base
`73be9a5c4217157a05c11fdd4f875d98877e16d4`. Reviewed read-only in
`t3-d8ad865d`; this report is authored only in `t3-f10c80be`.
Source cohort `8a37d10`, bindings `622a525`, qualification `7bc2aa3`,
Root registration `5a2fe5d`. No candidate or Main edits.

## Standards finding

**P2 — Keep unchecked track storage private to the reference adapter.**
`src/guest/music_state_reference.hpp:14–15` publicly returns the entire plain
TrackState without checking any of its six private availability bits. The
default-constructed adapter therefore exposes `current_song == -1` and
`song_queue_state == 0` despite knowing neither fact. More concretely, import
song 42, then `import_words({})`: `export_words()[0]` is absent, but
`tracks().current_song` still returns 42. A caller can pass `tracks()` directly
to `music::query_music_state` and obtain a fabricated known queue result. A
write through the mutable accessor also changes storage without establishing
availability, so `tracks().current_song = 42` on an unknown field remains absent
on export. The API provides no checked read/write contract or documented
precondition to distinguish these cases.

This violates the explicit-unknown boundary rule (AGENTS.md, Tagged results,
explicit unknowns; STYLE.md sections 1 and 5). Correct export filtering does not
protect reads through another public interface. Current maintained uses of
`tracks()` are confined to the new unit test, including reads after clearing
availability; no current production regression through this accessor is alleged.
NativeGame and the composed probe use dispatch/import/export correctly.

Remove both public accessors and use a private test friend/probe for storage
preservation/live-export characterization. That is sufficient for current
production consumers; a future behavior reader should validate only the facts
it consumes at its reference boundary. Do not require all six facts before the
unconditional partial resets, and do not mark missing defaults known.

## Spec and fidelity assessment

No further behavioral or ownership finding in the scoped diff. The six annotated
4-byte track words retain native defaults, signed high bits, extents and source
identity. The adapter retains one TrackState and six bits, clears only knowledge
on absent reimport, and exports live values with absence preserved. Dispatch
intercepts only `4257c0`/`4257e0`, calls the existing four/two-store cleanup
functions in original order, then marks exactly those fields known. Other
entries retain legacy results, nested failures and optional EAX. Six legacy
owning optionals and two duplicated bodies are removed; no other algorithm is
relocated and no game-tier dependency points into guest.

SchedulingState retains the old manager member position. Its manager is destroyed
before its timing/tracks; all three remain after the services and sound task
array, before the bank owners. PCM still borrows exactly
`sound_.stream_initialized`. Native consumers borrow the same subobjects and
live manager handle. Existing manager initializer six-store order, destructor
five-store order, preserved real_previous_song, close-media order, callback
publication, request-slot distinction and scheduler callback lifetime are intact.
No reference timing/task producer is invented.

## Independently checked observations

Read the context, SDK/symbol mapping, source evidence, baseline, qualification,
judge bindings, unchanged-logic claims, partial/boundary ledgers, mutant intent
and receipts, exact 13-file source diff and affected consumer/test/probe code.
Read-only hashing/decoding confirmed:

- All 13 current source hashes and all 19 base-source hashes match their commits.
- All 193 local receipt files exist and match size and SHA256.
- All seven baseline/current streams are byte-identical and match qualification
  hashes. Standalone reset is under baseline-reset/qualification, not baseline/current.
- Reconstructed every composed-reset output identity from the captured page and
  200-word result arrays: all 300 match baseline/current/qualification, including
  50 real `41010d` boundaries. The two reset entries are real source/native leaves,
  not fixture trace calls. Trace/page/failure prefixes remain in the full streams.
- All 270 title partial identities match baseline/current/qualification/tool capture.
  Title remains exit 1, zero value/call mismatches, FP partial. Music retains
  1,299 complete executions plus two duration[-1] bounded prefixes out of 1,301.
- Five original judge ASTs match base after normalizing only maintained-source
  fingerprint lists. Input generation, expectations, seeds and tolerances are intact.
- All 24 historical qualified mutant receipts equal the pre-edit baseline, with
  nonzero mismatch counts. Twelve new ownership/order receipts match their local
  summary and recorded assertion failures; compilation failures are not kills.
- Candidate manifest contains all 938 parent paths plus one new frozen mutant
  receipt; all 939 current hashes verify, including the 53 restored corpora.
  The two unit execution rows are additive. Final Main union remains Root-owned.
- Pinned PE fingerprint, all five recorded instruction-range hashes and four
  file-backed initial words match the actual local executable. Five BSS words
  remain non-file-backed as recorded. Inverse replacement of only the three
  scheduling subobject spellings restores the entire title-task file to base.

Six unit executions, 24 historical kills, 12 new kills and native-only scheduling
linkage are inspected existing receipts, not newly executed tests. The two private
rotation table fingerprint failures are inherited, outside this source delta.
No builds, oracles, mutation loops, gates, providers, network, storage operations
or delegates were run. No reserved holdout was consumed.

## Exact module disposition and limits

The complete new header-only game module `src/game/music_state.hpp` (44 lines,
SHA256 `cdad5447039cb43db11ca771d2f25f7c5e59c2c8d155cabc1d03c513f882ffe2`)
is **already_conforming within this declaration/owner scope**. Its complete
membership is this one header: semantic plain track/timing values, the existing
TaskHandle type and a justified SchedulingState grouping; all addresses are
provenance annotations. Owners and accepted consumers are the scoped native
Session/title/music/menu-voice/cleanup paths and reference reset adapter above.
Accepted evidence is the unchanged seven-stream cohort, source bytes, availability
tests, lifetime tests and qualified mutations. This does not classify the broader
music algorithm module or close reference-adapter API conformance.

The new reference module `music_state_reference.{hpp,cpp}` needs the P2 API fix
before unconditional approval. The implemented production dispatch/ownership
slice is otherwise approved on the inspected evidence. Root retains reserved
holdout, additive character integration, final manifest union, actual native
product/archive/force-load closure, journeys and coherent gate. No whole audio,
Session, native gameplay, platform parity, race/performance or campaign closure.

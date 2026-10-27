# Independent ordinary character texture core review

Bounded approval: no new STYLE or fidelity blockers found in `8052fac9fffd2546226dbaa6ef3339f6e3632dbd`
against `afc3e94fa4caf570607dc3ba8b86081c4b398a83`. First review round.
This approves recovery of ordinary `LoadCharTextures` (`421280..4212d8`), subject to the
stated borrowed-view contract. It does not approve runtime integration or N3b completion.

Tests NOT RERUN. No builds, tests, oracles, mutants, apps, gate, Ghidra, providers or
subagents were run. Only source, saved evidence and receipts were read; hashes were
calculated. The review branch starts at Root Main `6665718`. Only this review directory
is authored. Nothing is written to Main or the candidate worktree. No push.

## Standards

No new blockers. AGENTS, GOAL, STYLE (including §12), STATUS, HANDOFF and JUDGE were read.
The two new game files are short, plain named C++ with structured control flow, direct
lower calls and annotations. Saved proof reports zero readability debt in every metric;
static inspection agrees, without a checker rerun. There is no address dispatch, PE read,
new owner, Services type, vtable, runtime registration or relocation into a reference tier.
`State` and `Services` reuse the existing regular texture system. Catalog/table/list/text
storage is borrowed. `optional<string_view>` represents the source's actual NULL filename,
not missing fixture facts. Named Amy ID 5 matches the SDK. The explicit NOTSA bounds guard
protects the native imported view; it is not claimed as original invalid-index behavior.

The Amy callee is an unresolved named link-time STUB. Test/probe receipts are confined to
the test tier; no fake game implementation exists. Integer arithmetic is sufficient: the
explicit 127-to-minus-128 branch and subsequent increments implement live byte INC modulo
256 without signed overflow. There is no FP work or need for an arithmetic abstraction.
Existing seams permit alternate resource delivery without changing this wrapper; no
speculative manager or enhancement hook was added. No platform replacement occurs here.

Known integration obligation, not a newly discovered blocker: canonical FUNCTION `421280`
in `src/game/character_textures.hpp:32` duplicates the frozen STUB annotation in
`src/player/gamma_selection_core.hpp:119`. Saved provenance status is honestly 1. Root
must move/remove the obsolete integration annotation, preserving the checker and Gamma
behavior. This review neither reruns nor weakens provenance validation.

## Spec and source behavior

No new blockers. Complete source evidence
`ev_49279b700b5fec055393ce16c7aa84e0ef6d683b686745a628bcc0a54435447b` is present.
The raw dossier hash matches; its procedure, complete 89-byte assembly, pseudocode,
callees and native-value-flow completeness match the retained summary. The wrapper:

- Captures the table once; a NULL table returns without incrementing.
- Treats exactly zero as cold. Negative loaded bytes are warm.
- Dispatches named entries through profile-active `421180` → `LoadPVM_r`, represented by
  maintained `load_archive`; nameless entries use original `420d80` `load_no_name`.
  Neither selection `512550` nor its separate `4228e0` path is substituted.
- Reads each next descriptor after the lower callback, retaining the captured table even
  when the catalog slot is rebound. It neither prefetches entries nor caches the counter.
- Calls Amy extra `4b3da0` only on cold ID 5, including a present empty table; never warm.
- Increments the live borrowed byte after successful lower calls. A thrown lower failure
  preserves prior callback mutations and prevents the final increment.

The count/end adaptation is explicit in the header and PACKET. NULL entries means absent;
nonnull entries with count zero means present empty. The unchanged importer captures all
nine original tables through the NULL TexList sentinel and byte-round-trips the descriptors
including that sentinel. Its existing local provenance receipt confirms descriptor counts
2/1/1/2/1/2/5/3/1, with 24/16/16/24/16/24/48/32/16 source bytes respectively. Thus an
imported count can represent the omitted sentinel. Callbacks must preserve termination
inside capacity or at its implicit end; arbitrary extension beyond capacity is outside this
bounded API contract. The native diagnostic's future adapter must preserve that contract.

## Saved judge evidence

All six source/probe/tool hashes in proof.json match. Both saved build reports equal their
committed receipts and name the existing probe binary's matching SHA-256. Recorded results
are 5,297 development and 4,096 holdout cases, zero value mismatches, strict true and zero
FP partials. Holdout execution and its untouched chronology were not independently rerun.

The new oracle executes all original wrapper bytes, pins executable SHA and Unicorn
version, and compares complete output buffers including ordered lower receipts, all nine
counter bytes, catalog rebinding and descriptor identities. Its mutation hooks exercise
live byte changes, next-descriptor changes and lower-call failure prefixes. It is an upper
wrapper judge, not proof of lower algorithms, Amy behavior, archive payloads or GPU work.
The C++17 unit source and recorded compile command link real `regular_textures.cpp`;
only Amy has a unit test receipt definition. The differential probe deliberately substitutes
lower-call receipts. Those are distinct verification scopes, accurately reported.

All ten saved compiled-mutant sources match their summary hashes. Their detailed reports
contain actual value/receipt mismatches (4 or 8 each), and their first mismatch cases match
the summary. No crash or compile failure is being counted as a kill by the runner's logic.
The diff changes no existing lower implementation, oracle, probe, fixture, mutant,
expectation, tolerance, checker or judge manifest. Saved profile receipt reports zero
unhandled wrapper overlaps; the profile table confirms the named lower hook.

## Root handoff and limits

Root still owns runtime binding to canonical resources/counters, fail-loud native Amy seam,
source/test/probe registration and additions-only judge registration, the annotation move,
focused integration checks, source-only build, next normal-input diagnostic/image inspection
and full gate. The last full gate at `afc3e94` remains the prior CCL-stop qualification;
this review supplies no newer gate. Root `6665718` reaches
`native_gamma_load_character_textures`, frame 1382, diagnostic exit 1, per the supplied
brief and HANDOFF, not an app run by this reviewer. No actor, GPU, N3b, story or gameplay
qualification follows from this approval. Machine-readable audit details: `evidence.json`.

Standards: zero new findings, one known provenance integration obligation.
Spec: zero new findings within the stated wrapper and capacity contract.

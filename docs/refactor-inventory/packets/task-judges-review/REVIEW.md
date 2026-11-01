# Independent before-Source task judge review

Verdict: **bounded judge acceptance**, with no blocking finding for the
before-Source prerequisite. This accepts the frozen baseline and named development
contracts at `a89ebcd8f1a9eaeab590331190d8af5b3b717963`; it does not accept a Source
rewrite, integration, whole Task/player module, or campaign conformance.

Reviewed commits: `20a4cc1`, `d609cac`, `f511e9a`, `a89ebcd`, against immutable
`99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3`. Evidence was read in
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-686afca5`, branch
`packet/task-judges`. The report alone is authored in the assigned isolated review
worktree. TaskActor review `5f5c17f` remains context, with its original dispositions.

## Independently checked integrity

The following are fresh read-only checks by this reviewer, not new executions of
the native tests or original judges:

* All 650 `source-freeze.json` hashes match immutable base Git blobs and packet
  worktree source. The reviewed range has no `src/` diff or Source commit.
* The inherited ledger exactly equals the base manifest's 946 paths and hashes.
  The new manifest has 953 paths, no removal, and only two changed inherited
  hashes: Makefile and unit registration. Their diffs append the two new tests;
  old rules, fixtures, expectations, tolerances, oracle bodies, and raw corpora
  are unchanged. All 953 files are locally present and hash-exact **in the packet
  worktree**. This is not a full-manifest execution claim for the review checkout.
* All nine qualification bindings match, including the two one-line unit wrappers
  and their protected probe headers. The seven new manifest entries protect those
  headers and five tools; wrapper hashes additionally live in QUALIFICATION.
* All 601 dependency hashes, 288 object hashes, three archives, 22 logs and twelve
  link maps match their transitive ledger locally. The raw Clang AST and the
  immutable `5f5c17f` review blob also match. Parsed scheduler declarations contain
  eleven fields and 59 method entries; WalkGuard has one field and three entries.
  These are declaration/dependency evidence, not behavioral coverage of every
  archive member, SDK header, field or method.
* All four compressed native stream hashes match. Every record's symbol, result,
  call identity and every before/after buffer's decoded length and SHA-256 match
  its committed ledger. Uncompressed stream hashes match `comparator-self.json`;
  all four retained probe libraries match their recorded hashes.
* All seventeen copied-fault source hashes reproduce the recorded substitution
  into immutable base task.cpp or hle_task_bridge.cpp. No mutation was executed.

All 53 inherited ignored raw corpus inputs remain present with exact manifest
hashes in that packet worktree. Historical work and current work are the same
39,184 bytes, 16 sequences and 4,890 operations. Historical lifecycle remains
36,900 bytes / 17 sequences / 2,302 operations; current lifecycle remains distinct
37,132 bytes / 19 sequences / 2,316 operations. I decoded both generations' raw
record counts. Historical summaries have not become regenerated goldens.

## What the judges actually establish

| Frozen judge | Stored original comparison | Complete native capture |
| --- | --- | --- |
| task_work | exit 0; 16 sequences, 4,890 operations | 16 ABI records |
| task_lifecycle | exit 0; 19 sequences, 2,316 operations | 40 ABI records including callback traces and inactive boundary |
| task_display | exit 0; 236 outcomes: 179 complete, 55 callback/material failures, two saved-next boundaries | 236 ABI records |
| task_delete | exit 0; 244 compared of 250 attempted | 244 ABI records; six excluded source inputs separately preserved |

The lifecycle inactive experiment stops on entry to actual `0x401000`: unchanged
arena, retained work, unretired task and explicit native failure. It observes no
logger/CRT completion or source return. Display's 236 outcomes are not all
successful source walks. Delete excludes exactly 85, 87, 91, 121, 143 and 201.
I checked their historical IDs/reasons/retained input prefixes against current
complete vectors, and decoded all six separately captured 212,992-byte pools
against their hashes and register receipts. Reentrancy is a receipt boundary;
case 121's cycle is detected by serialization after execution. Neither becomes
a successful source/native comparison. There is no new hidden exclusion in this
packet's unchanged oracle logic or stored cohorts.

Full native buffer preservation includes padding and unused output capacity.
The source claim remains the unchanged oracle's actual comparison: encoded
graphs/contexts, bounded callback receipts, traces and arena bytes. Native capture
does not turn unspecified/void EAX, hardware flags, arbitrary callback behavior,
unknown pointer bytes or every captured byte into source-qualified behavior.

## Contracts, consumers and discrimination

The protected scheduler contract and unchanged scheduler agree on the named
development cases: unavailable default versus known-null; real slot zero versus
null links; pool failure publication; callback context registration, retention and
cross-world isolation; callback replacement and saved-next generation reuse;
graph-replaced dispatch/lifetime continuations; destructor-mutated sibling capture
and callback clearing; source callback failures at calls 1–3; distinct host exception
context rules; destructor failure prefixes at children 1–3 and retry; cleanup reads
at calls 1–2 with explicit inactive retry/resume; payload writes at calls 1–4;
native auxiliary release order; nonowning links; nested WalkGuard reclamation.
Twenty emitted observations accompany assertions; this is a bounded development
matrix, not exhaustive scheduler failure coverage.

The bridge header invokes actual HleProcess/task overrides. It checks lazy invalid
index/zero-free exits, absent arena/default metadata, unreadable pool publication
and an interior write failure at `0x40b4bd`, emitting the accessible first-page
prefix. It does not establish atomic failure. Actual source transport still owns
guest reads, graph reconciliation and callback resumption; no copied scheduler or
actor-state refresh framework replaces it.

Stored fresh receipts cover the five critical old task units, actual historical
raw HLE bridge replay (18 dispatch, six lifetime scenarios, twelve built-in input
faults), graph-sync/inactive-lifetime, actor-current/player-task and actual
NativeGame startup plus three survey frames. NativeGame still reports five
unported owners. Those assertions exercise existing routing/facts, not every
private scheduler state or a completed native game journey.

Fourteen task faults and three actual bridge faults compiled and were caught in
the recorded runs. Three historical lifetime intents are reconstructed from the
old journals and caught by the unchanged current lifecycle judge; their historical
counts are not reused as current runs. Canonical-argument retention is caught by
a crash in the old owning-handle regression, not a successful sanitizer run.
Initial skip-unlink-state survival is retained, and the added linked-state assertion
then catches it. The initial omitted player_task link, generic bridge recipe,
misaligned heap fixture, saved-next fixture shape and incorrect
cleanup-read boundary are disclosed development failures/corrections. None changes
the old judge or maintained source; invalid compilation is not counted as a fault.

Animation's arena interior-reference and animation allocation characterization
remains independently owned. This packet pins task_work as a dependency and
retains its old judge; it neither duplicates that Source work nor qualifies the
animation rewrite.

## Comparator and acceptance boundaries

I inspected the comparator and ran six entirely in-memory adversarial checks:
unchanged records return 0; changed padding, scalar return, scalar argument,
missing record and changed exclusion cohort each return 1. Every JSON ABI field
and encoded byte participates; there is no tolerance or deliberate output mask.
Stored tamper and self-check receipts are consistent with this behavior.

The comparator is deliberately narrower than an acceptance runner. It trusts
its baseline, compares only `source_unsupported` from reports, and does not itself
check oracle exit codes, source/probe provenance, source outcomes or the new
contracts' stdout. `check_baseline.py` checks old source and inherited inputs,
but iterates the supplied ledgers rather than proving their complete key sets,
and does not verify all transitive artifacts or new qualification bindings. My
independent checks above establish those sets/hashes for this exact packet.
Neither command alone is sufficient to accept a later candidate.

The mutation runners intentionally read immutable **old** Git bodies and reject
different current bodies. Their recorded discrimination is valid before Source;
running them unchanged later would not mutate or qualify the rewrite. Preserve
them and their receipts; candidate fault qualification must be a separate,
reviewed operation with unchanged fault intent and expectations.

## Required Source prerequisites and later acceptance

Before editing task Source, Root must accept this exact baseline, retain its full
streams/libraries/raw inputs, and reconcile the additive parent manifest union
without losing newer Main paths. Check complete key sets as well as hashes, all
nine qualification bindings, and the transitive closure; absent local raw files
must remain explicit missingness. Coordinate the task/animation owner seam before
changing shared arena or canonical task payload interfaces.

For candidate acceptance Root must:

1. Freeze the candidate Source commit and rebuild probes, actual consumers and
   contracts from its complete maintained dependency closure. Record candidate
   hashes separately; do not rebind or overwrite the old expected baseline.
2. Run all four unchanged original judges successfully, compare every full native
   stream to the pinned old streams, and independently check counts, outcome IDs,
   six exclusions, complete vectors, inactive boundary, both historical/current
   corpora and new contract **full stdout** as well as assertions/exit status.
   In particular, preserve the emitted bridge partial-write prefix byte for byte.
3. Demonstrate equivalent compiled faults against the candidate implementation,
   including historical intents, callback context, generation reuse, publication,
   failure retention, linked state, auxiliary order and canonical handle lifetime.
   Compile failures/timeouts are not discrimination; old-body reruns do not count.
4. Obtain fresh independent Source review, then run the reserved final independent
   holdout after Source freeze. **Holdout remains UNRUN in this review.** Run
   actual composed HLE/NativeGame/native consumers and Root's coherent integration
   gate, preserving inherited partial/failure identities and reporting limits.

Task scheduling/lifecycle behavior must remain in its game system, with reference
facts at the adapter boundary; moving it into HLE is not a conforming rewrite.
Source table/control-flow, ownership, readable interfaces and complete consumer
integration still require their own review. Unchecked cleanup-chain/metadata
callback combinations, corruption/reentrancy, Windows heap/hardware parity,
animation allocation, arbitrary callbacks, complete NativeGame state, player,
product journeys and campaign completion remain open.

No build, native execution, oracle, mutation loop, sanitizer, gate, provider,
network/storage operation or delegate was run for this review. No Main/candidate
source, Git state, build artifact, STATUS, HANDOFF or canonical memory was changed.
The frozen Main MPEG gate `35783` at `8c9f540` was neither touched nor polled.

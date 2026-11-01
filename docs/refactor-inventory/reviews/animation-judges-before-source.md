# Independent animation judge acceptance review

Reviewed 2026-11-01, candidate `2bb83b16b2f808c344b2ad54f874cec57851efc5`
on `packet/animation-judges-integration`, in worktree `t3-7dfc86b6`.
Worker Source base: `99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3`.
History reviewed: `e15d249`, `a4d6de5`, `6555772`, Root `5330b73`, `2bb83b1`.
Review checkout: `t3-517d041e`, branch `t3/animation-judges-review`.

**Conditional judge acceptance.** This is an adequate frozen foundation for the
complete shared animation table/resource/motion-state Source boundary described
in `context-animation.md`, subject to the documentation correction below and
Root's separate integration seal. No blocking algorithm, expectation-independence
or fixture-weakening defect was found in the reviewed judge changes. This is not
approval of a Source port, module, system, inventory disposition or campaign.

## Priority finding

**P2 — Reconcile the current acceptance narrative with Root's Sonic correction.**
`PACKET.md:36–40` still says there was an evidenced observer repair and that Sonic
has only a self-comparison. `MATRIX.md:22` says the unchanged Sonic unit passed
after an observer-only repair. These statements are stale at the reviewed head.
`verification.json:2–3` also retains the earlier 382-file / 395-artifact inventory
without identifying it as an earlier receipt; the current integrity run reports
383 / 411. These are documentation/evidence-version defects, not stream failures.

Bounded read-only reproduction: compare those lines with `OBSERVER-FIX.md`,
`root-frozen-sonic-comparison.json`, the Sonic entry's two compiler commands in
`current-baseline.json`, and the two distinct raw streams:
`build/animation-judges/sonic_player/cases.jsonl.gz` and
`build/animation-judges/baseline/sonic_player/cases.jsonl.gz`.
Their decompressed bytes are identical across 24,801,598 bytes and 28,800 cases,
SHA-256 `2833d480dec1c99bef187812d6718dcd7acb9f4c51db43cbe29be2fe794382d9`.
The current library is `root-frozen/sonic_player.dylib`, with the base probe
compiled separately using the existing forced include. Base Makefile lines
456–462 already provide this setup. There was no demonstrated registered-build
defect, and e15d249's optional-handle rewrite is unnecessary for integration.

Before final acceptance, update the prose to distinguish the earlier private
observer/unit receipts from Root's fresh frozen-probe capture and actual
different-observer equality. Do not promote the old unit receipt into a fresh
execution of the restored frozen unit. Label or refresh the earlier integrity
receipt. Preserve historical failed setup logs and the original blend failure.
This is a small Root documentation correction; no observer rewrite is requested.

## Standards and evidence checks

Maintained `src/`, the frozen Sonic probe/unit, all six original verifiers and the
inherited judge manifest are byte-identical to base. The only inherited protected
path whose content differs is `tests/unit/manifest.tsv`; its complete old prefix
is intact and exactly two registrations are appended. All 946 inherited paths
exist; all other inherited hashes match, including all 53 ignored binary corpora.
The unsealed new protected tools are intentional pending work, not a global
judge pass. No source/judge mixing or tolerance, seed, cohort or old-expectation
weakening was found in the final diff.

Root's build helper applies `actor_current_fixture.hpp` only when compiling the
Sonic probe object. The subsequent command compiles the maintained Source TUs
without that macro and links private source-built support archives. The fixture
is in the current closure. The corrected setup and Root's failed initial seal
attempts are explicitly preserved; they are not successful qualification runs.

`verify_packet.py --output /dev/stdout`, run with Python bytecode writes disabled,
exited 0 in the candidate checkout. It verified 383 closure files, 411 local
artifacts, the identity indexes and recorded results. I separately checked the
inherited hashes, old registration prefix, frozen probe/unit/verifier bytes and
the Root Sonic streams. I parsed all six raw captures and reconstructed every
FP-partial `(case, input digest, source flags, native flags)` tuple; the complete
ordered tuples match the tracked identity parts exactly.

| Frozen cohort | Cases | Individual FP partials | Scope |
|---|---:|---:|---|
| Sonic table | 103 | 0 | Exact stock table baseline |
| Motion update | 16,440 | 4,140 | Values exact; partial |
| Transition blend | 7,944 | 1,668 | Values exact; partial |
| Sonic | 28,800 | 2,840 | Bounded control/ground consumers; partial |
| Startup | 2,004 | 924 | Allocation/initialization consumers; partial |
| Gamma | 1,422 | 498 | Constructor/common tail; partial, verifier exit 2 |

Motion's 432, Sonic's 12,168 and startup's 2,004 native failure/boundary identities
remain pinned; Gamma retains 468 native boundaries. Those diagnostics do not
establish successful full animation execution. Value-only verifier exit 0 is not
a strict FP pass. The earlier failed blend report remains historical failure.

## Specification and independence assessment

The 8,727-case table judge derives expected writes from hooks on the pinned
original instructions, rather than from the generated recipe or native output.
It independently compares operation PCs, read ordinals, value slots, row offsets,
write widths and scalar bits. Only `operation.address` denotes an instruction PC;
the other metadata retain their documented semantic meanings. The 147 rows,
145 export reads, 882 stores, 149-slot input, literal nulls, zero source table,
retained old field masks, repeated identities and reached missing reads are
distinguished. Its zero-incomplete-new-row observation is asserted at every read.
Missing fixture availability is explicitly a prefix projection, not execution
of absent Windows memory. The existing retry unit adds repeated-call coverage
to the seeded-old-row characterization.

The 114 interface cases are appropriate discrimination for lazy reached facts,
unknown/null/unresolved resources, earlier state/header/output publications,
live 138/13c aliases, nested failures, all channel families, hierarchy repetitions
and the 111/112 capacity boundary. Source-order experiments and the stored
44a800 assembly support action-before-object/motion/data publication and null
malloc fault PCs. Generic unavailable headers, source malloc partial headers,
successful player calloc zero/null and experimental A5 fill are kept distinct.
Free/null-count and channel/data/motion/action order receive explicit checks.
The existing real-DLL unit plus original blend oracle retain outgoing MDATA3's
raw scale-pointer count, adjacent backing and declared-count distinction.
Some interface diagnostic PCs summarize several original reads; that existing
adapter contract is honestly identified rather than asserted to be a fault PC.

The 91-row Gamma stream preserves all scalar metadata and resource statuses,
78 declarations, export 61 known-null, request 0 Stand and constructed row 72
unavailable with its reason. Its JSON representation reconstructs the raw stream
losslessly. Assertions comparing imported recipes with converted rows establish
conversion preservation; they are not an independent new proof of all importer
semantics. The unchanged source-backed importer/recipe evidence supplies that
dependency, which must remain pinned through future work.

The 13 recorded faults alter actual private algorithm/import bodies and retain
successful compilation before a failing judge result. Four table faults, eight
motion/blend faults and one Gamma conversion fault exercise distinct observable
failures. I inspected replacement bodies, commands, receipts and retained logs;
I did not compile or execute them. The comparator's separate deliberate fault
retains 4,140 partials but changes an individual identity and is rejected at case
level. Full-stream byte comparison covers all recorded output words, flags,
controls, inputs and returns; aggregate equality cannot substitute for it.

## Remaining prerequisites and limits

1. Root corrects the stale narrative and seals a parent-preserving union of new
   protected tools and registrations, keeping every inherited path and all 53
   raw corpora. Follow the separate evidenced judge workflow in `docs/JUDGE.md`;
   obtain its real integrity/gate result. The current bounded integrity pass is
   not that gate. Preserve all existing local artifacts in their present worktree.
2. Future Source review covers the entire shared table/resource/motion-state
   boundary: canonical physics words and tail, join borrowing, both animation
   implementations, Sonic producer/decoder/injection/routing, startup/initialized
   views and Gamma conversion/publication. Sonic remains in scope outside the
   native archive. No duplicate motion owner or hidden importer/reference algorithm
   is authorized by these judges.
3. Rebuild and compare all six complete captures, all partial/failure/boundary
   identities, the new table/interface/Gamma streams and affected decoder/consumer
   units. The six-stream comparator alone does not compare the additional 114
   interface or 91 Gamma rows. Preserve their knownness and partial allocation
   observations independently. Rerun meaningful compiled faults against the new
   semantic implementation; old textual replacement anchors will require an
   evidenced adaptation after a rewrite, not changed expectations.
4. Freeze that Source, obtain fresh independent STYLE review, then implement/run
   the reserved final cohorts under Root's stated authorization. They currently
   remain `RESERVED_UNRUN`, with seeds/descriptions rather than executed holdout
   proof. Qualify actual native closure, affected bindings/weld/cursor/composition,
   existing journeys and a coherent integration gate before Source acceptance.

The evidence does not exhaust malformed graph/state combinations, all allocator
policies, source stack overflow behavior, Windows FP flags or hardware fidelity.
Actual PSetMotion remains `native_gamma_set_motion`; no pack-to-playback bridge,
actor rendering, N3b, gameplay, physical-controller or campaign closure is proved.

This reviewer ran only read-only Git/source/assembly/JSON/raw-stream inspection
and bounded Python integrity/static checks. No builds, oracles, mutation loops,
sanitizers, gates, providers, network, storage operations, delegates, T3 polling,
candidate/Main/artifact writes or relocation/deletion were performed. Only this
review report is committed in the isolated review checkout.

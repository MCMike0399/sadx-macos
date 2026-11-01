# Independent menu progress review

Verdict: bounded approval. No blocking correctness or STYLE section 12 finding in
the committed naming and shape migration. `src/game/menu_eligibility.hpp`,
`src/game/menu_eligibility.cpp`, `src/game/main_menu_eligibility.hpp`, and
`src/game/main_menu_eligibility.cpp` now conform as named files for their existing
recovered responsibilities. This does not certify the remaining UI, save,
bootstrap, Session, or composition modules, or completion of the refactor campaign.

Reviewed independently on 2026-11-01, without delegates. Candidate:
`9c0109554f51e9f6e03a772272b41ce39d813bfa`; source:
`98ffa2660b4e82068d37901e3c5e000741b27396`; separate judge revalidation:
`e821b72`; comparison base: `fdb1595a30027c8253b9af28a097ce2112dc1fdf`.
Source was inspected in the read-only packet checkout at
`/Users/burbujamc/Developer/sadx-wt-menu-progress-predicates`, with hashes checked
against committed refs. This report is the only authored repository change.

## Standards and behavior

Read canonical instructions, packet AGENTS, STYLE including section 12, GOAL,
STATUS, latest HANDOFF, and `docs/refactor-inventory/context-menu-progress.md`.
The source diff covers fourteen paths: character_select_menu.cpp,
game_bootstrap.hpp, game_cleanup.hpp/cpp, main_menu_eligibility.hpp/cpp,
menu_eligibility.hpp/cpp, save_creation.hpp/cpp, save_data.hpp, story_flags.hpp,
and title_menu.hpp/cpp, all under src/game. All fourteen candidate hashes match
the committed current summary.

* SDK SADXEnumsNew.h playable/completed flags agree with every shared story_flags
  constant. Miles/Tails and E102/Gamma are justified role names. Menu tables retain
  Sonic, Tails, Knuckles, Amy, Big, Gamma, extra slot, rather than raw character-ID
  order. The mission-clear array likewise keeps Big before Gamma.
* Metal unlocking still tests the borrowed flag for nonzero, counts emblem bits,
  stores exactly 1 into the live story owner at the threshold of 130, and returns
  a normalized result. Override returns before any flag mutation. The cached
  GetEachCharEnable and IsMetalSonicUnlocked pseudocode independently supports
  the preserved branch order, variant 1 and threshold.
* character_enabled still accepts an override before rejecting an unsupported
  character. UnknownCharacter reports the unsupported roster; the unchanged
  reference probe handles its marker and the judge handles incoming-register
  behavior. No register value or fixture absence was introduced into game logic.
* Extra slot 6 retains Super Sonic for other variants and Metal for variant 1.
  character_count still checks regular-only variant 2 before override, then
  override before Metal availability. Naming does not assert wider meanings for
  all menu variants. ADVERTISE_WORK::MainMenuSelectedMode supports selection_variant;
  the existing positional reference to flow_.tutorial_variant remains unchanged.
* Mission bit 7, thresholds 20 and 130, nonzero byte tests, signed language/result
  widths, and uint8_t count accumulation/conversions are preserved. Replacing
  table-index types with size_t does not change these small indices. Stage row
  IDs remain uint16_t; their products by 43 fit the former promoted integer
  range, so the size_t stride introduces no overflow/sign change for these reads.
* Trial count retains its initial-stage guard, requested-character intermediate
  reads, and explicitly commented final read through current_character. Subgame
  completion reads still precede the override test. Trial-stage builders retain
  source ordering, byte counters, final-stage catalog-size publication, and the
  existing trial-builder byte cast of the character ID.
* SAVE_DATA::flgCompletedActionStage[8][43] supports the dimensions. Although the
  SDK declares Sint8, byte storage and zero/nonzero tests preserve all bit patterns;
  this packet does not alter element signedness. StageCompletions aliases exactly
  std::array<uint8_t,344>, while Data retains exactly the nested
  std::array<std::array<uint8_t,43>,8> type. There is no wrapper, layout change,
  new owner, or reinterpretation of a nested array as a flat array.
* Load/capture retain their copies, traversal and store order, copying all eight
  rows with the same stride. Bootstrap remains the live flat-array owner.
  Completion, StageSelectionInputs, LoadSaveState and CaptureState borrow it.
  Inspected native main-menu and character-menu progress/availability construction,
  title selection inputs, Session::load_save and capture_live_save, bootstrap
  initialization, and unchanged save encoder/reader consumers. Reference probes
  still supply the identical concrete array types. No owning field or lifetime
  was moved by the migration.

The four named eligibility files contain semantic game code and provenance,
structured control flow, ordinary integer arithmetic, plain borrowed state and
source-backed constants. No recovered behavior moved to guest/HLE, no recovered
callee became a stub, and no new no-op, dispatch machinery, fixture optional or
unrequested enhancement architecture was added. This is a semantic naming cleanup,
not a claim to have retired lexical debt elsewhere.

## Existing evidence independently checked

Evidence below is under
`reference/production-integration-20261101/menu-progress-{baseline,shared}` in
the candidate. I read receipts and performed read-only hash/JSON comparisons;
I did not execute any judge, unit, build, mutation, or holdout.

* Eight unchanged judges retain their source hashes/contents across the base and
  candidate. No tests or tools changed. Cached predicate results have zero
  mismatches: menu eligibility 7,388; main-menu eligibility 11,488; availability
  2,920. Cleanup records 2,604 cases, zero mismatches and matching values/call order;
  save creation records 1,000 exact cases.
* Character Select has 3,326 cases and 733 inherited FP partials; title has 20,872
  cases and 270 inherited FP partials. Both aggregate judges still exit 1. Parsed
  the existing full native/original JSONL outputs and verified zero differences
  outside the designated FP slot, exact partial-ID lists, matching committed
  full-output digests, and byte-identical baseline/current JSONL files. Digests:
  Character Select `1795efd9814949d3aefbf6f80ba558ace5da38bc5b67cd7a3c3ebf09014fe93e`;
  title `79a30d1fb6636547e0e3c2a178bf01e9158ccb54a797295076bedc28c177204c`.
  These are preserved partials, not strict passes.
* title_factory is the actual stage-list body consumer: baseline/current receipts
  both report 2,268 exact cases with source-range hashes. This additional baseline
  and current qualification occurred after the private source naming edit, as the
  context document states; this review does not promote it to a pre-edit run.
* Cached build receipts show eight affected/header-dependent objects rebuilt and
  six existing unit programs exiting 0: menu_eligibility, main_menu_eligibility,
  save_file_codec, save_creation, game_cleanup and title_menu. Unchanged Root core
  dependencies were borrowed; this is not a clean production closure build.
* Six recorded mutants compile successfully and produce judge mismatches:
  Metal store 116, Big/Gamma order 27, Metal variant 8, override precedence 10,
  completion stride 37, current-character final stage 43. Their recorded source
  identities match this candidate. These are valid rejection receipts, not
  compilation failures counted as catches.
* Separate registry comparison confirms exactly two aggregate declaration
  fingerprints change: flags and clear_flags. Eleven explicit evidence entries
  are revalidated; the other nine declarations are unchanged. Every classification
  and retired entry is preserved, with no debt retired. The protected manifest
  removes no inputs, changes only the existing table-registry fingerprint, and adds
  the mutant receipt. Its evidence, registry and mutant hashes all verify.

## Remaining limits

Final fresh-seed predicate holdouts remain reserved and unrun. Full gate, clean
native closure and final integration qualification remain outside this review.
Private probes borrow unchanged Main dependencies; original allocation failure,
external-owner seams, excluded invalid indices and other existing judge limits
remain as documented in the unchanged judges. No new GPU, actor, journey,
gameplay, performance, complete save/UI/Session ownership or campaign-completion
claim follows from these finite cases. Main and its running render writers were
not modified. No providers, network/storage operations or workers were used.

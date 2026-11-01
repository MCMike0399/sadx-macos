Spec/fidelity review of `3bdbcf13df00601ca738f73d9b25d5f6348922cf`, against
`53da4de17a1c1f21745d8403cd5bf6068afeb9f5`; foundation `b0e53ed`.

- **P2 — Retained reference constants contract silently loses behavior.**
  Candidate `src/player/animation/animation_engine.hpp:199` hardcodes the reverse
  reset subtraction; lines 85, 172, 338, 442, 452 and 638 likewise replace the
  legacy runtime's three supplied constants. `src/guest/player_animation_reference.cpp:97`
  forwards the unchanged compatibility runtime without honoring those values.
  Criterion: `docs/PROMPT-complete-refactor.md:94`, “Retain all already implemented
  behavior,” and the review brief's explicit prohibition on narrowing the
  implemented reference contract by declaring substituted facts unsupported.
  Static counterexample: reverse reset, current motion length 10, legacy
  `constants.one = 2.0f`, ordinary masked FP control: before-source stores frame 8;
  candidate stores 9. Changed zero/correction values also lose their old effects.
  `JUDGE-ADAPTATION.md:27` declaring substitutions outside the contract does not
  authorize this narrowing. Preserve supplied values through reference access,
  with literal defaults for native access, under the same shared algorithm.
  Existing exact streams use only the pinned constants block and cannot detect it.

No additional concrete Source fidelity defect or unasked gameplay behavior found.
Read the complete affected animation modules and Sonic/startup/import/Gamma
consumer closure; checked signed modes, wrapping, lazy failures, partial stores,
live allocation aliases, canonical tail/table borrowing and retained STUBs.
The retired 1,027-operation table maps exactly to the independent publication
fixture, preserving 145 reads and 882 stores. Root's lineage/denominator,
protected-union, holdout, product-closure, composition, journey and gate work remains
pending; this report does not classify those declared integration tasks as Source defects.

Read-only integrity evidence: 660/660 Source hashes, 166/166 candidate artifacts,
279/279 compiler dependency entries, 970/970 sealed inputs, all 53 raw inputs,
and every identity-part hash match. Foundation checks match 383 closure files and
411 artifacts. Independently compared all six complete stored streams:
103/16,440/7,944/28,800/2,004/1,422 rows, byte-identical; FP partial and failure
identities therefore remain unchanged. Reviewed comparator, diagnostic mapping,
probe-only forced include and recorded eight-unit/13-fault/publication-fault receipts.

No builds, executable/library replay, original oracles, mutations, holdouts, gates,
providers or network operations ran. Producer execution receipts were inspected,
not rerun. No gameplay, physical-controller, 63 FPS or campaign acceptance is inferred.

# Independent source-lineage packet review

Candidate: `packet/inventory-source-lineage` at `96ebadd443b033be06fa3e09cb67582839019351`, checkout `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-b4c42380`. Fixed point `84337e542c150a7f8c1f96c20df631050764f404`; separate source/evidence freeze `409ca4e5b5866a3e609f953ad3882de02579325d`. Review performed from isolated `packet/inventory-source-lineage-review`, without candidate edits or delegation.

**Verdict: request changes before policy activation or integration acceptance.** Three reproduced enforcement gaps remain despite 47 passing focused tests. This is a review of inventory tooling and its claimed proof boundary, not acceptance of animation source or the complete refactor.

## Spec and judge findings

### R1 — P1: resealing can erase retained historical members and their debt

Location: candidate `tools/inventory/refactor_inventory.py:469` (baseline-root traversal), `:302` (replacement receipt), `:802` (historical counters). Criterion: PROMPT §§1/2/4 preserve baseline scope and remaining debt; RUNBOOK lines 23–24 explicitly prohibit retirement reducing accounted scope or discharging historical debt.

`reconcile_lineage` only roots traversal at paths removed from the *original* baseline. It never binds the new ledger to an earlier accepted receipt/history. In a fully committed synthetic repository, accept A→B→C, then reseal A→C, omitting B. Both render successfully. Current tracked denominator stays 1, but historical scope/open debt falls **2→1** and accounted scope/open entries falls **3→2**. B's specific consumer obligation disappears. The old baseline fingerprint, source freeze, policy hash and independent-review shape checks all pass. This is historical debt loss, not legitimate live-file editing.

The same absence of retained-metadata continuity lets a later seal replace a historical `needs_migration` member's counts `{entries:1027, reads:145, stores:882}` with `{}`, claim `already_conforming` as its *prior* disposition, and substitute an unrelated generic limit. The aggregate historical row still says debt open, but its actual recorded obligations and observations are lost. Source bytes alone do not preserve the reviewed inventory state.

Required correction: bind successor receipts to the prior accepted ledger/receipt and retain every prior historical identity, snapshot, observation, disposition and explicit obligation. Reject chain squashing or metadata replacement that drops historical debt; any separately authorized future closure needs its own explicit policy. Anchor initial prior review metadata, rather than merely validating enum/string shape. Preserve separate current membership and historical accounting. Supply the predecessor proof chain to archive verification; do not introduce an unconditional Git dependency there. Add regressions for both reproduced changes.

### R2 — P1: immutable policy can accept an uncommitted review contradicted by Git

Location: candidate `tools/inventory/refactor_inventory.py:233` (`fingerprinted_file`), `:294` (preparation only checks two policy files), `:380`, `:425`, `:428`. Criterion: committed-policy/immutable proof and independent-review requirements; PROMPT §2 honest enforcement and JUDGE's versioned evidence boundary.

Only the receipt and ledger are compared with committed policy blobs. Review and consumer/behavior anchors are checked against current working-tree bytes at tracked paths. Reproduction: leave the frozen ledger's review anchor naming the accepted bytes, commit that review file with verdict `pending`, commit a valid freeze and receipt, then restore the accepted review bytes **only in the working tree**. `render()` succeeds although `HEAD:docs/review-0.json` says `pending`. No source changed and all snapshot Git comparisons pass. Receipt preparation likewise checks neither review nor evidence blobs. Thus “tracked, fingerprinted independent acceptance” can be supplied by unversioned bytes, rather than the claimed committed proof set.

Required correction: in Git, bind all immutable review/evidence anchors to their exact blobs in the sealed policy's committed proof snapshot, both when preparing and consuming a receipt. Reject a frozen pending/missing/different review even when working-tree bytes match the ledger's desired hash. Archives must verify the supplied proof set without requiring Git. This is provenance validation; it does not authenticate reviewer identity or mechanically establish semantic fidelity. Keep the documented human independence review requirement.

### R3 — P2: replacement proof need never have been integrated

Location: candidate `tools/inventory/refactor_inventory.py:329`, `:441`, `:453`. Criterion: meaningful real current tracked successors, immutable replacement evidence and rejection of stale/fake evidence; PROMPT §1/§4 cannot infer preserved behavior from a path's existence.

The only replacement ancestry check is `before.commit`→`successor.commit`. The terminal check asks whether the *path* currently exists. A proof from an unmerged descendant side branch is accepted when the same live path contains entirely different bytes and never held the asserted snapshot in the integrated lineage. Reproduction creates `AllegedlyReviewedButNeverIntegrated` on a side branch, retains unrelated `TypedContent` at the current path, and seals the side-branch snapshot. Render accepts it; the snapshot commit is not a HEAD ancestor and its bytes differ from the integrated file.

Required correction: connect the reviewed replacement snapshot to an immutable integrated-source snapshot, by ancestry or an explicitly checked content-equivalent import/cherry-pick anchor, and validate subsequent continuity from that point. Merely having some blob somewhere in Git and a matching current pathname is insufficient. **Do not restore the prior R6 bug:** legitimate later edits must remain allowed; do not compare the historical snapshot with today's live bytes forever. Current conforming reviews must still bind today's exact source. Add an unmerged-snapshot regression alongside the existing later-edit test.

## Standards and scope assessment

No separate actionable style finding. The change names its helpers, localizes Git/address knowledge to tooling, leaves semantic review explicit, and introduces no source algorithm relocation. Existing file/module whole-member requirements, semantic conformity hashes, nested module evidence validation, STUB binding/debt checks, exact current path equality and 2 MiB/no-truncation shard behavior remain in the code. Their existing focused tests pass. The new live-edit test preserves immutable before/replacement snapshots while rejecting stale conformity; that requirement must survive fixes.

Purpose classification, readability approval, semantic completeness and integration acceptance remain distinct. Open historical debt is conservative, and neither `reviewed_migration` nor `reviewed_retirement` grants successor conformity. Added instruction tables remain `open_review`; this packet changes neither constexpr policy nor readability ratchets. Magnitude alone does not make slot/index/field/bits an executable address. No blanket exemption was added. These strengths do not repair R1–R3 or establish full Source conformity.

The animation demonstration is explicitly **inactive** and lacks the dedicated accepted lineage review required by the schema. Its copied standards/fidelity rechecks expressly defer retirement lineage. This review does not supply that acceptance. Root must author the real mapping/policy and independent acceptance, reconcile retired active file/module reviews without losing their obligations, seal the immutable receipt after source integration, and regenerate/review global membership/history separately.

## Checks actually performed

- Read shared instructions/memory, STATUS/GOAL/STYLE §12, PROMPT §§1/2/4, JUDGE, existing inventory/module/STUB guards, full generator/test/documentation diff, packet/runbook/verification/demonstration and copied source/review evidence. No prior review child reused.
- `python3 -m unittest discover -s tests/python -p 'test_refactor_inventory*.py' -v`: **exit 0, 47 tests, 41.695 seconds** (21 existing, 26 new). Includes temporary Git and archive fixtures; no original game execution.
- Four additional pure-metadata reproductions: all exit normally and are **accepted by the candidate**, as detailed above. Reproduction script and machine-readable results are adjacent to this report: `source-lineage-review-reproduce.py` and `source-lineage-review-checks.json`. They create disposable synthetic repositories only and do not edit the candidate.
- Current `check_judge.py`: **exit 0, 1,269 files, zero violations**, after Root's restoration. Parent 1,268 hashes remain identical, with exactly one added test path. The worker's stored exit 1 remains a truthful older missing-53-corpora result; it was not overwritten or treated as current.
- Current inventory `--check`: **exit 1**, expected drift in `modules.jsonl`, `modules-002.jsonl`, `boundaries.jsonl`, `summary.json`, `files/src-apps.jsonl`, `files/tests-python.jsonl`, `files/tools-inventory.jsonl`. No global artifacts regenerated and no pass claimed.
- All **650** tracked source files byte-match immutable `409ca4e` source blobs; no candidate Source change or relocation. All **10** animation copied proofs byte-match the exact named Git blobs and SHA-256. The old header is 1,035 lines, retaining the demonstrated 1,027 operations / 145 reads / 882 stores. These are static proof checks, not newly executed semantic tests.
- The requested packet `source-freeze.json` does **not** exist. The available full 650-path freeze is `before.json.source_freeze`, with its commit named by `verification.json`. No missing file was invented.

## Root follow-up and bounded acceptance

The generator was already **outside** the judge manifest scanner; this packet does not remove prior protection. The new test alone is added. Green `check_judge` therefore proves old protected inputs/cases unchanged, **not** generator immutability or correct lineage semantics. Before relying on this generator as enforcement, Root needs a separate evidenced judge-protection change for the inventory generator (and an explicit decision on authored policy/proof/generated-artifact protection), including a regression that generator drift is detected. Preserve all old protected hashes/corpora; do not blanket-refresh them. The 47 tests alone do not close this boundary.

Fix R1–R3 in the packet and obtain a fresh independent recheck before activation. Root still owns actual animation policy acceptance, source integration, complete global reconciliation, final integration verification and gate reporting. No builds, original oracles, mutations, holdouts, gates, journeys, worker launches, other project checkout writes, Main edits, host/network/provider/storage work, STATUS/HANDOFF/global-inventory/memory edits or push were performed. The active Main axis gate and Source freeze were left untouched. No complete-campaign pass, new green gate, gameplay qualification or whole-module/source semantic approval follows from this review.

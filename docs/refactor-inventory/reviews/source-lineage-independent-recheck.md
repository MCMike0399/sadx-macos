# Corrected source-lineage independent recheck

Candidate: `4491b20f32054f997343652e7d6707f9d69aafa4`; exact implementation diff `96ebadd..4491b20`. Reviewed independently in assigned checkout `t3-996d7c41`, branch `packet/source-lineage-independent-recheck`, without delegates or other checkouts.

**Request changes: one remaining P1 historical-accounting defect.** This reviews bounded tooling semantics only, not animation, Source conformity, integration or campaign completion.

## R1 — P1: committed rollback bypasses append-only history

Locations: `tools/inventory/refactor_inventory.py:478`, `:597`, `:322` and `:609`.

Consumption only compares the active receipt with HEAD and searches for its predecessor at its own `freeze_commit`. Restoring an older, once-valid receipt therefore rewinds that search before later accepted history. Preparation takes only the latest receipt bytes returned by `previous_receipt_at`; it does not validate that intervening committed versions preserved their predecessors. A committed rollback becomes an apparently valid predecessor for a fresh generation.

Reproduction: accept A→B, then A→B→C with B's specific intermediate consumer obligation; verify two historical members. In a **new commit**, restore the original receipt and ledger and move live C back to B. No reset/rebase, baseline change, policy identity change, fabricated review or proof deletion occurs. Consumption accepts: historical scope/open debt **2→1**, accounted scope/open entries **3→2**, and B's recorded obligation disappears. Then author the original A→B edge with the restored receipt as predecessor: production `prepare_lineage_receipt`, committed consumption and Git-free archive verification all succeed with one historical member. The archive faithfully verifies an already-invalid tip admitted by Git validation; this does not demand that archives reconstruct omitted Git history.

Run `python3 docs/refactor-inventory/reviews/source-lineage-recheck-reproduce.py`. It asserts the loss and prints both accepted paths. All candidate mutations occur in disposable synthetic repositories.

Required correction: validate monotonic receipt transitions across the bounded committed history, including the active tip's relationship to later commits than its declared freeze. Neither restoring an old receipt nor treating its rollback commit as the newest predecessor may skip an intervening accepted identity/debt. Reject before preparation writes and during consumption. Add rollback regressions alongside deletion/path-replacement cases; retain legitimate later Source edits and Git-free exports.

## Corrected boundaries and fixture integrity

Direct squash/rewrite, deletion/path replacement, committed-pending evidence substitution, symlink proof substitution, unmerged snapshots and `ours` merges now have discriminating passing regressions. Explicit content-equivalent imports, later edits, recursive predecessor proofs, archive tip hashes and ordinary legacy no-Git rendering also pass. No further R2/R3 finding emerged within this scope. Initial metadata is anchored, but its semantic completeness still requires Root review. No constexpr exemption, guessed successor or semantic closure follows from path accounting.

All original 26 lineage test IDs and assertion ASTs are unchanged; the other 21 original tests are unchanged. Fixture changes author the new inactive schemas/predecessors and supply archive fingerprints. All 20 original packet artifacts and copied prior report/reproducer remain byte-exact.

## Tested scope and limits

Focused discovery: **exit 0, 64 tests, 58.239 seconds** (47 retained plus 17 regressions). Additional rollback reproduction: exit 0, both attacks accepted. Git Source diff against `409ca4e` is empty. No builds, oracles, providers, storage work, activation, global reconciliation, manifest refresh, Main edits or push. No full protected-corpus hash pass claimed. Root's separate `48df570` generator protection is acknowledged; the worker's historical warning is not a new Main defect. Root still owns the parent-preserving manifest union/current generator hash, actual animation mapping/activation, integration, journeys and full gate.

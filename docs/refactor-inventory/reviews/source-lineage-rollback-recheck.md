# Source-lineage committed rollback recheck

Candidate: `770ff51ca3bd53e6f222acb500da0364857f5dc7`; exact diff `2430750..770ff51`. Independently reviewed in assigned checkout `t3-c3bae10a`, branch `packet/source-lineage-rollback-recheck`, pinned directly to the candidate. No delegation or other checkout access.

**Bounded acceptance: the remaining R1 committed rollback gap is closed. No actionable findings within this recheck.** This accepts tooling semantics only.

## Spec and continuity

`committed_receipt_continuity` at `tools/inventory/refactor_inventory.py:335` collects canonical identities from the active/proposed receipt and its fingerprinted predecessor chain. Before this call, `reconcile_lineage` recursively validates those predecessors, their committed proofs and exact historical edges. Exact edge equality preserves identity, obligations, counts and disposition; membership in the validated chain cannot substitute an unvalidated historical payload.

The new check compares that chain with all bounded first-parent receipt-path versions through HEAD, including versions newer than the active receipt's declared freeze. Restoring a once-valid older receipt in a new commit therefore cannot hide an intervening generation. Deletion entries are skipped, while earlier nondeleted versions remain required. Exceeding the bound fails explicitly. Historical recursive calls correctly avoid comparing an old prefix against today's complete history.

Both top-level consumption and production preparation invoke this check. Preparation validates before its receipt write; CLI `--write` renders before any output write. Git-free verification retains recursive supplied-chain and export-fingerprint checks; it does not claim to reconstruct omitted Git history. There is no new live Source hash freeze, baseline exception or conformity grant.

## Independent verification

- `python3 -m unittest discover -s tests/python -p 'test_refactor_inventory*.py' -q`: exit 0, **66 tests, 62.417 seconds**. All 64 prior test-method ASTs are identical; precisely two regression methods were added, with no prior assertions weakened.
- Unmodified `python3 docs/refactor-inventory/reviews/source-lineage-recheck-reproduce.py`: expected exit 1 at the first post-rollback consumption, with `Committed lineage receipt history cannot be rolled back or skipped`; neither prior accepted attack path is reached.
- Additional disposable metadata fixtures: committed rollback, then deletion/recreation of the restored receipt, both reject CLI export without modifying any inventory output. Both reject production preparation without overwriting the restored receipt.
- Positive fixture: append a valid generation using production preparation, commit, edit live Source in another commit, consume and export, then remove fixture Git metadata and verify the archive. All succeed; two historical members remain. Existing suite also retains explicit content-equivalent import coverage.
- Manifest key set remains 1,270; only the existing regression-test hash changes. All 650 Source paths are unchanged. Available protected files: 1,217 hash matches, zero mismatches; 53 ignored local corpora are absent here. The worker's full local hash pass was not independently repeated. Diff whitespace check passes.

## Standards and limits

No standards finding in this small structured metadata change under STYLE §12 and campaign §§1–2. Original reproducer and candidate code remain unchanged. Only this report is committed. No activation, global regeneration, compilation, oracle, gate, mutation, holdout, Main edit or push. Root retains actual animation ledger/evidence review, its separately accepted generator-protection union, integration and merged gate. No Source classification or campaign approval follows.

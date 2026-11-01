# Source lineage schema and Root integration runbook

This packet changes only the inventory generator. The active `review.json`, global
inventory, constexpr policy/checker, source, STATUS and HANDOFF remain untouched.
No source or module approval follows from lexical observations or this packet.

## Two denominators

Without activation, existing schema 1 behavior stays intact, including rejection of
baseline omissions. Activation produces schema 2 with `current_tracked_denominator`
(exact current tracked-path equality), `historical_retained_scope` (one whole-file
record for each retired baseline/intermediate member), `historical_baseline_members`,
`accounted_scope` (current plus historical), and `historical_open_debt`.
`current_open_entries` counts current maintained/script entries without conforming
review; `accounted_open_entries` adds the historical open members, so retirement
cannot silently lower that total. These counts do not prove behavior or closure.
Existing disposition, module, boundary and STUB counts retain their current-path
meaning. Historical members never become conforming/obsolete automatically and
never enter the current modules twice. `lineage-*.jsonl` retains before fingerprints,
module identity, prior disposition, explicit remaining debt, immutable source line
counts and authored observed counts. Counts are observations of that historical
snapshot; do not add chain observations as distinct live instructions. Retirement
alone cannot reduce accounted scope or discharge historical debt. Explicit future
semantic closure of this historical ledger is outside this policy.

All ordered historical shards have record counts and SHA-256 in `lineage_shards`.
The existing 2 MiB ceiling, complete-record partitioning, exact path coverage,
module/member reviews, STUB debt and unexpected-shard rejection remain enforced.
No allowances flow to added/renamed constexpr tables or successor files. Existing
constexpr and readability ratchets still apply independently.

## Ledger schema 1 (strict, whole-file only)

A ledger is `{ "schema_version": 1, "edges": [...] }`. Each edge contains exactly:

- `before`: `{path, sha256, commit, proof}` for the original source snapshot.
  `commit` is a full immutable 40-digit Git commit. `proof` is `{path, sha256}`
  naming a tracked regular snapshot file, byte-identical to `commit:path` in Git.
- `replacements`: nonempty list of the same snapshot objects. Every chain must
  terminate at real current tracked files. A later move/deletion needs another
  reviewed edge; a later edit at the same live path does not change old snapshots.
- `member`: `{module, prior_disposition, remaining_debt, observed_counts}`.
  An optional `baseline_snapshot` uses the same snapshot schema when a successor
  already existed at the campaign baseline and later moves: retain that original
  baseline identity separately from the later transition `before` snapshot.
  `module` must match the inventory's original whole-file module key;
  disposition uses the existing enum. Remaining debt is a nonempty list of explicit
  limits/open obligations. Observed counts are named nonnegative integer measures.
- `disposition`: `reviewed_migration` or `reviewed_retirement`. Neither means
  semantic conformity, obsolete behavior or permission to discard coverage.
- `rationale`: nonblank semantic explanation.
- `consumer_evidence` and `behavior_evidence`: nonempty lists of tracked,
  nonempty, fingerprinted preservation evidence (`{path, sha256}`).
- `review`: fingerprinted independent review JSON described below.

Unknown fields, duplicate JSON keys, repeated sources/replacements, overlapping
replacement ownership, missing/extra edges, cycles, wrong snapshots, untracked
proofs/successors and changed evidence fail. One edge covers the entire file;
partial-file mappings and convergence onto a successor already claimed by another
edge are intentionally unsupported. No AST, address inference, exception system,
or universal C++ framework is introduced. Evidence/proof files are bounded to 2 MiB.

Each removed baseline member must retain the exact campaign baseline commit and
SHA-256, using `before` or the explicit `member.baseline_snapshot`. The latter
prevents an existing baseline successor from losing its original identity during
a later reviewed move. Intermediate snapshots can reflect legitimate later edits. Git requires
forward ancestry from before to replacement, and from a predecessor replacement
to a subsequent before snapshot. A changed terminal source remains historical
continuity, but existing live file/module conformity requires fresh exact-source
review. Snapshot equality is never imposed on terminal live bytes.

## Independent review anchor

The dedicated review JSON contains exactly `edge_sha256`, `reviewer`, `independent`,
`verdict`, `limits`. Hash the edge **excluding** `review` with Python's
`json.dumps(edge, sort_keys=True, separators=(',', ':')).encode()` and SHA-256.
The reviewer must explicitly accept that exact mapping, name themselves, attest
independence with `true`, set verdict `accept`, and state nonblank limits. Then hash
that review file into the edge's `review` anchor. This is reproducible provenance,
not authentication of a human identity or an automated proof of semantic fidelity.
An implementer's fabricated acceptance is not independent review.

Existing animation rechecks explicitly leave retirement lineage pending. They are
source evidence, not this dedicated acceptance. `animation-demonstration.json`
therefore intentionally omits `review`; it cannot be activated as an accepted ledger.
Its ten tracked proof files are exact committed source/review/behavior snapshots,
with original paths and commit fingerprints. They do not certify semantic closure.
The old 1,035-line header retains its 1,027 entries / 145 reads / 882 stores. The
mapping covers typed content, shared ordered initialization, native publication and
reference diagnostics; it does not call this an unchanged move into guest code.
The PC field is instruction metadata; slot/index/record/field/bits are not all addresses.

## Receipt schema 2 and activation (Root-owned)

1. Obtain fresh independent review of this generator packet. Integrate its separate
   judge commits, preserving every parent protected-manifest path and receipt.
   Do not run a blanket refresh over missing corpora or edited source.
2. Integrate the separately qualified source candidate. Inspect the demonstration's
   proposed replacement set and evidence. Author the dedicated lineage review,
   retaining whole-module/STUB/behavior debts and all existing ratchets.
3. Put the final ledger/proofs/review at tracked paths. In active `review.json`, set
   `source_lineage_receipt` to a path under `docs/refactor-inventory/`. Reconcile
   removed active file/module reviews separately: they must not masquerade as live
   reviews, and their prior disposition/debt must survive in the ledger. This packet
   does not author those Root decisions. Commit source and authored policy first.
4. With `src/` identical to HEAD (including no staged/untracked source), prepare:

   ```sh
   python3 tools/inventory/refactor_inventory.py \
     --prepare-lineage-receipt docs/refactor-inventory/source-lineage-receipt.json \
     --lineage-policy docs/refactor-inventory/source-lineage-policy.json
   ```

   Preparation reads committed policy and HEAD only; it does not activate anything.
   Review and commit the receipt before `--write`/`--check` can consume it. Its exact
   fields are `schema_version: 2`, `base_commit`, `freeze_commit`, `policy` anchor,
   `baseline_members`, `archive_paths`, `source_freeze`. The full original scoped
   path/hash map (including gitlink identities) binds the baseline denominator.
   Full source hashes bind the immutable freeze commit. Git verification requires
   the live receipt equal its HEAD blob, the policy equal its freeze-commit blob,
   and freeze ancestry to HEAD. A freely edited tree cannot refresh these metrics.
5. Root regenerates and reviews global artifacts, including every historical shard
   and exact current membership. Then perform Root-owned integration checks/gate.
   No global campaign/source/module acceptance is implied by this packet's tests.

Evidence anchors must stay byte-identical; changing evidence requires new reviewed
fingerprints/receipt. Immutable snapshot proofs remain available after live source
edits/moves. Do not replace proofs with hashes of whatever the successor contains now.

## Archives and focused verification

Archives verify supplied proof bytes, reviews and exact current scoped membership;
Git-only ancestry/object checks run only in Git. `archive_paths` supplies the
exported tracked membership; evidence must belong to it. Newly generated inventory
metadata under docs does not alter the tracked source denominator. A later changed
membership needs a newly committed receipt/export; ordinary edits at an existing
successor do not. Supplied boundary records preserve upstream gitlinks as one path,
without importing their descendants. Legacy archives use supplied inventory records
for baseline membership and boundaries; they cannot establish missing Git ancestry.
Neither ordinary rendering nor archive checks prepares a receipt or requires Git
for a new unconditional check. Ordinary compilation is unchanged.

```sh
python3 -m unittest discover -s tests/python -p 'test_refactor_inventory*.py' -v
```

Tests create temporary synthetic Git repositories and archives, never another
project checkout. No build, oracle, game execution, mutation runner, provider,
network operation or global inventory refresh is part of this packet.

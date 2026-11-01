# Corrected inactive proposal: ledger 2 / receipt 3

This supersedes the unactivated schema-2 receipt proposal at `96ebadd`. It does not
activate lineage, approve animation, discharge debt, change Source or reconcile
any global inventory. The original packet, proof files, missing-corpora receipts,
independent review and four accepted-by-old-tool reproductions remain available.

## What is retained

A generation may only append complete new historical edges. Every previously
accepted edge must survive byte-for-byte as a JSON value, including its source
snapshots, member/module identity, counts, prior disposition, explicit obligations,
review, rationale and evidence anchors. A→B→C cannot become A→C. Neither a new
accepted review nor a new matching metadata document can rewrite prior debt.
No debt closure or history compaction is supported by this bounded policy.
Current tracked membership remains separate and exact; historical members never
become duplicate live paths, conforming files or automatic constexpr exceptions.
Existing module/STUB contracts and all ratchets remain independent.

## Initial prior-member review record

Each edge gains `prior_member`, using the existing snapshot fields
`{path, sha256, commit, proof}`. Its fingerprinted JSON must equal exactly:

```json
{
  "historical_path": "src/player/old.hpp",
  "source_sha256": "<original baseline/before SHA-256>",
  "member": {
    "module": "src/player/old",
    "prior_disposition": "needs_migration",
    "remaining_debt": ["Every explicit original obligation"],
    "observed_counts": {"entries": 1027, "reads": 145, "stores": 882}
  }
}
```

For an existing baseline successor later moved after edits, preserve the existing
`member.baseline_snapshot` field too; `source_sha256` then identifies that original
baseline member. The separate transition `before` remains intact.

The prior-member record must have its own immutable commit **before** the ledger
freeze, on its ancestry. Git verifies the exact record blob and the frozen proof
copy. This anchors the full authored prior state, rather than merely accepting an
enum and a nonblank generic limit. Root must construct that record from the actual
prior inventory/review metadata and independently verify its completeness. The tool
checks provenance and equality; it does not infer semantics/counts from C++ or
authenticate a reviewer's identity. Never omit old findings when constructing it.

## Committed proof set (R2)

Receipt 3 adds `proofs`, the exact path→SHA-256 map of **every** explicit anchor in
the ledger: independent reviews, consumer/behavior evidence, all before/replacement
snapshot proof files, integration proof copies, initial member-record copies, and
predecessor receipt/ledger copies. Missing, extra, conflicting or stale anchors fail.

Preparation and consumption both require these paths to be tracked regular files,
with the exact fingerprinted bytes in the immutable freeze tree. A committed pending
review cannot be overridden by accepted working-tree bytes. Committed symlink
payloads cannot be substituted with uncommitted regular files. Snapshot proofs
also retain their original `commit:path` byte checks in Git.

Use immutable proof copies, distinct from evolving live Source. Do not anchor a
historical proof to whatever today's successor happens to contain. Old evidence
files must remain tracked and unchanged; new generations use new artifact paths.
Archives verify the supplied proof bytes/fingerprints and the entire predecessor
chain without Git. They do not independently reconstruct Git commit ancestry.

## Replacement integration (R3)

A replacement's source snapshot must lie on the freeze commit's **first-parent
source history**. Ordinary ancestry alone could admit an `ours` merge that never
imported the reviewed bytes. For a merge, cherry-pick or equivalent import whose
original snapshot commit is not on that history, add `integrated` to that
replacement snapshot. It is another ordinary snapshot (no nested `integrated`),
with the same source path and SHA-256, a real integrated commit on the first-parent
history, and a tracked immutable proof copy. Both original and integrated blobs
must be byte-identical. Before→integrated and later chain continuity are checked.

A later edit at the same live successor remains legitimate. These checks use
historical committed snapshots, not today's bytes. The ordinary current file/module
conformity checks still demand fresh review when Source changes. Snapshot continuity
and semantic conformity are separate judgments.

## Predecessor descriptor and successor generation (R1)

Ledger 2 has exactly `schema_version: 2`, `edges`, and `previous`. Receipt 3 retains
the former receipt fields, changes `schema_version` to `3`, and adds `proofs` and
`previous`. Both `previous` values must match. Genesis uses `null` only when no
prior receipt exists in the committed policy history.

A successor descriptor contains exactly:

```json
{
  "receipt": {"path": "docs/.../previous-receipt.json", "sha256": "..."},
  "ledger": {"path": "docs/.../previous-ledger.json", "sha256": "..."},
  "commit": "<immutable integrated commit containing that accepted receipt>",
  "path": "docs/refactor-inventory/source-lineage-receipt.json"
}
```

1. Copy the prior committed receipt and its exact frozen ledger into distinct,
   tracked, immutable artifacts. Keep every older predecessor/proof artifact too.
   The descriptor's `path` remains the active receipt's original path; it is not
   the archival copy's path. The old ledger hash must match the old receipt.
2. Append new reviewed edges. Keep **all** old edges intact, including old anchors.
   Author new prior-member records and dedicated independent reviews. Put the
   predecessor descriptor in the ledger and commit the authored policy/proofs.
3. With clean committed `src/`, run the existing optional command:

   ```sh
   python3 tools/inventory/refactor_inventory.py \
     --prepare-lineage-receipt docs/refactor-inventory/source-lineage-receipt.json \
     --lineage-policy docs/refactor-inventory/source-lineage-policy.json
   ```

   It validates the entire current policy/proof/history set **before writing** the
   proposed receipt. All inputs come from immutable HEAD; uncommitted evidence
   cannot be blessed. Review and commit the new receipt before consuming it.
4. Consumption independently rechecks the same proof/history constraints. Git
   locates the previous committed receipt at the freeze, including through a later
   deletion. Removing the predecessor or changing the active receipt path cannot
   reset history. Even a committed draft cannot silently erase an earlier receipt;
   commit only validated receipts. Limits are 64 receipt/policy versions and 64
   predecessor links; exceeding a bound fails closed, never truncates history.

Receipt/ledger/proof artifacts remain bounded to 2 MiB. Existing JSONL partitioning,
complete-record manifests, stale-shard rejection and the repository's 2 MiB ceiling
remain in place. No implicit schema-2 upgrade or blanket edited-tree refresh exists.
The old proposal was inactive; its failure evidence is historical, not an accepted
schema-3 root. Root owns any real initial bootstrap.

## Archive export

Export the committed current receipt, ledger, every predecessor pair and all
fingerprinted proof copies, with current boundary metadata and generated summary.
`summary.json.lineage_receipt.sha256` supplies the current receipt's trusted export
fingerprint. The archive checks that tip and recursively verifies every predecessor,
ledger hash, unchanged edge, member record and proof set. Historical membership
uses each predecessor's supplied snapshot; it does not require old successors to
remain live after later moves. Current scoped membership remains exact.

As with other supplied baselines, changing both a trust root and its entire package
is not prevented cryptographically. Archives do not create/reseal receipts or prove
Git ancestry/reviewer identity. Root must publish a validated, committed export.
Ordinary legacy rendering without active lineage retains its no-Git behavior;
compilation is unchanged and was not run in this lane.

## Root-owned remaining work

The generator is still outside the manifest scanner, as it was at `96ebadd`.
Green `check_judge` proves protected inputs unchanged, **not generator protection**.
Root must separately protect this generator and decide policy/proof/artifact
coverage before enforcement, then obtain a fresh independent recheck of this new
candidate. Do not reuse the old reviewer. Root alone authors actual animation
retirement acceptance, integration anchors, activation and global reconciliation,
and runs the coherent integration gate. The old animation demonstration remains
unchanged and deliberately lacks activation/review approval; it needs schema-3
Root authoring and current integration proof, not an automatic conversion.

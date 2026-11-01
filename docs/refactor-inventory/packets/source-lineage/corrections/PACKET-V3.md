# Source-lineage implementation correction packet

Same implementation lane and checkout: `packet/inventory-source-lineage` at
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-b4c42380`.
Original candidate: `96ebadd443b033be06fa3e09cb67582839019351`.
Independent request-changes: `c4242a1`; the full report/reproducer/checks are copied
byte-exact here. Commit `9a3b443` preserves all four reproduced acceptances before
correction. This is implementation follow-through, not a new formal review.

## Corrections

- **R1:** ledger 2 / receipt 3 binds every successor to the previous committed
  receipt and exact frozen ledger, including after receipt deletion or attempted
  path replacement. Historical edges are append-only and immutable as JSON values:
  no lost identity, source snapshot, observation, prior disposition or obligation.
  Initial full member metadata has a separate earlier committed snapshot anchor.
  Archives verify the supplied predecessor/proof chain and current export tip
  without Git. History/debt compaction or closure is unsupported.
- **R2:** all review, consumer, behavior, snapshot, member-record and predecessor
  proof anchors are bound to regular files with exact committed freeze bytes,
  during both preparation and consumption. Working-tree acceptance cannot override
  committed pending/different/missing/symlink evidence. Preparation validates before
  writing its proposed receipt.
- **R3:** reviewed replacements require a snapshot on integrated first-parent
  source history, or an explicit byte-equivalent integration/import snapshot there.
  A side-branch blob, even behind an `ours` merge, is insufficient. Later legitimate
  Source edits remain allowed; current conformity still requires fresh review.

The [runbook](RUNBOOK-V3.md) defines the inactive versioned correction, exact Root
steps and limits. Current tracked coverage, historical accounting, module/STUB
contracts and 2 MiB/no-truncation shards remain separate and enforced. There is no
new C++ analyzer, generic framework, constexpr exemption or Source classification.

## Actual verification

[64 focused tests pass](focused-tests-v3.txt): **all original 47 test IDs retained,
plus 17 new regressions**, with Git, preparation/consumption and archive negatives
and positive append/import/later-edit cases. Original fixture plumbing migrates the
unactivated schemas, authors committed predecessor artifacts, and supplies archive
export fingerprints; the original negative purposes/assertions remain. Intermediate
development logs are retained separately and are not final passes.

[Judge check](judge-check-v3.txt): **exit 0, 1,270 files, zero violations**. Root's
53 restored corpora remain exact. The manifest update is a finite immutable-HEAD
binding of the known schema fixture and new regression file: **all original 1,268
protected paths/hashes remain unchanged**. It does not refresh unrelated inputs.

[Inventory check](inventory-check-v3.txt): **exit 1, expected generated drift** in
seven artifacts. No global artifact was regenerated. This is not an inventory pass.

[Verification and exact helper hashes](verification-v3.json) record all 650 Source
files unchanged from `409ca4e`, all 20 original packet artifacts byte-identical to
`96ebadd` (including ten animation proofs, old missing-53 receipts and original
47-test log), and copied review/reproduction provenance. Source-map SHA-256:
`25e001213cfb11f89890c4be1d75e085eb3f02d5d451b2f7aa6ad31e98a21c91`.
Generator SHA-256:
`5f5ffa5c310f7eb9426b7914acff73b7a756de0192c4b158c9bf159c5fece172`.

## Limits and Root handoff

**The generator remains outside the manifest scanner.** Green judge verification
proves protected-input integrity; it does not protect this generator or establish
correct lineage semantics. Root owns a separate evidenced protection fix, including
policy/proof/artifact coverage decisions, before relying on enforcement.

No Source/game edits, actual policy activation, global inventory reconciliation,
HANDOFF/STATUS edits, builds, Source oracles, mutations, gates, providers, network,
host/storage operations, push, delegates or additional project checkouts occurred.
Tests use only disposable synthetic metadata repositories/archives. No animation,
consumer qualification, whole-module, product or campaign approval follows.
The original animation demonstration is unchanged and inactive; it needs actual
Root authoring of initial prior metadata, dedicated independent lineage acceptance,
real integration anchors and a committed schema-3 receipt after integration.

Root next: fresh **new** independent recheck of this candidate, separate generator
protection, actual mapping/activation and global reconciliation, then Root's coherent
integration gate. Do not reuse the old reviewer. Root's consumer fault/positive
composition work and the already-qualified Main axis gate are outside this lane.

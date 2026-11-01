# Inventory source-lineage packet

Branch: `packet/inventory-source-lineage`, isolated checkout
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-b4c42380`.
Base: `84337e5`. First separate evidenced judge commit: `409ca4e`
(source freeze and reproduction of the original blanket deletion rejection).

The implementation adds an opt-in, strict whole-file historical ledger to the
inventory generator. It preserves exact current tracked coverage and separately
retains historical identities, snapshots, observed counts, dispositions and open
debt. Both migration and retirement require real successors and independent
fingerprinted review. Forward reviewed chains permit later moves; immutable
snapshot proofs permit legitimate later edits without requiring live successor
bytes to remain frozen. Current conformity still requires fresh source review.

Receipt preparation uses committed policy and unchanged HEAD source. Activation
requires a separately committed schema 2 receipt binding the complete immutable
baseline and source freeze. Archives verify supplied proofs/current membership;
ordinary legacy archive rendering does not acquire a Git requirement. The existing
module/STUB contracts, ratchets and 2 MiB/no-truncation shard rules remain intact.

Scope: generator, 26 new adversarial Python tests, inventory documentation and
packet proofs/receipts, plus one narrowly added test hash in the judge manifest.
All 1,268 old manifest entries retain their exact hashes. No blanket refresh ran.
The generator was not already manifest-protected; this packet does not broaden
the judge scanner or alter constexpr enforcement. Root may separately review that
existing protection boundary.

[Verification](verification.json): **47 tests pass (21 existing + 26 new)**.
The preserved [test log](focused-tests.txt) includes missing/stale/duplicate and
unreviewed mappings; wrong immutable source proofs; receipt refresh/HEAD attacks;
added instruction tables; missing/renamed/deleted/symlink successors; cycles;
reviewed chains after later source edits; archive and legacy no-Git cases.
All 650 maintained source hashes equal the frozen base. All ten animation proof
files are byte-identical to their committed originals, including the old header's
1,035 lines / 1,027 entries / 145 reads / 882 stores.

The [judge check](judge-check.txt) actually exits **1**, solely for the original
**53 missing corpora**. No present protected hash changed; no unmanifested input
remains. The earlier pre-binding check is preserved separately and reports those
53 plus the then-unregistered new test. A full gate was not run and is not green
by inference. Builds, oracles, mutation runners, providers and network were not run.

[Animation demonstration](animation-demonstration.json) proposes the real committed
mapping from the deleted operation header to typed content, shared initialization,
native publication and reference diagnostics at `38e1c69` / seal `a833f75`.
It intentionally has no dedicated lineage acceptance and cannot activate a policy.
Existing source reviews explicitly leave retirement lineage pending. No replacement
content is exempted, no unchanged guest relocation is claimed, and no whole-source,
module, product or campaign conformity is certified here.

Root next steps: fresh independent review of this packet; integrate the separate
judge commits; independently author/accept the concrete retirement mapping after
source integration; commit policy then prepare/commit its receipt; reconcile all
global inventory artifacts; run Root's coherent integration gate. Detailed schema,
commands and limitations are in the [runbook](RUNBOOK.md). Source, active review
policy, global generated inventory, constexpr policy, HANDOFF, STATUS and Main were
not changed. No delegate, additional project checkout or public push was used.

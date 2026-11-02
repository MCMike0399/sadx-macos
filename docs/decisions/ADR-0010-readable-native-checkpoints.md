# ADR-0010: Deliver readable native checkpoints

Date: 2026-11-01. Status: accepted for the user's renewed direction to pursue
readable code and native game results together, with Astra planning/reviewing and
GPT-6.1 Sol workers implementing at medium effort and priority service tier.

The assessment at Main `75f5a20` found working native startup/menu/save systems,
integrated renderer and Task foundations, and explicit actor/caller stops.
ADR-0009's exhaustive prerequisite covers 1,129 inventoried modules, 1,063 awaiting
review. These are scope counts, not an effort estimate. Completing the whole
inventory before route advancement postpones the user's visible result.

## Decision

Restore GOAL's native checkpoint ordering. Supersede ADR-0009's requirement to
finish the entire maintained-code refactor before route advancement. Retain its
inventory, completed migrations, exact regression evidence and unfinished debt.
The complete refactor is not declared complete or discarded.

Each checkpoint includes readable implementation of the systems it needs, with
complete affected owners and borrowers migrated together. Integrate maintained
implementations before new recovery. Keep one shared implementation and canonical
ownership; preserve explicit stops for genuinely missing behavior. Untouched debt
remains open. Address-shaped shipping code, skipped actors and substitute behavior
do not satisfy this decision.

Astra owns planning, shared-owner integration, review and acceptance. Two or three
Sol workers at medium effort and priority service tier own disjoint packets in
durable worktrees. Apply GOAL section 7's proportionate verification: affected
checks during development, independent STYLE/source review, existing mutants and
holdouts where required, then a full gate at coherent integration checkpoints.
Preserve individual inherited failures and partials. Judge fixes stay separate.
Review new observation code before final qualification; unchanged valid evidence
does not need rerunning merely for a metadata correction.

Delivery means normal-input native screens and game states with content checks,
reached audio and source-only closure. A later explicit stop is dependency progress,
not a completed visible checkpoint. N3b CharacterSelect is the active target.

Product scope, roadmap milestones, fidelity, licence, asset policy and the connected
Sonic route through Windy Valley remain unchanged. External decompilations can help
source investigation; target-version, platform and reuse conditions still apply.
No upstream gameplay source is adopted by this decision.

The preserved reset packet remains unmerged and its reserved vectors unconsumed.
Resolve its adequacy findings before using it; it is not a demonstrated prerequisite
for Gamma setup. Previously stopped schedules and storage work remain stopped.

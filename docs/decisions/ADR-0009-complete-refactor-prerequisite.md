# ADR-0009: Complete the maintained-code refactor before further game development

Date: 2026-11-01. Status: accepted by the user's invocation of
[`PROMPT-complete-refactor.md`](../PROMPT-complete-refactor.md).

## Context

The user requested that all existing maintained code become readable, modular and
maintainable before further gameplay development. ADR-0007 defines that destination,
but GOAL's route-first ordering only migrates modules reached by the next native
checkpoint. The source investigation in
[`RESEARCH-complete-refactor-2026-10-31.md`](../RESEARCH-complete-refactor-2026-10-31.md)
also identifies recovered behavior outside the native build and inside reference
composition. Restricting this campaign to the shipping source list would miss it.

## Decision

1. Complete the acceptance contract in `PROMPT-complete-refactor.md` before resuming
   new gameplay, route advancement or feature development. It temporarily supersedes
   GOAL's route-first scheduling and ADR-0008 §1's alternating journey requirement.
   The product, milestones, fidelity, licence and asset policy remain unchanged.
2. Inventory every maintained module, including apps, platform, tooling, tests,
   excluded game modules and recovered behavior in guest/HLE composition. Separate
   unrecovered STUBs from readability debt. Record semantic dispositions, owners,
   consumers, destinations and verification. Lexical counts are triage evidence.
3. Establish a fresh gate, native frontier and existing journey baseline before
   behavior-bearing edits. Obtain affected system oracle results before its rewrite;
   freeze inherited failing-case identities and outputs, rather than treating an
   unchanged aggregate failure count as regression protection. Failed, interrupted
   and unavailable checks remain explicitly limited evidence.
4. Migrate complete systems in ownership/dependency order with shared readable game
   implementations and narrow reference adapters. Preserve all recovered behavior,
   aliases, callback mutation, lifetimes, lazy reads and observable store order.
   Moving an algorithm into the reference tier does not satisfy this campaign.
5. Retain ADR-0008's proportionate verification, existing journey regression checks
   and coherent integration checkpoints. Existing judge changes remain separate,
   evidenced commits under `JUDGE.md`. Each system migration receives independent
   STYLE §12 review, unchanged oracle/mutant/holdout checks and native closure proof.

## Completion and resumption

Completion requires every criterion in `PROMPT-complete-refactor.md` §4, including
zero unresolved migrations in the reconciled inventory, reviewed exceptions, a
real green integrated gate, clean native source build and preserved journeys.
An audit, pilot, lower lexical score or unchanged failing verdict is insufficient.
The final report identifies the exact verified checkpoint from which GOAL's native
Sonic route resumes. Unrecovered gameplay and unmeasured 63 FPS remain open product
work; this campaign neither invents them nor marks them complete.

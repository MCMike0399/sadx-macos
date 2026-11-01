# Complete-refactor inventory packet

Base: `f6c4e538af463a2eaf4c1fe961f04e8443d51878`.
Branch: `packet/complete-refactor-inventory` in the assigned T3 worktree.
Scope: `docs/refactor-inventory/` and `tools/inventory/refactor_inventory.py` only.

This is source inventory and review planning. No production/source behavior,
protected judge, baseline, gate, ADR, GOAL, STATUS or HANDOFF is changed. It does
not integrate preserved packets or advance the route. No delegates or providers
were launched; no build, gate, oracle, mutant, journey or game run was performed.

Read [README.md](README.md) for counts, schema, evidence limits and proposed next
system. `review.json` separates authored body/ownership findings from generated
path/include/provenance observations. All unreviewed code is explicitly open.
The full campaign's semantic-inventory and baseline requirements remain open.

Integration check: cherry-pick this inventory-only commit, then run the generator's
`--check` in root's checkout. Additional root-owned tracked files can correctly
cause drift; review additions and use `--write` to reconcile them without losing
authored findings. Do not refresh judges to accommodate this packet.

Verification is limited to deterministic regeneration, `--check`, independent
tracked-path set reconciliation, artifact/link/size validation, and deliberate
inventory-corruption rejection. Results are recorded in [validation.json](validation.json).
Root decides baseline execution and when to start the first coherent system.

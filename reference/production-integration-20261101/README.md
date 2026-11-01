# Refactor campaign baseline in progress

Behavior and inherited judges: `f6c4e53`. ADR-0009 is `d43df6f`; the initial
inventory is integrated at `ba801c1`. No behavior-bearing source has changed.

`refactor-start-state.json` preserves packet branch/worktree identities. Existing
saves, packets, game assets and captures were retained. `inherited-oracle-identities.json`
fingerprints the 67 initially present local artifacts; their age/coverage is not
a fresh oracle-suite verdict. Full copies remain in the ignored baseline directory.
`checker-gaps-before.json` reproduces dispatch argument and constexpr blind spots.
These lexical examples establish a measurement gap, not an address-classification rule.

Fresh unchanged render oracle: `ninja_render.json`, 8,209 cases, zero mismatches.
Fresh unchanged legacy/native queue lifecycle: `ninja_queue_lifecycle.json` and
`ninja_queue_native.json`, each 600 cases, zero compared value mismatches and
402 FP flag mismatches. Both remain partial.

`queue-case-outputs.jsonl` freezes each fixture identity/input and all 80 original,
legacy native and native-queue output words. Word 52 is FP status. The companion
summary identifies the exact failing case indices and hashes the complete file.
It was captured through the unchanged lifecycle judge's `Source` and `fixtures`
and the two unchanged probe ABIs; no original judge or comparison was edited.
This is baseline evidence, not permission to ignore future FP differences.

`render-symbols.json` preserves symbol lookup decisions before migration. Cached
source evidence is `reference/inventory-raw/bridge-blend-{4030d0,77ec60}.*` and
`runtime-tables-depth.*`. All source/probe identities are retained in the fresh reports.
The render oracle covers blend command production; depth, stage state, consumers
and actual GPU behavior require their own evidence. The first proposed behavior
migration is the shared render-state/packed-blend/device-write boundary, including
live device reacquisition and canonical packed-state borrowing. Full queue traversal,
matrix math, texture lifetime and Session splitting follow their own system closures.

The full gate is still running in `build/refactor-baseline-20261101/gate.log` in
the main worktree. Windowed smoke and native closure have passed, but no fresh
green gate is claimed until its real terminal status is captured. Clean source
export is prepared; native build/frontier and journey qualification remain pending.

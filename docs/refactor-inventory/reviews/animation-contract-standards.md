# Animation contract: independent standards review

Candidate `3bdbcf13df00601ca738f73d9b25d5f6348922cf`, fixed point
`53da4de17a1c1f21745d8403cd5bf6068afeb9f5`. Standards axis only.
No blocking finding; three minor findings remain.

- **P3 — Documented standard:** `src/player/animation/animation_engine.hpp:48`
  introduces `record` without the declaration-level `NOTSA` required by
  STYLE §4 and §12.9. The same omission affects new lookup, conversion,
  allocation, wrapping and sampling helpers, including lines 56, 66, 77,
  95, 104, 110, 117, 121, 164, 333, 348, 375, 392, 425, 519–741.
  The class-level annotation explains sharing but does not distinguish these
  introduced steps from recovered functions. Annotate the added helpers;
  retain the existing FUNCTION annotations on recovered entries.
- **P3 — Judgment call, Primitive Obsession:**
  `src/player/animation/animation_engine.hpp:195`, 380–385, 430–464 and
  476–478 still express established modes as numbers (`6`, `7`, `9`,
  `10`, `11`, etc.), despite the meaningful `Mode` enum. This leaves much
  of playback harder to understand than the typed dispatcher. Compare named
  modes while preserving the signed-wide cleanup check and unnamed value 14.
  Criterion: STYLE §2 semantic names; heuristic, not a fidelity defect.
- **P3 — Judgment call, Duplicated Code:**
  `src/player/animation/animation_engine.hpp:95` and :549 contain identical
  arena-failure translation under `arena_result` and `checked`.
  Consolidate that introduced helper without changing publication order.
  This is a smell heuristic, not a second owner or behavioral mismatch.

Read all 953 engine lines, affected source modules and necessary Sonic,
startup, Gamma, scene/physics/tail and arena-binding consumers. The complete
`sonic_animation_content.hpp`, `sonic_animation_initialize.hpp`,
`sonic_animation_table.cpp`, `player_animation_blend.cpp` and
`animation_state_binding.hpp` conform within this bounded review. Typed
recipes describe fields, not opcodes; arithmetic uses readable exact values.
Canonical tail/live aliases remain borrowed; playback was not relocated into
guest. Compatibility declarations, partial headers and wider Sonic/scene debt
prevent whole-system approval.

Read-only integrity: 660 candidate sources, 166 candidate artifacts, foundation
383 closure files/411 artifacts, and 970 protected inputs match hashes.
Six stored full streams and three plain-native streams compare exactly;
producer receipts retain all five FP partial cohorts and explicit boundaries.
These are inspected receipts, not fresh executions. No builds, original
oracles, mutations, holdouts, gates or journeys ran. Product closure,
constexpr/inventory reconciliation and Main integration remain pending.

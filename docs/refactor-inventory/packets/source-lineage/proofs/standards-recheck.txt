# Animation contract standards recheck

Candidate `a833f7584320eb47b8e1ff48009fe111e11c8fb7`; fixed point
`53da4de17a1c1f21745d8403cd5bf6068afeb9f5`; correction comparison
`3bdbcf1...a833f75`. Standards axis only. **No findings.** All three
prior P3 findings are closed.

- `src/player/animation/animation_engine.hpp:58` and the other introduced
  engine helper declarations now carry explanatory NOTSA annotations.
  Recovered entries retain FUNCTION annotations. Criterion: STYLE §4/§12.9.
- `src/player/animation/animation_engine.hpp:217`, :408, :460 and :506
  use named modes through `mode_value` (:32). The signed-wide cleanup
  guard (:519) and unnamed 14 (:509) remain explicit. Criterion: STYLE §2/§4;
  prior Primitive Obsession judgment resolved without inventing a mode.
- `src/player/animation/animation_engine.hpp:112` owns arena-result
  translation; :593 and :599 reuse it. The duplicate `checked` body is
  removed. Criterion: prior Duplicated Code judgment; STYLE §4 restraint.

Read the complete 996-line engine, native access/implementation, reference
access/diagnostic mapping, content/initializer and table adapters; inspected
full and correction diffs plus affected Sonic, startup, Gamma import/binding,
physics/tail, scene-resource and arena consumer boundaries. No new actionable
smell, pooled native game constants, address-shaped algorithm or fixture
transport was identified. Required bindings and faithful switches/bit operations
are justified by STYLE. Native literal access (:23–27) and reference supplied
values (`src/guest/player_animation_reference.cpp:20–25`) feed the same engine.
Compatibility headers, existing partial allocation headers and wider actor
state remain explicitly bounded debt; this is no whole-player approval.

Independent read-only checks: 660 current Source hashes, 172 current artifacts,
166 historical artifacts, 971 sealed inputs and 53 raw inputs match; all 948
parent paths remain. Exactly three Source hashes differ from the historical
660-file ledger. Decompressed current stock streams match frozen hashes for
103/16,440/7,944/28,800/2,004/1,422 cases; three native streams match for
103/16,440/7,944. Execution receipts for constants, faults and units were
inspected, not rerun. Five FP partial cohorts and explicit failures remain.
The extra publication fault is historical evidence only.

No builds, library execution, original oracles, mutations, gates, holdouts or
journeys ran. Main’s widened protection union, retirement/inventory lineage,
product closure and integration acceptance remain pending. PSetMotion and other
existing STUBs remain explicit. No gameplay or campaign-completion claim.

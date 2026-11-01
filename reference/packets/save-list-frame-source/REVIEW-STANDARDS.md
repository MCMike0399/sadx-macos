# Standards review

Candidate `d71e1cf4247748a445816ed37c31917adb15f911`; Source
`a77dd954ee02cf4ad70114cf6893b430897db027`; fixed base
`a799c518a2dc4ae9fceb3aeb05f7a41f270abb93`.
Reviewed `git diff a799c51...d71e1cf`, all four complete module files,
and all 13 changed Source paths in full, including their consumer context.
Criteria: AGENTS, complete-refactor brief, STYLE §§7/12, ARCHITECTURE,
JUDGE, and packet context/bindings/freeze/qualification.

**Documented-standard finding — P2:** `src/game/save_frame.hpp:7–14,33–37,52–58`
retains an incomplete semantic public boundary: `now_saving` and `pause_state`
are borrowed integers, while the shared policy interprets saving phases `1/-1`
and the pause transition `2 -> 1` without naming those states or documenting
unknown meanings. `docs/GOAL.md:271–273` explicitly requires names for known
mode/flag values and comments for unknown meanings; the refactor brief §3.2
requires meaningful state and enums. This inherited debt now lives in the
shared header; moving the CPP body does not complete its readability migration.
Use evidence-backed constants/enums or explicit unresolved-meaning comments.
Retain integer widths, canonical borrowing, lazy conversions and operation order;
no owner redesign or framework is requested.

**Complete-module disposition:** `save_file_list.hpp/.cpp` conform within their
bounded save-list/codec purpose. `save_frame.cpp` conforms; the complete
`save_frame.hpp/.cpp` module remains incomplete for the finding above. BOTH
modules therefore cannot yet receive unconditional conformance.

Ownership/closure inspection found one native Session list, borrowed by load,
writer, confirmation, display, observation and cleanup; no cached shadow.
Sentinel/incomplete/released payload identity is explicit. Shared frame policy
stays in game code; lazy knownness and diagnostics stay in the reference adapter.
Real message/close/writer implementations remain connected. Narrow existing
interfaces, NOTSA/provenance and retained Profiles/Completion/backend algorithms
show no additional breach. Unrelated Session/GameLoop compatibility debt is
honestly retained, without whole-consumer conformance credit.

**Heuristics:** no separate actionable smell finding; Source-order repetitions
and thin compatibility wrappers do not justify speculative extraction.

Read-only checks: all 660 Source hashes and 164 stored JSON/log hashes match.
Stored qualification retains 938 Controls FP partial identities and strict exit 1.
No builds, execution, or decompression performed. Reserved finals remain UNRUN;
product/reference closure, journeys, protection reconciliation and gate are
Root-owned and unverified here.

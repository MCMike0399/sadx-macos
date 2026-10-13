# Sonic source state machine: initial native slice

Original `sonic.exe` SHA256 is `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. Original game files remain unchanged. This is a source-native C++ owner, not execution of a guest game, and it does not yet complete a playable Sonic frame.

## Retained evidence

The original Hopper main export is `sonic-main.pseudo.c`, `sonic-main.asm`, and `sonic-main.evidence.json`, Evidence `ev_770cabb55d61b4a23c3627b0124d5a90bdf99e7ea931779db6179db1ee3190c9`. The new source-seeded Ghidra 12.1.4 batch is `ghidra-gameplay-live-recovery/`; source entry annotations are caller assertions, explicitly recorded by REA.

| Procedure | Ghidra Evidence |
|---|---|
| Main `49a9b0` | `ev_ba234573e6c1cb6b8bfca460cf97a528b7eeb9d3603e124fb4fcfaca6f8082a6` |
| Action checks `496f50` | `ev_e3a6565f7df411064b095eb1715902fa6b6d847bc7600ed63256bb8673543971` |
| Inverse floor transform `43ed20` | `ev_b94391fbff8ffb5d69caf075002572a6111274d544d00b788b608b048a224613` |
| Local speed post-update `43ee70` | `ev_7197120874332a82613ed2f0cc52c4e901774fd83b54759dd3bd499dbdd85525` |
| Ground input heading `44bb60` | `ev_2c1bcb797a910e47a62e19bc6daff909a369e1e49fe96ff5c4ab8a70d5d7de5f` |
| Damage/event guard `492d70` | `ev_ba21fb905fe24d545028597f83c1e9e2667e5a7cd19d7b77e232767614e15fde` |

The complete 14,592-byte action-check function exceeded the MCP stdio response budget, but completed and was retained. `sonic-action-checks-complete.evidence.json` and its pseudocode are extracted from the export bundle; p-code coverage is explicitly truncated. No retry provider session was opened by this owner. Collider-center `4915f0` also has retained earlier Hopper Evidence `ev_fd197234650886481716075e560c977c6aeed1cf884ea5a8685eb53173bfe45b`. LLVM disassembly and direct format-aware PE reads anchor instruction order, not decompiler types alone.

## Implemented ownership and behavior

`sonic_player.hpp/.cpp` resolves a prepared frame from the real `Task.primary_state`, explicit live world motion, the arena's `motionwk+0` reference and the player registry allocation owner. Unknown references, observed null player pointers (valid before initialization), unresolved allocations, interior references, and conflicting world views remain distinct. Attached typed task state wins over historical arena bytes. The binder does not create a task or invent caller motion state.

The main prefix implements pointer identity stores, the exact vertical-speed flag predicate, pitch then roll copies from the collision owning task (live actor view preferred), signed forced-word reads in source order, action-check gating, collider byte shape selection and second-info attribute bit, wrapped timer increment and signed range-byte writes. The real `49bfbc` table has 88 entries; raw action codes are preserved rather than Hopper's regrouped case numbers.

Action checks implement the actual `4146b0` stage predicate, mandatory special-stage pickup/event calls, word+6 damage flag/control timer paths, the source `492d70` closed guards and the timer store before mandatory `4bcc10`. The remaining predicate dispatch at `497025` is explicitly incomplete. This owner does not return a fabricated “no action transition” result.

Ground input `44bb60` implements its entry flag gates, mandatory direction reset, actual GetAnalog angle-output contract, inactive-ground comparison (both branches still call the genuine passive owner), inactive-air yaw, and active ordinary yaw. Its surface-attribute path retains signed-short raw X/Y times256, explicit Bochs P5/P6 FPATAN, binary64 memory multiplies and actual CRT `_ftol`, camera transforms, axis rotation, vector-angle projection, blended yaw then regular yaw. Optional raw/camera resources and mathematical owner boundaries remain explicit. Caller-selected x87 context is required throughout; host `atan2` and trig are absent. The entry4000 return excludes GetAnalog's event-only active/no-output route; an uninitialized angle is never silently replaced by zero.

Local `43ee70` uses the actual inverse-floor `43ed20` transform or the shared world-to-player owner, then preserves ground/air deadband comparisons, retained threshold, zero-Y rule, local speed writes, high-half angle clears and gravity/normal dot order. Unknown low halves of optional full world angle fields currently stop precisely at their high-half store after earlier state updates; the type cannot yet retain a known high16 with unknown low16. This is an explicit incomplete representation boundary, not a claim that unknown low bits are zero.

Collider center `4915f0` calls actual task-to-world, writes the transformed first-info center before reading task position, then performs X/Y/Z FLD-task-position + FADD-memory-center + FST32 in source order. Missing position retains the already written offset and matrix effects.

Recovered action-body composition uses genuine ground/air/gravity, speed, PSetPosition and local-speed owners for actions1,2,9,11,23,30,72–75,80. Action2 retains its animation comparison, jog-speed addition, collision-result transition, mandatory sound before action/requested-animation stores, mandatory `440790` and bounce X/Z stores. Action0 sets character0 and calls real PInitialize; it stops at the unresolved animation-table resource immediately afterwards. Remaining action bodies are tagged incomplete. Main common-tail animation `44a800` is mandatory, and a completed callback does not certify the remaining model/display/collision/shadow work.

## Verification and limits

`verify_original_sonic_player.py` runs unchanged original instruction bytes in Unicorn2.1.4 with private, fully known fixture memory and all 12 masked PC/RC control profiles. Its companion C++ probe uses current task/work source directly in a private build; it writes no shared archive or game file.

The final comparison contains 28,800 cases: 10,608 explicitly seeded `43eecc` local-speed suffix cases, 7,008 actual action-check-prefix cases, 7,152 main-prefix cases and 4,032 actual ground-input-prefix cases. Stored states/control-word and observed control boundaries match. There are 2,840 retained software/oracle exception-status differences (including denormal/NaN behavior); strict `passed` remains false, while the narrower `stored_state_matches` is true and is the CLI's value gate. Source/fixture/native-source/library hashes are in `original-sonic-player-comparison.json`. The table and32-byte constants are extracted directly from the PE into adjacent `.dispatch.bin` and `.constants.bin` files.

The local-speed comparison seeds the transformed stack vector at43eecc; it does not prove producer43ec00/43ed20 equivalence. Ground-input cases execute original GetAnalog and stop at required rotation/raw-input boundaries; they do not claim complete alternate-surface/physical input proof. Fully native transform and collider-center unit tests consume the source sine/unit assets, test matrix restoration and partial availability stores. Optimized and UBSan tests cover all 12 explicit CW profiles.

Required next work includes the remaining `496f50` predicates/transitions, authoritative event/task fields for those predicates, action bodies, the actual `3c56210` animation table and `38f6e80` model tree, `44a800` animation advance, `491660` idle animation, `495cd0`, `440790`, audio/task effects, `43fa90`, `4948c0`, held-object update, source byte+7c lifetime, collision-entry/radius updates, shadow/material work and final `446f10`. Retained Ghidra bodies are available for several of these; no success placeholders close them. Main cannot be called complete until these mandatory owners are bound.

No Windows reference game/mod runtime, Intel Ultra FPATAN measurement, native playable Mac frame, OS input producer or physical-controller gameplay equivalence is established by these fixtures. Hardware testing remains deferred. Source-derived C++ code was written from instruction and retained decompilation evidence, without copying SDK/game source text. It reuses the existing native math owners with their pinned SoftFloat and Bochs notices; no new third-party source or license grant was introduced.

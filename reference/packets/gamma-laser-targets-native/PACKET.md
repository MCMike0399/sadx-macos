# Gamma buffered input restoration

Branch `packet/gamma-laser-targets-native`, base Main `06f6d652`. Product source
is `ace1a719`; judge commits are `aacb5e98` and `3a6f30d2`, including diagnostic-only adaptations.
Root owns independent review, registration, admission and the next full gate.

The actual `GammaUpdateLaserTargets` body is input restoration, not targeting.
Complete source dossier `ev_55d320b6e82244cb91f5bdce7fc47fd7fa7a838c8adfaa3c3240b272437eb801`
shows exactly two raw word copies from `E102WK.input_buf[7]` into the selected
`input_dataG` record: angle read/store, then stroke read/store. There are no
callees, lists, task lookups, vectors, matrices or imported resources in this body.
The retained historical symbol name is explicitly described as a former hypothesis.

The implementation keeps the buffer in the existing E102WK arena allocation.
It does not add a typed mirror or initialize unknown fields. The destination
borrows the actual native Input object's normalized analog table, which now has
canonical `input_dataG`, 64-byte provenance. GetControllers and analog_snapshot
use that same owner. The source callback supplies its passed work and player
index; the captured null current-player alias and the existing PlayerSystem,
registry, scheduler, arena, SceneState, geometry and motion ownership persist.

The adjacent `4c3780` producer was analyzed: it shifts eight buffered inputs,
appends current input and optionally publishes the oldest. Its native seam
remains explicit. First initialization skips it, then CAlloc zeroes the actual
580-byte block. Native consumption and original E102 with unchanged concrete
lower receipts independently observe all sixteen buffer words zero, correct
call order and the selected output slot. This is not a PE BSS or menu-empty
assumption. Later player modes and the history producer remain unimplemented.

The real source-built normal-input app advances to `native_gamma_get_model_matrix`
at frame 1332, exit 1, completed frames 1330. The inspected outgoing-title capture
is byte-identical to baseline, SHA-256
`c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.
GetMMMatrix and its persistent buffer/count/limit/active producer remain missing;
no matrix or successful actor frame is manufactured. Active welding still stops
at CalcMMMatrix. This is not completed welding, an actor frame or N3b.

Development 636 cases and final holdout 2,172 cases have zero mismatches,
including 469 and 1,755 failure prefixes. The original/native observer compares
all 1,024 words of shared memory, selected slots, alias effects and ordered
access denials. Stroke bits include signed zero and signaling NaNs; no arithmetic
or FP partial is introduced. Access denial prefixes do not prove hardware fault PCs.
Four compiled copy/order faults and three compiled canonical native-binding faults
are caught. The actual native caller uses distinct input values, raw nonzero
buffer words, different passed/current work identities and explicit angle,
stroke and owner failures. The source buffer remains byte-identical after reads.

The app and force-load closure build at `-j2`; the new generated caller command,
inherited motion and weld callers pass. Their 11,144-byte motion and 12,494-byte
topology observations remain identical. Only their stop diagnostic changes:
old fixtures do not supply the newly required input owner and correctly fail there.
Existing numeric inputs, tolerances, mutants and oracles are unchanged; inherited
Gamma summaries that contain FP counts do not establish individual partial identities.

Provenance, unique owners, layers, readability and profile checks pass, with
zero active unhandled overlaps. Finite registration and two unchanged input-table
consumer fingerprints remain Root-owned. All 1,412 inherited protections, including
53 ignored worktree-absent inputs, remain present. No blanket refresh was performed.
See ROOT-WIRING.md, REGISTRATION-PROPOSAL.json, SOURCE-EVIDENCE.json,
VERIFICATION.json and PRESERVATION.json for commands, exact owners and limits.

No Main write, push, agent, new checkout, full gate, reserved Task/reset execution,
schedule, storage or provider change occurred. The owned Ghidra session used 2 GiB
and closed cleanly. Only copied, dereferenced public SDL/libusb dependencies were used.

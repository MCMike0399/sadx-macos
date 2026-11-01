# Independent Task outcome observer review

Candidate Main `40b4634704b362c5d9338b9b56271e6ff1544727`; implementation
`f7977d9`, development definition `6b15cec`, results `667decd`.
**Disposition: changes required for complete outcome/fault observation, before
adding the parent runner.** No Task Source, campaign, or final execution approval.

## Prioritized finding and reproducer

**P2 — Direct lifecycle emulation has no invalid-access observer.**
`reserved_observer.py:119–127` samples the pool initializer's end PC only when
the next helper begins, and installs `record_faults` only around `previous(call)`.
That hook is deleted on helper return. The unchanged lifecycle judge executes
the pool initializer directly at lines 41 and 98, and inactive destruction
directly at line 99, outside that scope. An invalid emulated access there aborts
but loses PC/access/address/width; the final receipt retains only the outer
exception and preceding successful helper rows. Pool initialization that raises
also never gets its sampled end PC. This violates the claimed complete fault
signature/partial-observation contract, independently of the missing parent.
It is an evidence-loss defect, not demonstrated false acceptance.

Fake-only reproduction, run for both direct phases: provide a machine whose
`hook_add`/`hook_del` maintain a dictionary and whose helper returns at STOP.
Enter `audit_helpers('task_lifecycle', ..., [[(1,2,0,0)]], events, read_ip)`;
call `40b1ce`. For inactive destruction, additionally call `40b860`, then start
the control with `40b1ce` and `40b860`. At either direct phase the hook dictionary
is empty. Set the fake PC to the direct entry and raise a synthetic invalid-read
exception. Every retained `memory_faults` list remains empty, and the active
event has no error PC. `original.call` is correctly restored. No emulator ran.

Observe these direct executions through their full duration, retain their own
attempt/end/error records and fault signatures, and test both failing phases
with fakes. Preserve STOP versus `401000` semantics and unchanged comparators.

## Complete-file Standards and Spec dispositions

| Complete file | Standards | Purpose-specific Spec |
|---|---|---|
| `reserved_observer.py` (401 lines) | Accept reference-tier addresses, semantic helpers, structured control flow and restoration; no independent style blocker. | Partial: direct lifecycle fault coverage above prevents adequacy approval. |
| `test_task_reserved_observer.py` (362 lines) | Accept isolated fakes and meaningful real-recorder checks. | Partial: exercises traversal interruption and wrapped helper boundaries, but misses both direct lifecycle failure paths. |
| `reserved_inputs.py` (63 lines) | Accept deterministic, bounded generators and explicit ownership bookkeeping. | Accept supported-domain generation: allocation indices include children, only live top-level parents/destruction candidates are selected, and final owned releases are appended. |
| Retained development driver (62 lines) | Accept bounded one-shot characterization and truthful limits. | Accept development purpose only: immutable BEFORE identities, pinned interpreter/Unicorn, repeated delete labels and synthetic fault separation. |

The prior `d83ab15` capture-binding disposition survives: N/2N+2/N/N-minus-
exclusions, ordered symbols, returns, scalars, buffer extents, input hashes and
exhausted full-record validation remain intact. Original judges, recorder,
comparator and generator are unchanged since that review.

Traversal attempts bind actual ordered inputs; STOP, callback/material/saved-next
stops, exact known exclusions and unexpected errors stay distinct. Returned
source with excluded projection remains excluded. Helper entries/return PCs,
successful pool end PCs and inactive `401000` are recorded without claiming
whole gameplay return. Finally receipts preserve attempted/unattempted IDs.
Fixture/main/argv, Source.run, helper and installed fault hooks restore on exits.

## Actual checks and limits

All **44 focused tests passed**, exit 0, using system Python with bytecode writes
disabled. Both additional direct-phase fakes reproduced P2. Read-only validation
matched driver/helper/observation/input/report hashes and accepted every retained
report and complete native capture: work 1 call, lifecycle 4, display 48, delete 2.
Retained outcomes match the declared returns/stops/exclusions. Traversal expected
words are retained; helpers retain scalar results/PCs and arena hashes rather
than separate full original arena/trace copies.

No originals/native/emulator replay, builds, mutations, providers/network,
children, reserved inputs, protected edits, refresh, gate or push occurred.
Pending parent recipes, committed closure seal and single-use admission are
acknowledged prerequisites, not additional findings. Source72 review supplies
no helper-conformance evidence here.

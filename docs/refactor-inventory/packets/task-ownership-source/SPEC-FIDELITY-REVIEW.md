# Independent terminal Source spec/fidelity review

Verdict: **bounded acceptance; no introduced spec/fidelity defect found** in the
assigned complete Source and helper/test scope. This accepts the Task ownership
migration against its immutable predecessor and existing supported contracts. It
does not approve integration or expand original-game coverage.

Reviewed immutable before `bd22ee423ea015f00152a97fa8fc2e1b4034b411`, exact Source
`72d0e83ead1255aa658ef6b78220c04b1b737b93` (`78702db`, `bceff1b`, Root enum
correction), bindings `4d4cf1061800cd0312f03c6a3c5ee636f6ed6a18`, and terminal
qualification `54d27d6`. I read all four complete engine files, every actual
Source consumer diff, adapter, final probe/fingerprint changes, and all seven
complete bodies in `REVIEW-SCOPE.json`. Older Main Task was not the candidate.
The prior bounded Standards approvals/recheck remain limited to those four
engine files; this review does not infer broader consumer conformity from them.

The following checks were read-only inspections of exact Git blobs and retained
files under `t3-c8fa9287/build/task-source/`. No builds, executable tests,
oracles, mutants, holdouts, reserved inputs, providers, network, delegation,
global regeneration or push were run. Only this report is committed.

## Source fidelity

`task.cpp` retains allocation-before-fill publication, primary/force/motion/
auxiliary ordering, rollback behavior and the late default-resource read.
`initialize_source_pool` requires an explicit native resource, including known
null. Reference missingness is installed separately and throws only when creation
actually consumes the default; a live callback resource bypasses that read.
NativeGame installs that unavailable boundary explicitly. No missing fact is
converted into a successful guessed zero.

Saved-next capture, slot-zero engagement and current-generation resolution remain
intact. Execution context is cleared after successful callbacks, retained on
tagged callback failure, and follows the existing distinct host-exception rules.
Nested walkers do not restore context that inner dispatch mutated. Display
reloads children after callbacks. Graph validation precedes publication; the
temporary plan adds no persistent graph owner and retains native work/state
identities. The actual Bridge still imports callback mutations before advancing
its continuation; register, pool and diagnostic conversions remain reference work.

Cleanup extraction preserves unlink-before-resource-read order, collision and
object-chain lazy reads, next-before-free snapshots, staged failures and explicit
resume. Native auxiliary release still occurs after arena release and before
retirement. Outermost `WalkGuard` controls deferred reclamation. Destruction
retains its argument by value across a destructor clearing the canonical handle;
reentry rejection and generation checks remain unchanged.

`task_work.cpp` preserves splitting/coalescing, stale header bytes, padding,
metadata publication and failure order. Live payload reads precede cached
reference resolution. Weak released-generation identity, interior offsets and
saved allocation offsets remain separate. The external pointer encoder supplies
representation, not allocation identity or policy. Player child-state creation
still initializes the child before reading the parent's position and copies
position then angles without copying shadow arena pose bytes back. All recovered
scheduler/allocator policy remains in engine; native sources gain no upward guest
dependency.

The complete `TaskStep` switch maps interrupted operations to their former PCs,
including `Membership` to `0x401000`; callback dependency diagnostics retain the
auxiliary source boundary. Scene logic keeps its preexisting nested `TaskResult`
ownership. Auxiliary results instead carry plain `TaskStep`: the correction in
`bceff1b` introduces no diagnostic `TaskHandle` retention. Actual probe bindings
serialize those semantic diagnostics into the old observations.

## Complete helper/test dispositions

These dispositions come from direct full-body review, independently of Source
Standards acceptance:

| Complete file | Spec and bounded Standards disposition |
| --- | --- |
| `src/guest/task_reference.hpp` | Accepted reference infrastructure: exhaustive named-step conversion, lazy unavailable-default adapter and pure pointer encoding; no recovered allocation/scheduling policy. Its player diagnostic dependency remains reference-only. |
| `tools/oracles/task_judges/mutate.py` | Accepted qualification infrastructure for this packet: historical table remains intact; fourteen candidate anchors preserve fault intent. Three unmodified drivers run first; compile failures and timeouts are nonkills, and historical candidate kills require reported semantic mismatches. Explicit immutable base and copied source keep mutations out of maintained engine files. |
| `tools/oracles/task_judges/bridge_faults.py` | Accepted qualification infrastructure: all three fault intents remain; current-base copying and actual private archive linkage are justified. Nonzero-run classification is bounded by the inspected positive consumers, fault logs and link maps, not a universal infrastructure-failure discriminator. |
| `tests/unit/task_arena_transport.cpp` | Accepted focused reference contract: metadata/payload ordering, live null/redirect, interior/self aliases and expired/reused generation assertions are meaningful. No reference policy migrates into engine. |
| `tests/unit/task_native.cpp` | Accepted guest-free native contract: explicit resource defaults, full flag/index widths, slot zero, detached/list aliases and isolated worlds. Selected flag combinations supplement existing lifecycle coverage; they do not exhaust every mask. |
| `tests/unit/game_scene_logic.cpp` | Accepted consumer contract: previous cases remain; added failure checks preserve prefix, task identity, semantic PC and live context. Dense inherited formatting remains maintenance debt; no whole game-scene module Standards approval. |
| `tests/probes/task_scheduler_contract.hpp` | Accepted prepared reference contract: all eleven scenario bodies retain lazy availability, exact numeric boundary assertions, context/reentry, graph publication, failure-prefix and reclamation observations. Numeric PCs are appropriate here. Its test-only friendship is structural inspection, not production policy. |

## Evidence and limits

I independently verified all seven scope hashes, every `FINAL-ARTIFACTS.json`
hash/size, changed Source blobs against Source72 and the frozen checkout, all
18 compiler-derived probe dependency hash sets (including Sonic's forced
fixture), and all 20 changed Source MMD memberships. Fingerprint additions track
dependencies; comparison logic, cases, tolerances, seeds and original ranges
were not relaxed or replaced with approved output hashes. Added tests are
registered; inline reference dependencies have explicit Make prerequisites.

Using the byte-exact retained `9a6707e` comparator's validation function, I checked
every old/current record and complete identity ledger: **16/40/236/244**, with
equal padding, returns, buffers, context and arena observations. Original receipts
retain work **16 sequences/4,890 operations**, lifecycle **19/2,316**, display
**236 = 179 complete + 55 failures + 2 saved-next boundaries**, and delete
**244/250**, exclusions **85,87,91,121,143,201**. Inactive `401000` is a stop;
original return is unobserved.

All **23 focused/borrower units plus five actual Bridge/NativeGame programs**
have retained exit-zero receipts. Eighteen probes compile; private fresh native
app/force-load/closure receipts pass. Fourteen scheduler and three actual Bridge
faults compile and discriminate, with three scheduler controls zero. I inspected
raw logs and actual mutant link maps. Invalid setups and the initial skip-unlink
survivor remain nonkills. I also verified the entire **800-case scene** outcomes
and complete startup/auxiliary/collision observation equality, excluding only
declared library/source identities and case-output path. Startup remains
full-emulator false/return unobserved; auxiliary strict false; collision remains
**exit 2, 6,462 FP partials, zero value mismatches**, preserving every row and
identity. These are inherited limits, not passes or introduced defects.

Root retains reserved qualification and final runner/per-case STOP/fault accounting/
admission, source-only isolation, journeys, protection union, inventory closure
and coherent integration gate. Reserved inputs remain ungenerated. Separately
reviewed Root generator/observer helpers (`5967027`, `d83ab15`) receive no approval
from this Source verdict. No Windows/hardware parity, arbitrary-callback closure,
broader PlayerSystem, gameplay or complete-campaign approval is given.

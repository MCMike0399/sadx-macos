# GOAL 6.3: reference task graph synchronization cost

Base e101efc. Resident real-stage probes reproduce 187–189 ms guest-only frames;
frozen replay of the same 443-draw frame takes about 7.3 ms through Metal.
Sampling identifies task bridge import/export as the dominant cost, especially
export_graph's 4096 * 13 scalar reads per callback. After bulk comparison,
set membership construction/destruction dominates import. Hypotheses and one-variable
probes preceded edits; no renderer or native override is disabled to obtain speed.

The focused regression ran red before the fix: 128 real guest callback dispatches
perform 6,871,571 read API calls. The fix reads the entire pool once per export,
retaining per-word changed stores and their order. Failed bulk reads keep the
previous scalar behavior. Import uses slot bitsets only for membership; original
free-list order, bounds/duplicate/link/control/work checks and callback resync remain.
Per-guest diagnostic counters count API reads, including failures, not guest instructions.

The regression runs twice at 2,708 reads with exactly the same 54,966,380 requested
bytes, callback order, own-task resource mutations and opaque free-slot stores.
Forced scalar fallback makes it fail. The unchanged complete task bridge suite
passes all arena/trace/register/mutation checks. No game-tier task algorithm changed.

Actual guest-only medians: 187.09 ms before, 107.13 after bulk reads, 65.17 after
bitsets. Rendered 100-frame stage advance improves 4.91 -> 14.47 FPS, with an
identical world screenshot and identical frozen-frame pixel hash. Native intro,
controller and stage digest/mode/stage traces equal the existing census exactly.
This is progress toward R5; the 63 FPS budget and full practice readiness remain open.
All numeric evidence and scope limits are in task-sync-167-summary.json.
One-off profiling/capture code and captured assets stay in ignored/debug locations.

# API binding changes

Source commit a77dd95 replaces optional list payload storage with a tagged sentinel /
unpopulated file / FileInfo variant and a borrowed nullable filename parameter.
Five probes and two units require mechanical binding changes: reads use file(),
assignments use set_file(), null filename uses nullptr, known strings are borrowed.
No original input, output layout, return declaration, expected stream, tolerance,
case selection or exclusion changes. Full native IO/frame streams after rebinding
remain byte-identical to both the fresh baseline and historical foundation.

The shared algorithm moves from save_frame.cpp into save_frame.hpp templates so
reference-only lazy field proxies can call precisely the same expressions. Existing
FrameStateView/FrameCalls and actual native message Context bindings are unchanged.
Historical mutant text locations therefore require a separate rebinding runner;
original specifications and prior successes/survivor remain immutable.

`rebind_mutants.py` parses the immutable fourteen-fault specification as data.
Payload release now targets clear_file(); frame policy faults target the shared
header using an include overlay. Eager phase explicitly captures int32_t, because
`auto` would copy the lazy proxy and accidentally defer the read. Reference eager
exec captures the same optional before completion and substitutes it only for the
exec load. Every variant must compile and fail the unchanged characterization;
setup/compile failures cannot count as kills. Five completion faults use the
unchanged historical runner/specification directly.

Additional `save-policy-boundaries` registration appends one standalone native
contract, without changing existing cases or frozen outputs. It distinguishes the
sentinel from a published incomplete/freed file node, retains full-width time,
and drives nested before/after scheduling on the same state and message Context.
Writer/close exceptions assert reached stores and pending values. This adds
specific identity/reentry coverage beyond the immutable systematic streams.
The added boundary target lists its complete compiler-discovered local dependency
closure (`clang++ -MM` over all five translation units), preserving header-triggered
rebuilds. Existing targets and forced reference fixtures remain unchanged.

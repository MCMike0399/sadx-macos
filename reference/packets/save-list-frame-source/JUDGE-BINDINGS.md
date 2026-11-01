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

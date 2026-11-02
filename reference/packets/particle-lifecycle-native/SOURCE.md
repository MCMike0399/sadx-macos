# Source recovery and ownership

Pinned PC image SHA-256 is
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
source-context.json retains REA evidence identities, body ranges, pseudocode,
complete assembly and direct callers/callees. Original names are hypotheses;
assembly establishes all field widths and operation order.

The generated broad seed import did not retain a procedure at 4cc1e0. The
minimal reproducible seeds.tsv/config creates it, without changing Root seeds
or symbols. Both owned runs used the configured 2 GiB maximum and closed.
The retained destructor evidence is ev_2a023d150672aa4194aa422bf0cb3185b0ac69ca5df7844fbe1e6de0dfa51f45.
No constructor-address extrapolation defines its behavior. The two new symbol
proposals are finite additions for Root registration.

The existing 4cc210 owner and constructor are unchanged. Task.awp's typed Work
remains authoritative; raw allocation scalar/pointer bytes are not synchronized.
Particle slots and both index buffers remain the original TaskWorkArena handles.
Particle is a borrowed access view, not another slot allocation or copied state.
Imported Frame storage remains mutable and borrowed, with its original lifetime.
No scheduler, texture list, frame array, geometry or queue owner is introduced.

4cc080 reads signed-short active count and signed-short active indices. For each
slot it adds velocity to position X/Y/Z, in order, then applies drag to X,
drag minus vertical acceleration to Y, and drag to Z. Every scalar store occurs
before the next scalar operation. It captures the frame pointer before physics,
then invokes 4cc470 on the slot's animation tail. Only -1 removes the slot.

4cc150 passes the same slot, a reference to the retained work's two parameters,
and its live frame pointer to the custom updater. Exactly zero removes; other
return values keep the slot. The callback is re-read for each invocation.
Both update variants always call 4cc040 after the loop, including empty pools.

Removal reloads Task.awp, but loop bounds continue reading the originally
captured work. Positive removal count is decremented before the active-index
read and free-index store. The active buffer is captured before decrement,
reloaded after the free-index store, and its last index replaces the removed
slot. The loop repeats that slot. Callback replacement and buffer aliasing are
observable. A fresh display call captures the task's then-current work; its
loop retains that identity across rendering callback reentry.

4cc470 uses integer frame/loop words, despite Ghidra's float-pointer guesses.
It looks at descriptor frame_index + 1, using wrapped 32-bit stride arithmetic.
An initially negative texture index returns -1 before arithmetic. Otherwise it
adds animation speed to time, spills binary32, stores time, and compares against
that descriptor's parameter 0. Unordered and less return 0. On reaching the
threshold, the following descriptor's -1 sentinel clamps time and returns -1.
A -2 sentinel increments the unsigned loop counter first; below its global-index
word interpreted as an unsigned limit, it resets frame index to zero, stores
remainder time and returns -2. Otherwise it clamps and returns -1. Other
following indices advance frame_index once and return 0, even for large time
steps. Frame increment, loop increment and descriptor multiplication wrap.

4cc040 makes one 4cc540 call per live entry. The four source arguments are the
same slot base twice, that slot's animation tail, and the captured frame-table
pointer. The lower routine consumes position, appearance (including scale at
slot +24), animation, shared suppression/render/texture/matrix owners. Its
recovery dossier is retained, but its renderer is not implemented here.

4cc1e0 captures work once, calls FreeMemory on free indices, then reloads and
frees active indices, then reloads particle storage and tail-calls FreeMemory.
It clears none of these fields. The scheduler subsequently owns primary/auxiliary
release and recycling; the destructor must not release the work32 allocation.
Null buffers reach FreeMemory normally. Duplicate/foreign/released native arena
handles fail explicitly instead of inventing allocator behavior for corruption.

49fb30, retained for Root, calls FreeTask on a nonnull GammaBurstTask then
clears the global after the call returns. FreeTask is delayed destruction:
the old pool may coexist with its replacement. Binding this PlayerSystem-owned
helper remains outside this packet.

# Reference guest heap readability context

Base1300815. This is synthetic reference allocation, not the source Ninja heap-handle
global owner or native product memory. GuestHeap backs32bit guest-visible tokens with
optional bytes: fresh bytes stay indeterminate, zero-size requests have one readable
byte, and writes establish only their addressed byte. One token is reused by create;
alloc need not follow create. Caller bases are not real Windows address observations.

Actual readers/writers/free trace and private block ownership remain unchanged. Preserve
foreign-handle traces, strict upper-address guard, eight-byte span/gap, successful zero
token versus absent result, untouched fresh bytes and failed writes. Preserve RHS byte
allocation before map publication and next-address advance. Do not add reuse, source
address claims, guessed zero contents or host-OOM recovery. Main full gate runs while
this private module packet changes; no Main source/build is touched.

Unchanged unit and full mixed-operation contract are frozen before source edits.
The contract observes results/live block count and complete C/A/F trace, including zero,
unaligned and exhaustion boundaries. Source-only narrow builds need no game assets.
NativeGame reference composition, CRT instance, Ninja-memory and ownership consumers
must retain their existing boundary contracts. Full hardware/Windows allocator parity
and uncaptured source addresses remain outside this module's replacement policy.

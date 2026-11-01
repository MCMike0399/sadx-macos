# Complete reset-system packet: implementation scope

This continues ADR-0009 after the qualified Task checkpoint. Task source 72d0e83
is integrated at c94ef72. Gate bf61403 passes 306 native programs and 607 Python
tests; the same-source qualified Python supplement leaves one inherited skip.
The full refactor remains open. This document records scope, not source approval.

Use the existing context-game-reset.md and mapping, then verify current callers.
Architectural consultation after Task confirmed that queue/render is not a prior
whole-system rewrite. Queue, depth, view and fog remain named established calls.
No new gameplay, guessed allocation or whole Session redesign belongs here.

## One implementation and all maintained leaves

Consolidate the duplicate reset sequence into one semantic implementation based
on cleanup::reset. Keep its native borrowed view and named operations. A narrow
internal state-access/operation adapter may preserve reached-read availability,
immediate stores, logical steps and reference-only PC/pending-call translation.
Do not snapshot all inputs, copy back at return, or add a universal registry.

The complete scope includes every maintained run_reset_leaf entry and borrower:
volume, depth/queue setters, loop/list reset, level destructor and SET/CAM release,
sound records, media close/init, null-message reset, original vibration compatibility
and constant debug formatting. Share existing cleanup/audio policy. Extract any
remaining recovered algorithm into readable system helpers. Guest may retain
routing, ABI conversion and explicit support limits, not the unchanged switch.
WinMain/config and sound borrowers must migrate with the corresponding owner.

Concrete source trap: legacy dsStop_all reads flags BEFORE timer publication;
cleanup::stop_all_sounds currently stores timer first. Freeze unavailable-flags
behavior before changing the shared access sequence. Known POD native values
agree, while reference partial stores distinguish the order.

Preserve loop list-presence read before zero stores, media player reacquisition
after stop, SET/CAM release before level callback read, the separate mission-mode
read and callback clearing only after success. Preserve initialized-reference
StopPCM cut 41010d at its reached seam; native stop_pcm stays fully functional.
Pinned-profile reset uses Input replacement; standalone original vibration
compatibility stays qualified separately. Unknown readiness is not cold zero.

## Canonical control values and opaque identities

Put common plain declarations low enough for player/Sonic: one u32 late-frame/
message-work value and one u16 editor value per world; knowledge is reference-only.
Migrate owner extent, import/export/defaults and every maintained borrower.
Native reset_status_ and text_window_pending_ currently have only zero production;
canonicalize source identity without claiming arbitrary divergent native values
were previously equivalent. Keep reached native_pending_text_window failure.

Editor is u16 despite SDK Bool. Retain signed16 widening on Sonic read and upper
adjacent source bytes. Read the separate forced word at 4258f0 BEFORE editor at
4258f7; BOTH reads occur even when the first is nonzero. Unknown fails before
action 87 publication. Use a semantic lazy access/observation seam, not optional
fixture returns or shadow state in game code. Establish NativeGame's currently
unwired borrow and prove reset-to-Sonic alias through a real bounded consumer;
that is new composition, not evidence Sonic gameplay is already executed.

Reference buffer tokens may be stable addresses of real host token objects,
with a reference-only guest-identity map. Zero is nullptr; unknown is sidecar
knowledge. Never reinterpret a guest integer as a host pointer, dereference a
token as payload, or free it with a real allocator. Preserve map identity through
callbacks, including aliasing: two source releases stay two calls; clear slots
only after success and reacquire slot 2 after slot 1 callback. Reject foreign
unmapped tokens. Token lifetime is not original allocation lifetime. Native
nonnull release remains native_heap_buffer_release; producers/generations/reuse
remain unqualified. Existing arena/scheduler replacement order stays unchanged;
do not silently change to original allocation reuse.

Seed publication is a WRITE. Preserve existing CRT owner through a write seam;
never require its old availability or invent a seed cache for a borrowed view.
Three calls precede both unconditional word stores; ten level calls precede
buffer releases; fog precedes the final seed write. Pending reference payload
is published before the owner call and survives missing/cancelled/failed outcomes.

## Qualification and handoff

BASELINE.json freezes current 300 reset and 300 composed cases. The composed
judge maps all 14 pages and substitutes heap/task initializers; it does not prove
missingness or real replacement lifetime. Existing current cleanup 2,604 cases
were already rerun during Task integration, not rerun just for metadata here.
SETUP.json pins interpreter, PE, numerical archives and registered recipe.

Before Source edits, freeze complete immutable-old output identities for changed
loop/Sonic/stream-player/WinMain/sound consumers and a small boundary matrix:
unknown/known-null, early/intermediate/late failures, pending payload, high bits,
aliases and callback replacement of later buffer/player/level callback. Include
no fog/reseed after earlier failure and live frame/editor reads. Preserve composed
audio boundaries and all per-case inherited failures/FP partials. Task case 207
remains inherited source FAIL with eleven differences; never add an exclusion.

Use separate evidenced judge/binding commits and Source commits, meaningful
mutants and a reserved final cohort left unrun for Root. Complete native/reference
consumer closure, source-only/force-load, journeys, independent system review,
finite parent protection/inventory reconciliation and full gate remain mandatory.
If a leaf cannot migrate, leave that module/system item explicitly open. Stop at
actual allocator/media/gameplay recovery rather than guessing missing behavior.

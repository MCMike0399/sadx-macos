# Root binding contract

The new lifecycle source ends in _native.cpp and is already selected by the
existing native wildcard; no source-list edit is needed. The four callback
identities stay unchanged. Empty newly created pools run update, display and
scheduler teardown without requiring arithmetic or render bindings.

For nonempty standard update, register one particles::Binding on
particles::update_task through TaskScheduler::register_callback_context.
Its arithmetic member borrows the actual existing live fidelity::Context;
this packet supplies no default precision, rounding mode or fresh FP owner.

Rendering remains the explicit DisplayParticle (4cc540) lower boundary.
Register the same or another Binding on particles::display_task only after
recovering/composing that complete lower routine against its retained dossier.
Binding::display takes (particles::Particle &, particles::Frame *). The particle
is a borrowed canonical arena slot; storage and byte_offset retain identity.
The source passes the same slot as position and appearance and its +32 tail as
animation. Do not clone positions, appearance, geometry, frames, queue state,
textures or matrices. Pass shared owners to the existing renderer. The callback
must observe frame changes immediately and preserve the original suppression
check, blend changes, descriptor interpolation, texture selection, matrix order,
four-vertex draw and restored render states. The native default throws
Operation::DisplayBinding, and the tests explicitly check that stop after real
physics. A sink observer in tests proves arguments/order, not rendering.

Particle::word/scalar access the arena individually. Use Field::FrameIndex and
Field::LoopCount as integer words. The remaining appearance words at offsets
24 and 28 are deliberately not named/recovered for production access here.
Recover their complete typed meaning when implementing DisplayParticle; extend
the same borrowed view, not a second slot owner. Frame::parameters[0] is the
source animation threshold. Frame::global_index on a -2 sentinel is also the
unsigned repeat limit; no immutable converted frame cache is valid.

Keep imported frames alive through pending task destruction. Work::frames is a
borrow, as in the accepted constructor. FreeTask alone does not end that borrow.
Root's Gamma release helper must call scheduler.free_task(old), then clear the
PlayerSystem global only after successful return. A new effects initialization
can publish another task before the old one runs its destructor. The scheduler
releases primary/auxiliary storage and recycles the old task after its three
particle buffers are freed. Do not clear the old Work buffer handles prematurely.

Failure carries Operation, Field and TaskWorkFailure. These are semantic facts;
source PCs live in source-context.json and the reference oracle, never engine
code. Supported protected-page store faults map to 4cc0b6/0bf/0c8/0d4/0e3/0ef,
4cc49c, 4cc4bc and 4cc4d3. Null-frame consumption is 4cc485; custom null call is
4cc17d/target zero. Arbitrary callback/transport failures at repeated field reads
are not qualified as a unique-PC diagnostic. Do not report them as proved.

Remaining boundaries: DisplayParticle's actual renderer, particle spawning,
NeonuLoadTexture's nonzero flag path, Root's Gamma global-release binding and
actor/common-tail integration. Cross-allocation negative-index/corrupt-pointer
aliases, malformed frame-table bounds, host allocation failures, unmasked or
deferred hardware FP faults and arbitrary concurrent mutations are unqualified.
No actual actor frame, native N3b or app-frontier advance is claimed.

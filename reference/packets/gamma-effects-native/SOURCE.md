# Gamma effects source context

Base 5c1d44d72e03aad77a300df823d47a55271c9571. Pinned original PC image
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.

The source entries, not the old gamma-actor.json descriptive labels, identify
this cluster. E102 483430 calls 49fb10 after setup 4c5420 and the five material
stores. 49fb10 binds 91d168 against E102_HIT_TEXLIST 91d15c with flags zero,
then tail-calls 49f9f0. The latter reads live task owner 3c56df0; only if null
it binds 91d458 against the same texture list, then calls 4cc210 with count 40,
a stack pair {0.03f, 0.0f}, the same 91d458 table, and null custom updater.
The final returned task, including null, overwrites 3c56df0.

4cc330 walks signed-short texture indices until the first negative sentinel.
For nonnull live texture memory it publishes the memory's global index. Flag 1
in the record loads unsigned width/height, flag 2 computes half-width/height.
Null texture memory leaves the complete frame unchanged. The optional function
flag 1 invokes NeonuLoadTexture before reading any frames; the actual two calls
pass zero and reuse already loaded character textures. No private texture owner.

4cc210 calls CreateElementalTask(2,4,4cc080), then MAlloc(32). A null task returns
null; a null work allocation destroys the just-created task and returns null.
Success publishes task.awp before allocating free indices, active indices, and
48-byte particle slots, in that order. Buffer allocations are not checked.
It copies the two parameter words, frame-table identity and custom updater;
a nonnull custom updater changes task.exec to 4cc150. It initializes only the
free-index words 0..count-1, then installs display 4cc040 and destructor 4cc1e0.
Negative counts preserve wrapped allocation arithmetic and skip the index loop;
capacity is truncated to a short. No buffers are cleared by this constructor.

TaskArena and TaskScheduler remain canonical. Typed task auxiliary payload holds
the actual pool fields, with arena handles for all three buffers. Gamma setup,
sprite registry, mutable materials, current-player and character lists are borrowed
unchanged. Actual imported texture binding is character 6/list 4, E102HIT,
seventeen entries. Declarations 91d168 and 91d458 include their sentinels and all
padding/parameter bits. Frames are mutable stable imported storage; initialization
must re-read and mutate them on each source call.

No initializer calls particle update, display, cleanup, spawn, RNG or animation
stepping. These remain named explicit stops with the real callback identities.
The next Gamma caller boundary after successful effects initialization is PSetMotion.

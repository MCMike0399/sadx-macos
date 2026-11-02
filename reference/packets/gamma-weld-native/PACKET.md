# Gamma PJoinVertexes recovered prefix

Source cba33e6b on packet/gamma-weld-native, supplied worktree t3-83362981,
base acdb5a04. This is a qualified initialization/backup-restoration prefix,
not complete welding, a completed actor frame or N3b. Root owns admission.

Actual normal input advances from native_gamma_join_vertices to
native_gamma_update_laser_targets at frame 1332, exit 1, completed frames 1330.
The inspected outgoing-title image remains byte-identical: c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427.
The actual passed initialized Player is consumed; its entry-captured current
player alias remains null. No actor, display or later common-tail seam was skipped.

The first real call sees join mode 0 and the source's four mode 4–7 rows plus
sentinel. It clears original handles without freeing overwritten blocks, resolves
source node indices 37, 46, 20 and 8, clears destination depths and publishes mode 1
only after the table returns. Full original PJoinVertexes over the actual 589-object
imported graph independently gives those same five rows and mode. The native
common-tail witness asserts these source results and canonical object/table identities.

The core also implements modes 1–3's real MAlloc backup initialization and the
separate cleanup branch: source point, source normal, destination point and
destination normal, each three raw words, with 48 bytes per vertex. It borrows
SceneState.geometry and immutable model metadata through one read-only accessor.
No mutable pose, model, geometry or backup owner is copied. Pair indices are
unsigned 16-bit; mode is signed 8-bit; vertex count is unsigned 8-bit. Invalid
modes preserve row fields. Both object-to-model pointer reads occur even for zero
count, but model/vector dereferences occur only at their actual consumption.
The source review found and fixed an initially eager descriptor read: 22 new
original mismatches exposed it, and the final cases preserve all earlier inputs.

The actual TaskWorkArena owns original data. Overwritten handles remain live;
cleanup reads current arena contents, restores every affected component before
FreeMemory and then clears the row. Free receives a captured handle even when a
metadata callback replaces the row field during release. Reinitialization and
repeated cleanup retain those lifetimes and shared-array identities.

GetNodeIndex and its tightly coupled helper are recovered completely. They count
every node, ignoring evaluation flags, traverse children before siblings and
include root siblings. Null target returns -1; only the low byte is published
into a weld row. SOURCE-EVIDENCE.json pins the accepted cached PJoin dossier and
five fresh owned Ghidra dossiers. The fresh provider used 2 GiB and closed cleanly.
One semantic helper name is proposed, not written into Root's inventories.

Active join mode 1 retains native_gamma_join_vertices_CalcMMMatrix before the
join table can hide missing processing. Its argument selection, motion traversal,
persistent matrix buffer/scratch, geometric welding and attachment outputs remain
unqualified. The producer and its traversal were analyzed, not replaced by another
animation or matrix algorithm. ACTIVE-BOUNDARY.md describes their exact dependencies,
source locations, traversal differences and required existing matrix machinery.
There are no fabricated identity matrices, normals or successful active returns.

Verification in VERIFICATION.json: 1,200 development cases and 2,224 final holdout
cases, zero mismatches, including 248/395 failure prefixes and 170/302 explicit
active-boundary cases. The active cases compare the stop, not deformation. Both
node traversal and complete affected geometry, backup images, sizes, identities,
modes and depths are observed. Exact exception instruction addresses and the
allocator implementation itself are outside this judge. Receipt metadata had an
exact-full sentinel defect; its correction is separate from game source and uses
the unchanged prior contract, without reserved Task/reset execution.

Seven compiled core faults and four actual caller/reentry faults are caught.
Seven focused programs pass. The inherited motion witness changes only its old
stop diagnostic; all 11,144 observed bytes remain identical, SHA-256
3bcab50e1e0d58096e2b1c174c20c0ad4fed6cb4ce9bc6800fed66ed008fca34.
Native app build and force-load/nm closure pass at -j2. All arithmetic is integer
or raw word copy; no new ExactFloat or floating-point operations were introduced.
Existing motion/effects/Sonic algorithms, verifiers, arithmetic binding and inherited
numeric partials are reused only through exact unchanged hashes in PRESERVATION.json.

Readability, layers, unique owners, constexpr checks and provenance with the
proposed symbol pass. Canonical provenance still needs that one helper proposal.
Canonical judge sealing is deliberately pending: three changed protections, eight
new paths and 53 absent inherited inputs explain its 64 violations. All 1,398 old
protections remain intact. REGISTRATION-PROPOSAL.json lists the finite delta.
No full gate, fresh unavailable-images export, Main write, push, subagent,
reserved Task/reset campaign, schedule, storage or provider change was performed.

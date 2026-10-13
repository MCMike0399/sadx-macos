# Native fixed-pool mesh registry and triangle producer

The implementation in `sadx-native/src/player_mesh_registry.*` lifts the source
registry and its mesh producer into native C++ arithmetic and resource owners.
It uses the supplied PE32 `sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Retained Hopper dossiers in `mesh-producers/`, exact executable instruction
bytes, and the pinned LLVM listings in `mesh-registry-byte-evidence/` are the source evidence. Provider
pseudocode is an interpretation; the arithmetic/store/branch sequence follows
the instructions. Evidence IDs and profile digests are in
`player-mesh-registry-ledger.json`.

Implemented entries are initializer451030 with4510e0/451110/451160,
group allocation451790, release451800/451860, buckets4518d0/451910,
node allocation452160/4521c0, vertex transformation452230,
meshset production451ad0, recursive object traversal451a00,
unculled registration43b580 and registry refresh4511d0.

The scene's `groups` and `nodes` remain the only owners of intrusive links and
polygon fields. A typed group backlink distinguishes a native group identity
from the source's complemented registry-head address. Source addresses appear
in provenance and oracle serialization only; production never casts them into
native pointers. Pool slot identities are supplied by the native loader. The
initializer owns256 physical groups and16,384 nodes, but allocation tests the
next link before consuming a slot. It preserves the terminal sentinel, yielding
255 usable groups and16,383 usable nodes. Release returns the whole node chain
and then splices and recycles its group. Payload bytes not written by an entry
remain unknown or retain their prior explicit facts.

There are two distinct land arrays. `dynamic_land_entries` owns source3b32728,
whose logical prefix is `dynamic_count`. `candidates` owns source3b2f720, the
shared1024-record work array bounded by count3b32724. Both registration paths
copy accepted dynamic records into both arrays; accepted static records append
only to the work array. Broadphase reads the dynamic array and overwrites the
same work array. Registry4511d0 consumes that work array. The initially proposed
missing-static-tail repair was rejected after tracing these addresses. No such
behavior change remains, and the old `dynamic_entries` name has been removed.
The comparison snapshots both arrays independently before later consumers can
hide a disagreement.

Primitive452230 reads signed16-bit vertex indices from the shared index-array
owner. Strip headers preserve bit15 winding and the low15 length. Lengths above
1024 clamp the number transformed and advance the cursor only by the clamped
length, exactly as the source does. Subsequent strip interpretation can therefore
differ from conventional format parsers. Counts below3 emit no triangles.
Negative indices whose source range is unavailable in the decoded native array
remain tagged failures, rather than guessed vertices.

Object traversal pushes the current frame, translates, rotates Z/Y/X using the
low16 angle arguments, applies actual788a50 scaling, traverses children and then
its own meshsets, pops, and returns the next sibling. It does not introduce
evaluation-flag skips or null-model success paths. Cycles are explicit incomplete
failures. The source matrix owner preserves capacity-exhausted push/pop behavior
and preceding mutations on failure.

Triangle production preserves the source sliding triples, winding swap, literal
centroid multiplier `0x3eaaa64c`, binary32 stores, actual CRTftol buckets, three
CRTfabs calls and redundant binary32 spills, max-Z comparisons, centroid-relative
radius arithmetic and raw20-byte material-array flag lookup. Allocation failure
returns the source0 with previously produced nodes intact. Registry refresh
ignores that completed source return and joins the partial geometry, as the
executable does; unavailable resources or CRT state remain failures. It also
preserves the source's forced-object clearing,256-item scheduling/removal edge,
and ignored return from bucket joining. There is no successful allocator or
empty-triangle callback substitute.

Unculled registration's128-dynamic diagnostic uses the exact CP932 literal at
7e0030 (`82 a0 82 d3 82 ea 82 ad 20 97 dc 31 0a`), ignores the stdio return and
continues static registration. The regular43aef0 entry remains the distinct
native culling owner. Initializer451030 receives `PlayerMeshQueryState&` and
resets its existing3b42ff4 counter; registry does not cache a second counter.

The original-instruction verifier uses zero-filled prior pool bytes as an
explicit synthetic state profile, masked precision/rounding CWs and CRT policy1.
It executes original functions without function overlays. Selected Emerald
Coast roots supply actual object/model/vertex/index/material arrays, and bounded
landtable count views are marked synthetic metadata. It compares all256 group
and16,384 node records, free/active heads,128 buckets, both land arrays, eight
matrix frames, returns, CW and exception flags. Additional cases check primitive
types, winding, lengths0/1/2/3/4/1024/1025/32767 and repeated refresh reuse.

Strict optimized and UBSan native tests pass source sentinel capacities, link
repair/reuse, full-chain recycling, real ECoast native triangle production and
unknown-owner boundaries. The initial720 source evaluations had zero stored
value/return/CW differences and480 status-only differences. The final expanded
report `original-player-mesh-registry-comparison.json` covers1,068 evaluations
(89 fixtures ×12 CWs), with zero stored-value/return/CW differences and504
status-only differences. Its strict result is false. The report retains source,
compiler, library, verifier and case digests.
The final pin uses the repository's exact C++17 compile flags. Same-width
`memcpy` conversions preserve raw float/integer bits without changing the
repository's language version or introducing host floating arithmetic.
Unicorn exception-status limitations are preserved; no native arithmetic was
changed to match them.

The separate optimized and UBSan `player_mesh_registry_contacts` test passes
the real first-record ECoast resource view through actual registry production,
native broadphase and453820 contact output, without a selected-node seed or
callback. This integration test is native behavior evidence, separate from
the instruction-comparison profiles.

This closes a native producer feeding the existing453820 query. It does not
establish complete449500/445d10 contact positioning,43a260 stage/mesh conversion
and Win32 startup, physical Intel equivalence, or behavior of the pinned updated
mods on the user's Windows machine. Those remain explicit continuation work.

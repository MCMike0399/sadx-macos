# Native scene positioning: closed resource queries and remaining boundaries

The investigation roots are ordinary scene positioning `0x449500` and alternate
scene positioning `0x445d10`, called by the existing local position owner.
Their retained dossiers contain 1,357 and 880 instructions respectively.
Those large positioning bodies are **not yet implemented**. This pass closes
their actual contact-offset, land-registration and bound-query dependencies;
these results must not be adapted into successful final collision responses.

Source: user-supplied `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Byte spans, hashes, constants and original CP932 messages are retained in
[`scene-source-inventory.json`](scene-source-inventory.json). Source dossiers,
sorted assembly and exported Evidence bundles are in `position-dependencies/`,
`scene-dependencies/`, `scene-mesh-dependencies/` and `scene-producers/`.
All native game math uses the explicit software-x87 context. The separate
comparison executes original instructions under Unicorn 2.1.4; native gameplay
does not execute guest code.

## Closed native owners

| Owner | Source | Evidence |
|---|---|---|
| Contact-center offset | `0x43c7d0..0x43ca3c` | `ev_70ef324690a7ea034e40ed6d38581ba34ea0af1229b7f1a59205ac1336b025fa` |
| Land candidate selection | `0x43acd0..0x43aee8` | `ev_4da2a88f176402cad7d619c369b06b89a056bdab858f303db4eaa1192011ef73` |
| Registered mesh-bound traversal | `0x450c10..0x451026` | `ev_82be9e7ecf4507c76d927ed4406d39a19e9fef7ff329258e16ab47263f8a0b8c` |
| Camera/player culling and registration | `0x43aef0..0x43b2e0` | `scene-producers/land-registration.json` |
| Native PE32 land-table decoding | GeoLists lookup at `0x43b728..0x43b751`, LANDTABLE/basic-model/object reads | `scene-producers/land-stage-binding.json`, original source bytes |

`player_contact_initializer.cpp` preserves the complete source offset routine,
including X/Z zero stores before later missing-state boundaries, prior
attributes/flags, speed branches, dot thresholds, retained values and actual
Ninja length/normalization. Its earlier rotation normalization dependency was
migrated to the shared Ninja `normalize_vector_extended` owner without changing
the instruction sequence. Actual source zero/one constants are supplied.

`PlayerContactWork` is the shared native owner of playerwk `+0x64` contact
fields, distinct from task CCL collision-info resources. The former independent
`collision_vector_at_1c` representation was removed. Ordinary ground now
derives direction through `player_contact_direction`; its tests and three
acceleration probes construct the same contact owner. Missing whole contact
state and missing direction remain unavailable. This migration prevents the
scene and acceleration paths from maintaining stale independent directions.

`player_scene_decode.cpp` parses original PE32 resource pointers at the format
boundary into native IDs. It validates file-backed ranges, preserves object/
model alias identity, distinguishes observed null references and retains raw
float bits. Source virtual addresses and file offsets are separate provenance
metadata, never native pointer casts. LANDTABLE records retain their center,
radius, object reference, attributes and three uninterpreted source words.
Object/model decoding currently covers the position, angles and bounding facts
consumed by these closed queries; it does not decode complete polygon geometry.

The real stage-1/act-0 GeoLists entry selects LANDTABLE `0xe99cb8`, with 480
36-byte records at `0xe95938`. Source header flags are `0x000c`. These are actual
records from the supplied artifact, not regenerated geometry or flattened
renderer bounds. The caller retains the complete artifact digest beside the
native decoded owner.

Registration borrows live camera/player/task owners. An absent selector is
unknown; a present readview containing `nullptr` is observed nullness. A null
camera returns before registry resets, as in the source. Required positions
are never supplied as origins. Registered dynamic task eligibility reads the
borrowed task flags. Dynamic and static culling retain their distinct retained
versus binary32-spilled component arithmetic, source order, first-view radius
and second-view constant. The second-view float is explicitly bound from
`0x7e0028`, bytes `00 00 20 42` (40.0).

At dynamic count 128 the source calls printf with the message at **0x7e0018**,
not the different `0x7e0030` message. Native stdio writes its exact 13 CP932
message bytes and continues matrix-pop/static registration; no geometry fact
depends on a logger return. Console encoding/text-mode equivalence across
Windows/macOS has not been observed.

Land candidate selection retains reverse dynamic traversal, conditional unit
matrix initialization, actual Z/X/Y rotation and CalcPoint, strict bounding
tests and forward static traversal. The 1,024-entry candidate owner retains
unused bytes after its logical end. Broadphase retains the source FST32 extent
used beneath an ext80 multiply, linked-group/object identity selection, forward
and backward node walks, range predicates, staged squared-distance spills and
the 128-node global cap. Group begin/count stores and unvisited groups survive
partial exits exactly in compared profiles. Unknown fields, unresolved links,
cyclic nonterminating resources and invalid matrix ownership produce tagged
failures; failed matrix operations retain their original source call site and
preceding resets. No identity transform or successful geometry callback fills
an unavailable dependency.

## Verification and precise scope

| Experiment | Executions | Compared outcome |
|---|---:|---|
| Full contact initializer and actual vector callees | 4,800 × 12 = 57,600 | Offset words and CW match |
| Real ECoast records: binding prefix, separate static registration/query | 480 × 12 = 5,760 | Counts, copied records, candidate order, matrix frames and CW match |
| Synthetic linked bounds and dynamic object-bound rotations | 249 × 12 = 2,988 | Selected identity/order, group partial stores, range/cap behavior, matrix frames and CW match |

Twelve masked CW profiles cover precision 24/53/64 and all four rounding modes.
Reports retain compiler/library/source/verifier identities, binary fixture
digests and discrepancies:

- `original-player-contact-initializer-comparison.json`
- `original-player-scene-comparison.json`
- `original-scene-mesh-bounds-comparison.json`

All exception-flag disagreements remain recorded. These successful limited
value/CW comparisons do not prove Intel hardware status or transcendental
semantics. Real-stage and synthetic-graph profiles are explicitly separate.
The real-stage profile selects an empty registered dynamic-task list and a
known-null mesh-group head as experimental inputs; it does not establish those
selectors in a running stage. The synthetic profile exercises native-owned
doubly linked nodes mirrored into source memory, including both traversal
directions, selected/unselected objects, range edges and the 128-node cap. It
also executes source dynamic-object bounds/rotation paths. Its shapes remain
synthetic and are not evidence of loaded polygon geometry.

`tests/player_contact_initializer.cpp` verifies lazy unavailable-state failures
and partial stores. Its arguments are parameter rows, initializer constants
(52 bytes) and rotation constants (88 bytes). `tests/player_scene.cpp` verifies
the actual ECoast decoder/resource identity, live borrowed pose reads, null/
unknown registry semantics, invalid matrix ownership and continuation after
the dynamic-128 diagnostic. Its arguments are source EXE, quarter sine table,
unit matrix and scene constants (12 bytes: `0x7df1c8`, `0x7e0600`, `0x7e0028`).
Strict O2 compilation and native tests pass; the scene test also passes UBSan.
No shared build archives were modified by this worker.

## Stage-initialization boundary

The actual source binding experiment executes `0x43b700` through the table store
at `0x43b751`, stopping at `0x43b757` before `0x43a260`. It compares the observed
GeoLists-selected table with native decoding. Registration and query are then
separate source experiments using that observed table and its real records.

An attempted complete initializer reached NJS mesh conversion
`0x787140 → 0x787010 → 0x794840 → 0x780f90`, then unresolved Win32 HeapAlloc IAT
`0x7db074`. Raw import words led to an invalid read at EIP `0x49127c`; they are
not initialized Windows imports. No allocator overlay was added to make the
profile pass. `0x43a260`, polygon-buffer conversion, heap setup, rendering and
texture initialization remain unfinished. Bound queries alone do not close
stage startup or mesh resources.

## Required continuation

Full `0x449500` and `0x445d10` positioning bodies, actual object/polygon query
`0x453820`, its mesh helpers, grounded/contact response, water resources and
final position updates still require native closure. Recovered further helpers
include `0x450810`, `0x450920`, `0x4509d0`, `0x787630`, `0x43cce0`, `0x43d180`,
`0x43cf90` and `0x43d8b0`; dossiers are retained, not equivalent to implemented
native owners. Source callback/import state and full engine task/scene registry
lifecycle remain separate dependencies. Real keyboard/controller/mod playback,
Windows Intel Ultra hardware observations and playable speedrun parity remain
unverified. The overall native-Mac goal stays active.

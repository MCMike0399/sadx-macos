# Source-native mesh arithmetic and object contacts

Source executable SHA-256:
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The native runtime executes ARM64 C++, explicit integer-backed software x87,
and named Bochs FPATAN math. Source x86 instructions execute only in comparison
tools. No guest instruction interpreter was added to native gameplay.

## Implemented owners and evidence

- `player_mesh_math.cpp`: triangle normal `0x450810`, normalize-to-zero
  `0x450920`, normalize-to-up `0x4509d0`. The two normalizers preserve m64 CRT
  sqrt arguments, binary32 length/reciprocal spills, differing zero stores and
  complete unavailable-CRT diagnostics. No generic host normalize substitutes
  these bodies.
- `player_mesh_matrix.cpp`: full determinant `0x795000`, inverse `0x787630`,
  vector-scale `0x788a50`. The determinant/inverse are generated **static C++
  expressions**, with explicit source-order spills and register arithmetic.
  There is no runtime opcode table, guest register loop or memory emulator.
  `emit_mesh_matrix_math.py` validates straight-line evidence coverage and
  refuses unsupported operations or read-before-spill. Source singular inverse
  writes finite bits `0x7f7fc99e` into all 16 elements and returns 0; it does
  not substitute an identity matrix. Source vector-scale retains X/Y FLDs
  before stores and MOV-copies raw Z before four later FLDs, unlike the
  separate scale-at-784c70 owner.
- `player_mesh_query.cpp`: full local object-contact query
  `0x453820..0x4546d9`, including actual selected-node lookup, triangle
  expansion, source floor-angle/CRT owners, sample walk, sidedness, plane
  tests, inverse transforms, contact-buffer copies, nearest selection,
  source contact-work fields and aggregate stores. Missing resources/math
  state remain tagged failures; a completed no-contact outcome comes only
  from the recovered source branches.

REA dossiers and exported bundles are retained in `scene-dependencies/`,
`scene-mesh-dependencies/`, `mesh-math-dependencies/`,
`mesh-resource-dependencies/` and `mesh-producers/`. The object-query Evidence is
`ev_c458ee88080e6c6f8644e072dc861ff8fafd50b7ab369bd8e8429e62e4d2279e`.
The byte comparison binds the exact source digest and function span; assembly
labels are not interpreted as original C++ source.

## Real resource decoding and ownership

The former bounds-only model owner was promoted to `SceneBasicModel`; the old
name was removed. Original PE pointer words normalize once into native resource
IDs. Model vertex/normal/meshset/material references point to shared owning array
tables, preserving aliases instead of copying buffers into every model. Native
source locations retain original virtual addresses, file offsets and byte
extents separately from resource identity. No guest pointer is cast into a
native pointer.

The actual Emerald Coast table at `0xe99cb8` contains 480 land records and 183
unique root basic models, with 578 mesh sets. All those mesh sets use kind
`0xc000`. Strip words preserve bit 15 winding and low-15 length; e.g. the first
stream starts `0x8010` followed by 16 indices. Decoder support also follows the
observed count-prefixed `0x8000` physics path. Empty/short strips are retained,
since source construction can produce no triangles from them. The resource
decoder preserves index words; the source producer must separately preserve
its 1,024-vertex consumption clamp and its resulting cursor behavior.

Raw vertex and model-normal floats, material 20-byte words, object scale,
evaluation flags, child/sibling references and shared array locations are
decoded. Object graphs use a visited-ID queue to preserve aliases/cycles
without recursive decoder overflow. Node triangle data at `+0x18/+0x24/+0x30`
and material flags at `+0x3c` have explicit optional fields. The latter is a
source material flag word, not padding or a manufactured zero.

Shared `PlayerContactWork` now also owns count/index at `+0x28/+0x2a` and
vectors at `+0x3c/+0x48/+0x54/+0x60/+0x6c/+0x78`. These are distinct source
stores, with previous unknowns or sentinel values retained on early returns.
At `+0x3c`, source copies the original normalized normal; `+0x48` gets the
possibly reoriented normal. The first comparison exposed an incorrect initial
`+0x3c` point mapping; it was corrected from the exact `0x4544a0` payload read,
not by changing arithmetic to fit the oracle.

## Contact-query details that affect fidelity

The query takes the actual registered group identity and selected node array.
It retains each triangle's source first-vertex-relative coordinates, centroid
division, 1.04 expansion literal `0x3f851eb8`, and conditional matrix ownership.
Source material flag `0x800000` changes signed-radius/normal orientation.
Sampling uses source half-radius steps and the original final-sample branch.

The recovered CRT atan2 call is formal
`atan2(y=oriented_normal.x, x=oriented_normal.y)`. The binary64 asin call is
the distinct argument entry `0x645034`, not the retained-ST0 `0x645020` entry.
Angle scaling divides by source **6.283184**, bytes at `0x7e05f8`, not a host pi
constant. Native CRT state and handler identities remain explicit. Complete
sqrt/fabs/asin/atan2 lower diagnostics survive unavailable runtime boundaries.

At 16 accepted contacts, source jumps from `0x45433a` to `0x45434b`, skipping
the following matrix pop. Native retains that pushed frame. Partial contact
buffers, untouched source record word 0, previous response slots and source
store order are preserved; no RAII restoration or rollback hides them.
An explicit caller sampling budget or detected stagnant source walk returns
an incomplete tagged failure, never a successful no-contact result. This is
native resource handling, not proof the corresponding infinite source walk
would return.

## Verification

`verify_original_player_mesh_math.py` executes all original primitive bodies
and their real CRT/vector callees without overlays. **39,264** evaluations
across 12 masked PC/RC profiles have zero compared value/return/CW differences.
Fixtures include 80 real ECoast local triangle inputs plus synthetic raw,
nonfinite, singular and general matrices. All status discrepancies remain
recorded in `original-player-mesh-math-comparison.json`.

`verify_original_player_mesh_query.py` executes the original local query and
actual math/coordinate callees. **3,972** evaluations (331 cases × 12 CWs)
have zero compared outcome/contact-field/88-byte response/matrix-frame/CW
differences. Cases include 80 real ECoast local triangles, synthetic flat and
tilted triangles, varied sweeps, both sidedness modes, prior output sentinels,
and counts 0/1/2/15/16/17/20. Exception-flag differences remain recorded in
`original-player-mesh-query-comparison.json`.

These selected triangle/group/node seeds are explicit test inputs. The query
profile does **not** establish that the stage registry generated/selected those
nodes in a running game. The real local geometry inputs are source bytes, but
stage/object transformation and allocation must be verified through the actual
producer before claiming complete scene activation.

Primitive and query unit tests pass strict native compilation and UBSan. Query
unit checks cover unavailable radius/selector/nodes/vertices/material flags,
pending single-/two-argument CRT states, actual finite contacts, matrix-frame
retention and explicit budget failure. Its arguments are quarter table, unit
matrix, floor constants (36), query constants (32) and axis constants (8).
Query constants are floats `0x7dcc98`, `0x7dccd8`, `0x7dcb0c`, literal
`0x3f851eb8`, then doubles `0x7dccd0` and `0x7e05f8`.

## Required continuation

The nine mesh producer dossiers are retained: `0x4511d0`, `0x451790`,
`0x4518d0`, `0x451910`, `0x451a00`, `0x451ad0`, `0x452160`, `0x4521c0`,
`0x452230`. Their actual node pool/buckets, object hierarchy transforms,
source material flag stores, group lifetime and registry refresh remain the
next native closure. Full positioning bodies `0x449500` and `0x445d10`, water
and engine task/scene composition still need implementation. Native Windows
Intel Ultra observations, updated-mod activation, real input/tick traces and
playable native-Mac speedrun equivalence remain unverified. The goal is active.

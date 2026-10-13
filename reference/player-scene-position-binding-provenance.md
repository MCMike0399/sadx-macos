# Native position-to-scene binding

The new `player_scene_position_binding` owner connects the recovered PSetPosition
(0x44cdf0) collision calls to the native full 0x449500/0x445d10 scene bodies.
It borrows the same landscape, query/contact state, constants and runtime. The
PSetPosition caller supplies its live mutable parameter row, matrix owner and
x87 context on every invocation. The binder copies none of those mutable owners.
The scene's actual 0x43cb90 therefore modifies the startup-owned parameter row
that subsequent Sonic movement consumes.

Both collision results and PSetPosition results retain the complete immutable
`PlayerScenePositionResult` diagnostic object. This preserves actual resource,
query, response, CRT, candidate and nested helper facts on success and failure.
Sonic's existing position field carries this object. The action dispatcher now
retains that position result when a later speed-copy/animation/common-tail result
replaces its local result. No duplicate scene scalar state is introduced.

The binder replaces only ordinary/alternate collision callbacks. Normal, hold
and reaction owners remain explicit. A missing native matrix owner prevents the
scene invocation and has no invented child result. Actual scene renderer and
nonnull platform callback boundaries remain unchanged, including partial stores
and an already pushed matrix frame at the renderer boundary 0x44499e.

## Evidence and validation scope

The child source bodies and resource producers reuse the original instruction
comparisons and genuine Ghidra Evidence recorded in
`player-scene-position-ledger.json` and `player-mesh-registry-ledger.json`.
This change adds native composition tests, not a new original-instruction or
physical Intel comparison. Before-change source fingerprints are retained in
`player-scene-position-binding-baseline.json` for the parent's 53-program gate.

The integration test reads the pinned 136-case source-derived ECoast fixture
corpus and actual PE arrays. It executes native registry generation, PSetPosition,
scene broadphase, mesh query and contact responses at CW 0x007f. Its caller CCL
resource has explicitly zero flags, and its bounded table views, zero-filled
caller work, CRT policy1, renderer selector and parameter shadow-attribute inputs
are controlled test profiles, not live Windows observations. Alternate stage9/2
selection with ECoast resources is explicitly synthetic; it does not establish
the geometry table for that stage. No selected triangle or flat plane callback
is supplied.

The test checks both scene entries, retained successful query/response facts,
in-place mutation of the sole startup parameter owner, nested missing-field
failures, the actual renderer boundary and matrix depth, and Sonic action11
carrying position diagnostics through a later missing 0x44a800 owner. Existing
controlled PSetPosition unit/probe callbacks are adapted only to the new native
argument shape; their original comparison scope remains controlled dependencies.

Physical Intel x87 behavior, updated Windows mods, native renderer/effect tasks,
nonnull scene platform handlers, real stage initialization and a playable game
remain separate unverified work. No native arithmetic is altered to satisfy the
Unicorn oracle.

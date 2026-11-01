# Canonical Ninja length attempt

Base: `3efc9b7`. Scope: MATH-LENGTH-CLOSURE, one numerical primitive in the
complete-refactor campaign, not a player/module/campaign completion claim.
No providers, network, Main mutations, asset mutations, gates or journeys.
Final fresh holdout is reserved for Root after independent review.

The SDK `SADXFunctions.h:2766` declares fastcall `njScalor(const NJS_VECTOR*)`
at the recovered entry. Cached source assembly is tracked at
`reference/production-integration-20261026/collision-component-core/scalar-allocation.assembly.txt:1`.
The six square/load operations are ordered x then y then z. Addition is
`(zz + yy) + xx`, followed by a required binary32 squared store and the signed
square root's double-zero signaling compare, reload, conditional sign treatment,
and unrounded register return. Source bytes and profile ranges are already
reviewed in `docs/refactor-inventory/reviews/math-foundations.json`.

The two current interfaces borrow the same caller-owned context, with no persistent
state. `vector_length` returns `ExactFloat`; `rotation_vector_length` returns
`X87Value`. No raw-value accessor, extra store, context reconstruction or change to
these signatures is justified. The primitive-specific CPU compatibility operation
`ExactFloat::vector_length_return` uses the existing signed-root implementation and
exports only this primitive's retained return. Both semantic wrappers delegate to it.
Its three binary32 component references preserve interleaved loads and squares.
No data member or existing public signature changes. The single njScalor FUNCTION
annotation moves with the arithmetic body; the typed math wrapper is tagged NOTSA.
There is no vector/game type or upward dependency in that CPU operation.

## Actual named source consumers

`vector_length` is called by `collision_component::calculate_range` and by the
component scalar probe. Range calculation is also called by component initialization;
the real native Gamma binding calls that initializer. Gamma's core differential
probe substitutes the collision seam and therefore cannot qualify that composition.

`rotation_vector_length` is called directly in:

- `player_rotation.cpp`: passive and gravity-directed rotation.
- `player_position.cpp`: contact and velocity zero guards.
- `player_contact_response.cpp`: displacement, speed, normal/direction and magnitude paths.
- `player_acceleration_alternate.cpp`: horizontal event acceleration length.
- `player_direction_reset.cpp`: projected speed and delta guards.
- `player_reaction.cpp`: motion and normal guards/magnitudes.
- `player_contact_initializer.cpp`: velocity magnitude.
- `player_mesh_query.cpp`: displacement magnitude.
- `player_scene_broadphase.cpp`: displacement plus contact radius.
- `player_scene_position.cpp`: retained exact-value `Context::length` used throughout positioning.
- `sonic_player.cpp`: Sonic speed magnitude.

These are maintained callers even where `native-sources.txt` excludes their modules.
The corresponding unchanged judges are collision_component, player_rotation,
player_position, player_contact_response, alternate_acceleration (all four registered
variants), player_direction_reset, player_reaction, player_contact_initializer,
player_mesh_query, player_scene (broadphase), player_scene_position,
player_scene_helpers, sonic_player and sonic_actions (singular native probe slug).
The scene helper and Sonic judges include paths that stop before some producers;
source linkage alone is not evidence of execution. Supplemental original-entry
visit counts distinguish their actually exercised length calls.

Source search found no direct length caller in guest/HLE. The original reference
may have dynamic callers outside this maintained-source census. No full-program
or physical Windows closure claim is made.

## Qualification rules

Before editing the length source, build each probe from its complete current
maintained-source linker closure, with those objects ahead of the immutable
`/Users/burbujamc/Developer/sadx-wt-menu-progress-predicates/build/menu-progress-current/stable-root-core.a`.
Its required SHA-256 is
`bfcaf022672ca30c74fac35b7649a2f837bba5f533a740886de2fab13e216c29`.
Link maps must leave only third-party objects to that archive. Root's Python venv,
pinned EXE, SDK and cached evidence are read-only. Native calls and original
evaluator returns are captured separately without changing the evaluators or cases.
Keep full output streams, argument/case identities and all native failure words.
Compare complete streams after the source change, not just mismatch counters.

Historical component qualification combined length and signed-root scalar tests:
888 comparisons and 834 FP partials, not 888 length-only cases. The full dev
judge had 7,964 comparisons, zero value mismatches and 6,462 FP partials (exit 2).
Its historical `length_sum_order` fault changed the order of the three squared
terms and caused 45 value mismatches. Relocate that semantic fault to the shared
operation and rerun the unchanged full dev judge. Do not consume its holdout here.

The original software context models low-six exception flags and immediate native
failure, not full x87 stack/tag state or deferred hardware delivery. Preserve these
limits even if every pre/post native output is equal.

# Animation ownership and consumer map

This is a packet-local map, not a global inventory disposition.

| Canonical owner | Native and reference access / producers / consumers |
|---|---|
| `PlayerPhysicsState` and its `animation_tail` | Physics retains the first five words, including join mode. Tail retains next, timer, flags, frame, start, table, generated buffer and borrowed join table. Native `animation::View` accepts the player and allocation only; its binding always uses that player's tail. No second motion state or scalar cache exists. Reference `PlayerAnimationView` keeps the old separate-tail fixture contract. The native compatibility bridge rejects a foreign tail. |
| Existing player allocation in `TaskWorkArena` | `read_live_multiplier` resolves the actual speed and self references on every consumption. Same allocation and interior offsets remain required. Physics owns speed and charge; the arena owns reference identity. Neither scalar is copied into animation state. |
| `animation::Table` | Plain native records own action readiness/identity/reason, count, signed mode, next, transition and frame step. One shared immutable Gamma table is published into the initialized player. `TableStorage` supplies identity through the existing canonical pointer field for either a native table or a development table. It contains no second table or snapshot. |
| `GammaActorAssets` in `PlayerSystem` | App conversion creates all 91 typed rows, preserving 78 declarations, export 61 null, request 0 Stand and constructed row 72 unavailable with reason. Existing initialized binding borrows that same table. Scene, weld and cursor owners remain separate and unchanged. |
| Caller-owned Sonic table and export declarations | `initial_sonic_table` establishes the real 147-row BSS zero state; it does not initialize arbitrary reference fixtures. `initialize_sonic_rows` performs 145 ordered reads and 882 ordered publications, with typed content from `kSonicRecipes`. Both native import and the reference compatibility initializer invoke it. Old rows remain intact after a reached missing export. |
| `SonicPlayerRuntime::initialized_animation_table` | The actual initialization path retains unpublished versus known-null checks, then publishes the same table and request 0. Existing dispatch routes the known updater before generic callbacks. An explicit native runtime selects the typed API; existing reference runtimes retain their path and diagnostics. Both reach the shared engine. |
| `animation::Resources` | Plain actions, motions, nodes and key backing are caller-owned. Resources borrow the existing scene object owner. Null references, unresolved identities, payload readiness, raw scale-pointer bits, declared counts and captured backing extent remain distinct. No CharacterPack-to-Gamma playback bridge is invented. |
| Existing generated action/motion headers and arena allocations | Typed headers remain authoritative, including genuinely partial malloc initialization. Arena storage owns generated MDATA/key bytes and references. Shared core publishes/frees in the old order; native and reference bindings report unavailable header reads at the reached logical boundary. Raw arena headers are not a second initialized snapshot. |
| `DecodedPlayerAnimations` | The PE parser is development/import infrastructure in `guest/player_animation_decode_reference.cpp`, retaining its old compatibility result. It owns one stable scene map and borrows it through resources. It uses the same typed Sonic content for maximum node counts. No recovered playback algorithm moved into the reference tier. |

`animation_engine.hpp` contains the single implementation of update, reset, next,
release, blend, transition generation, angle delta and angle unwrap. The native
binding reads the canonical owners; `guest/player_animation_reference.cpp` supplies
legacy fixture availability and maps logical failures to old source PCs/results.
There is no snapshot/copyback pass over callbacks.

`player/player_animation.hpp` is the explicit development compatibility surface:
its legacy optional records/results remain for existing probes, Sonic and the
reference importer. Those declarations are not the product contract, and their
functions forward to the shared engine. Product Gamma and the native animation
translation units include the plain headers instead. Broader Sonic/physics and
scene-owner migrations remain campaign work; this packet does not rename or move
those owners.

The existing Gamma `PSetMotion` seam and `native_gamma_set_motion` failure remain
loud. They are not replaced by table playback, callback fabrication or success.
Material/setup/effect/model-matrix/draw/other-mode limits remain unchanged. The
existing seam retains the unique PSetMotion source-entry annotation; this packet
does not add a competing entry claim to the shared updater.

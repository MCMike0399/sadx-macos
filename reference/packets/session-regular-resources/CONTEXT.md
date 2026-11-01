# Session regular resource closure

Base e5cf19d. Scope is the seven contiguous Session members from common_textures_
through regular_texture_state_, in their existing declaration order. Source native
attachment is frozen verbatim in tests/fixtures/regular_resource_baseline.hpp;
the reduced transport host supplies live resync/language and a Display callback spy.
This is a native implementation baseline, not original Windows equivalence.

Construction: regular storage follows character slots and precedes MenuTextureBindings.
Destruction is reversed: menu aliases die before regular State, catalog, groups,
lists and assets. State dies before its localization referent, and the enclosing
loop/language outlive this storage. There is no attachment-time copy of either.
State remains null until all asset/list/group/catalog stores succeed. Its unique
allocation is published before Display validation and archive-loader binding.
A failed display/binding leaves State present; another attach must reject it.
Earlier declaration/group/catalog failures retain ordered partial storage and may
be retried; group regular entries append on retry exactly as in the native body.
Never add rollback, eager validation, clearing or pre-callback reference snapshots.

Consumers: Session texture_state borrows common/32 regular cleanup slots alongside
unchanged character owners. Session regular/menu/Japanese archive loads borrow State;
MenuTextureBindings borrows regular_resource_lists_ by vector reference. TitleScene
individual loading has its own native_font_texture_loader guard. Selection preview
named loading has native_regular_texture_declarations; Gamma accepts a nullable
State pointer. The archive callback captures Session and resolves State/services at
invocation. Display/LogoRenderer owns the callback and GPU allocations separately.
Application destruction order already requires no callback invocation after Session
ends; extraction does not extend or shorten that contract or install unbinding.

Shared game behavior remains game/regular_textures and engine/native_texture_lifetime;
cleanup remains game/game_cleanup. Only imported binding and owner storage move.
No new source function recovery, address range, game behavior or guest dependency.
Original regular-texture DLL-profile, menu-resource and texture-lifetime judges stay
unchanged. Root owns manifest/registration, full gate/journeys, native closure, fresh
review and reserved final cohort. Local compilation is serial and packet-local.

Fresh pre-edit original judges: regular_textures609, menu_resource_lifetime1401,
native_texture_lifetime1416; all exit0, zero mismatches, no inherited partials.
Fresh native fixture emits full observations in native-baseline.txt. Initial test
input with a null localized filename correctly failed at the existing Filename
boundary; valid input was corrected before freezing, no product change.

# Gamma actor imported resource closure

Branch `packet/gamma-actor-assets`, durable checkout
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-3c7e487d`.
Root owns runtime/native metadata consumption, mutable scene state, app and renderer.
The Gamma core packet owns `player/gamma_selection_core.*`.

Status: focused asset verification completed after Root explicitly reported gate
60866 terminal rc0. Root independently confirmed all five matrix-node identities.
This packet has run no apps, Ghidra, original producers or full gate.

## Import API

`tools/import/gamma_actor_resources.py MODEL_DLL GAME_EXE OUTPUT` rebuilds the
existing Sonic/selection graph with the SAME builder and appends only the reached
Gamma closure. Output remains `sonic-selection.actions`, schema SDXACT03, one
CharacterPack namespace. It also emits `gamma-actor.json` and the pack provenance.
It neither executes x86 nor imports an emulator/original producer.

The first 3302 encoded records remain byte-identical at bytes 368..760167;
the 22 descriptor bindings remain byte-identical at bytes 16..367. The header's
record count necessarily changes. Reconstructing the original count yields old
whole-file SHA `df53cf2eb5427491cd444350b231f9b168060602b65a8cf93371f6ad0f38d423`.
Existing sidecar metadata is not overwritten by the Gamma importer. Run the
existing selection/Sonic import hooks before this append to generate those files.
No old fixture, decoder, schema limit, native source list or judge is changed.

## Metadata contract

`gamma-actor.json` version 1 has:

* `character`, `texture_character`: 6; texture declarations identify the existing
  five archive/list owners plus the actual NULL terminator. No duplicate manager.
* `initial_animation_export_index`: 0; `initial_animation_action`: actual Stand
  ResourceId. Existing export 2 is not a substitute.
* `declarations`: 78 rows (`index`, `status`, nullable `action`, nullable
  `resource_boundary`). `known_null` means genuine export61 NULL; `unavailable`
  means nonnull source payload not loaded; `loaded` carries the existing/new ID.
* `recipes`: all 91 rows with core-proved `row`, `declaration` (index or
  `constructed`), `object_count`, signed `mode`, `next`, hexadecimal
  `transition_bits` and `speed_bits`. Each adds explicit status/action/boundary.
  No zero/idle replacement for unavailable resources. `constructed_action`
  describes row72's proved dependency separately from payload availability.
* `objects`: source table `index` to canonical `object` ResourceId for rows
  0/24/25/26/27. Borrow the existing shared mutable graph.
  `matrix_nodes` independently binds preorder node54→10201ae4,
  node30→1020491c, node39→10202c9c, node2→10207228 and node37→1020323c
  to existing ResourceIds. These are NOT E102_OBJECTS export rows. Provenance
  records all77 child/sibling traversal identities and source range hashes.
  `matrix_cache_dependency` requires actual animated CalcMMMatrix/ScanMotionModel
  results in the existing cache3c58278 with valid count3c58270. These identities
  never authorize static pose matrices or identity fallback. The initial export
  row54 interpretation was rejected by the strict importer, not accommodated.
* `joins`: four source weld rows and full zero sentinel, with native resource IDs
  in object/source/destination fields. Same key layout as existing Sonic metadata.
* `materials`: ordered rows1/5/4/3/2 and indices6/2/2/2/2, with canonical
  `materials` array ResourceId and `record_index`. Borrow this exact shared record;
  do not copy per actor. Table aliases map to the same array/record identity.
* `collision_words`: exactly36 uint32 source words (three48-byte CCL records).
  `line_color_words`: exactly2 uint32 words. `line_point_count`: 2. The live line
  points belong to Gamma State; they are not imported animation data.
* `cursor_templates`: two source WIREMODEL records using shared vector/color/index
  ResourceIds, `line_count`, padding, and actual initially null point-color fields.
  The runtime allocator/expander must still perform the real setup; imported
  templates do not fabricate the allocated point/color buffers.
* `setup_dependencies`: explicit cursor allocator, neutral resource loader and
  effect setup source boundaries. These are required behavior, not success flags.

The importer validates the exact pinned EXE and CHRMODELS hashes. Every appended
pack record uses the existing graph deduplication and inverse proof. Full recipe
and weld metadata invert to static MOV/XOR declaration folding with source zero-fill
proof and register known-bit checks. These are reconstructed initializer results,
not claims that BSS originally contained those bytes.

## Focused verification

Final result:33 focused Python tests pass (14new,9unchangedSonic,
10unchangedSelection). The strict C++ decoder accepts3328 records, preserves
old3302 records and all22bindings, and sees real77-part/16-frame Stand.
The independent complete producer receipt matches91 recipes and5weld rows.
One actual source mutant disabling unknown-register rejection is caught;
five distinct content corruptions are caught. Profile10 ranges has0 active
unhandled and0 undetermined overlaps. `proof.json` records counts, limits,
initial failures and their corrections. No skipped focused tests remain.

The additions-only judge registration adds exactly the new Python test and
private C++ probe. Initial registration found53 missing ignored corpus blobs;
only existing manifest-hash-matched bytes were copied locally. No expectations
changed. Final judge manifest has895 protected files and0violations.

The final local output is `build/gamma-actor-assets/extended/sonic-selection.actions`,
766112bytes, SHA `c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f`.
New IDs3303..3328: Stand action3303, motion3304, tracks3305,
position/rotation keys3306..3324, cursor points3325, first colors3326,
shared cursor indices3327 and second colors3328. All geometry/material/weld
object identities reuse existing records; no duplicate graph owner.
`proof.json` lists every new ID with source/payload hashes.

Old encoded record bytes SHA:
`10c09181d59fc16c13fba8a10326ff16cf06189a7fe13f5ecd60a1643e3a6baa`.
Old binding bytes SHA:
`93dda5e55a9739385e1a9ba6059b6be451ee9bfa0e91549784c8eed22ae7282b`.
The emitted78 declarations expose2 loaded actions,75 unavailable and1trueNULL.
All91 recipes are bound as metadata, never declared fully payload-loaded.

`tools/analysis/verify_gamma_import.py PACK CORE_FULL_PRODUCER_JSON`
compares all91 static recipe rows and the sentinel digest against the core
worker's independently executed producer receipt, plus weld identity/counts,
Stand, CCL/colors, material aliases and old prefix/binding hashes. No producer
execution is needed in the importer or verifier.

`tests/probes/gamma_asset_pack_check.cpp`, compiled only with the unchanged
`src/engine/character_pack.cpp`, loads old/new packs through the strict decoder
and checks the old namespace/bindings and real Stand motion. This is a private
development check, not a native source-list or runtime metadata-reader change.

The focused Python module covers inverse reconstruction, aliases, old record and
binding preservation, explicit missing/NULL states, unsupported declaration and
motion rejection, output hardlinks, unchanged sidecars and failed publication.

## Root integration command

After existing importer hooks have generated the unchanged selection/Sonic
sidecars, replace the ONE existing pack using the appended builder:

```sh
reference/python/bin/python tools/import/gamma_actor_resources.py \
  "$GAME/system/CHRMODELS_orig.dll" "$GAME/sonic.exe" \
  game/imported/characters/sonic-selection.actions
```

`GAME` is the user's owned install. This emits `gamma-actor.json` beside the
pack. Root must add the native metadata reader and bind shared mutable resources;
no runtime file or source list is changed in this packet. Run the original-proof
comparison with core's full `build/gamma-selection-core/assets.json` receipt.
Compile the private strict checker with:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -Isrc \
  tests/probes/gamma_asset_pack_check.cpp src/engine/character_pack.cpp \
  -o build/gamma-actor-assets/strict-pack-check
```

Pass NEW_PACK OLD_SONIC_PACK3302 and3303 to that checker. The old pack is a
verification input only; the application retains its ONE CharacterPack owner.

## Evidence and limits

Authoritative full dossiers: Gamma483430, InitGammaAnimData7ca300 and
InitGammaWeldInfo7cb560 in Root's closed Gamma recovery inventory. Core contract:
`packet/gamma-selection-core`, commit3477fda; its source proof independently executed
both complete producers. Full private producer receipt is core `build/gamma-selection-core/assets.json`,
SHA726af0fee81cd355dbb3e833ba0196c4f304d5bb5767ce92d3c956850ccf33ea;
committed `asset-requirements.json` is a summary without91 full recipe rows.
The complete static import matches every recipe and weld row.

Constructed action3c5d530 is statically defined by7d2330: E102_OBJECTS[0] plus
motion9c1050. Recipe72 records the dependency; request0 closure does not claim its
payload loaded. Controller483430 has no direct event580 call. Source49fb10 and
4c5420 remain neutral setup requirements. Globals3c53b60..6c are NJS_POINT3COL.

No actor, render/GPU, N3b, product-build, native-app or full-gate claim. No push.

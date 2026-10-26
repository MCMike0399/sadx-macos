# Reached Question basic-model closure context

Frozen source-context packet on `packet/basic-model-closure-context`, based on
`940dca3`. Prior context: `8ae7ea3` on `packet/basic-model-renderer` in
`/Users/burbujamc/Developer/sadx-wt-basic-model`.
No renderer implementation or N3b completion is claimed. Root owns the runtime,
source-only closure checks and full gate. No compilation, apps, Ghidra, make,
oracles, workers or cache writes were performed here.

The evidence index is
[basic-model-closure-context.json](reference/production-integration-20261026/basic-model-closure-context.json).
It records exact local paths, SHA-256 hashes, source IDs, inclusive body ranges,
missing-body navigation records and static profile intersections. Ignored dossiers
remain in `/Users/burbujamc/Developer/sadx-macos/reference`; they are not copied
into this packet. The source executable identity is `6b0937ca…24cd74`.

## Reached route and closure

Root reports the normal-input route now passes queued/current texture selection,
the actual `403250` list gate and shared mutable object traversal, stopping at
`native_basic_model_renderer`, frame 1382. This packet does not rerun that route.
The prior packet's imported `sonic-selection.actions` observation has 3302 records:
Question object 2942, flags `0x17` (user-supplied observation), model 2943, 350
vertices, one material, one meshset, 37 strips, normals and UV, no runtime buffer.
The original layout index is 5, primitive index 3, target `7835f0`.
This is initial resource evidence, not an executed callback or pixel receipt.

The following is the required closure, including its unresolved leaves. An arrow
does not certify that its target has a recovered native implementation.

| Source entry | Required behavior and source facts | Existing implementation / remaining seam |
|---|---|---|
| `77ef70 njDrawModel` | Begin `781cc0`, dispatch `77eda0`, tail reset `781df0` | Current native `object_draw::draw_model` throws; replace only after lower closure is judged |
| `781cc0 CopyConstantMaterial` | `791a80(current)`; device states 22,145,147 = 1; cull cache = 1; control bit 16 packs constant color through `644340`; bit 32 checks four components and sets flag 4 | Full dossier `ev_d287…aac`; use Display material/control owners, not new copies; CPU conversion already exists |
| `791a80` | `770804(3d12e00,current,389d358)`; `770d10(3d0fd40,null,current)` | Full dossier `ev_f7ea…975`; use existing D3DX multiply/inverse implementation after confirming dispatch slots; distinct inverse-current scratch |
| `77eda0 njDirectDrawModel` | Flag bits 2/4 select `784ae0` alternate renderer; otherwise `791a50`, `791ab0`, material transitions and original dispatch table, `791ad0`, stream/index unbind | Full dossier `ev_8156…b57`; retain alternate renderer as explicit STUB until reached/recovered |
| `791a50 ProjectToWorldSpace` | Multiply current by actual inverse view into `3d12900`; multiply current by inverse-current scratch into `3d128c0` | Full nested dossier `extra-791a50.json`; do not substitute guessed transform math |
| `791ab0 / 791ad0` | Set world transform from `3d12900`, then restore from `389d318` | Full nested dossiers; actual device world is `Display.device_state_.world`; source storage and device state are different roles |
| `784850 Direct3D_ParseMaterial` | Build actual material; publish two packed colors; apply constant attributes; address/blend tables; `78d140` or null texture; lighting; environment transition/reset; shading mode | Full dossier `ev_2bc1…48d8`; assembly must govern, pseudocode loses material fields and a reset branch |
| Material callback | Snapshot `3d08420` once before mesh loop; call saved callback after every changed material | Bootstrap selects `CorrectMaterial` (`40a340`), not a guaranteed null hook; borrow bootstrap's existing enum; body and actual route value still need evidence |
| `78d140 Direct3D_SetTextureNum` | ECX index, wrapping index×12; current-list entry +8, tail-call `78cf20` | Full five-instruction dossier `ev_fb45…9db`; needs its own plain function, including sprite use; `77f440` is observably different |
| `78cf20 / 78cdc0` | Existing cache and palette logic, device texture binding and format result | `texture_selection::cache` already shares canonical owners; dirty-palette upload remains an explicit stop after version publication |
| `7835f0 polybuff_normal_uv_strip` | Original conversion selected by layout 5/primitive 3 | Full body not found in inspected caches; only callee list `ev_8738…9f7` survives. Do not derive the algorithm from loader replacement code |
| `794600 / 794630 / 794750 / 794760` | Set current PolyBuff, lock strip, unlock, draw strip | Names/ABIs in pinned `direct3d.h`; callee lists survive, full bodies not located; API calls, state, counts, lock flags and failure behavior remain unknown |
| Device draw → Metal | Owned vertex/index bytes + current state → `graphics::Draw` → existing `graphics::Frame` → `MetalDeviceFrameRenderer::encode` | Existing HLE supplies reference API shape, not native game logic; native app already consumes `display.output().frame()` |

`77eda0` assembly proves cdecl primitive operands are `(meshset, points, normals)`
at `77eef9..77ef06`. The old header reverses the last two parameter names for
`7835f0`; do not propagate that declaration as established semantics. It also
proves meshset stride 28, material stride 20, material index mask `0x3fff`, and
dispatch from model normals plus meshset colors/UV. The imported cold meshset's
buffer is absent. The buffered branch is nevertheless real: SetVertexShader,
SetStreamSource, optional SetIndices, DrawPrimitive/DrawIndexedPrimitive. Keep it
explicitly unrecovered if the first bounded implementation covers only the cold
strip path. Never fabricate a buffer or silently ignore an unexpected one.

## Canonical ownership map

These associations use source address flow and existing producers/consumers,
not equal C++ types or equal values. Future annotations must be checked for
existing legacy owners before adding or migrating a whole owner.

| Source storage | Canonical owner / evidence | Design constraint |
|---|---|---|
| Current matrix `3d0f818`, stack metadata | `Display.matrices_`, `coordinates_`; native Question and traversal already mutate them | Borrow current slot by reference; no per-object matrix shadow |
| Inverse view `389d358` | `Display.view_.inverse`; `view::set_projection` computes it from the same current matrix as legacy `791170`; reference view bridge flushes that value to `389d358` | `791a80` and `791a50` both consume this exact source address. No second inverse-view owner |
| Inverse current `3d0fd40` | Produced by `791a80`, consumed by `791a50`; no native value owner found in this base | Add only recovered renderer scratch under Display's system-owned state; singular inverse preserves prior destination as existing D3DX does |
| World `3d12900`, no-view `3d12e00`, extracted `3d128c0` | Distinct destinations in transform dossiers; native mutable source storage not found | Do not alias any of these to inverse view, `view_copy`, or device world because all are matrices |
| Reset world source `389d318` | Source `791ad0` reads it; existing reference device initialization owns its source mapping | Do not alias to `screen_transform_`/device world merely because startup values are identity. Establish its producer/lifetime or immutable data provenance before binding |
| Constant geometry material `3d0f7f0..7ff` | `Display.material_argb_`; queue setter copies through `Display::set_constant_material`, raw sprite path also writes it | Borrow this array for begin/constant-material conversion |
| Queue material `3ab9864` | `Display.queue_state_.current_material`; `queue::set_material` clamp then geometry-copy behavior | Distinct from geometry material even when equal |
| Reset `3b18230`, view material | `Display.reset_material_`, `view_material_` existing reset/screen routes | Neither is a replacement for current constant geometry material |
| Control `3d0f9c8`, attribute AND `3d0f840`, OR `3d0f9c4` | `Display.control_3d_`, `constant_attribute_mask_`, `constant_attributes_`; same refs passed into queue consumer | Model parsing must read these live canonical values |
| Current list `3d0fa24`, requested `3ce7128`, cached ID `3d0fdf4`, active memory `3d129a0` | `Display.current_texture_list_`, `requested_texture_memory_`, `cached_texture_global_index_`, `active_texture_memory_` via borrowed `texture_selection::State` | Preserve each separate owner and callback/store order |
| Queued list `3abd950`, queued index | `Display.queued_texture_list_`, `queued_texture_index_`; actual `403250` gate reads queued list | Do not gate on current list or republish queued selection from material parsing |
| Palette globals `3d11f80`, `3cfc000`, `3d128a8` | `Display.texture_palettes_`, annotated in `native_texture_palette.hpp` | Same versions/colors/surface handles as sprites; retain dirty upload stop |
| Material callback `3d08420` | `Session` bootstrap state `material_callback`, source `413c00 → 783260(40a340)` | Borrow this existing logical owner; source loop snapshots identity, not all mutable state |
| RenderFlags `3d08498`, packed colors `3d08488/8c/94`, cull cache `3d08490` | Symbols and stores established in six dossiers; no native owning values found here | Renderer state needs these distinct values, not aliases to `graphics::Draw.material` or HUD color |
| Model/mesh/material/vector payloads | `Session.character_pack_`; object identity `Session.character_objects_` | Read the existing pack records by ID; share mutable ObjectState. Pass pack by const reference into the existing traversal composition; no reload, per-draw graph copies or model registry |
| Device/material/light/sampler state | `Display.device_state_`; `device_lights_`; retained bound surface in LogoRenderer | Modify actual shared Draw state, then snapshot device command state at submission; never duplicate the game object for that snapshot |

Source materials are imported model payloads (`NJS_MATERIAL` is 20 bytes), distinct
from constant-material arrays and the current `graphics::Material`. Add typed
borrowed mesh/material views or decode once at pack load only after checking the
existing payload schema. `CharacterPack::model()` returns metadata by value;
that does not authorize copying a mutable object graph or re-decoding all assets
at every draw. No runtime reads of original EXE/DLLs are part of this design.

## Full material and texture interface requirement

Implement the actual `78d140` wrapper, not a renamed `77f440`. Its full assembly
is LEA index×3, load current list, load names array, load memory at index×12+8,
JMP `78cf20`. It has no null-list guard, no requested-memory store, and returns
the cache's format result. `77f440` guards null, stores requested memory and
returns 1 or -1. Native unavailable-resource errors must retain the failing
source read, rather than invent success for missing/null resources.

Use the same wrapper from the SpriteImmediate composition currently routed by
`Display::sprite_services` through `LogoRenderer::select_texture_index` (77f440).
Recover the sprite call-site operand/return use and retain its own filter restore
order. The wrapper body itself contains no sampler calls. The full model
material sequence separately sets stage address states 13/14, blends 19/20,
lighting 137, environment transform transitions, and shading mode `791990`.
Both pieces are required; conflating a cache selector with that full sequence
would leave rendering incomplete.

Material assembly builds Diffuse and Ambient from packed chosen color using
FILD/FSTP, Specular from material bytes, Emissive zero and exponent from +8.
Do not invent division by 255: this source assembly has none. Source texture
index is material +12 masked to 16 bits. Destination blend is the independent
`(flags >> 26) & 7` lookup, not the pseudocode's reused address-mode variable.
The environment-off reset at `784aa9` exists in assembly despite Ghidra's
unreachable warning. Retain begin/reset mask behavior and callback ordering.
The `88ae0c` address table and `88ae1c` blend table need exact maintained literal
bytes; equal-looking existing blend arrays do not establish source aliasing.

## Proposed small interface

Use an engine module with ordinary functions `draw_model`, `parse_material`,
`copy_constant_material`, and the actual texture wrapper. Keep primitive and
buffer helpers private except ordinary named link-time STUB declarations for
unrecovered source entries. Function names must be settled against the symbol
database; unnamed `791a80` needs a semantic override decision before source code.

The draw function takes a model resource ID, const reference to the existing
CharacterPack, system-owned renderer state, and borrowed references to the
existing matrix/view/texture/device/frame owners. A plain non-owning references
aggregate is reasonable if the parameter list becomes long; it must not acquire
storage, dispatch algorithms, optional fixture fields or a virtual hierarchy.
Keep source render scratch in one system-owned state value inside Display.
No new Calls/Services hierarchy, attach method, manager or registry is needed.
The existing opaque `object_draw::Context` remains runtime composition: define
its model handoff beside its current definition, or share its private definition
within runtime. Do not move model behavior there just to gain private access.

The native device adapter writes `graphics::Draw` and appends immutable command
snapshots to the already owned Frame, retaining the selected texture independently
of manager release. Reference adapters marshal guest buffers/registers and record
API receipts. API lock/unlock/draw behavior must be recovered before choosing its
native storage rules. Reuse `graphics::decode_vertices` where its input contract
matches recovered bytes; it is not a substitute for original strip conversion.
`sadx_native.mm:405` already sends the resulting Frame to shared Metal; no new
rendering framework is needed.

## Explicit missing facts and bounded next work

1. Recover full `7835f0` plus `794600/794630/794750/794760` dossiers after Root's
   gate. Existing `inv-callees-*` records have IDs in the JSON, but no bodies;
   empty direct-callee lists do not mean they do no work (COM calls are indirect).
   Inspect original strip winding/count/degenerate vertices, signed indices,
   UV scale, FVF/stride, PolyBuff identity and initialization, buffer exhaustion,
   lock offsets/flags/HRESULT behavior, unlock and exact draw calls. Do not infer
   these from the inactive GPL replacement implementation.
2. Recover `40a340 CorrectMaterial` and verify the actual callback value on this
   route. Preserve its changes and the caller's saved identity. Existing source
   proves bootstrap selects it; no full body was located by this inspection.
3. `781ec0`, `7944b0`, `794500`, alternate renderer `784ae0` need full bodies if
   reached. `644340` has a maintained `crt_ftol_low32` implementation/oracle;
   use its actual correction semantics, not a host cast. Missing full dossier
   here is not a claim that its existing port is absent. Retain explicit STUBs
   for unrecovered branches rather than restricting source flags to the fixture.
4. ABI facts: old headers and Ghidra defaults are hypotheses. Resolve strip
   operands from dispatch assembly, fastcall ECX/EDX and stack cleanup for each
   lower helper, x87 return/constant conversions, D3DX slot identities and SSE
   reciprocal estimate. Existing multiply/inverse ports are candidates, not a
   licence to replace original inverse with a host matrix library.
5. No new Question-specific FPCW/MXCSR receipt was acquired. The standing
   post-device census supports 0x007f at observed entries only; it does not list
   this complete lower cluster. Audit each arithmetic expression's m32/m64,
   FILD/FISTP and retained widths. Do not prescribe plain-float conversion before
   census/oracle premises are met; existing `crt_ftol` and D3DX helpers retain
   their documented limits (including inverse FP partials).
6. Static profile intersection finds no active patch in the 18 available dossier
   ranges. Five overlaps are inactive (CrashGuard cache/select, mipmapping,
   projection FOVFix, test-spawn). FixVertexColorRendering and E102NGonFix remain
   false. Missing lower bodies cannot be range-certified; repeat full-range
   profile checking when their dossiers exist, including indirect table writes.

Next bounded implementation candidate: the full `78d140 → existing cache`
wrapper and its sprite/model-facing composition, with actual index ABI and
format-return behavior. This is small enough to judge before the larger strip
conversion. The next bounded recovery cluster is original `7835f0` plus its four
PolyBuff helpers, not every polygon layout or actor preview.

Oracle proposal (not executed): actual original instructions for `78d140`, then
the constant/material/world/strip closure through the same plain functions;
reference adapter compares all global stores, return values, matrix words,
ordered material/render/sampler/texture calls, buffer lock/unlock/draw operands
and output vertex bytes. Include same-ID/different-memory texture cache cases,
callback mutation and saved callback identity, flags and material transitions,
singular transforms, strip winding/count edges and buffer failures. API receipts
prove the source/API seam only; separately submit those bytes/state through the
real shared Metal path and use normal-input Question image/draw assertions with
draw-suppression, wrong-material, wrong-world and wrong-UV mutants. Holdout and
unchanged sprite/texture/object judges precede Root's integration gate. No source
algorithm, tolerance or fixture is narrowed to make the cold Question pass.

Validation here: JSON parse, evidence/source hash verification and git diff
inspection only. All builds, apps, recovery, oracles and gates were deliberately
skipped under the task's read-only source-preparation constraint.

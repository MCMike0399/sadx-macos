# Basic-model wrapper and material core

Candidate on `packet/basic-model-core`, based on `04c40ae` in this durable
worktree. Root owns runtime integration, the independent STYLE review, the full
gate and normal-input journeys. There is no GPU, N3b, actor or story claim.

## Source and ownership

`reference/production-integration-20261026/basic-model-core-source.json` records
full dossier paths, source IDs, original body ranges, dossier hashes and original
PE bytes/hashes for the independent address/blend/primitive tables, scale, zero
and reset-world identity. No loader replacement code was used. Source executable
SHA-256 is `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

The engine module is `src/engine/basic_model.{hpp,cpp}`. `State` owns only nine
previously unowned source scratch values: five packed-color/flag/cull values and
four matrices. All nine initial ranges are source virtual zero fill, proved from
the PE section extents. No identity defaults were invented for render scratch.
The existing object-draw handoff's annotation changes to NOTSA because the actual
source wrapper now has its single provenance declaration in this module. Its
signature and behavior are unchanged. The sole semantic symbol override names
`791a80 PrepareBasicModelMatrices`; old SDK names govern the other annotations.

`Owners` borrows canonical control, attribute AND/OR, geometry constant material,
current matrix, inverse view and reset-world source. Root must bind inverse view
to `Display.view_.inverse`. Reset-world is the distinct source identity mapped by
`native_game_reference.cpp:643-664`, existing screen initial/base-transform
storage, never guessed from current device world or screen-transform equality.
World, no-view, inverse-current and extracted scratch remain four distinct values.
The device's actual world lives in the existing shared command-state owner.

CharacterPack is borrowed by const reference. Model metadata is a small value;
meshes remain `(pack, resource ID, ordinal)` views and each changed material reads
its own five source words. There is no copied mutable object graph, model registry,
per-draw pack reload or importer change. Mesh stride is 28 after the count prefix;
material stride is 20, with packed diffuse/specular and a raw floating exponent.
Imported runtime-buffer words are zero today. Nonzero words take an explicit
buffered-draw stop; they are never asserted globally absent.

## Link-time binding contract

Root defines `basic_model::Context` as a borrowed integration context. The module
uses ordinary functions, with no new Services/Calls hierarchy or stored callbacks.

* `exact(Context&,float)` constructs existing `fidelity::ExactFloat` from the
  caller's actual arithmetic context. Constants use the real MSVC conversion,
  including low-byte wrap, not host casts/clamps. Quiet zero comparisons retain
  B,G,R,A order. No Question-entry FP-control receipt is inferred.
* `multiply_matrices` binds existing D3DX slot 3 / `d3dx_matrix_multiply_774310`.
  `invert_matrix` binds slot 18 / `d3dx_matrix_inverse_7757e0`, storing only an
  available result and preserving the destination on singular/unordered input.
  Preserve existing CPU-estimate and FP partial limits; use no host matrix library.
* Device/material/texture/world/stream/index seams write Root's actual shared
  device state, not a private copy. Each source operation reacquires that device
  through the context. Device HRESULTs are ignored by the source, while a truly
  unavailable native binding propagates an explicit failure.
* `set_texture_number` binds the already exact shared
  `texture_selection::set_texture_number` (`78d140`), never `select_index`
  (`77f440`). Its return is intentionally ignored by the material parser.
* `set_null_texture` binds existing
  `texture_lifetime::unbind_cached_texture` (`78bba0`) using canonical State.
  Cache -1 returns without touching active memory or image. Otherwise the real
  SetTexture receipt precedes cache=-1 and active=null. No duplicate owner or
  null-texture algorithm is added here.
* `snapshot_material_callback` reads the existing bootstrap callback owner once
  and returns an ordinary function pointer. For CorrectMaterial it must return
  a stable wrapper for that identity, rather than a wrapper which rereads the
  mutable callback enum later. The same saved callback runs after each changed
  material, even if a preceding invocation changes the global hook. It is not
  universally null. Root's actual CorrectMaterial component remains independent.
* CorrectMaterial's Power getter must borrow
  `Session.lighting_->active_palette.upper_specular_power`, the existing
  LSPalette/SP_pow field, not a new render-State value or zero default. The
  active palette's entire 96-byte owner already has source/oracle evidence.
* `draw_normal_uv_strip` hands off `(mesh,points,normals)` to Root's binding of
  the independently frozen `polybuff::normal_uv_strip` packet `1960fed`.
  The original header's reversed last two parameter names are not followed.
  Root builds typed borrowed signed16 index/UV and vector-component views from
  its existing validated resources; no per-draw owned graph is introduced.
  Color/cull inputs are snapshots of this `State` at the actual lower call.
* Other primitive selections and the buffered path must throw a tagged
  `Unavailable` boundary. The alternate renderer remains an explicit named stop. Environment setup/reset
  and offset material are recovered ordinary engine functions, not binding stubs.
  No substitute rendering or guessed device outcome is permitted.

Boundary mapping: alternate call is `77edb3`, primitive table call `77ef03`,
buffer branch begins `77ee3a`, offset material `784878`, environment setup
`784a97`, texture-transform reset `784aa9` or reset-wrapper `781df9`.
The native integration retains these source boundaries in its diagnostics.

## Reached material and remaining work

Root's actual immutable Question material is diffuse `ffffff00`, specular
`ffffffff`, exponent 11, texture 7, flags `9461a400`. Environment mapping IS
reached. Normal/UV strip alone is not Question closure. Root's original helper batch is now closed and consumed: `7944b0` writes stage
transform flags 2, uploads its exact environment matrix, then selects camera-space
normal generation. `794500` writes flags 0, uploads the separately proved identity
texture matrix, then restores coordinate index 0. API order and surrounding live
render-flag read/modify/write are preserved. `781ec0` retains alpha normalization,
spills the other normalized channels, clamps four retained sums and converts
B,G,R,A with the existing MSVC conversion. No Ghidra provider was started here.

Both texture matrices are maintained literals with separate source-byte hashes,
not runtime PE reads or aliases to other identity matrices.

Texture shading `791990` is recovered from the full cached original dossier.
Modes Modulate/DecalAlpha/ModulateAlpha write alpha-enable, stage alpha-operation
and alpha-reference in source order; mode 3 and unknown modes do nothing. Parser
USE_ALPHA selects ModulateAlpha (2), not the SDK's Decal (3).

## Private verification

Run, with all outputs inside this worktree and at most one compiler process:

```
/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python \
  tools/oracles/check_basic_model_packet.py \
  --python /Users/burbujamc/Developer/sadx-macos/reference/python/bin/python \
  --exe /Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe \
  --mutants
```

`tests/unit/basic_model.cpp` is registered as `basic-model` in the unit manifest.
The new probe/backend and new original-instruction judge do not change any
existing judge/probe/fixture/tolerance. Root registers new judge files in its
separate judge integration; this packet does not refresh the judge manifest,
Makefile, native-sources list, oracle manifest or generated STATUS.

The judge executes the complete original wrapper/parser/transform/shading bodies
with unmodified original instructions. D3DX operations currently have explicit
operand/store receipts, and texture/strip calls have explicit lower receipts. Environment, reset, offset
and shading helpers execute their actual original bodies, down to API receipts. D3DX numerical behavior, texture-cache algorithms, strip conversion and GPU output
are not proved by those lower receipts.
It compares all renderer values/matrix words and ordered receipt words. Separate
seeds produce dev/holdout cases, raw input binaries and full source/native JSONL
receipts in `build/basic-model-core`. FP flags are compared separately and any
difference is reported as partial, never a pass. Both broad oracle runs exit 2 when values agree but FP status differs.
The packet runner accepts that explicitly recorded partial status; it never
labels a partial oracle as pass.

Mutants change normalized color,
destination blend field, environment-off reset, callback identity, reset-world
owner, constant packing and environment-coordinate generation. Their failures must be value
mismatches; compilation failure and FP-status differences do not kill a mutant.

Root must initiate a fresh independent STYLE review of the frozen final diff and
evidence before integration. Full gate, original Question FP premises, actual
CorrectMaterial binding, live generated-coordinate device delivery and journeys
remain Root owned and unverified here.

Final focused results: 2,427 dev / 3,427 holdout, zero value/ordered-call
mismatches; exit 2 FP partials on 172 / 274 cases. Unit exit 0; seven value
mutants exit 1. Profile checker sees all 13 ranges and 0 UNHANDLED overlaps.
Provenance, layers, ownership and readability checks pass; new engine files
have zero counts in all ten readability metrics. Reports, source hashes and
raw-receipt paths are frozen in
`reference/production-integration-20261026/basic-model-core-proof.json`.

The private symbol generator could not regenerate from absent sa-tools Git
metadata in this worktree. Provenance was checked against Main's existing
canonical symbol index merged with this packet's one evidence-backed override
using the repository's unchanged symbol merger; the private result lives in
`build/basic-model-core/symbols.json`. Root regenerates normally on integration.

## Current-matrix representation follow-up

The frozen base is `16f8805`; the follow-up preserves its algorithm, original
oracle, harness, fixtures, seeds and seven mutations byte for byte. `Owners`
now borrows `const graphics::Matrix &current_matrix`, the same float-array type
as the canonical Ninja current matrix. `multiply_matrices` takes that live float
matrix as its first operand; `invert_matrix` takes it as its source. Inverse
view, reset world and all four render scratch matrices remain D3dxMatrix values.

At each actual numeric/reference call boundary, copy the floating matrix's
object representation with `memcpy` into a temporary D3dxMatrix operand. This
is a per-call representation adapter, never an arithmetic float conversion,
reinterpret cast, cached shadow or additional owner. Signed zero and signaling
NaN bits are preserved. Native probe initialization and the unit's current-matrix
setup use the same raw-word memcpy; unit assertions and expected words do not
change. The engine implementation basic_model.cpp is byte unchanged.

Rerun results are identical: 2,427 dev and 3,427 holdout, zero value/order
mismatches, FP-partial exit 2 on 172/274 cases, unit exit 0, seven value mutants
caught. Full source/native API/state receipt files and both case binaries have
identical SHA-256 hashes to the frozen base. Exact oracle/harness hashes and
comparison receipts are in `basic-model-current-matrix-proof.json` beside the
main packet proof. Root's fresh review and native binding are still pending.

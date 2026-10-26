# Native model render binding review

**Verdict: approve the frozen route-to-prefix binding stage. No blocking defect
found in the assigned new code.** This does not approve canonical InitPolyBuffers,
complete rendering, normal-input startup, actor pixels, N3b or GPU fidelity.

Reviewed `83f0ded4eb0fca29ffaa003e09cf6a3c8feaf9f0` against `6e74521`, including
source `faf1d87` and judge registrations `7eaec87` / `83f0ded`. Source worktree
`/Users/burbujamc/Developer/sadx-wt-basic-model-binding` was read only and remained
at the frozen commit. This review and `REVIEW-evidence.json` are the only authored
files, on `packet/review-native-model-binding` in the assigned isolated worktree.
Read AGENTS, GOAL, STYLE, STATUS, latest HANDOFF, relevant shared-device,
strip/model, preview and lighting memory, prior component reviews and receipts.
No workers, builds, apps, Ghidra, gate, tests or mutation runs were launched.
**Tests not rerun.** Hash/receipt inspection and image viewing are review evidence,
not test executions.

## Standards — STYLE §12 and ownership

No blocking findings.

1. New game-tier code uses semantic names, structured loops and ordinary link-time
   bindings. No address dispatch, guest memory, executable reads, emulator math,
   decimal-encoded addresses or hidden raster algorithm was introduced. Existing
   graphics decoding and maintained D3DX helpers are called directly. Producer
   provenance/readability/layer/owner checks report zero errors, new debt and
   regressions; these tools were not rerun.
2. Recovered source-entry names/annotations remain in the byte-unchanged reviewed
   components. Added resource, geometry and context structure is explained with
   NOTSA comments, and D3D storage/draw replacement is tagged platform:d3d8.
   No new registry, attach mechanism, service hierarchy or enhancement is added.
3. Actual source state remains canonical: Session owns one SceneState containing
   the existing ObjectState and new GeometryState. Display owns stream state and
   the reviewed model scratch; contexts borrow actual Display/Session fields.
   No per-draw object graph or copied palette/matrix owner appears.
4. Current matrix is the live Ninja float-array reference; numeric calls memcpy
   into existing D3DX word arrays per operation (`model_draw.cpp:43–52`). Inverse
   view and reset-world borrow distinct `view_.inverse` and
   `view_.depth.base_transform` owners. Four scratch matrices remain distinct.
   ExactFloat and SSE contexts remain the actual Display contexts. No arithmetic
   simplification or new precision premise was introduced; inherited FP partials
   remain partial.
5. Imported content remains immutable. Geometry converts every validated resource
   once into genuine uint32/int16 objects; no byte-vector alias cast is used.
   CharacterPack validation proves array header/count/stride divisibility before
   GeometryState construction. Signed short conversion preserves all 16 bits.
   One vector array per ResourceId is shared by all callers and remains mutable
   for future genuine deformation producers; none is invented here.
6. `model_buffer_seam.cpp:5` deliberately throws
   `native_polybuff_owner_binding`. There is no second record 8, guessed capacity,
   partial startup or successful fallback. Alternate model, buffered mesh and
   unsupported primitive paths remain explicit stops. Empty observation bindings
   are observers only; engine stores already happened.
7. All 22 stage source hashes match frozen git blobs. Strip hpp/cpp match
   `1960fed`, material correction hpp/cpp match `1a5a0f3`, and model hpp/cpp match
   `9220d1b` byte for byte. Their bounded approvals `95913d8` / `857b0cf` are reused;
   this review does not claim to repeat their complete algorithm audits.
8. Judge registration preserves all previous protected hashes except the strictly
   appended unit manifest. Four previously unregistered model judge files are
   added; all 882 protected files exist in the producer worktree and independently
   hash correctly. Existing expected values, tolerances and assertions are not
   changed by this stage. The old object-binding unit adapts its context/Scene API
   while retaining its assertions and pre-existing original-byte read.

## Spec — platform and binding fidelity

No blocking findings.

- `model_device.cpp:24–35` matches the local HLE SetStreamSource contract
  (`hle_d3d8.cpp:226`): retain new before releasing old only on identity change;
  same-resource binding updates stride without ref churn. Source handle owns ref
  1, stream owns its independent reference, and stream destruction releases it.
  Engine Free clears source identity; the platform release does not fake it.
- Resource, RAM backing and VertexCursor are distinct. Invalid Lock leaves the
  caller output untouched; actual null resource throws a tagged DataFault.
  `model_device.cpp:101–105` wraps uint32 relative byte arithmetic first, checks
  the full four-byte range, then writes. Checked range subtraction cannot
  underflow. Create publishes the new handle only after allocation succeeds;
  bad_alloc preserves the prior handle. Argument words are uninitialized until
  engine stores; no RAII cleanup repairs the source's successful-old-handle leak.
- `model_device.cpp:56–79` checks strip type, actual stream/stride and selected
  start/count range with widened arithmetic. It snapshots the actual bound image
  and shared device before decoding, copies the selected bytes, then submits an
  owned Draw to the actual Frame. Geometry, indices, material, transforms, lights,
  render/texture states and immutable image references survive later buffer reuse,
  state changes, handle Free and stream clearing. No Metal code changes occur.
- `model_draw.cpp:91–111` reads actual pack mesh descriptors (four-byte header,
  28-byte stride; count at 2, indices at 4, UV at 20), and borrows bounded scene
  geometry arrays. Pack.model supplies the real points/normals identities.
  The reviewed core selects only its implemented normal/UV strip path; other
  layouts do not silently draw through a substitute algorithm.
- Material Get/Set copies the actual shared device's whole 17-word material.
  Power reads the live active palette at invocation, independently of asset power.
  Callback selection returns a stable wrapper once; that wrapper does not reread
  the mutable callback enum. Texture index calls actual LogoRenderer
  set_texture_number / 78d140. Null texture follows existing 78bba0 lifetime logic,
  including its cache -1 guard, rather than unconditional image clearing.
- Question/preview forwarding passes one borrowed ModelDrawEnvironment through
  existing traversal into actual model drawing. The environment's lifetime spans
  the synchronous call; no callback retains its local reference. The immutable
  selection pack independently hashes to `df53cf2e…f38d423`; direct record parsing
  confirms Question model 2943, material 2949, words `ffffff00 ffffffff 41300000
  00000007 9461a400`. Environment mapping is required, not an optional bypass.

## Evidence, coverage and remaining qualification

The committed stage receipt reports four unit exits 0: model-device,
character-geometry, model-binding and object-draw-binding. Separately retained
`model-binding-unit.exit` is 0; the other three unit exits were accepted from the
committed summary, not inferred from build exits. New tests were read: independent
raw-array memcmp, live resource aliases, immutable pack, kind errors, actual strip
and shared Frame, invalid/stale Lock output, stream retention after source Free,
state/geometry immutability, distinct live matrix inputs, late Power/callback
identity and environment-state snapshot versus reset are meaningful checks.

Coverage is bounded: no new binding mutation receipt is supplied, and the device
unit does not directly exercise changed stride on the same handle, a different
resource replacing a bound one, nonzero start, wrapped-write offsets, bad_alloc or
texture release after a model submission. These code paths were inspected, not
experimentally qualified here. The completed integration still needs GOAL §7's
deliberate break/content check and full combined gate. Prior component mutation
results do not substitute for new binding mutation results.

Producer native app build and forced native closure exits are 0; the retained
closure log reports PASS and its JSON names all new product sources. Closure
proves dependencies, not behavior. Raw normal-input command matches the stage
receipt; exit.json records actual exit 1, and application.log ends at
`native_polybuff_owner_binding`. Capture 1382 PPM independently hashes to
`c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.
The retained PNG was inspected: outgoing title fade, no preview actors visible.
The stage's equality-to-prior-frontier claim was not separately recaptured.

The composed model unit replaces ONLY the pending normal-buffer getter with a
real fixture initialized by the reviewed helper. It exercises real imported
Question/model/material/environment/texture/strip/device code, with a synthetic
texture image. It proves neither native InitPolyBuffers nor normal-input actor
rendering or GPU delivery. Product getter still throws. Canonical owner migration
is another packet and requires fresh review of its final integration.

Existing strictness limits persist: strip 1,014/2,662 cases have 154/198 FP-status
partial rows; model 2,427/3,427 have 172/274; material correction 4,227/6,227 have
3,231/4,567. Prior reports have zero value mismatches but are not strict passes.
No new algorithm oracle, holdout or mutants were rerun. Main's last gate at
04c40ae (262 native, 386 Python) excludes this stage. Full startup, all actors,
actual pixels, Windows GPU parity, N3b, gameplay and 63 FPS remain unverified.

Standards: zero blocking findings. Spec: zero blocking findings. Approval is
limited to the frozen route-to-prefix stage and the evidence described above.

# Queue consumption, sprite display and Metal checkpoint — 2026-10-12

Root requested an exhaustive handoff checkpoint; producing source is held stable. This is not a goal pause and the full native updated-PC game remains unfinished. No worker provider was launched, no Makefile/shared archive was edited, and there are no live worker tool/session handles.

## Shared producing changes, renewed freeze already released to root

- ninja_render.hpp/.cpp: exposes the single command issuing owner with explicit source load/dereference/call sites, adds the distinct D3D8 texture-stage command/result/callback contract. Existing blend/depth bodies use that same owner in their previous sequence. Private optimized old unit passes; actual6160 blend cases have0 mismatches in original-ninja-render-shared-comparison.json.
- ninja_sprite_queue.hpp/.cpp: captured source header+c now owns a typed optional texture-list reference alongside raw provenance. Raw0 establishes observed-null; nonzero without a real binding stays unknown. Conflicting nullness stops after the raw store with partial bytes retained. A lazy bucket-front query plus owned-record check permits next-edge reads after callbacks instead of eagerly snapshotting the whole list. Optimized original queue unit passes;17,664 allocator/header/sort operations across12 CW profiles plus256 immediate-state and63 texture-gate cases still match.17,256 FP flag differences remain and strict=false. Report: original-ninja-sprite-queue-owned-texture-comparison.json.
- Startup's approved actual512-byte calloc facts were added earlier: animation-tail words12a/12c/12e/floats130/134 zero and table140/buffer148 observed-null, generic state unknown. Focused eight-row startup test passed and root was notified to repin production. No new startup edits in this consumer checkpoint.

The canonical previous queue report remains historical. The private refreshed reports listed here pin the newer shared header/producer files. Root owns production gate/rebuild/repins; the64-program integration snapshot predates these later changes.

## New unintegrated WIP files

- src/ninja_sprite_display.hpp/.cpp
- src/ninja_queue_consumer.hpp/.cpp
- src/metal_sprite.hpp/.mm
- tests/ninja_sprite_test_fixture.hpp
- tests/ninja_queue_consumer.cpp
- tests/metal_sprite.mm

The new CPU producing files strict-compile but are not source-frozen for integration: the focused consumer test has only progressed through its first complete source sprite path. It currently fails its next standalone missing-depth test because SpriteFixture's standalone projection color is intentionally unknown; actual77e53b correctly stops at missing_color before the intended77e637 depth boundary. Next test-only correction: explicitly establish projection.color_argb_at_3d0f7f0 in that standalone fixture, remove the temporary stderr diagnostic, then continue the remaining mutation/missing-edge/two-pass tests. Do not change the producing implementation merely to bypass that true missing-color boundary.

## Sprite-display body and limits

ninja_display_sprite reconstructs actual77e390 list/index binding before numeric locals, real source geometry, addressU/addressV clamp3 stage commands, actual78b840 state preparation,791940 alpha state/stage operation, source depth comparison, source vertices, source FVF and DrawPrimitiveUP shape. It rereads the native device between backend calls, preserves ignored/omitted HRESULTs and partial artifacts. Source78c5a0 is verifiedRET.

Actual TEST AH,05 / JP at77e61c..61f: LESS alone executes FLDzero; greater/equal/unordered require actual78dbe0 after reciprocal FDIV/FST32. The unrounded source depth is retained. There is no default/identity/host divide provider. The world142 branch stops at791a50 because its required transform owners are not integrated. Missing external native vertex/draw/stage backends are explicit pending boundaries.

The current native first consumer fixture exercises the real positive-camera-Z below-minimum branch, yielding source FLDzero without replacing78dbe0. This is a supported conditional source path, not a playable-game or general projected-depth claim.

## Queue-consumer body and limits

Actual4086f0 traverses buckets0..2047 in stable source order, with an optional two-pass loop from909eb4 bits0/1. New consumer borrows the existing queue, packed-render, projection and texture-selection owners. It implements source initial color clamp/copy, queue/display gates, source maxima/mode stores, matrix capture/restore, known fog-disable/unchanged-sentinel and special-bit fog paths, source per-record captured state, required header texlist selection, depth/blend/color commands, actual type3 sprite dispatch, legitimate observed-null/default skips and final state effects. Unsupported selected bodies stop at their dispatch entry rather than pretending they reached an unlifted callee after omitted stores.

Critical facts:408f24 reads NEXT after the callback; missing/foreign/cyclic/sentinel-mismatched edges preserve prior dispatched facts. Final412400 transforms saved3b12208 as(signed>>1)&63 rather than exactly restoring it.411f30 really calls51f680RET; Ghidra's switch-case alias is lexical. The supported3/default/null branches never change local FOV; equality of that saved register with itself is determined even when the value is unknown. No guessed angle is supplied. A private actual7815c0/787e30 FOV reconstruction exists but is not invoked/validated by supported consumer branches.

Fog range/table paths4119b0/411a20 requiring787240/787260/787250 remain explicit unlifted boundaries. Generic type1/2/4..15 selected bodies are not reconstructed. Full buffer initialization/reset/reuse, immediate rectangle mode2, full draw dispatch, original hardware FP behavior and GPU parity remain open. No live source interpreter is present.

## Actual native Metal evidence

metal_sprite is a separate native Apple GPU consumer of real NinjaSpriteDrawCommand bytes plus explicit selected texture-binding snapshot, typed MetalTextureResource, source D3D8 render/stage-state map and optional originating record. It validates source device, shared upload-owner identity/archive/ordinal and ready(), never casts opaque void or substitutes record130 automatically. It supports finite screenFVF144/RHW1 strips only; missing state/depth/texture and unresolved world142 are explicit boundaries.

Source PE import is d3d8.dll / Direct3DCreate8. Source77e5d0/77e5e4 sets D3D8 TSS addressU/addressV toclamp3. Shader translates integer D3D8 pixel centers to Metal half-integer centers, preserves source112-byte vertices and samples retainedRGBA32Float texture data. Supported stage ops are selectarg1/selectarg2/modulate; unsupported state/filter/format domains stop. Original Windows fixed-function quantization/filter/blend/alpha-test/GPU parity is not established.

Actual Apple M5 Pro GPU test passed: supplied EFF_REGULAR.PVM record130,16,384 nearest-readback channels all bitwise match original packed16 channel ratios. Later opaque draw wins, fartherLESS-depth draw rejects, source-alpha blend has0 max absolute error against the declared finite test formula. Missing filter, conflicting null upload, world142, missing depth attachment and a later missing-state prefix all produce explicit failures. Encoded prior draws are committed and their real GPU completion is tested; the submission/completion closure retains exact draw/resource/record owners even after caller releases originals. No uncommitted failure can claim GPU completion.

Private report:/private/tmp/sadx-port-sources/metal-sprite-inspection.json. Shader SHAde2d6cf1f5214a36587436a6988c32a7b785ed6213965278c9e1b9715330f83b; vertex fixture SHA69916ae8829a89e6c7dfaefea457305f87c4ac5d06cc8bc63a2ab403249a8688; archive SHAe7972e401ef9744cecc01740d47d23343c82add0044b74b13fce77bcfc64a530. GPU inspection vertices/state are explicitly declared fixtures, not original runtime capture or complete queue/game execution.

Primary pixel-center references:Microsoft DirectX functional specification(https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm) says D3D9 and prior origin is the top-left pixel center; Apple Metal Render Context guide(https://developer-mdn.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Render-Ctx/Render-Ctx.html) specifies half-integer pixel centers. No primary documentation can replace future actual Windows GPU comparison.

## Commands / assets / resume

CPU unit assets in order: sprite40B,blend56B,depth32B,trig4100B,unit64B,texture-return16B(VA389d458),consumer8B(VA7dcb0c +7dce50). Private last command:

`/private/tmp/sadx-port-sources/ninja-queue-consumer-test sadx-native/build/assets/ninja-sprite-constants.bin sadx-native/build/assets/ninja-blend-tables.bin sadx-native/build/assets/ninja-depth-tables.bin sadx-native/build/assets/ninja-trig.f32 sadx-native/build/assets/ninja-unit.f32 /private/tmp/sadx-port-sources/ninja-texture-returns.bin /private/tmp/sadx-port-sources/ninja-queue-consumer-constants.bin`

Metal CLI:metal-sprite-test EFF_REGULAR.PVM REPORT.json. Private compile links new metal_sprite.mm +existing metal_texture.mm +read-only production libsadx-core.a with C++17,ARC,-ffp-contract=off,-Wall/-Wextra/-Werror,Metal+Foundation frameworks. Actual GPU execution requires approved escalated prefix `/private/tmp/sadx-port-sources/metal-sprite-test`; sandbox exposes no Metal device. All runs are terminal.

Next CPU work:fix the explicit color fixture only; complete unit mutation/two-pass tests; create reusable actual77e390 and4086f0 source-instruction probe/verifier. Those new original-body probes/verifiers do not yet exist. Use actual source bytes with declared externalCOM backend only; stop at real missing78dbe0, never stub its result. RunASan/UBSan for new producing code, add full CPU→Metal supported-path composition proof, then create canonical source/digest/evidence ledgers and freeze/release modules to root.

Retained consumer/fog dossiers already exist in ghidra-game-loop-recovery:4086f0,411af0,411b40,412160,411f30,7815c0,784d00,402f40,4119b0,411a20,794950,794980,787220,787230,787e30. Sprite791940/791a50/791990/791ab0/791ad0/78c5a0 dossiers are in ghidra-action-fp-recovery/extra-*.

Requested but not yet visible in that folder:408630,407ab0,407330,412400; remaining fog dependency candidates787240/787250 are verified thunks to7949b0/794e40, plus787260. Root serializes all providers; worker must not launch any.

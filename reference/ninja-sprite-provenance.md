# Native renderer state and sprite numeric slices

Target executable SHA256:6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74. Modules `ninja_render` and `ninja_sprite` are native C++17 source, not a runtime x86 interpreter. The58-program production gate is retained by the root; later queue work is separate.

## Evidence and asset identity

Ghidra4030d0 Evidence ev_f9595eefc708b72152e335fe411f99d85ec12bc0b4b84f4cd553bb35350b3b07 and77ec60 ev_b0380f491803c8f751c5fe8f9a6b99e9ea6d97cecd86a21c356d851e8bc8f79c support the blend owner. Sprite77e390 Evidence ev_1559a418ab96877c2d84eeb12e2d0cc38c05f456063c3c69577b7e3178ebeef0 is retained in ghidra-action-fp-recovery. Exact LLVM resolved register calling conventions and store/compare order where pseudocode lost them. 78dbe0 Evidence ev_685f753969366dc16ea135a00445b38ea76db02f6bb69de22df8585d7033d0cb and772615 ev_f60a0492b7253c9aa67bbe59b61ad0f028597ef8ab2ec1ed74fa5a43193ad6f8 establish the unresolved depth path.

Source94a2a8 is EFF_REGULAR's136-entry texture list, referenced by original archive tables90ec24/90ec5c. Animation9279d8 is20 bytes: width2,height2,center1/1,UV0..255, signed texture index130,attributes0. Exact PVM record130 is `bubble`, global index184f, PVRT offsetda730,64×64, source pixel format0/data format9, chunk SHA555f9a3c830740379d0814fb5eb26899ed0ee676fa8336cf09a91dc76b50e8dd. Facts are retained in sprite-bubble-asset-facts.json. This corrected the misleading old player_shadow name; current owner is player_auxiliary_effect, with the observed bubble sprite rather than an assertion of the effect's complete purpose. Historical shadow report/ledger remain cross-linked.

UV constant7db2c4 is bits3b808081 (rounded1/255), not1/256. Other exact sprite constants are color scale7dce6c, minimum depth7e9b40, zero7dcb2c and one7dcb10. No host reciprocal/trigonometry/projection substitutes are used.

## Implemented state and math

4030d0/77ec60 preserve packed-byte mutation, source selector bypass for special values0/1, exact constant-table mapping, live device read for each call, ordered state commands and ignored original HRESULTs. Missing device/table/backend boundaries are explicit; native backends without an HRESULT retain that receipt as omitted, not invented0. Native resource identities are never guest COM pointers.

Sprite geometry implements actual77e3c9..77e5c2 after required texture binding: signed16 dimensions/center, actual current PlayerCoordinates point transformation, positive-z clipping, extended division and intermediate source stores, scale/position summation, exactUV multiplication, source color FTOL/BGRA byte construction and UV flips. Sprite angle is not read by this source body. Unknown projection/color owners retain preceding locals. The vertex phase reproduces four triangle-strip vertices, FVF144/stride28 for source projected XY/Z/RHW/color/UV and FVF142/stride24 for worldXYZ/color/UV. Its unrounded projected depth is a required explicit input; absence does not become0,1/z or a host projection.

These are numeric/store/command-shape slices, not a completed77e390 wrapper. Texture-list/texture-index binding, queue ownership, intervening graphics states/transforms and actual renderer submission remain required owners. No geometry or GPU success is fabricated when those owners are unavailable. Native unmasked exception handling retains failure metadata; original hardware exception timing and every intermediate callee partial store remain unverified.

## Verification and limits

Blend unit tests cover6144 packed-byte/selector/mode combinations plus live-device mutation, missing owners and HRESULT/receipt omission. The actual4030d0/77ec60 instruction verifier covers6160 cases with zero packed-state/command/boundary mismatches. Only the external COM SetRenderState boundary is a declared test backend; this is command-production evidence, not Windows/GPU runtime evidence.

Sprite unit tests use the actual40-byte frame/constants asset,4100-byte trig table and64-byte unit matrix. Its original-instruction verifier executes239 fixtures×12 PC/RC profiles=2868 evaluations, including actual781670 and CRT_ftol without overlays. All local-value, boundary and CW comparisons match. Vertex bytes match except two positive/negative seeded signaling-depth cases per profile (24 total): Unicorn retains SNaN7f812345/ff812345 in stores where the native x87 load/store quiets them. Those discrepancies and2760 FP-status differences remain retained; strict emulator match isfalse. Depth is explicitly seeded external input in this slice test, not a fake D3DX provider.

Production reports are retained at production-integration-20261012/original-ninja-render-comparison.json and original-ninja-sprite-comparison.json with current library/source/blob digests. The sprite exit1 is retained, not converted into a false whole-body or hardware-equivalence pass.

Full depth78dbe0 calls generic D3DX project772615 via770295/389b7b0. Its matrix/vector subcalls are themselves CPU dispatched. Source Intel/SSE2 vector transform774150 uses RCPPS plus Newton refinement; an exact or host divide is not an equivalent replacement. The depth owner remains missing pending a supported arithmetic implementation/profile. Native Metal texture upload is a separately owned real GPU component; actual texture-list/cache/lifetime binding, sampler/blend/pixel parity, the full sprite queue/dispatcher and the playable game remain incomplete.

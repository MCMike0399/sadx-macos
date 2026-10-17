# GOAL 6.3: shared transform/lighting device slice

Base a0044d5. The old backend accepted only XYZRHW. At frame 1900 the old
frame list contained 132 unlit XYZ strips; buffer draw methods only incremented
counters. Recording those methods reveals normal-bearing meshes (FVF 0x152),
including lighting and generated texture coordinates. State must be copied at
the draw, not taken from the transforms left over at Present.

Implemented: an owned graphics command stream independent of HLE/guest memory,
a shared Metal device backend, and a guest resource adapter. Four draw APIs,
16/32-bit indices/base/minimum, per-draw matrices/viewport/material/lights,
ordered clears, perspective W, lighting, camera-normal/position UV generation,
texture transforms/projection, mipmaps, alpha test, blend, depth, cull and W fog.
Packed 16-bit textures retain normalized FP32 channel fractions before filtering.
Unknown states/unsupported modes are counted. Visible singular normal transforms
remain unsupported; exactly zero-area transition meshes are raster no-ops,
verified by both zero-area and visible-area tests. No epsilon was introduced.

Contract sources (Microsoft, not recovered game algorithms):
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/fixed-function-fvf-codes
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/mathematics-of-lighting
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/diffuse-lighting
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/automatically-generated-texture-coordinates
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/texture-coordinate-transformations
- https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-lightenable
The DirectX 8.1 SDK DrawIndexedPrimitiveUP contract specifies minimum indices
relative to vertex zero, owned-call access and clearing stream/index bindings.

Preflight: three new native programs pass. They exercise asymmetric transforms,
FVF layout/range failures, all four guest draw APIs, owned bytes/state snapshots,
GPU depth/cull/alpha/clip, interleaved clears, large vertex buffers, perspective
and projective sampling, generated UVs, lighting and FP32 channel preservation.
The GPU program uses synthetic vertices/colors only; no guest, EXE or assets.
Removing fragment texture-projection division makes the GPU fixture fail.

An offscreen driver reaches Station Square and passes the full R4 prefix with
no skipped draws. Frame 2000 has 443 draws / 439 textured and 75,008 colors;
its screenshot was inspected. Frame 1900 still has the game's black fade, so
frame 2000 is the stable world-image checkpoint. A 100-frame advance plus boundary
readback measured 20.38 s (4.91 FPS). This is not yet practice-ready performance.
The measured binary identity is retained in the small final evidence summary;
a later input-range validation guard and static ABI assertions change no valid
scene pixels. The full gate will exercise the final source version.

Replayed guest streams equal gate 165 exactly: intro 7228, controller 900 and
stage 2150 frames, including mode/stage transitions. These tests retain the old
digest algorithm and expectations. Full numeric hashes are in the final summary.
Source and journey-promotion judge changes are committed separately. No original
images, assets, captured vertices or one-off scripts are committed. Reset,
vertex fog, reflection-coordinate generation, active multi-stage textures,
DXT/paletted formats, unvisited scenes and Windows GPU parity remain unverified.

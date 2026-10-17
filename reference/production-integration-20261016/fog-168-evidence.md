# Shared fixed-function vertex fog (GOAL 6.3)

The shared device rejected every active FOGVERTEXMODE and RANGEFOGENABLE state.
This step implements the vertex-fog feature of the R4 renderer. Camera-space
absolute Z or Euclidean distance feeds linear, exponential or squared-exponential
fog before rasterization. XYZRHW vertices supply specular alpha instead. Missing
specular input supplies factor 1. Disabled fog and table fog ignore vertex parameters.
A separate prepared factor preserves texture-stage specular input and diffuse alpha.
Active unknown formulas, nonfinite distances/parameters, zero linear intervals and
out-of-contract density return explicit unsupported reasons.

Contract sources:
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/vertex-fog
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/fog-formulas
- https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-interpolation
- https://github.com/doitsujin/dxvk/blob/master/src/d3d9/shaders/d3d9_fixed_function_vert.vert
  (calculateFog, including absent-specular and NONE behavior).

New CPU and native Metal tests require no EXE or game assets. CPU cases cover
all formulas, range distance after world/view, negative Z, clamping, supplied and
absent factors, reversed intervals, invalid parameters and precedence. GPU readback
checks vertex interpolation, all formulas, range, diffuse alpha and table precedence.
For unequal vertex depths the exponential result is the average of vertex factors,
not an exponential of the interpolated depth. Pixel comparisons allow one 8-bit LSB.
Existing interpolation conventions and the table-W implementation remain unchanged.

Deliberately disabling vertex factor generation fails both new tests (rc 1).
Routing FOGVERTEXMODE to the fragment formula fails the new GPU test (rc 1).
Both changes were reverted and both tests passed again. No existing judge changed;
only two unit registrations are appended. Standalone game oracles are unnecessary
for this device-contract addition and are not rerun. Windows GPU bit parity, affine
Z table fog and arbitrary projection matrices remain unverified; broader renderer
coverage, performance, audio and native game journeys remain open.

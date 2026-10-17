# Correct the existing W-fog fixture before extending table fog

The `linear W fog` readback in tests/unit/metal_device_frame.mm supplies XYZRHW
vertices with z=0.5, RHW=1, a linear fog interval [0,2], but leaves projection at
identity. The expected half-red/half-blue pixel explicitly tests W=1.

Microsoft's Pixel Fog contract says the CURRENT projection applies even when
vertices are pretransformed. Fourth column [0,0,0,1] selects device-Z fog; a W-friendly
perspective column [0,0,1,0] selects eye-relative W. Therefore identity describes
Z=0.5 and implies a 0.75 fog factor under that interval, not the intended 0.5.

This separate judge correction sets projection[11]=1, projection[15]=0 for the
existing W test. Geometry and expected pixel remain unchanged. No tolerance,
comparison, case, draw or assertion is removed. The old renderer already draws
this corrected W fixture; subsequent source changes can implement affine Z
without changing the existing judge to accommodate their output.

Primary contract:
https://learn.microsoft.com/en-us/windows/win32/direct3d9/pixel-fog
https://learn.microsoft.com/en-us/windows/win32/direct3d9/projection-transform
D3D8 corroboration: Wine's upstream dlls/d3d8/tests/visual.c explicitly sets the
projection to identity for Z table fog and records Windows GPU results.
https://github.com/wine-mirror/wine/blob/master/dlls/d3d8/tests/visual.c

Metal Shading Language specification, fragment function inputs: position is
window-relative (x,y,z,1/w). The new path will use fragment Z or reconstructed
true W after rasterization, rather than applying fog to interpolated factors.
https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf

This fixture is synthetic and requires no EXE/assets. Windows hardware pixel-bit
parity remains unverified. Full gate follows the complete feature step.

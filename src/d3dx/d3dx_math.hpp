#pragma once
#include <optional>
#include "cpu/sse_math.hpp"
#include "cpu/x87_math.hpp"
#include "cpu/x87_transcendental.hpp"
#include <array>
#include <cstdint>
#include <functional>
namespace sadx {
// The D3DX8 math linked into sonic.exe, from exact instructions. Calls go through
// the 0x39-slot table at 389b778, which 77350c (on first use) copies from 389b860
// and then patches for the CPU: 773f67 always, then 7738e5 when 3DNow! is present,
// else 773700 (SSE2) or 7735f0 (SSE). Both SSE tables put 774310 in slot 3
// (MatrixMultiply, thunk 770804) and 774150 in slot 10 (Vec3TransformCoord, thunk
// 77020b). Slot 14 (Vec3Project, thunk 770295) keeps the default 772615 on every
// path. The reference profile assumes an SSE CPU without 3DNow! and no
// DisableD3DXPSGP registry value.
using D3dxMatrix=std::array<std::uint32_t,16>;  // float bits, _11.._44
using D3dxVector3=std::array<std::uint32_t,3>;  // float bits
struct D3dxViewport {std::uint32_t x=0,y=0,width=0,height=0,min_z=0,max_z=0;}; // D3DVIEWPORT8 (MinZ/MaxZ float bits)
// One RCPPS lane. The approximation is the CPU's own table (it differs between
// Intel and AMD generations); the reference machine's CPU was not captured.
using SseReciprocalEstimate=std::function<std::uint32_t(std::uint32_t bits,std::uint32_t mxcsr)>;
// 1/x by IEEE single division in MXCSR's rounding mode, raising no flags (the
// Unicorn/QEMU model of RCPPS). It is not any real CPU's estimate.
std::uint32_t sse_reciprocal_by_division(std::uint32_t bits,std::uint32_t mxcsr);
// 774310: out row i = ((a[i][0]*b0 + a[i][1]*b1) + a[i][2]*b2) + a[i][3]*b3, all
// loads before the stores (so out may alias a or b).
D3dxMatrix d3dx_matrix_multiply_774310(const D3dxMatrix& a,const D3dxMatrix& b,SseContext&);
// 774150: (x*r0 + y*r1) + (z*r2 + r3), then every lane times lane 3 of one
// Newton step 2r - (r*v)*r on RCPPS r. `aligned` selects the movaps path (the
// matrix address & 0xf == 0), which only changes operand order.
D3dxVector3 d3dx_vec3_transform_coord_774150(const D3dxVector3&,const D3dxMatrix&,bool aligned,SseContext&,const SseReciprocalEstimate&);
// 772615: the world/view/projection product (NULL arguments skipped; none gives the
// identity built on the stack), 774150 on it, then the x87 viewport mapping when a
// viewport is given. A matrix's `aligned` flag follows its source address; the
// stack product is 16-byte aligned.
struct D3dxMatrixArgument {D3dxMatrix matrix;bool aligned=false;};
D3dxVector3 d3dx_vec3_project_772615(const D3dxVector3&,const std::optional<D3dxViewport>&,
    const std::optional<D3dxMatrixArgument>& projection,const std::optional<D3dxMatrixArgument>& view,const std::optional<D3dxMatrixArgument>& world,
    SseContext&,X87Context&,const SseReciprocalEstimate&);
// 7757e0 (slot 18, MatrixInverse via thunk 770d10, both SSE tables), transcribed
// instruction by instruction (Intel's SSE cofactor inverse). COMISS(det, 0) equal or
// unordered returns NULL with nothing stored; otherwise the determinant goes to
// pDeterminant (when given) and every cofactor is scaled by one Newton step on RCPSS.
struct D3dxInverseResult {std::optional<D3dxMatrix> matrix;std::optional<std::uint32_t> determinant;};
D3dxInverseResult d3dx_matrix_inverse_7757e0(const D3dxMatrix&,SseContext&,const SseReciprocalEstimate&);
// 7716df (stdcall, not dispatched): x87 throughout. s, c = FSINCOS(fovy * 0.5) (each
// stored to a float), cot = c / s; _11 = cot / aspect, _22 = cot, _33 = far / (far -
// near) (FST: the register keeps it unrounded), _34 = 1, _43 = -(_33 * near), the
// rest +0. nullopt when |fovy * 0.5| >= 2^63 (FSINCOS leaves ST(0) and sets C2; the
// source then stores from an empty register).
std::optional<D3dxMatrix> d3dx_perspective_7716df(float fovy,float aspect,float near_plane,float far_plane,X87Context&);
} // namespace sadx

#pragma once
#include "guest/x86_guest.hpp"
#include "loader/executable_image.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>
namespace sadx {
// The override table (ADR-0001, M1 4d): native owners bound to guest addresses. Each entry's bridge loads the
// owner's state from guest memory (the `*_at_<hex>` globals and the stack arguments), runs the native port with
// the guest's x87 control word, writes the owner's globals back and returns the callee's pop count. A port
// that cannot proceed (a failure result) stops the guest with the port's boundary instead of returning.
// `writes` are the guest ranges the original may store to: override-equivalence compares exactly those between
// the original and the override (these functions return nothing: EAX is leftover, e.g. an FNSTSW result).
struct OverrideEntry {
    std::uint32_t address=0;std::string name;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> writes; // (address, bytes)
    X86Guest::Override bridge;
    bool returns_eax=false;
    // (stack argument index, bytes): output ranges addressed by an argument (the D3DX pOut / pDeterminant).
    std::vector<std::pair<unsigned,std::uint32_t>> pointer_writes={};
    // ECX, EDX and the x87 condition bits C3/C2/C1/C0 the source leaves are part of the comparison too.
    bool returns_scratch=false;
    // Returns a value in ST(0): ST(0), TOP and the tag word are part of the comparison.
    bool returns_x87=false;
};
// The frame-loop owners ported so far whose state is guest globals: 7899a0 (frame multiplier), 7815e0
// (projection range clamp), 7815c0 (sprite projection scale), 781570 (camera block). `image` supplies the
// .rdata/.data constants the ports take as inputs (the 88af78 sine table, the 7dcb0c/7dce50 floats).
// 402f40 (cdecl, colour pointer): the render-queue colour 3ab9864, clamped to [-1, 1], then 77ff90's copy to 3d0f7f0.
// 78dbe0 (stdcall, w): the projected depth through 772615, returned in ST(0) (SSE dispatch table only).
std::vector<OverrideEntry> frame_loop_overrides(const ExecutableImage& image);
// Startup / registration owners whose state is plain guest globals: 789930 (exit-callback slots 3d084b0,
// returns 1 or 0 in EAX), 40a8a0 (menu block 900e88 -> 9008b4), 77ffc0 (callback list 3ce712c / 3d0fa40) and
// 7959c0 (frame hooks 3d08af0). `returns_eax`: EAX is part of the comparison (the others leave it as scratch).
std::vector<OverrideEntry> startup_overrides();
// The D3DX math leaves (stdcall, EAX = pOut): 774310 MatrixMultiply, 774150 Vec3TransformCoord, 7757e0
// MatrixInverse (EAX NULL and nothing stored when singular), 7716df PerspectiveFovLH and 772615 Vec3Project
// (its nested multiply/transform calls go through dispatch slots 3/10: the override binds only the SSE
// table's 774310/774150 there and stops on any other slot value, e.g. 77350c's table not yet built). SSE under the guest's
// MXCSR rounding mode; RCPSS/RCPPS by the IEEE-division estimate model (sse_reciprocal_by_division, an assumption);
// MXCSR exception flags are not accumulated (the guest CPU, Unicorn, raises none either). `writes` names fixed
// ranges for the test; these entries write through their pOut argument instead (`pointer_writes`).
std::vector<OverrideEntry> math_overrides();
// Every bound owner (frame-loop + startup + math).
std::vector<OverrideEntry> all_overrides(const ExecutableImage& image);
bool install_overrides(X86Guest&,const std::vector<OverrideEntry>&,std::string* error);
} // namespace sadx

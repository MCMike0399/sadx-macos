#pragma once
#include "guest/x86_guest.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace sadx {
// FPU shims (M1 4e): at every confirmed FPATAN / FSINCOS / FPTAN / FSIN / FCOS / F2XM1 / FYL2X / FYL2XP1 site of
// sonic.exe (reference/inventory/fpu-sites.json, tools/analysis/fpu_sites.py) the guest's instruction is replaced
// by the native Bochs model (src/cpu/x87_transcendental) under the guest's control word: results, stack effect,
// C2 for |x| >= 2^63 (the trigonometric ones) and the exception flags OR-ed into the status word. Unicorn computes
// these with host doubles; the shims make the guest agree with the native ports.
// RCPSS / RCPPS / RSQRTSS / RSQRTPS are computed with the project's estimate model (src/math/d3dx_math:
// IEEE single division, and SQRTSS then division, in MXCSR's rounding mode, no flags). That is an assumption:
// real CPUs return an implementation-specific ~12-bit estimate, and which one the reference machine has is
// unknown. A memory-operand RCPPS/RSQRTPS whose address is not 16-byte aligned stops the guest by name (#GP is
// not modelled). Any other kind is counted when executed (`unshimmed`). A non-canonical x87 operand is left to
// Unicorn and counted in `fallback`.
// dst_xmm/src_xmm (-1: memory)/base/index/scale/disp/length: the SSE operands, recorded by fpu_sites.py.
struct FpuSite {std::uint32_t address=0;std::string instruction;int dst_xmm=-1,src_xmm=-1;std::string base,index;std::uint32_t scale=1;std::int32_t disp=0;std::uint32_t length=0;};
struct FpuShimStats {std::map<std::string,std::uint64_t> shimmed,unshimmed,fallback;}; // fallback: a non-canonical operand left to Unicorn
std::vector<FpuSite> load_fpu_sites(const std::string& fpu_sites_json,bool confirmed_only=true);
bool install_fpu_shims(X86Guest&,const std::vector<FpuSite>&,FpuShimStats&,std::string* error);
} // namespace sadx

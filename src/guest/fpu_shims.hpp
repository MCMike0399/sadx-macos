#pragma once
#include "guest/x86_guest.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace sadx {
// FPU shims (M1 4e): at every confirmed FPATAN / FSINCOS / FPTAN / FSIN / FCOS site of sonic.exe
// (reference/inventory/fpu-sites.json, tools/analysis/fpu_sites.py) the guest's instruction is replaced by the
// native Bochs P5/P6 model (src/cpu/x87_transcendental) under the guest's control word: results, stack effect,
// C2 for |x| >= 2^63 and the exception flags OR-ed into the status word. Unicorn computes these instructions
// with host doubles; the shims make the guest agree with the native ports. F2XM1 / FYL2X(P1) have no native
// model yet and RCPSS/RCPPS/RSQRTSS/RSQRTPS keep Unicorn's IEEE-division results (the project's RCP model):
// those sites are counted when executed (`unshimmed`), not changed.
struct FpuSite {std::uint32_t address=0;std::string instruction;};
struct FpuShimStats {std::map<std::string,std::uint64_t> shimmed,unshimmed,fallback;}; // fallback: a non-canonical operand left to Unicorn
std::vector<FpuSite> load_fpu_sites(const std::string& fpu_sites_json,bool confirmed_only=true);
bool install_fpu_shims(X86Guest&,const std::vector<FpuSite>&,FpuShimStats&,std::string* error);
} // namespace sadx

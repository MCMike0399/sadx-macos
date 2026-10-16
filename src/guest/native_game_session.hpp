#pragma once
#include "guest/native_game_reference.hpp"
#include <memory>
#include <string>
namespace sadx {
// The boot configuration both front ends (sadx-boot CLI, sadx-app window) build the same way:
// the game image, the Input mod DLL (its .rdata constants), the model DLLs next to the executable,
// .\sonicDX.ini, and the pinned profile's pad settings. The clock is the host's (mac_clock).
struct BootLoadResult {
    std::optional<NativeGameConfig> config;
    std::string error;     // which file could not be read or parsed
    explicit operator bool()const noexcept{return config.has_value();}
};
BootLoadResult load_boot_config(const std::string& exe_path,const std::string& input_mod_path,std::uint32_t frames,OwnerPolicy policy,const SpeedrunLimiterClock& clock);
// The report text sadx-boot prints (policy line, frames, stop site, D3D8 counts, assumptions, unported owners).
std::string describe_report(const NativeGameReport& report,const D3D8StateOwner& d3d8,OwnerPolicy policy);
} // namespace sadx

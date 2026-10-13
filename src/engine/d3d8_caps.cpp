#include "engine/d3d8_caps.hpp"
#include <cstddef>
namespace sadx {
D3D8CapsBytes assumed_d3d8_caps() noexcept {
    D3D8CapsBytes caps{};
    const auto put=[&](std::size_t at,std::uint32_t value){
        for(unsigned k=0;k<4;++k)caps[at+k]=static_cast<std::uint8_t>(value>>(8*k));
    };
    put(0x00,1);        // D3DDEVTYPE_HAL
    put(0x24,0x100000); // D3DPRASTERCAPS_WFOG (table fog emulated by the backend)
    // PERSPECTIVE|ALPHA|PROJECTED|CUBEMAP|VOLUMEMAP|MIPMAP|MIPVOLUMEMAP|MIPCUBEMAP.
    // 78cfa0 clears the mipmap-format flags without MIPMAP; 7914b0/78db80 read CUBEMAP.
    put(0x3c,0x1ec05);
    return caps;
}
const char* d3d8_caps_assumption() noexcept {
    return "D3D8 GetDeviceCaps: a HAL device with TextureCaps PERSPECTIVE|ALPHA|PROJECTED|CUBEMAP|VOLUMEMAP|MIPMAP|MIPVOLUMEMAP|MIPCUBEMAP and RasterCaps WFOG, every other cap 0 (the reference GPU's D3DCAPS8 were not captured)";
}
} // namespace sadx

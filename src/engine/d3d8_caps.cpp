#include "engine/d3d8_caps.hpp"

#include <cstddef>

namespace sadx {

D3D8CapsBytes assumed_d3d8_caps() noexcept {
    constexpr std::size_t kRasterCapsOffset = 36;
    constexpr std::size_t kTextureCapsOffset = 60;
    constexpr std::uint32_t kHardwareDeviceType = 1; // D3DDEVTYPE_HAL
    constexpr std::uint32_t kRasterCapsWFog = 0x100000;
    // PERSPECTIVE | ALPHA | PROJECTED | CUBEMAP | VOLUMEMAP |
    // MIPMAP | MIPVOLUMEMAP | MIPCUBEMAP.
    constexpr std::uint32_t kAssumedTextureCaps = 0x1ec05;

    // Every unspecified capability remains zero under the existing assumption.
    D3D8CapsBytes caps{};
    // NOTSA: serialize the three established D3DCAPS8 words as little endian.
    const auto write_word = [&](std::size_t byte_offset, std::uint32_t value) {
        for (std::size_t byte = 0; byte < sizeof(value); ++byte)
            caps[byte_offset + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
    };
    write_word(0, kHardwareDeviceType);
    write_word(kRasterCapsOffset, kRasterCapsWFog);
    write_word(kTextureCapsOffset, kAssumedTextureCaps);
    return caps;
}

const char* d3d8_caps_assumption() noexcept {
    return "D3D8 GetDeviceCaps: a HAL device with TextureCaps "
           "PERSPECTIVE|ALPHA|PROJECTED|CUBEMAP|VOLUMEMAP|MIPMAP|MIPVOLUMEMAP|MIPCUBEMAP "
           "and RasterCaps WFOG, every other cap 0 "
           "(the reference GPU's D3DCAPS8 were not captured)";
}

} // namespace sadx

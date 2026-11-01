#pragma once

#include <cstdint>

namespace sadx::render_detail {

// NOTSA: named write roles let adapters retain their own device/failure diagnostics.
enum class BlendWrite { Source, Destination, Mapped };

// NOTSA: the shared packed-byte step of njColorBlendingMode, borrowing its real owner.
inline void store_packed_blend(std::uint32_t selector, std::uint32_t mode, std::uint8_t &packed) {
    constexpr std::uint32_t kSourceMask = 0x0F;
    constexpr std::uint32_t kDestinationMask = 0xF0;
    constexpr std::uint32_t kSourceColor = 0;
    constexpr std::uint32_t kDestinationColor = 1;
    if (selector == kSourceColor)
        packed = static_cast<std::uint8_t>((packed & kDestinationMask) | (mode & kSourceMask));
    else if (selector == kDestinationColor)
        packed = static_cast<std::uint8_t>((packed & kSourceMask) | ((mode & kSourceMask) << 4));
}

// NOTSA: one njColorBlendingMode_ algorithm for real native and reference adapters.
// Lookups remain lazy: mode, current device, then target. Each special write
// reacquires the device because its predecessor can replace or invalidate it.
template <class ReadMode, class ReadTarget, class AcquireDevice, class Issue>
void apply_device_blend(std::uint32_t selector, std::uint32_t mode, ReadMode read_mode,
                        ReadTarget read_target, AcquireDevice acquire_device, Issue issue) {
    constexpr std::uint32_t kSourceBlend = 19;
    constexpr std::uint32_t kDestinationBlend = 20;
    // Ninja's two paired-alpha modes use the D3DBLEND source-alpha factors.
    constexpr std::uint32_t kBothInverseAlpha = 0;
    constexpr std::uint32_t kBothSourceAlpha = 1;
    constexpr std::uint32_t kSourceAlpha = 5;
    constexpr std::uint32_t kInverseSourceAlpha = 6;
    if (mode <= kBothSourceAlpha) {
        const bool inverse_source = mode == kBothInverseAlpha;
        auto device = acquire_device(BlendWrite::Source);
        if (!issue(device, kSourceBlend, inverse_source ? kInverseSourceAlpha : kSourceAlpha,
                   BlendWrite::Source))
            return;
        device = acquire_device(BlendWrite::Destination);
        issue(device, kDestinationBlend, inverse_source ? kSourceAlpha : kInverseSourceAlpha,
              BlendWrite::Destination);
        return;
    }
    const auto mapped = read_mode(mode);
    auto device = acquire_device(BlendWrite::Mapped);
    const auto target = read_target(selector);
    issue(device, target, mapped, BlendWrite::Mapped);
}

} // namespace sadx::render_detail

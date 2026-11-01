#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sadx {
// Metadata from a bounded MPEG-1 system-stream walk, used by the reference
// DirectShow HLE. Only video stream 0xE0 contributes timestamps and sequence
// headers. This does not decode pictures or count frames.
struct MpegTiming {
    std::uint64_t first_pts = 0; // Minimum observed PTS, in 90 kHz ticks (not unwrapped).
    std::uint64_t last_pts = 0;  // Maximum observed PTS, in 90 kHz ticks.
    std::uint32_t frame_rate_numerator = 0;
    std::uint32_t frame_rate_denominator = 1;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t duration_100ns = 0;
    std::size_t video_packets_with_pts = 0;
};

// Duration is the PTS span plus one frame period, each truncated to 100 ns.
// HleDshow reports this as an assumption: the Windows splitter rule is uncaptured.
// Returns nullopt and writes error for unsupported/malformed or missing metadata.
// Success leaves error unchanged; a null error pointer is allowed.
std::optional<MpegTiming> mpeg1_timing(const std::vector<std::uint8_t> &stream,
                                       std::string *error = nullptr);
} // namespace sadx

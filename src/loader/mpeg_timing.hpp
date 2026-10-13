#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace sadx {
// Timing facts of an MPEG-1 system stream (the game's system\*.MPG movies), from a walk over its packs and
// packets: the video stream's (0xE0) first and last presentation time stamps, its frame period (the first
// sequence header's frame_rate_code) and picture size. `duration_100ns` = last PTS - first PTS + one frame
// period (the time from the first picture's presentation to the end of the last one). nullopt with `error`
// on a stream this walk does not understand (MPEG-2 packs, a broken packet length, no video PTS).
struct MpegTiming {
    std::uint64_t first_pts=0,last_pts=0;           // 90 kHz
    std::uint32_t frame_rate_numerator=0,frame_rate_denominator=1;
    std::uint32_t width=0,height=0;
    std::uint64_t duration_100ns=0;
    std::size_t video_packets_with_pts=0;
};
std::optional<MpegTiming> mpeg1_timing(const std::vector<std::uint8_t>& stream,std::string* error=nullptr);
} // namespace sadx

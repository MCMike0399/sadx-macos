#include "loader/mpeg_timing.hpp"
#include <algorithm>

namespace sadx {
namespace {
constexpr std::uint8_t kEndCode = 0xb9;
constexpr std::uint8_t kPackStartCode = 0xba;
constexpr std::uint8_t kSystemHeaderCode = 0xbb;
constexpr std::uint8_t kVideoStreamId = 0xe0;
constexpr std::uint8_t kSequenceHeaderCode = 0xb3;
constexpr std::uint8_t kStuffingByte = 0xff;
constexpr std::uint8_t kHighNibbleMask = 0xf0;
constexpr std::uint8_t kLowNibbleMask = 0x0f;
constexpr std::uint8_t kMpeg1PackMarker = 0x20;
constexpr std::uint8_t kStdBufferMask = 0xc0;
constexpr std::uint8_t kStdBufferPrefix = 0x40;
constexpr std::uint8_t kPtsPrefixMask = 0xe0;
constexpr std::uint8_t kPtsPrefix = 0x20;
constexpr std::uint8_t kPtsDtsPrefix = 0x30;
constexpr std::size_t kPackHeaderSize = 12;
constexpr std::size_t kPacketHeaderSize = 6;
constexpr std::size_t kSequenceHeaderSize = 8;
constexpr std::size_t kTimestampSize = 5;
constexpr std::uint64_t kHundredNanosecondsPerSecond = 10000000;

struct FrameRate {
    std::uint32_t numerator;
    std::uint32_t denominator;
};
// frame_rate_code 1..8 (ISO/IEC 11172-2 table 2-D.4).
// These are format fractions; index zero is not a usable rate.
const FrameRate kFrameRates[] = {{0, 1},  {24000, 1001}, {24, 1},       {25, 1}, {30000, 1001},
                                 {30, 1}, {50, 1},       {60000, 1001}, {60, 1}};

std::uint64_t time_stamp(const std::uint8_t *bytes) {
    // Extract 33 data bits, skipping the prefix nibble and interleaved marker bits.
    // The existing metadata walk does not validate marker bits.
    return (std::uint64_t((bytes[0] >> 1) & 7) << 30) | (std::uint64_t(bytes[1]) << 22) |
           (std::uint64_t(bytes[2] >> 1) << 15) | (std::uint64_t(bytes[3]) << 7) | (bytes[4] >> 1);
}

// NOTSA: one exit shape keeps the existing diagnostic text and optional result together.
std::optional<MpegTiming> timing_failure(std::string *error, const std::string &why) {
    if (error)
        *error = why;
    return std::nullopt;
}

// NOTSA: named step of the packet walk; a sequence header cannot span packets.
bool read_sequence_header(const std::vector<std::uint8_t> &stream, std::size_t payload,
                          std::size_t packet_end, MpegTiming &timing) {
    for (auto at = payload; at + kSequenceHeaderSize <= packet_end; ++at) {
        if (stream[at] != 0 || stream[at + 1] != 0 || stream[at + 2] != 1 ||
            stream[at + 3] != kSequenceHeaderCode)
            continue;
        const auto rate_code = stream[at + 7] & kLowNibbleMask;
        // An invalid first header ends this packet's search, but a later packet
        // may still provide a usable header. Do not search past this one.
        if (rate_code < 1 || rate_code > 8)
            break;
        timing.width = (std::uint32_t(stream[at + 4]) << 4) | (stream[at + 5] >> 4);
        timing.height = (std::uint32_t(stream[at + 5] & kLowNibbleMask) << 8) | stream[at + 6];
        timing.frame_rate_numerator = kFrameRates[rate_code].numerator;
        timing.frame_rate_denominator = kFrameRates[rate_code].denominator;
        return true;
    }
    return false;
}

// NOTSA: named step that retains the permissive MPEG-1 packet-header walk.
std::size_t read_video_timestamp(const std::vector<std::uint8_t> &stream, std::size_t header,
                                 std::size_t packet_end, MpegTiming &timing) {
    while (header < packet_end && stream[header] == kStuffingByte)
        ++header;
    if (header < packet_end && (stream[header] & kStdBufferMask) == kStdBufferPrefix)
        header += 2;

    auto payload = header + 1;
    if (header + kTimestampSize <= packet_end && (stream[header] & kPtsPrefixMask) == kPtsPrefix) {
        const auto pts = time_stamp(&stream[header]);
        if (timing.video_packets_with_pts++ == 0) {
            timing.first_pts = pts;
            timing.last_pts = pts;
        } else {
            timing.first_pts = std::min(timing.first_pts, pts);
            timing.last_pts = std::max(timing.last_pts, pts);
        }
        // A PTS+DTS prefix advances past both fields even if DTS is truncated.
        payload = header + ((stream[header] & kHighNibbleMask) == kPtsDtsPrefix ? 2 * kTimestampSize
                                                                                : kTimestampSize);
    }
    return payload;
}
} // namespace

std::optional<MpegTiming> mpeg1_timing(const std::vector<std::uint8_t> &stream,
                                       std::string *error) {
    MpegTiming timing;
    bool have_rate = false;
    std::size_t at = 0;
    while (at + 4 <= stream.size()) {
        if (stream[at] != 0 || stream[at + 1] != 0 || stream[at + 2] != 1)
            return timing_failure(error, "no start code at byte " + std::to_string(at));
        const auto code = stream[at + 3];
        if (code == kEndCode)
            break;
        if (code == kPackStartCode) {
            if (at + kPackHeaderSize > stream.size() ||
                (stream[at + 4] & kHighNibbleMask) != kMpeg1PackMarker)
                return timing_failure(error,
                                      "not an MPEG-1 pack header at byte " + std::to_string(at));
            at += kPackHeaderSize;
            continue;
        }
        if (code < kSystemHeaderCode)
            return timing_failure(error, "unexpected start code at byte " + std::to_string(at));
        if (at + kPacketHeaderSize > stream.size())
            return timing_failure(error, "truncated packet at byte " + std::to_string(at));
        const std::size_t length = (std::size_t(stream[at + 4]) << 8) | stream[at + 5];
        const auto packet_end = at + kPacketHeaderSize + length;
        if (packet_end > stream.size())
            return timing_failure(error, "packet past the end at byte " + std::to_string(at));
        if (code == kVideoStreamId) {
            const auto payload =
                read_video_timestamp(stream, at + kPacketHeaderSize, packet_end, timing);
            if (!have_rate)
                have_rate = read_sequence_header(stream, payload, packet_end, timing);
        }
        at = packet_end;
    }
    if (timing.video_packets_with_pts == 0)
        return timing_failure(error, "no video packet carries a PTS");
    if (!have_rate)
        return timing_failure(error, "no video sequence header in the packet payloads");

    // Convert 90 kHz ticks to 100 ns (x 1000 / 9), then add a separately
    // truncated frame period. No timestamp unwrapping or picture count is used.
    timing.duration_100ns =
        (timing.last_pts - timing.first_pts) * 1000 / 9 +
        kHundredNanosecondsPerSecond * timing.frame_rate_denominator / timing.frame_rate_numerator;
    return timing;
}
} // namespace sadx

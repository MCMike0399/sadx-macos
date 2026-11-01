// Freeze full synthetic byte/result identities against the pre-refactor source.
// --capture emits every input, optional result, field and error; default checks
// the frozen stream digest and size. No decoder or Windows parity claim.
#include "loader/mpeg_timing.hpp"
#include "mpeg_timing_stream.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace mpeg_fixture;
std::string observe(const Bytes &bytes) {
    std::ostringstream out;
    for (const auto byte : bytes)
        out << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
    out << std::dec << '|';
    std::string error = "unchanged sentinel";
    const auto result = sadx::mpeg1_timing(bytes, &error);
    const auto no_error = sadx::mpeg1_timing(bytes);
    if (bool(result) != bool(no_error))
        throw std::runtime_error("null error pointer changes result");
    out << bool(result) << '|' << error;
    if (result) {
        const auto fields = [&](const sadx::MpegTiming &t) {
            out << '|' << t.first_pts << ',' << t.last_pts << ',' << t.frame_rate_numerator << ','
                << t.frame_rate_denominator << ',' << t.width << ',' << t.height << ','
                << t.duration_100ns << ',' << t.video_packets_with_pts;
        };
        fields(*result);
        fields(*no_error);
    }
    out << '\n';
    return out.str();
}
std::vector<Bytes> corpus() {
    std::vector<Bytes> cases;
    const auto base = stream();
    // Every truncation and every replacement byte at every byte offset.
    for (std::size_t size = 0; size <= base.size(); ++size)
        cases.emplace_back(base.begin(), base.begin() + size);
    for (std::size_t at = 0; at < base.size(); ++at) {
        for (unsigned value = 0; value < 256; ++value) {
            auto bytes = base;
            bytes[at] = value;
            cases.push_back(bytes);
        }
    }
    // Exact fractions, repeated/descending/wrapped PTS, zero/maximum dimensions.
    for (unsigned rate = 0; rate < 16; ++rate) {
        for (const auto ticks : {0ULL, 1ULL, 8ULL, 9ULL, 90000ULL, 0x1ffffffffULL}) {
            cases.push_back(stream(rate, ticks, 0));
            cases.push_back(stream(rate, ticks, ticks));
            Bytes bytes;
            packet(bytes, 0xe0, video(rate, ticks, 2, 0, 4095));
            cases.push_back(bytes);
        }
    }
    // All stream IDs; end/pack/packet control edges with short payloads.
    for (unsigned id = 0; id < 256; ++id) {
        Bytes bytes;
        packet(bytes, id, video(1, 0x1ffffffffULL));
        cases.push_back(bytes);
        for (unsigned size = 0; size <= 12; ++size) {
            Bytes short_packet;
            packet(short_packet, id, Bytes(size, 0xff));
            cases.push_back(short_packet);
        }
    }
    // Invalid first sequence stops this packet's search, retries next packet.
    for (unsigned rate = 0; rate < 16; ++rate) {
        Bytes bytes;
        auto body = video(rate, 12);
        const auto next = sequence(3, 4095, 0);
        body.insert(body.end(), next.begin(), next.end());
        packet(bytes, 0xe0, body);
        cases.push_back(bytes);
        packet(bytes, 0xe0, video(7, 5));
        cases.push_back(bytes);
    }
    // Header without PTS, truncated DTS, no pictures, STD truncation, tails.
    for (unsigned prefix = 0; prefix < 256; ++prefix) {
        Bytes bytes;
        auto body = sequence(5);
        body.insert(body.begin(), prefix);
        packet(bytes, 0xe0, body);
        cases.push_back(bytes);
        auto second = video(2, 17);
        second.resize(5);
        packet(bytes, 0xe0, second);
        cases.push_back(bytes);
    }
    for (unsigned length = 0; length <= 12; ++length) {
        auto bytes = base;
        bytes.insert(bytes.end(), length, 0x55);
        cases.push_back(bytes);
        bytes.resize(base.size() - 4);
        bytes.insert(bytes.end(), length, 0x55);
        cases.push_back(bytes);
    }
    return cases;
}
} // namespace

int main(int argc, char **argv) {
    try {
        const bool capture = argc == 2 && std::string(argv[1]) == "--capture";
        if (argc != 1 && !capture)
            throw std::runtime_error("usage: mpeg-timing-contract-test [--capture]");
        std::uint64_t hash = 14695981039346656037ULL;
        std::size_t size = 0;
        const auto cases = corpus();
        // Public aggregate defaults are part of the API, including no frame count.
        const sadx::MpegTiming defaults;
        if ((defaults.first_pts | defaults.last_pts | defaults.frame_rate_numerator |
             defaults.width | defaults.height | defaults.duration_100ns |
             defaults.video_packets_with_pts) != 0 ||
            defaults.frame_rate_denominator != 1)
            throw std::runtime_error("MpegTiming defaults");
        for (const auto &bytes : cases) {
            const auto row = observe(bytes);
            size += row.size();
            for (const unsigned char byte : row) {
                hash ^= byte;
                hash *= 1099511628211ULL;
            }
            if (capture)
                std::cout << row;
        }
        std::cerr << cases.size() << " cases, " << size << " bytes, FNV " << hash << '\n';
        // Filled only from the fresh, unchanged source before any rewrite.
        if (!capture && (size != 7774713 || hash != 5361094107316205512ULL))
            throw std::runtime_error("full MPEG timing identity changed");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

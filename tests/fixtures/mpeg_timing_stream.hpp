#pragma once

#include <cstdint>
#include <vector>

// Synthetic bytes for the existing metadata walker; no movie asset or decoder.
namespace mpeg_fixture {
using Bytes = std::vector<std::uint8_t>;

inline void timestamp(Bytes &bytes, std::uint8_t prefix, std::uint64_t ticks) {
    bytes.push_back((prefix << 4) | ((ticks >> 29) & 14) | 1);
    bytes.push_back(ticks >> 22);
    bytes.push_back(((ticks >> 14) & 254) | 1);
    bytes.push_back(ticks >> 7);
    bytes.push_back(((ticks << 1) & 254) | 1);
}

inline void packet(Bytes &bytes, std::uint8_t id, const Bytes &body) {
    bytes.insert(bytes.end(), {0, 0, 1, id, static_cast<std::uint8_t>(body.size() >> 8),
                               static_cast<std::uint8_t>(body.size())});
    bytes.insert(bytes.end(), body.begin(), body.end());
}

inline Bytes sequence(unsigned rate, unsigned width = 640, unsigned height = 480) {
    return {0,
            0,
            1,
            0xb3,
            static_cast<std::uint8_t>(width >> 4),
            static_cast<std::uint8_t>((width << 4) | (height >> 8)),
            static_cast<std::uint8_t>(height),
            static_cast<std::uint8_t>(0x10 | rate)};
}

inline Bytes video(unsigned rate, std::uint64_t ticks, unsigned prefix = 2, unsigned width = 640,
                   unsigned height = 480) {
    Bytes body;
    timestamp(body, prefix, ticks);
    if (prefix == 3)
        timestamp(body, 1, ticks - 3003);
    const auto header = sequence(rate, width, height);
    body.insert(body.end(), header.begin(), header.end());
    return body;
}

inline Bytes stream(unsigned rate = 4, std::uint64_t first = 37245, std::uint64_t last = 937245) {
    Bytes bytes = {0, 0, 1, 0xba, 0x21, 0, 1, 0, 1, 0x80, 0x43, 0xff};
    packet(bytes, 0xbb, {0x80, 0x43, 0xff, 4, 0xe1, 0xff});
    auto body = video(rate, first, 3);
    body.insert(body.begin(), {0xff, 0xff, 0x40, 0x20});
    packet(bytes, 0xe0, body);
    packet(bytes, 0xc0, video(8, first + 100));
    packet(bytes, 0xe0, video(8, last, 2, 1, 2));
    packet(bytes, 0xe0, {0x0f, 0});
    bytes.insert(bytes.end(), {0, 0, 1, 0xb9});
    return bytes;
}
} // namespace mpeg_fixture

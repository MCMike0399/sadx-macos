#include "game/game_view_native.hpp"
#include <algorithm>

extern "C" int sadx_camera_view_angle(const std::uint32_t *input, std::uint32_t *output) {
    const auto camera = static_cast<std::int8_t>(input[0]);
    const auto interpolate = static_cast<std::uint8_t>(input[1]);
    auto previous = static_cast<std::int32_t>(input[2]);
    auto last = static_cast<std::int32_t>(input[3]);
    sadx::screen::CameraAngleState state{camera, interpolate, previous, last};
    std::fill(output, output + 6, 0);
    struct Stop { int unused = 0; };
    try {
        sadx::screen::set_view_angle(static_cast<std::int32_t>(input[4]), state,
            [&](std::int32_t angle) {
                output[2] = static_cast<std::uint32_t>(angle);
                output[3] = static_cast<std::uint32_t>(previous);
                ++output[4];
                if (input[5])
                    previous = static_cast<std::int32_t>(input[6]);
                if (input[7])
                    throw Stop{};
            });
        output[5] = 1;
    } catch (Stop) {}
    output[0] = static_cast<std::uint32_t>(previous);
    output[1] = static_cast<std::uint32_t>(last);
    return 1;
}

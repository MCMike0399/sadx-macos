#include "engine/d3d8_caps.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

extern "C" std::size_t sadx_native_d3d8_caps(std::uint8_t* output, std::size_t capacity) {
    const auto caps = sadx::assumed_d3d8_caps();
    if (!output || capacity < caps.size())
        return 0;
    std::copy(caps.begin(), caps.end(), output);
    return caps.size();
}

extern "C" const char* sadx_native_d3d8_caps_assumption() {
    return sadx::d3d8_caps_assumption();
}

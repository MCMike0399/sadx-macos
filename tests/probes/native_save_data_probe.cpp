#include "game/save_data.hpp"
#include <algorithm>
#include <cstdint>

extern "C" std::uint16_t sadx_native_save_checksum(const std::uint8_t *input) {
    sadx::save::EncodedData bytes{};
    std::copy_n(input, bytes.size(), bytes.begin());
    return sadx::save::checksum(bytes);
}

extern "C" std::int16_t sadx_native_save_initialize(std::uint8_t *output,
                                                   std::int16_t initial_rings) {
    sadx::save::Data data{};
    data.lives.fill(255);
    data.event_flags.fill(255);
    data.total_time = 1234567;
    sadx::save::initialize(data, initial_rings);
    const auto encoded = sadx::save::encode(data);
    std::copy(encoded.begin(), encoded.end(), output);
    return initial_rings;
}

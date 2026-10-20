#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace sadx::save {
struct ClearTime {
    std::uint8_t minutes{}, seconds{}, frames{};
};
struct CircuitRecords {
    std::array<ClearTime, 3> times{};
    std::array<ClearTime, 2> laps{};
};
struct Sequence {
    std::int8_t time{}, padding{};
    std::int16_t number{}, section{}, next_section{};
    std::uint16_t stage{}, destination{};
};
struct FishRecord {
    std::int16_t weight{}, type{};
};
// SAVE_DATA. Native values are serialized explicitly; host padding is never written to disk.
struct Data {
    std::uint32_t checksum{}, total_time{};
    std::array<std::int32_t, 32> best_scores{};
    std::array<ClearTime, 28> best_times{};
    std::array<std::array<std::int16_t, 3>, 4> best_weights{};
    std::array<FishRecord, 4> biggest_fish{};
    std::array<std::int16_t, 32> best_rings{};
    std::array<std::array<std::int32_t, 3>, 2> sky_chase{}, sky_chase_two{};
    std::array<std::array<std::int32_t, 3>, 2> snow_board{}, sand_board{};
    std::array<std::int32_t, 3> hedgehog_hammer{};
    std::array<CircuitRecords, 6> circuit{};
    std::array<std::array<ClearTime, 3>, 6> bosses{};
    std::array<std::uint8_t, 17> emblems{};
    std::uint8_t options{};
    std::array<std::uint8_t, 7> lives{};
    std::uint8_t last_character{}, vibration{}, reserved{};
    std::uint16_t last_stage{}, last_action{};
    std::array<std::uint8_t, 64> sequence_variables{}, event_flags{};
    std::array<std::uint8_t, 8> playable_scene_flags{};
    std::array<Sequence, 8> sequences{};
    std::array<std::array<std::uint8_t, 43>, 8> completed_stages{};
    std::array<std::uint8_t, 60> missions{};
    std::uint32_t total_rings{};
    std::array<std::int32_t, 10> metal_scores{};
    std::array<ClearTime, 10> metal_times{};
    std::array<std::int16_t, 10> metal_rings{};
    std::array<std::uint8_t, 2> metal_alignment{};
    std::array<std::int32_t, 3> metal_snow_board{}, metal_sand_board{};
    CircuitRecords metal_circuit{};
    std::array<ClearTime, 3> metal_bosses{};
    std::uint32_t metal_emblems{};
};
using EncodedData = std::array<std::uint8_t, 1392>;
// NOTSA: explicit little-endian codec for the game's persistent SAVE_DATA record.
EncodedData encode(const Data &);
// FUNCTION: SONIC 0x42cf90 createCRC
std::uint16_t checksum(const EncodedData &);
// FUNCTION: SONIC 0x42d3b0 InitSaveData
void initialize(Data &, std::int16_t &rings);
} // namespace sadx::save

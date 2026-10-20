#include "game/save_data.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace sadx::save {
namespace {
// NOTSA: serializes the file contract without relying on native record alignment.
class Writer {
  public:
    EncodedData bytes{};
    std::size_t position = 0;
    template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    void put(T value) {
        using Unsigned = std::make_unsigned_t<T>;
        const auto bits = static_cast<Unsigned>(value);
        for (std::size_t i = 0; i < sizeof(T); ++i)
            bytes.at(position++) = static_cast<std::uint8_t>(bits >> (i * 8));
    }
    template <class T, std::size_t N> void put(const std::array<T, N> &values) {
        for (const auto &value : values)
            put(value);
    }
    void put(const ClearTime &value) {
        put(value.minutes);
        put(value.seconds);
        put(value.frames);
    }
    void put(const FishRecord &value) {
        put(value.weight);
        put(value.type);
    }
    void put(const CircuitRecords &value) {
        put(value.times);
        put(value.laps);
    }
    void put(const Sequence &value) {
        put(value.time);
        put(value.padding);
        put(value.number);
        put(value.section);
        put(value.next_section);
        put(value.stage);
        put(value.destination);
    }
};

// FUNCTION: SONIC 0x4b5870 FUN_004b5870
void reset_snow_board(Data &data) {
    data.snow_board = {{{9000, 6000, 3000}, {9000, 6000, 3000}}};
}
// FUNCTION: SONIC 0x4b58e0 FUN_004b58e0
void reset_sand_board(Data &data) {
    data.sand_board = {{{10000, 6000, 3000}, {10000, 6000, 3000}}};
}
// NOTSA: shared literal record written by the two circuit reset functions.
CircuitRecords initial_circuit_record() {
    return {{{{10, 0, 0}, {15, 0, 0}, {20, 0, 0}}}, {{{4, 0, 0}, {7, 0, 0}}}};
}
// FUNCTION: SONIC 0x4b5950 FUN_004b5950
void reset_circuit(Data &data) {
    data.circuit.fill(initial_circuit_record());
}
// FUNCTION: SONIC 0x4b58b0 FUN_004b58b0
void reset_metal_snow_board(Data &data) {
    data.metal_snow_board = {9000, 6000, 3000};
}
// FUNCTION: SONIC 0x4b5920 FUN_004b5920
void reset_metal_sand_board(Data &data) {
    data.metal_sand_board = {10000, 6000, 3000};
}
// FUNCTION: SONIC 0x4b59f0 FUN_004b59f0
void reset_metal_circuit(Data &data) {
    data.metal_circuit = initial_circuit_record();
}
// FUNCTION: SONIC 0x42ced0 FUN_0042ced0
void reset_boss_records(Data &data) {
    data.bosses = {{{{{12, 0, 0}, {13, 30, 0}, {15, 0, 0}}},
                    {{{5, 0, 0}, {6, 30, 0}, {8, 0, 0}}},
                    {{{6, 30, 0}, {7, 30, 0}, {8, 30, 0}}},
                    {{{2, 30, 0}, {3, 0, 0}, {3, 30, 0}}},
                    {{{4, 0, 0}, {3, 30, 0}, {3, 0, 0}}},
                    {{{0, 15, 0}, {0, 30, 0}, {0, 45, 0}}}}};
}
// PATCH: INPUT +0x9240 -> 0x42d52d
void enable_default_vibration(Data &data) {
    data.vibration = 1;
}
} // namespace

EncodedData encode(const Data &data) {
    Writer writer;
    writer.put(data.checksum);
    writer.put(data.total_time);
    writer.put(data.best_scores);
    writer.put(data.best_times);
    writer.put(data.best_weights);
    writer.put(data.biggest_fish);
    writer.put(data.best_rings);
    writer.put(data.sky_chase);
    writer.put(data.sky_chase_two);
    writer.put(data.snow_board);
    writer.put(data.sand_board);
    writer.put(data.hedgehog_hammer);
    writer.put(data.circuit);
    writer.put(data.bosses);
    writer.put(data.emblems);
    writer.put(data.options);
    writer.put(data.lives);
    writer.put(data.last_character);
    writer.put(data.vibration);
    writer.put(data.reserved);
    writer.put(data.last_stage);
    writer.put(data.last_action);
    writer.put(data.sequence_variables);
    writer.put(data.event_flags);
    writer.put(data.playable_scene_flags);
    writer.put(data.sequences);
    writer.put(data.completed_stages);
    writer.put(data.missions);
    writer.put(data.total_rings);
    writer.put(data.metal_scores);
    writer.put(data.metal_times);
    writer.put(data.metal_rings);
    writer.put(data.metal_alignment);
    writer.put(data.metal_snow_board);
    writer.put(data.metal_sand_board);
    writer.put(data.metal_circuit);
    writer.put(data.metal_bosses);
    writer.put(data.metal_emblems);
    if (writer.position != writer.bytes.size())
        throw std::logic_error("SAVE_DATA serialization size");
    return writer.bytes;
}

std::uint16_t checksum(const EncodedData &bytes) {
    std::uint16_t crc = 65535;
    for (std::size_t i = sizeof(std::uint32_t); i < bytes.size(); ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = static_cast<std::uint16_t>((crc >> 1) ^ ((crc & 1) ? 33800 : 0));
    }
    return static_cast<std::uint16_t>(~crc);
}

void initialize(Data &data, std::int16_t &rings) {
    // The original constructs a complete local record before publishing SaveData.
    Data initial{};
    std::fill_n(initial.best_times.begin(), 23, ClearTime{99, 59, 59});
    reset_snow_board(initial);
    reset_sand_board(initial);
    reset_circuit(initial);
    initial.sky_chase = {{{8000, 6000, 3000}, {8000, 6000, 3000}}};
    initial.sky_chase_two = {{{20000, 15000, 10000}, {20000, 15000, 10000}}};
    initial.hedgehog_hammer = {2000, 1500, 1000};
    reset_boss_records(initial);
    initial.lives.fill(4);
    initial.sequence_variables[8] |= 2;
    initial.sequences[0].stage = 6656;
    initial.last_stage = 100;
    // Pinned Input mod enables rumble in a new save before its checksum is computed.
    enable_default_vibration(initial);
    initial.metal_times.fill(ClearTime{99, 59, 59});
    reset_metal_snow_board(initial);
    reset_metal_sand_board(initial);
    reset_metal_circuit(initial);
    initial.metal_bosses = {{{12, 0, 0}, {13, 30, 0}, {15, 0, 0}}};
    initial.checksum = checksum(encode(initial));
    data = initial;
    rings = 0;
}
} // namespace sadx::save

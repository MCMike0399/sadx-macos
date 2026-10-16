#include "guest/native_game_reference.hpp"

namespace sadx {
namespace {
constexpr std::array<unsigned, 12> resource_offsets{36, 37, 38, 39, 40, 41, 42, 43, 48, 49, 50, 51};
// NOTSA: PDS_PERIPHERAL name, extend and info are reference-only resource facts.
bool resource_byte(unsigned byte) { return (byte >= 36 && byte < 44) || byte >= 48; }
// NOTSA: serialize only data fields into the reference record; retain its resources.
void store_input_values(GameLogicController &record, const input::ControllerInputValues &values) {
    record.write_u32(0, values.id);
    record.write_u32(4, values.support);
    record.write_u32(8, values.held);
    record.write_u32(12, values.not_held);
    record.write_u32(16, values.pressed);
    record.write_u32(20, values.released);
    record.write_u16(24, values.right_trigger);
    record.write_u16(26, values.left_trigger);
    record.write_u16(28, static_cast<std::uint16_t>(values.left_x));
    record.write_u16(30, static_cast<std::uint16_t>(values.left_y));
    record.write_u16(32, static_cast<std::uint16_t>(values.right_x));
    record.write_u16(34, static_cast<std::uint16_t>(values.right_y));
    record.write_u32(44, values.previous_held);
}
} // namespace

void NativeGame::initialize_input_defaults() {
    const auto *exe = config_.executable.get();
    const auto byte = [exe](std::uint32_t address) -> std::optional<std::uint8_t> {
        return exe ? exe->byte(address) : std::nullopt;
    };
    if (config_.input_defaults) {
        const auto &values = *config_.input_defaults;
        for (unsigned row = 0; row < 256; ++row) {
            for (unsigned lane = 0; lane < 3; ++lane)
                edges_.raw_at_3b0e3e2[row][lane] = values.edges.raw[row][lane];
            edges_.aggregate_at_3b0eaf0[row] = values.edges.accumulated[row];
        }
        edges_.special_at_3b0e3ba = values.edges.special;
        edges_.aggregate_special_at_3b0eae8 = values.edges.accumulated_special;
        for (unsigned i = 0; i < 2; ++i)
            edges_.bytes_at_3b0e3e0[i] = values.edges.clear_prefix[i];
        edges_.byte_at_3b0e3b9 = values.edges.cleared_neighbor;
        store_input_values(controller_template_, values.controller_template);
        for (unsigned i = 0; i < 4; ++i)
            store_input_values(*controllers_raw_[i], values.engine_raw[i]);
        for (unsigned i = 0; i < 8; ++i) {
            store_input_values(*input_raw_[i], values.mod_raw[i]);
            store_input_values(pads_[i].record, values.dream_pads[i]);
        }
    } else {
        for (unsigned row = 0; row < 256; ++row) {
            for (unsigned lane = 0; lane < 3; ++lane)
                edges_.raw_at_3b0e3e2[row][lane] = byte(0x3b0e3e2 + 3 * row + lane);
            edges_.aggregate_at_3b0eaf0[row] = byte(0x3b0eaf0 + row);
        }
        edges_.special_at_3b0e3ba = byte(0x3b0e3ba);
        edges_.aggregate_special_at_3b0eae8 = byte(0x3b0eae8);
        edges_.bytes_at_3b0e3e0 = {byte(0x3b0e3e0), byte(0x3b0e3e1)};
        edges_.byte_at_3b0e3b9 = byte(0x3b0e3b9);
    }
    // Data and resources have different owners. Native values never synthesize a
    // name, extension or info pointer, even when the pinned bytes happen to be zero.
    const auto load = [&](GameLogicController &record, const ExecutableImage *image,
                          std::uint32_t address) {
        for (unsigned i = 0; i < GameLogicController::byte_count; ++i)
            if (!config_.input_defaults || resource_byte(i))
                record.write_byte(i, image ? image->byte(address + i) : std::nullopt);
    };
    load(controller_template_, exe, 0x3b0eaa8);
    for (unsigned i = 0; i < 4; ++i)
        load(*controllers_raw_[i], exe, 0x3b0e9c8 + 52 * i);
    const auto *dll = config_.input_mod_dll.get();
    for (unsigned i = 0; i < 8; ++i) {
        load(*input_raw_[i], dll, 0x1000e748 + 52 * i);
        load(pads_[i].record, dll, 0x1000e950 + 96 * i);
    }
}

std::array<std::optional<std::uint8_t>, 1869> NativeGame::input_data_facts() const {
    std::array<std::optional<std::uint8_t>, 1869> facts{};
    unsigned next = 0;
    for (const auto &row : edges_.raw_at_3b0e3e2)
        for (const auto &value : row)
            facts[next++] = value;
    for (const auto &value : edges_.aggregate_at_3b0eaf0)
        facts[next++] = value;
    facts[next++] = edges_.special_at_3b0e3ba;
    facts[next++] = edges_.aggregate_special_at_3b0eae8;
    for (const auto &value : edges_.bytes_at_3b0e3e0)
        facts[next++] = value;
    facts[next++] = edges_.byte_at_3b0e3b9;
    const auto record = [&](const GameLogicController &source) {
        for (unsigned i = 0; i < GameLogicController::byte_count; ++i)
            if (!resource_byte(i))
                facts[next++] = source.read_byte(i);
    };
    record(controller_template_);
    for (const auto &source : controllers_raw_)
        record(*source);
    for (const auto &source : input_raw_)
        record(*source);
    for (const auto &source : pads_)
        record(source.record);
    return facts;
}

std::array<std::optional<std::uint8_t>, 252> NativeGame::input_resource_facts() const {
    std::array<std::optional<std::uint8_t>, 252> facts{};
    unsigned next = 0;
    const auto record = [&](const GameLogicController &source) {
        for (unsigned offset : resource_offsets)
            facts[next++] = source.read_byte(offset);
    };
    record(controller_template_);
    for (const auto &source : controllers_raw_)
        record(*source);
    for (const auto &source : input_raw_)
        record(*source);
    for (const auto &source : pads_)
        record(source.record);
    return facts;
}
} // namespace sadx

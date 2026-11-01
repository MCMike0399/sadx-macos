#include "../fixtures/animation_table_publications.hpp"
#include "player/animation/sonic_animation_initialize.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace sadx::animation;
namespace {
std::uint32_t bits(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, 4);
    return word;
}
struct Sink {
    Table table = initial_sonic_table();
    std::size_t event = 0, completed = 0;
    void observe(std::array<std::uint32_t, 4> actual) {
        if (event == kOriginalAnimationPublications.size() ||
            actual != kOriginalAnimationPublications[event])
            throw std::runtime_error("original publication order differs at event " +
                                     std::to_string(event));
        ++event;
    }
    ActionResource load_action(std::uint16_t, std::uint16_t ordinal) {
        observe({0, ordinal, 0, 0});
        return {ResourceStatus::Loaded, 0x21000000U + (ordinal % 7) * 32U, {}};
    }
    ActionResource null_action() { return {}; }
    void action(std::uint16_t row, const ActionResource &value) {
        table.records[row].action = value;
        observe({1, row, 0, static_cast<std::uint32_t>(value.identity)});
    }
    void object_count(std::uint16_t row, std::uint8_t value) {
        table.records[row].object_count = value;
        observe({1, row, 1, value});
    }
    void mode(std::uint16_t row, Mode value) {
        table.records[row].mode = value;
        observe({1, row, 2, static_cast<std::uint8_t>(value)});
    }
    void next(std::uint16_t row, std::uint16_t value) {
        table.records[row].next = value;
        observe({1, row, 3, value});
    }
    void transition(std::uint16_t row, float value) {
        table.records[row].transition_step = value;
        observe({1, row, 4, bits(value)});
    }
    void speed(std::uint16_t row, float value) {
        table.records[row].frame_step = value;
        observe({1, row, 5, bits(value)});
    }
    void complete_row() { ++completed; }
};
float real(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, 4);
    return value;
}
bool same(const Record &a, const Record &b) {
    return a.action.status == b.action.status && a.action.identity == b.action.identity &&
           a.action.unavailable_reason == b.action.unavailable_reason &&
           a.object_count == b.object_count && a.mode == b.mode && a.next == b.next &&
           bits(a.transition_step) == bits(b.transition_step) &&
           bits(a.frame_step) == bits(b.frame_step);
}
void native_prefixes() {
    // The same fully known old-row seeds and export-size cohort as the frozen
    // instruction-hook judge. Unknown fixture fields stay in the reference adapter.
    for (std::uint16_t size = 0; size <= 149; ++size) {
        Table actual;
        for (std::uint16_t row = 0; row < 147; ++row) {
            actual.records[row] = {{ResourceStatus::Loaded, 0x31000000U + row * 32U, {}},
                                   static_cast<std::uint8_t>(row + 81),
                                   static_cast<Mode>(static_cast<std::int8_t>(row + 17)),
                                   static_cast<std::uint16_t>(0x8100U + row),
                                   real(0x3e000000U + row),
                                   real(0x3f000000U + row)};
        }
        Table expected = actual;
        SonicExports exports;
        for (std::uint16_t ordinal = 0; ordinal < size; ++ordinal)
            exports.push_back({ResourceStatus::Loaded, 0x21000000U + (ordinal % 7) * 32U, {}});
        std::array<std::uint8_t, 147> masks{};
        TableImportResult wanted;
        for (const auto &event : kOriginalAnimationPublications) {
            if (event[0] == 0) {
                if (event[1] >= size) {
                    wanted.failure = TableImportFailure::MissingExport;
                    wanted.required_export = static_cast<std::uint16_t>(event[1]);
                    break;
                }
                continue;
            }
            auto &row = expected.records.at(static_cast<std::uint16_t>(event[1]));
            switch (event[2]) {
            case 0:
                row.action = {
                    event[3] ? ResourceStatus::Loaded : ResourceStatus::KnownNull, event[3], {}};
                break;
            case 1:
                row.object_count = static_cast<std::uint8_t>(event[3]);
                break;
            case 2:
                row.mode = static_cast<Mode>(static_cast<std::int8_t>(event[3]));
                break;
            case 3:
                row.next = static_cast<std::uint16_t>(event[3]);
                break;
            case 4:
                row.transition_step = real(event[3]);
                break;
            case 5:
                row.frame_step = real(event[3]);
                break;
            default:
                throw std::runtime_error("invalid independent source field");
            }
            masks[event[1]] |= static_cast<std::uint8_t>(1U << event[2]);
        }
        for (const auto mask : masks)
            wanted.completed_records += mask == 63;
        const auto result = initialize_sonic_table(actual, exports);
        if (result.failure != wanted.failure || result.required_export != wanted.required_export ||
            result.completed_records != wanted.completed_records)
            throw std::runtime_error("native import prefix result differs");
        for (std::uint16_t row = 0; row < 147; ++row)
            if (!same(actual.records.at(row), expected.records.at(row)))
                throw std::runtime_error("native import retained/publication prefix differs");
    }
}

} // namespace
int main() {
    try {
        Sink sink;
        if (sink.table.records.size() != 147)
            throw std::runtime_error("Sonic BSS extent");
        for (const auto &entry : sink.table.records) {
            const auto &row = entry.second;
            if (row.action.status != ResourceStatus::KnownNull || row.action.identity ||
                !row.action.unavailable_reason.empty() || row.object_count ||
                row.mode != Mode::Initialize || row.next || bits(row.transition_step) ||
                bits(row.frame_step))
                throw std::runtime_error("Sonic BSS is not source zero");
        }
        detail::initialize_sonic_rows(sink);
        if (sink.event != 1027 || sink.completed != 147)
            throw std::runtime_error("incomplete producer");
        native_prefixes();
        std::cout << "150 native prefixes; 147 rows; 145 ordered export reads and 882 ordered "
                     "field publications\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

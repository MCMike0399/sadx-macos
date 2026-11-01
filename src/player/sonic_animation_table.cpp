#include "player/animation/animation_content.hpp"
#include "player/animation/sonic_animation_initialize.hpp"

namespace sadx::animation {
namespace {
// NOTSA: immediate publication into the caller's one table owner.
struct ContentSink {
    Table &table;
    const SonicExports &exports;
    TableImportResult result{};

    ActionResource load_action(std::uint16_t, std::uint16_t ordinal) {
        if (ordinal >= exports.size()) {
            result.failure = TableImportFailure::MissingExport;
            result.required_export = ordinal;
            throw result;
        }
        return exports[ordinal];
    }
    ActionResource null_action() { return {}; }
    void action(std::uint16_t row, const ActionResource &value) {
        table.records[row].action = value;
    }
    void object_count(std::uint16_t row, std::uint8_t value) {
        table.records[row].object_count = value;
    }
    void mode(std::uint16_t row, Mode value) { table.records[row].mode = value; }
    void next(std::uint16_t row, std::uint16_t value) { table.records[row].next = value; }
    void transition(std::uint16_t row, float value) { table.records[row].transition_step = value; }
    void speed(std::uint16_t row, float value) { table.records[row].frame_step = value; }
    void complete_row() { ++result.completed_records; }
};
} // namespace

// NOTSA: the executable's Sonic table is BSS, independently of arbitrary reference fixtures.
Table initial_sonic_table() {
    Table table;
    for (std::uint16_t row = 0; row < animation::kSonicRecipes.size(); ++row)
        table.records.emplace(row, Record{});
    return table;
}

// FUNCTION: SONIC 0x7cec90 InitializeSonicAnimationTable
TableImportResult initialize_sonic_table(Table &table, const SonicExports &exports) {
    ContentSink sink{table, exports, {}};
    try {
        detail::initialize_sonic_rows(sink);
        return sink.result;
    } catch (const TableImportResult &failure) {
        return failure;
    }
}
} // namespace sadx::animation

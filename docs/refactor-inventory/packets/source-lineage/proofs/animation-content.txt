#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sadx::animation {
// mtnjvwk motion modes; signed source values outside this set select cleanup.
enum class Mode : std::int8_t {
    Initialize = 0,
    Select = 1,
    Transition = 2,
    Loop = 3,
    Next = 4,
    Hold = 5,
    ReverseNext = 6,
    ReverseHold = 7,
    SavedNext = 8,
    SpeedLoop = 9,
    ChargeLoop = 10,
    ChargeNext = 11,
    Wrap = 12,
};

// NOTSA: imported payload closure is distinct from a genuine null declaration.
enum class ResourceStatus { Loaded, KnownNull, Unavailable };
struct ActionResource {
    ResourceStatus status = ResourceStatus::KnownNull;
    std::size_t identity = 0;
    std::string unavailable_reason;
};

struct Record {
    ActionResource action;
    std::uint8_t object_count = 0;
    Mode mode = Mode::Initialize;
    std::uint16_t next = 0;
    float transition_step = 0;
    float frame_step = 0;
};
// NOTSA: canonical table identity can be borrowed by native and development views.
struct TableStorage {
    virtual ~TableStorage() = default;
};
struct Table final : TableStorage {
    std::unordered_map<std::uint16_t, Record> records;
};
struct SonicRecipe {
    bool action_export;
    std::uint16_t action_index;
    std::uint8_t object_count;
    Mode mode;
    std::uint16_t next;
    float transition_step;
    float frame_step;
};
using SonicExports = std::vector<ActionResource>;

enum class TableImportFailure { None, MissingExport };
struct TableImportResult {
    TableImportFailure failure = TableImportFailure::None;
    std::size_t completed_records = 0;
    std::uint16_t required_export = 0;
};

Table initial_sonic_table();
TableImportResult initialize_sonic_table(Table &, const SonicExports &);
} // namespace sadx::animation

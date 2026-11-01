#include "player/sonic_animation_table.hpp"

#include <cstring>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
float real(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
}

// Separate characterization ABI; existing production/probe interfaces are unchanged.
// in: missing ordinal, export size, six-field old-row mask, seed rows,
//     known-null ordinal, outer source state (0 available, 1 unknown, 2 null).
// out: result triple, then 147 rows of presence + six (known,value) pairs.
extern "C" int sadx_animation_table_characterization(const std::uint32_t* in,
                                                     std::uint32_t* out) {
    using namespace sadx;
    PlayerAnimationTable table;
    if (in[3]) {
        for (unsigned row = 0; row < 147; ++row) {
            auto& r = table.records[row];
            if (in[2] & 1) r.action = AnimationResourceReference{0x31000000U + row * 32U};
            if (in[2] & 2) r.object_count = static_cast<std::uint8_t>(row + 81);
            if (in[2] & 4) r.mode = static_cast<std::int8_t>(row + 17);
            if (in[2] & 8) r.next = static_cast<std::uint16_t>(0x8100U + row);
            if (in[2] & 16) r.transition_step = real(0x3e000000U + row);
            if (in[2] & 32) r.frame_step = real(0x3f000000U + row);
        }
    }
    auto exports = std::make_shared<SonicAnimationExport>(in[1]);
    for (unsigned index = 0; index < exports->size(); ++index) {
        // Repeated identities deliberately disagree with ordinal magnitude.
        exports->at(index) = AnimationResourceReference{0x21000000U + (index % 7) * 32U};
        if (index == in[4]) exports->at(index) = AnimationResourceReference{};
        if (index == in[0]) exports->at(index).reset();
    }
    std::optional<std::shared_ptr<const SonicAnimationExport>> source = exports;
    if (in[5] == 1) source.reset();
    if (in[5] == 2) source = std::shared_ptr<const SonicAnimationExport>{};
    const auto result = initialize_sonic_animation_table(table, source);
    *out++ = static_cast<std::uint32_t>(result.failure);
    *out++ = result.source_boundary;
    *out++ = static_cast<std::uint32_t>(result.completed_records);
    for (unsigned index = 0; index < 147; ++index) {
        const auto found = table.records.find(index);
        *out++ = found != table.records.end();
        const PlayerAnimationRecord empty;
        const auto& row = found == table.records.end() ? empty : found->second;
        *out++ = row.action ? (row.action->id ? 2 : 1) : 0;
        *out++ = row.action && row.action->id ? static_cast<std::uint32_t>(*row.action->id) : 0;
        *out++ = row.object_count.has_value();
        *out++ = row.object_count.value_or(0);
        *out++ = row.mode.has_value();
        *out++ = static_cast<std::uint8_t>(row.mode.value_or(0));
        *out++ = row.next.has_value();
        *out++ = row.next.value_or(0);
        *out++ = row.transition_step.has_value();
        *out++ = bits(row.transition_step.value_or(0));
        *out++ = row.frame_step.has_value();
        *out++ = bits(row.frame_step.value_or(0));
    }
    return 0;
}

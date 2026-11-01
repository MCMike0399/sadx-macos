#include "player/animation/sonic_animation_initialize.hpp"
#include "player/sonic_animation_table.hpp"
#include <array>

namespace sadx {
namespace {
// Reference-only source load boundaries, indexed by semantic output row.
constexpr std::array<std::uint32_t, 147> kExportReadBoundaries{{
    0x7cec95, 0x7cecd2, 0x7ced0c, 0x7ced3f, 0x7ced6f, 0x7ceda4, 0x7cedd9, 0x7cee0d, 0x7cee42,
    0x7cee77, 0x7ceeaa, 0x7ceedd, 0x7cef10, 0x7cef43, 0x7cef76, 0x7cefaa, 0x7cefde, 0x7cf010,
    0x7cf045, 0x7cf075, 0x7cf0a5, 0x7cf0d5, 0x7cf107, 0x7cf139, 0x7cf16f, 0x7cf1a2, 0x7cf1d4,
    0x7cf206, 0x7cf238, 0x7cf268, 0x7cf29a, 0x7cf2ca, 0x7cf2fc, 0x7cf32f, 0x7cf361, 0x7cf393,
    0x7cf3c8, 0x7cf3fd, 0x7cf42e, 0x7cf463, 0x7cf495, 0x7cf4c8, 0x7cf4fa, 0x7cf52d, 0x7cf55f,
    0x7cf594, 0x7cf5c9, 0x7cf5fe, 0x7cf633, 0x7cf668, 0x7cf69d, 0x7cf6d2, 0x7cf707, 0x7cf73c,
    0x7cf771, 0x7cf7a6, 0x7cf7d8, 0x7cf80a, 0x7cf83c, 0x7cf86e, 0x7cf8a1, 0x7cf8d4, 0x7cf907,
    0x7cf93a, 0x7cf96b, 0x7cf9a0, 0x7cf9d6, 0x7cfa0c, 0x7cfa41, 0x7cfa76, 0x7cfaac, 0x7cfae1,
    0x7cfb17, 0x7cfb4c, 0x7cfb80, 0x0,      0x0,      0x7cfc12, 0x7cfc47, 0x7cfc7c, 0x7cfcb1,
    0x7cfce6, 0x7cfd1b, 0x7cfd51, 0x7cfd85, 0x7cfdb8, 0x7cfdee, 0x7cfe24, 0x7cfe5a, 0x7cfe8f,
    0x7cfec7, 0x7cfefc, 0x7cff31, 0x7cff67, 0x7cff9a, 0x7cffd2, 0x7d0005, 0x7d003a, 0x7d0070,
    0x7d00a3, 0x7d00d8, 0x7d010d, 0x7d0142, 0x7d0177, 0x7d01ad, 0x7d01e3, 0x7d0218, 0x7d024d,
    0x7d0283, 0x7d02b9, 0x7d02f1, 0x7d0324, 0x7d035c, 0x7d038f, 0x7d03c4, 0x7d03f9, 0x7d042e,
    0x7d0461, 0x7d0494, 0x7d04c7, 0x7d04fb, 0x7d0530, 0x7d0565, 0x7d0598, 0x7d05cb, 0x7d0600,
    0x7d0634, 0x7d066c, 0x7d069f, 0x7d06d2, 0x7d0705, 0x7d073d, 0x7d0770, 0x7d07a3, 0x7d07d6,
    0x7d080e, 0x7d0843, 0x7d0878, 0x7d08ad, 0x7d08e2, 0x7d0917, 0x7d094f, 0x7d0982, 0x7d09b5,
    0x7d09eb, 0x7d0a1f, 0x7d0a57,
}};
struct Sink {
    PlayerAnimationTable &table;
    const std::optional<std::shared_ptr<const SonicAnimationExport>> &source;
    SonicAnimationTableResult result{};

    AnimationResourceReference load_action(std::uint16_t row, std::uint16_t ordinal) {
        if (!*source)
            fail(PlayerAnimationFailure::null_resource, row);
        if (ordinal >= (*source)->size() || !(*source)->at(ordinal))
            fail(PlayerAnimationFailure::unknown_field, row);
        return *(*source)->at(ordinal);
    }
    [[noreturn]] void fail(PlayerAnimationFailure failure, std::uint16_t row) {
        result.failure = failure;
        result.source_boundary = kExportReadBoundaries[row];
        throw result;
    }
    AnimationResourceReference null_action() { return {}; }
    void action(std::uint16_t row, const AnimationResourceReference &value) {
        table.records[row].action = value;
    }
    void object_count(std::uint16_t row, std::uint8_t value) {
        table.records[row].object_count = value;
    }
    void mode(std::uint16_t row, animation::Mode value) {
        table.records[row].mode = static_cast<std::int8_t>(value);
    }
    void next(std::uint16_t row, std::uint16_t value) { table.records[row].next = value; }
    void transition(std::uint16_t row, float value) { table.records[row].transition_step = value; }
    void speed(std::uint16_t row, float value) { table.records[row].frame_step = value; }
    void complete_row() { ++result.completed_records; }
};
} // namespace

SonicAnimationTableResult initialize_sonic_animation_table(
    PlayerAnimationTable &table,
    const std::optional<std::shared_ptr<const SonicAnimationExport>> &source) {
    if (!source)
        return {PlayerAnimationFailure::unknown_field, 0x7cec90, 0};
    Sink sink{table, source, {}};
    try {
        animation::detail::initialize_sonic_rows(sink);
        return sink.result;
    } catch (const SonicAnimationTableResult &failure) {
        return failure;
    }
}
} // namespace sadx

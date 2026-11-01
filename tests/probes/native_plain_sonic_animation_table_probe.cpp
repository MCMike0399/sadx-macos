// Same frozen C ABI and fixture vectors, through the typed native content producer.
#include "player/animation/animation_content.hpp"
#include <cstring>

extern "C" int sadx_native_sonic_animation_table(const std::uint32_t *identities, std::size_t count,
                                                 std::uint8_t *bytes, std::size_t size) {
    using namespace sadx::animation;
    if (!identities || count != 149 || !bytes || size != 147 * 16)
        return 1;
    SonicExports exports;
    for (std::size_t i = 0; i < count; ++i)
        exports.push_back({identities[i] ? ResourceStatus::Loaded : ResourceStatus::KnownNull,
                           identities[i],
                           {}});
    auto table = initial_sonic_table();
    const auto result = initialize_sonic_table(table, exports);
    if (result.failure != TableImportFailure::None || result.completed_records != 147)
        return 2;
    for (std::size_t i = 0; i < 147; ++i) {
        const auto &row = table.records.at(i);
        const auto action = static_cast<std::uint32_t>(row.action.identity);
        const auto mode = static_cast<std::int8_t>(row.mode);
        std::memcpy(bytes + 16 * i, &action, 4);
        bytes[16 * i + 4] = row.object_count;
        std::memcpy(bytes + 16 * i + 5, &mode, 1);
        std::memcpy(bytes + 16 * i + 6, &row.next, 2);
        std::memcpy(bytes + 16 * i + 8, &row.transition_step, 4);
        std::memcpy(bytes + 16 * i + 12, &row.frame_step, 4);
    }
    return 0;
}

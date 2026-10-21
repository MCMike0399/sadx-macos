#pragma once
#include "apps/native_assets.hpp"
#include "game/sound_effects.hpp"

namespace sadx::native::app {
// NOTSA: typed imported data; executable bytes are never read by the native runtime.
inline sound_effects::BankTable load_sound_effects_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "sound-effects.json");
    if (integer(source[@"schema"], 1, 1, "sound-effect schema") != 1)
        throw std::runtime_error("invalid imported sound-effect schema");
    const auto tables = dictionary(source[@"tables"], "sound-effect tables");
    if (integer(tables[@"alternating_pan"], INT32_MIN, INT32_MAX, "sound-effect initial pan") != 8)
        throw std::runtime_error("imported sound-effect initial pan differs from the native literal");
    sound_effects::BankTable result{};
    const auto records = array(tables[@"bank_thresholds"], result.size(), "sound-bank thresholds");
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto row = dictionary(records[i], "sound-bank threshold");
        auto &target = result[i];
        target.tone = static_cast<std::int32_t>(integer(row[@"tone"], -1, INT32_MAX, "sound-bank tone"));
        const auto bank = integer(row[@"bank"], -1, 15, "sound-bank slot");
        target.bank = static_cast<std::uint32_t>(bank);
        if (i + 1 == result.size()) {
            if (target.tone != -1 || bank != -1)
                throw std::runtime_error("imported sound-bank thresholds have no terminal record");
        } else if (target.tone < 0 || bank < 0 || (i && result[i - 1].tone <= target.tone)) {
            throw std::runtime_error("invalid imported sound-bank threshold order");
        }
    }
    return result;
}
} // namespace sadx::native::app

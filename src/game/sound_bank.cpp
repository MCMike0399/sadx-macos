#include "game/sound_bank.hpp"
#include <cstdio>
#include <cstdlib>

namespace sadx::sound_bank {
namespace {
// NOTSA: pinned bank names/drive paths are ASCII. Do not silently approximate a
// different Windows locale/codepage for imported content outside that domain.
bool equal_path(std::string_view first, std::string_view second) {
    const auto lower = [](unsigned char value) {
        if (value > 127)
            throw Error(Boundary::Path, "non-ASCII sound-bank comparison is unrecovered");
        return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
    };
    if (first.size() != second.size())
        return false;
    for (std::size_t index = 0; index != first.size(); ++index)
        if (lower(static_cast<unsigned char>(first[index])) !=
            lower(static_cast<unsigned char>(second[index])))
            return false;
    return true;
}
std::string path(std::string_view prefix, std::string_view name, std::string_view suffix = ".dat") {
    std::string result(prefix);
    result += "SoundData\\SE\\";
    result += name;
    result += suffix;
    if (result.size() >= 260)
        result.resize(259);
    return result;
}
} // namespace
void release_samples(std::uint32_t slot, Channels &channels,
                     const std::function<void(std::int32_t)> &stop_pcm) {
    for (std::size_t index = 0; index != kChannelCount; ++index) {
        auto &record = channels.records[index];
        if (record.tone == -1 || channels.bindings.bank_slots[index] != slot)
            continue;
        if (!stop_pcm)
            throw Error(Boundary::ReleaseSamples, "sound-bank release requires actual StopPCM");
        stop_pcm(static_cast<std::int32_t>(index));
        record.timer = 0;
        channels.bindings.identities[index] = nullptr;
        record.flags = 0;
        record.tone = -1;
        channels.positional_tasks[index] = {};
    }
}
void select(std::int32_t index, const Catalog &catalog, State &state,
            const std::uint32_t &initialized, bool voice_enabled, std::string_view cd_prefix,
            const Services &services) {
    if (index >= static_cast<std::int32_t>(kListCount))
        return;
    if (initialized == 0 || !voice_enabled)
        return;
    if (index < 0)
        throw Error(Boundary::ListIndex, "negative sound list reaches unrecovered loader data");
    for (const auto &entry : catalog[static_cast<std::size_t>(index)]) {
        if (entry.slot >= kSlotCount)
            throw Error(Boundary::Slot, "sound-bank slot is outside the recovered native owner");
        auto &bank = state.slots[entry.slot];
        const auto system_path = path("SYSTEM\\", entry.name);
        const auto pack_path = path("SYSTEM\\", entry.name, "\\");
        if (bank && (equal_path(pack_path, bank->loaded_path()) ||
                     equal_path(system_path, bank->loaded_path()) ||
                     equal_path(path(cd_prefix, entry.name), bank->loaded_path())))
            continue;
        if (bank) {
            if (!services.release_samples)
                throw Error(Boundary::ReleaseSamples, "sound-bank sample release is unavailable");
            services.release_samples(entry.slot);
            if (!services.close)
                throw Error(Boundary::Close, "sound-bank close is unavailable");
            const auto closing = bank;
            services.close(closing);
            bank.reset();
        }
        if (!services.mod_index)
            throw Error(Boundary::Priority, "sound-bank replacement priorities are unavailable");
        const auto pack_priority = services.mod_index(pack_path);
        const auto file_priority = services.mod_index(system_path);
        if (pack_priority >= file_priority) {
            if (!services.open_pack)
                throw Error(Boundary::Pack, "loose sound-pack loader is unavailable");
            bank = services.open_pack(pack_path, entry.slot);
        }
        if (!bank) {
            if (!services.open_file)
                throw Error(Boundary::Open, "file sound-bank loader is unavailable");
            bank = services.open_file(system_path, entry.slot);
            if (!bank)
                bank = services.open_file(path(cd_prefix, entry.name), entry.slot);
        }
    }
}
std::optional<std::string> normalize_name(std::string_view input, std::uint32_t slot,
                                          std::int32_t &counter, bool force_adx) {
    const auto slash = input.find_last_of("/\\");
    std::string name(input.substr(slash == std::string_view::npos ? 0 : slash + 1));
    if (!name.empty() && name.front() == '_')
        return std::nullopt;
    const auto dot = name.find_last_of('.');
    const auto extension = dot == std::string::npos ? std::string{} : name.substr(dot);
    if (dot != std::string::npos)
        name.resize(dot);
    const bool patterned =
        name.substr(0, 2) == "B0" && name.substr(3, 2) == "_0" && name.substr(6, 1) == "_";
    const auto sound_id = patterned ? std::atoi(name.substr(7, 2).c_str()) : counter;
    counter = static_cast<std::int32_t>(static_cast<std::uint32_t>(counter) + 1);
    char result[14];
    std::snprintf(result, sizeof(result), "B%02d_00_%02u%s", static_cast<int>(slot),
                  static_cast<unsigned>(sound_id), force_adx ? ".ADX" : extension.c_str());
    return std::string(result);
}
} // namespace sadx::sound_bank

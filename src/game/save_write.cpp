#include "game/save_write.hpp"
#include <cstdio>
#include <stdexcept>
namespace sadx::save {
namespace {
// NOTSA: only source-created canonical ASCII save names reach this comparison.
// The original CompareStringA(9,NORM_IGNORECASE,...) is not a generic Unicode
// claim.
bool same_save_name(std::string_view left, std::string_view right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        auto lower = [](char value) {
            return value >= 'A' && value <= 'Z' ? char(value + 32) : value;
        };
        if (static_cast<unsigned char>(left[i]) >= 128 ||
            static_cast<unsigned char>(right[i]) >= 128)
            throw std::invalid_argument("non-ASCII CompareStringA save-name boundary");
        if (lower(left[i]) != lower(right[i]))
            return false;
    }
    return true;
}
std::string slot_name(std::uint8_t slot) {
    std::array<char, 14> name{};
    std::snprintf(name.data(), name.size(), "SonicDX%02u.snc", static_cast<unsigned>(slot));
    return name.data();
}
} // namespace
std::int32_t write_file(WriteState &state, WriteCalls &calls, SaveNameCalls &names,
                        const std::function<void()> &capture_live) {
    state.save_retry = 0;
    state.save_ok = 0;
    if (state.saving_disabled != 0)
        return -1;
    if (state.save_count > 4) {
        state.now_saving = 0;
        state.save_ready = 0;
        state.save_exec_flag = 0;
        state.alert_flag = 0;
        state.save_count = 0;
    }
    calls.ensure_directory();
    if (state.file_delete_flag == 0)
        capture_live();
    std::string path;
    if (state.first_create_flag != 0) {
        state.first_create_flag = 0;
        if (state.file_count > 98)
            return 0;
        if (!state.list)
            throw std::invalid_argument("new-save source list sentinel is missing");
        std::uint8_t slot = 1;
        auto name = slot_name(slot);
        auto entry = state.list->next;
        while (entry) {
            if (!entry->file)
                throw std::invalid_argument("new-save list filename is indeterminate");
            if (same_save_name(name, entry->file->name)) {
                entry = state.list->next;
                ++slot;
                if (slot > 99)
                    return 0;
                name = slot_name(slot);
            } else
                entry = entry->next;
        }
        if (state.save_name.value != nullptr) {
            names.release(state.save_name.value);
            state.save_name.value = nullptr;
        }
        auto *destination = names.allocate(14);
        state.save_name.value = destination;
        ++state.file_count;
        state.selected_file = slot;
        if (!destination)
            throw std::invalid_argument("source save-name formatter reached a null allocation");
        names.copy(destination, name);
        path = "./SAVEDATA/" + name;
    } else {
        if (!state.save_name.value)
            throw std::invalid_argument("source lstrlenA reached a null SaveName");
        path = std::string("./SAVEDATA/") + state.save_name.value;
    }
    const auto file = calls.open_write(path);
    if (!file)
        throw std::invalid_argument("source fwrite reached a missing FILE owner");
    calls.write_save(file, encode(state.data));
    state.card_complete = 0;
    calls.reset_input();
    state.save_count = 0;
    state.now_saving = 0;
    state.save_retry = 0;
    state.save_ready = 0;
    state.save_exec_flag = 0;
    state.alert_flag = 0;
    calls.close_write(file);
    state.save_ok = 1;
    return 0;
}
} // namespace sadx::save

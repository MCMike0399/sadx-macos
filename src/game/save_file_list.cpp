#include "game/save_file_list.hpp"
#include "game/file_select.hpp"
#include <array>
#include <cstdio>
#include <type_traits>

namespace sadx::file_select {
namespace {
// NOTSA: reads the established save::encode byte contract without host padding.
class Reader {
  public:
    const save::EncodedData &bytes;
    std::size_t position = 0;
    explicit Reader(const save::EncodedData &source) : bytes(source) {}
    template<class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    void get(T &value) {
        using Unsigned = std::make_unsigned_t<T>;
        Unsigned result = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i)
            result |= static_cast<Unsigned>(bytes.at(position++)) << (i * 8);
        value = static_cast<T>(result);
    }
    template<class T, std::size_t N> void get(std::array<T, N> &values) {
        for (auto &value : values) get(value);
    }
    void get(save::ClearTime &value) {
        get(value.minutes);
        get(value.seconds);
        get(value.frames);
    }
    void get(save::FishRecord &value) {
        get(value.weight);
        get(value.type);
    }
    void get(save::CircuitRecords &value) {
        get(value.times);
        get(value.laps);
    }
    void get(save::Sequence &value) {
        get(value.time);
        get(value.padding);
        get(value.number);
        get(value.section);
        get(value.next_section);
        get(value.stage);
        get(value.destination);
    }
};
// NOTSA: names the repeated checks of filename bytes7/8 followed by atoi.
std::uint8_t file_slot(std::string_view filename) {
    if (filename.size() < 9)
        throw Error(Boundary::ShortFileName, "save enumeration name has no source slot bytes");
    if (filename[7] < '0' || filename[7] > '9' || filename[8] < '0' || filename[8] > '9')
        return 0;
    std::uint32_t value = 0;
    for (std::size_t i = 7; i < filename.size() && filename[i] >= '0' && filename[i] <= '9'; ++i)
        value = value * 10U + static_cast<unsigned>(filename[i] - '0');
    return static_cast<std::uint8_t>(value);
}
// NOTSA: preserves the original256-byte formatted path boundary.
std::string save_path(std::string_view name) {
    auto path = std::string("./SAVEDATA/") + std::string(name);
    if (path.size() >= 256)
        throw Error(Boundary::Path, "save filename exceeds the source local path buffer");
    return path;
}
// NOTSA: incomplete fread items still publish the bytes that were read.
std::uint32_t read_live_save(const std::shared_ptr<File> &file, save::Data &live, SaveFileCalls &calls) {
    if (!file) throw Error(Boundary::MissingFile, "source fread reached a missing FILE owner");
    auto bytes = save::encode(live);
    const auto count = calls.read_save(file, bytes);
    live = decode_save(bytes);
    return count;
}
} // namespace
save::Data decode_save(const save::EncodedData &bytes) {
    save::Data data;
    Reader reader(bytes);
    reader.get(data.checksum);
    reader.get(data.total_time);
    reader.get(data.best_scores);
    reader.get(data.best_times);
    reader.get(data.best_weights);
    reader.get(data.biggest_fish);
    reader.get(data.best_rings);
    reader.get(data.sky_chase);
    reader.get(data.sky_chase_two);
    reader.get(data.snow_board);
    reader.get(data.sand_board);
    reader.get(data.hedgehog_hammer);
    reader.get(data.circuit);
    reader.get(data.bosses);
    reader.get(data.emblems);
    reader.get(data.options);
    reader.get(data.lives);
    reader.get(data.last_character);
    reader.get(data.vibration);
    reader.get(data.reserved);
    reader.get(data.last_stage);
    reader.get(data.last_action);
    reader.get(data.sequence_variables);
    reader.get(data.event_flags);
    reader.get(data.playable_scene_flags);
    reader.get(data.sequences);
    reader.get(data.completed_stages);
    reader.get(data.missions);
    reader.get(data.total_rings);
    reader.get(data.metal_scores);
    reader.get(data.metal_times);
    reader.get(data.metal_rings);
    reader.get(data.metal_alignment);
    reader.get(data.metal_snow_board);
    reader.get(data.metal_sand_board);
    reader.get(data.metal_circuit);
    reader.get(data.metal_bosses);
    reader.get(data.metal_emblems);
    if (reader.position != bytes.size())
        throw std::logic_error("save decode size");
    return data;
}
void append_save_list(List &head, const std::optional<std::string> &name, const FileInfo &info) {
    if (!head) {
        head = std::make_shared<ListNode>();
        return;
    }
    auto tail = head;
    while (tail->next) tail = tail->next;
    tail->next = std::make_shared<ListNode>();
    if (!name)
        throw Error(Boundary::IndeterminateSentinel, "nonnull list reached a null source filename");
    tail->next->file = FileInfo{*name, info.last_write_time};
}
void free_save_list(List &head) {
    while (head && head->next) {
        auto previous = head;
        while (previous->next->next) previous = previous->next;
        previous->next->file.reset();
        previous->next.reset();
    }
    head.reset();
}
std::int32_t load_selected_save(LoadState &state, SaveFileCalls &calls) {
    if (state.saving_disabled != 0) return -1;
    auto selected = state.list;
    std::string name;
    if (selected) {
        save::initialize(state.live, state.rings);
        for (std::uint16_t remaining = state.selected_file; remaining != 0; --remaining) {
            selected = selected->next;
            if (!selected)
                throw Error(Boundary::ListIndex, "selected save traversed beyond source list");
        }
        if (!selected->file)
            throw Error(Boundary::IndeterminateSentinel, "selected sentinel has an indeterminate filename");
        name = selected->file->name;
    } else {
        if (!state.fallback_name)
            throw Error(Boundary::MissingFile, "save load has no source fallback filename");
        name = state.fallback_name;
    }
    const auto file = calls.open_file(save_path(name));
    read_live_save(file, state.live, calls);
    calls.close_file(file);
    return 0;
}
std::uint8_t count_save_files(List &list, save::Data &live, SaveFileCalls &calls) {
    append_save_list(list, std::nullopt, {});
    const auto first = calls.find_first("./SAVEDATA/SonicDX??.snc");
    if (!first) return 0;
    std::uint8_t count = 0;
    auto info = std::optional<FileInfo>(first->info);
    do {
        const auto slot = file_slot(info->name);
        const auto file = calls.open_file(save_path(info->name));
        const auto read = read_live_save(file, live, calls);
        if (read != 0 && slot != 0 && slot < 100 && check_save(save::encode(live))) {
            std::array<char, 32> name{};
            std::snprintf(name.data(), name.size(), "SonicDX%02d.snc", slot);
            append_save_list(list, std::string(name.data()), *info);
            ++count;
        }
        calls.close_file(file);
        if (count > 98) break;
        info = calls.find_next(first->search);
    } while (info);
    calls.close_search(first->search);
    return count;
}
} // namespace sadx::file_select

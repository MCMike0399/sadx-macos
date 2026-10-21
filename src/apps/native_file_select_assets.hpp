#pragma once
#include "apps/native_regular_texture_assets.hpp"
#include "runtime/font_assets.hpp"
#include "game/menu_voice.hpp"
#include "runtime/file_select_display_assets.hpp"
#include <set>
#include <limits>

namespace sadx::native::app {
// NOTSA: decode source-proved data declarations; runtime receives no source pointers.
inline FontAssets load_font_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "font-assets.json");
    const auto ascii = dictionary(source[@"ascii"], "ASCII font");
    FontAssets result{};
    const auto dimensions = array(ascii[@"dimension_words"], 4, "font dimensions");
    const auto proportions = array(ascii[@"proportions"], 128, "font proportions");
    const auto colors = array(ascii[@"colors"], 4, "font colors");
    for (std::size_t index = 0; index < result.dimension_words.size(); ++index) {
        result.dimension_words[index] = static_cast<std::uint32_t>(
            integer(dimensions[index], 0, UINT32_MAX, "font dimension word"));
        float value;
        std::memcpy(&value, &result.dimension_words[index], sizeof(value));
        if (!std::isfinite(value) || value <= 0)
            throw std::runtime_error("invalid imported font dimension");
        result.colors[index] = static_cast<std::uint32_t>(
            integer(colors[index], 0, UINT32_MAX, "font color"));
    }
    for (std::size_t index = 0; index < result.proportions.size(); ++index)
        result.proportions[index] = static_cast<std::uint8_t>(
            integer(proportions[index], 0, UINT8_MAX, "font proportion"));
    result.texture_list = texture_declaration(ascii[@"texture_list"]);
    if (result.texture_list.texture_count != 1)
        throw std::runtime_error("unsupported imported ASCII font texture count");
    return result;
}
inline menu_voice::Catalog load_menu_voice_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "menu-voice.json");
    id entries = source[@"entries"];
    if (![entries isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid imported menu voice entries");
    menu_voice::Catalog result;
    std::set<std::pair<std::uint8_t, std::int32_t>> identities;
    for (id item in entries) {
        const auto entry = dictionary(item, "menu voice entry");
        menu_voice::Entry value{
            static_cast<std::uint8_t>(integer(entry[@"character"], 0, 8, "menu voice row")),
            static_cast<std::int32_t>(integer(entry[@"request"], 0, 8, "voice request")),
            static_cast<std::int32_t>(integer(entry[@"voice"], -1, 2048, "menu voice ID"))};
        if (!identities.emplace(value.character, value.request).second)
            throw std::runtime_error("duplicate imported menu voice request");
        result.push_back(value);
    }
    return result;
}
// NOTSA: source tables are fixed-size integer arrays, decoded without pointer identities.
template<class Integer, std::size_t Size>
inline void file_select_integer_table(id value, std::array<Integer, Size> &destination,
                                      const char *name) {
    const auto values = array(value, Size, name);
    for (std::size_t index = 0; index < Size; ++index)
        destination[index] = static_cast<Integer>(integer(values[index], 0,
            std::numeric_limits<Integer>::max(), name));
}
inline FileSelectDisplayAssets load_file_select_display_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "file-select-display.json");
    FileSelectDisplayAssets result{};
    auto &tables = result.tables;
    const auto rows = array(source[@"frame_rows"], tables.frame_rows.size(), "menu frame rows");
    const auto scalar = [](id value) {
        if (![value isKindOfClass:NSNumber.class])
            throw std::runtime_error("invalid imported menu frame coordinate");
        const auto number = [value doubleValue];
        const auto converted = static_cast<float>(number);
        if (!std::isfinite(converted) || static_cast<double>(converted) != number)
            throw std::runtime_error("inexact imported menu frame coordinate");
        return converted;
    };
    for (std::size_t index = 0; index < tables.frame_rows.size(); ++index) {
        const auto row = dictionary(rows[index], "menu frame row");
        tables.frame_rows[index] = {
            scalar(row[@"right_x"]), scalar(row[@"bottom_y"]), scalar(row[@"depth"]),
            static_cast<std::uint32_t>(integer(row[@"top_color"], 0, UINT32_MAX, "top color")),
            static_cast<std::uint32_t>(integer(row[@"bottom_color"], 0, UINT32_MAX, "bottom color")),
            static_cast<std::uint8_t>(integer(row[@"corner_texture"], 0, UINT8_MAX, "corner")),
            static_cast<std::uint8_t>(integer(row[@"horizontal_texture"], 0, UINT8_MAX, "horizontal")),
            static_cast<std::uint8_t>(integer(row[@"vertical_texture"], 0, UINT8_MAX, "vertical")),
            static_cast<std::uint8_t>(integer(row[@"padding"], 0, UINT8_MAX, "padding"))};
    }
    const auto tiles = array(source[@"sky_tiles"], 8, "menu sky tiles");
    for (id item in tiles) {
        const auto tile = array(item, 3, "menu sky tile");
        tables.sky_tiles.push_back({
            static_cast<std::int16_t>(integer(tile[0], INT16_MIN, INT16_MAX, "sky texture")),
            static_cast<std::int16_t>(integer(tile[1], INT16_MIN, INT16_MAX, "sky x")),
            static_cast<std::int16_t>(integer(tile[2], INT16_MIN, INT16_MAX, "sky y"))});
    }
    file_select_integer_table(source[@"detail_labels"], tables.detail_labels, "menu labels");
    file_select_integer_table(source[@"unlocked_icons"], tables.unlocked_icons, "unlocked icons");
    file_select_integer_table(source[@"locked_icons"], tables.locked_icons, "locked icons");
    file_select_integer_table(source[@"character_names"], tables.character_names, "character names");
    file_select_integer_table(source[@"stage_names"], tables.stage_names, "stage names");
    file_select_integer_table(source[@"sequence_indices"], tables.sequence_indices, "sequence indices");
    file_select_integer_table(source[@"clear_flags"], tables.clear_flags, "clear flags");
    file_select_integer_table(source[@"digits"], tables.digits, "digits");
    return result;
}
// [platform:time] The original requests local date fields from a Windows FILETIME.
inline file_select_display::Calendar file_calendar_in_zone(std::uint64_t filetime, NSTimeZone *zone) {
    // Calendar display ignores subsecond ticks; integer seconds avoid rounding into
    // the next day before converting from the1601 epoch to NSDate's Unix epoch.
    const auto seconds = static_cast<double>(filetime / 10000000U) - 11644473600.0;
    const auto date = [NSDate dateWithTimeIntervalSince1970:seconds];
    const auto calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
    calendar.timeZone = zone;
    const auto fields = [calendar components:NSCalendarUnitYear | NSCalendarUnitMonth |
        NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond
                                  fromDate:date];
    if (fields.year < 1 || fields.year > UINT16_MAX || fields.month < 1 || fields.month > 12 ||
        fields.day < 1 || fields.day > 31 || fields.hour < 0 || fields.hour > 23 ||
        fields.minute < 0 || fields.minute > 59 || fields.second < 0 || fields.second > 59)
        throw std::runtime_error("native FILETIME cannot be represented as local calendar fields");
    return {static_cast<std::uint16_t>(fields.year), static_cast<std::uint16_t>(fields.month),
        static_cast<std::uint16_t>(fields.day), static_cast<std::uint16_t>(fields.hour),
        static_cast<std::uint16_t>(fields.minute), static_cast<std::uint16_t>(fields.second)};
}
inline file_select_display::Calendar local_file_calendar(std::uint64_t filetime) {
    return file_calendar_in_zone(filetime, NSTimeZone.localTimeZone);
}
} // namespace sadx::native::app

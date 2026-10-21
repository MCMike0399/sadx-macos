#pragma once
#include "apps/native_assets.hpp"
#include "runtime/menu_dialog_assets.hpp"

namespace sadx::native::app {
// NOTSA: decode exact binary32 resource words at the imported-asset boundary.
inline float dialog_coordinate(id value) {
    const auto word = static_cast<std::uint32_t>(integer(value, 0, UINT32_MAX, "dialog coordinate"));
    float result;
    std::memcpy(&result, &word, sizeof(result));
    if (!std::isfinite(result))
        throw std::runtime_error("nonfinite imported dialog coordinate");
    return result;
}
inline std::string dialog_message(id value) {
    const auto hex = string(value, "dialog message bytes");
    if (hex.size() % 2 != 0 || hex.size() > 8192)
        throw std::runtime_error("invalid imported dialog message length");
    const auto digit = [](char character) -> unsigned {
        if (character >= '0' && character <= '9') return unsigned(character - '0');
        if (character >= 'a' && character <= 'f') return unsigned(character - 'a' + 10);
        throw std::runtime_error("invalid imported dialog message byte");
    };
    std::string result;
    for (std::size_t index = 0; index < hex.size(); index += 2) {
        const auto byte = digit(hex[index]) * 16 + digit(hex[index + 1]);
        if (byte == 0)
            throw std::runtime_error("embedded terminator in imported dialog message");
        result.push_back(static_cast<char>(byte));
    }
    return result;
}
inline MenuDialogAssets load_menu_dialog_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "menu-dialog.json");
    MenuDialogAssets result{};
    const auto definitions = array(source[@"definitions"], 1, "reached dialog definitions");
    const auto definition = dictionary(definitions[0], "file selection dialog");
    (void)integer(definition[@"dialog"], 0, 0, "dialog index");
    title_menu::DialogParameters parameters{};
    parameters.style = static_cast<title_menu::DialogStyle>(
        integer(definition[@"style"], 5, 5, "file selection dialog style"));
    if (string(definition[@"content"], "dialog content") != "file_select_help")
        throw std::runtime_error("unrecovered imported dialog content");
    parameters.content = title_menu::DialogContent::FileSelectHelp;
    result.panel_texture_bindings.push_back(static_cast<std::uint32_t>(
        integer(definition[@"menu_texture"], 0, 29, "dialog menu texture")));
    parameters.choice_count = static_cast<std::int8_t>(
        integer(definition[@"choice_count"], 3, 3, "dialog choices"));
    parameters.cancel_choice = static_cast<std::int8_t>(
        integer(definition[@"cancel_choice"], 0, 2, "dialog cancel choice"));
    auto panels = std::make_shared<std::vector<title_menu::DialogPanel>>();
    for (id entry in array(definition[@"panels"], 3, "dialog panels")) {
        const auto panel = dictionary(entry, "dialog panel");
        const auto coordinates = array(panel[@"position_words"], 2, "dialog panel coordinates");
        panels->push_back({dialog_coordinate(coordinates[0]), dialog_coordinate(coordinates[1]),
            static_cast<std::uint8_t>(integer(panel[@"texture"], 0, UINT8_MAX, "dialog panel texture"))});
    }
    parameters.panels = std::move(panels);
    const auto cues = array(definition[@"sounds"], 5, "dialog sounds");
    const auto cue = [&](std::size_t index) {
        return static_cast<std::int32_t>(integer(cues[index], INT32_MIN, INT32_MAX, "dialog sound"));
    };
    parameters.sounds = std::make_shared<title_menu::DialogSounds>(
        title_menu::DialogSounds{cue(0), cue(1), cue(2), cue(3), cue(4)});
    parameters.upper_color = static_cast<std::uint32_t>(
        integer(definition[@"upper_color"], 0, UINT32_MAX, "dialog upper color"));
    parameters.lower_color = static_cast<std::uint32_t>(
        integer(definition[@"lower_color"], 0, UINT32_MAX, "dialog lower color"));
    const auto geometry = array(definition[@"geometry_words"], 7, "dialog geometry");
    parameters.center_x = dialog_coordinate(geometry[0]);
    parameters.center_y = dialog_coordinate(geometry[1]);
    parameters.depth = dialog_coordinate(geometry[2]);
    parameters.width = dialog_coordinate(geometry[3]);
    parameters.height = dialog_coordinate(geometry[4]);
    parameters.scale_x = dialog_coordinate(geometry[5]);
    parameters.scale_y = dialog_coordinate(geometry[6]);
    result.definitions.push_back(std::move(parameters));
    const auto curve = [&](id value, auto &destination) {
        const auto rows = array(value, destination.size(), "dialog curve");
        for (std::size_t index = 0; index < destination.size(); ++index) {
            const auto row = array(rows[index], 2, "dialog curve point");
            destination[index] = {dialog_coordinate(row[0]), dialog_coordinate(row[1])};
        }
    };
    curve(source[@"opening"], result.tables.opening);
    curve(source[@"closing"], result.tables.closing);
    const auto directions = array(source[@"directions"], 6, "dialog directions");
    for (std::size_t index = 0; index < result.tables.directions.size(); ++index) {
        const auto row = array(directions[index], 2, "dialog direction");
        result.tables.directions[index] = {
            static_cast<std::uint32_t>(integer(row[0], 0, UINT32_MAX, "previous button")),
            static_cast<std::uint32_t>(integer(row[1], 0, UINT32_MAX, "next button"))};
    }
    result.cursor_texture_binding = static_cast<std::uint32_t>(
        integer(source[@"cursor_menu_texture"], 0, 29, "dialog cursor texture"));
    id resources = source[@"help_resources"];
    if (![resources isKindOfClass:NSArray.class] || [resources count] == 0 || [resources count] > 15)
        throw std::runtime_error("invalid imported dialog message resources");
    std::vector<std::shared_ptr<const std::string>> messages;
    for (id resource in resources)
        messages.push_back(std::make_shared<const std::string>(dialog_message(resource)));
    const auto identities = array(source[@"help_resource_ids_by_choice"], 3, "dialog message identities");
    const auto contents = array(source[@"help_bytes_by_choice"], 3, "dialog message contents");
    for (std::size_t choice = 0; choice < result.help.size(); ++choice) {
        const auto ids = array(identities[choice], 5, "dialog message languages");
        const auto bytes = array(contents[choice], 5, "dialog message bytes by language");
        for (std::size_t language = 0; language < result.help[choice].size(); ++language) {
            const auto index = static_cast<std::size_t>(integer(ids[language], 0,
                static_cast<std::int64_t>(messages.size()) - 1, "dialog message identity"));
            if (*messages[index] != dialog_message(bytes[language]))
                throw std::runtime_error("inconsistent imported dialog message identity");
            result.help[choice][language] = messages[index];
        }
    }
    return result;
}
} // namespace sadx::native::app

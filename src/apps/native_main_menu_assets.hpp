#pragma once
#include "apps/native_menu_dialog_assets.hpp"
#include "runtime/main_menu_assets.hpp"

namespace sadx::native::app {
// NOTSA: validate and bind the three actual source MainMenu descriptors, with no synthetic gap.
inline MainMenuAssets load_main_menu_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "main-menu-dialog.json");
    const auto definitions = array(source[@"definitions"], 3, "main menu dialog definitions");
    MainMenuAssets result{};
    constexpr std::array<std::int32_t, 3> counts{6, 5, 3};
    for (std::size_t index = 0; index < result.definitions.size(); ++index) {
        const auto definition = dictionary(definitions[index], "main menu dialog");
        const auto identity = static_cast<std::int32_t>(index) + 2;
        (void)integer(definition[@"dialog"], identity, identity, "main menu dialog identity");
        title_menu::DialogParameters parameters{};
        parameters.style = static_cast<title_menu::DialogStyle>(
            integer(definition[@"style"], 4, 4, "main menu dialog style"));
        const auto content = index == 2 ? "trial_menu_backdrop" : "main_menu_backdrop";
        if (string(definition[@"content"], "main menu dialog content") != content)
            throw std::runtime_error("invalid main menu dialog callback");
        parameters.content = index == 2 ? title_menu::DialogContent::TrialMenuBackdrop
                                        : title_menu::DialogContent::MainMenuBackdrop;
        (void)integer(definition[@"menu_texture"], 19, 19, "main menu panel texture");
        parameters.choice_count = static_cast<std::int8_t>(integer(
            definition[@"choice_count"], counts[index], counts[index], "main menu choices"));
        parameters.cancel_choice = static_cast<std::int8_t>(integer(
            definition[@"cancel_choice"], counts[index], counts[index], "main menu cancel choice"));
        auto panels = std::make_shared<std::vector<title_menu::DialogPanel>>();
        for (id entry in array(definition[@"panels"], counts[index], "main menu panels")) {
            const auto panel = dictionary(entry, "main menu panel");
            const auto coordinates =
                array(panel[@"position_words"], 2, "main menu panel coordinates");
            panels->push_back({dialog_coordinate(coordinates[0]), dialog_coordinate(coordinates[1]),
                               static_cast<std::uint8_t>(integer(panel[@"texture"], 0, UINT8_MAX,
                                                                 "main menu panel texture"))});
        }
        parameters.panels = std::move(panels);
        const auto cues = array(definition[@"sounds"], 5, "main menu dialog sounds");
        const auto cue = [&](std::size_t item) {
            return static_cast<std::int32_t>(
                integer(cues[item], INT32_MIN, INT32_MAX, "main menu dialog sound"));
        };
        parameters.sounds = std::make_shared<title_menu::DialogSounds>(
            title_menu::DialogSounds{cue(0), cue(1), cue(2), cue(3), cue(4)});
        parameters.upper_color = static_cast<std::uint32_t>(
            integer(definition[@"upper_color"], 0, UINT32_MAX, "main menu upper color"));
        parameters.lower_color = static_cast<std::uint32_t>(
            integer(definition[@"lower_color"], 0, UINT32_MAX, "main menu lower color"));
        const auto geometry = array(definition[@"geometry_words"], 7, "main menu dialog geometry");
        parameters.center_x = dialog_coordinate(geometry[0]);
        parameters.center_y = dialog_coordinate(geometry[1]);
        parameters.depth = dialog_coordinate(geometry[2]);
        parameters.width = dialog_coordinate(geometry[3]);
        parameters.height = dialog_coordinate(geometry[4]);
        parameters.scale_x = dialog_coordinate(geometry[5]);
        parameters.scale_y = dialog_coordinate(geometry[6]);
        result.definitions[index] = std::move(parameters);
    }
    const auto words = array(source[@"variant_words"], 7, "main menu variant words");
    const auto mapping = array(source[@"variant_mapping"], 7, "main menu variant mapping");
    for (std::size_t index = 0; index < result.variant_mapping.size(); ++index) {
        const auto word = static_cast<std::uint32_t>(
            integer(words[index], 0, UINT32_MAX, "main menu variant word"));
        const auto byte =
            static_cast<std::uint8_t>(integer(mapping[index], 0, UINT8_MAX, "main menu variant"));
        if (static_cast<std::uint8_t>(word) != byte)
            throw std::runtime_error("main menu variant word and byte disagree");
        result.variant_mapping[index] = byte;
    }
    const auto layout = array(source[@"trial_backdrop_layout_words"], 4, "trial backdrop layout");
    result.trial_layout = {dialog_coordinate(layout[0]), dialog_coordinate(layout[1]),
                           dialog_coordinate(layout[2]), dialog_coordinate(layout[3])};
    (void)integer(source[@"main_common_texture"], 19, 19, "main common texture");
    (void)integer(source[@"main_background_texture"], 20, 20, "main background texture");
    return result;
}
} // namespace sadx::native::app

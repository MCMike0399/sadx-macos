#pragma once
#include "apps/native_menu_dialog_assets.hpp"
#include "runtime/character_selection_assets.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <fstream>
#include <iterator>
#include <vector>

namespace sadx::native::app {
namespace {
// The canonical combined graph. An append-only display import may only follow
// these records; its provenance must prove the prefix count and digest so no
// existing resource identity can shift.
constexpr std::size_t kCanonicalSelectionRecords = 3328;
constexpr std::size_t kCanonicalSelectionBytes = 766112;
constexpr const char *kCanonicalSelectionPrefix =
    "c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f";
// NOTSA: digest text at the imported-content boundary (same helper shape as save seeds).
inline std::string sha256_text(const unsigned char *data, std::size_t size) {
    if (size > UINT32_MAX)
        throw std::runtime_error("selection pack exceeds supported digest input size");
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data, static_cast<CC_LONG>(size), digest);
    constexpr char digits[] = "0123456789abcdef";
    std::string text;
    for (const auto byte : digest) {
        text += digits[byte >> 4];
        text += digits[byte & 15];
    }
    return text;
}
// NOTSA: the append changes only the record-count word; rebuild the canonical
// prefix from the loaded bytes instead of trusting the imported declaration.
inline bool selection_prefix_matches(std::vector<unsigned char> bytes) {
    if (bytes.size() < kCanonicalSelectionBytes)
        return false;
    bytes[8] = kCanonicalSelectionRecords & 0xff;
    bytes[9] = (kCanonicalSelectionRecords >> 8) & 0xff;
    bytes[10] = (kCanonicalSelectionRecords >> 16) & 0xff;
    bytes[11] = (kCanonicalSelectionRecords >> 24) & 0xff;
    return sha256_text(bytes.data(), kCanonicalSelectionBytes) == kCanonicalSelectionPrefix;
}
// NOTSA: rejects partial graphs and unproven appends before any consumer binds.
inline void require_selection_graph(const std::filesystem::path &directory,
                                    const character_pack::CharacterPack &pack) {
    const auto count = pack.record_count();
    if (count == 3302 || count == kCanonicalSelectionRecords)
        return;
    const auto path = directory / "characters/sonic-selection.provenance.json";
    if (!std::filesystem::exists(path))
        throw std::runtime_error("CharacterSelect requires the complete Sonic/selection resource graph");
    const auto proof = read_metadata(path);
    const auto append = dictionary(proof[@"display_append"], "display append provenance");
    const auto prefix = static_cast<std::size_t>(integer(
        append[@"prefix_records"], kCanonicalSelectionRecords, kCanonicalSelectionRecords,
        "display append prefix records"));
    const auto added = static_cast<std::size_t>(integer(
        append[@"new_records"], 1, static_cast<std::int32_t>(count), "display append records"));
    if (prefix + added != count)
        throw std::runtime_error("imported display append count mismatch");
    if (string(append[@"prefix_sha256"], "display append prefix digest") !=
        kCanonicalSelectionPrefix)
        throw std::runtime_error("imported display append changed the canonical selection prefix");
    (void)array(append[@"declarations"], 8, "display append declarations");
    if (static_cast<std::size_t>(integer(append[@"prefix_bytes"], 1, INT32_MAX,
                                           "display append prefix bytes")) !=
        kCanonicalSelectionBytes)
        throw std::runtime_error("imported display append prefix length differs");
    std::ifstream file(directory / "characters/sonic-selection.actions", std::ios::binary);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file), {}};
    if (!file || bytes.empty())
        throw std::runtime_error("cannot read the imported selection pack");
    if (sha256_text(bytes.data(), bytes.size()) !=
        string(proof[@"pack_sha256"], "selection pack digest"))
        throw std::runtime_error("imported selection pack bytes differ from provenance");
    if (!selection_prefix_matches(std::move(bytes)))
        throw std::runtime_error("imported display append changed the canonical prefix bytes");
}
} // namespace
// NOTSA: declarations bind to the one validated combined pack owned by the app.
inline CharacterSelectionAssets load_character_selection_assets(
    const std::filesystem::path &directory, const character_pack::CharacterPack &pack) {
    require_selection_graph(directory, pack);
    CharacterSelectionAssets result{};
    const auto preview = read_metadata(directory / "characters/selection-preview.json");
    if (string(preview[@"pack_schema"], "selection pack schema") != "SDXACT03")
        throw std::runtime_error("invalid imported selection pack schema");
    constexpr std::array<std::uint16_t, 7> characters{0, 2, 3, 5, 7, 6, 0};
    constexpr std::array<const char *, 7> kinds{
        "sonic", "miles", "knuckles", "amy", "big", "gamma", "question_mark"};
    const auto imported_characters = array(preview[@"characters"], 7, "selection characters");
    const auto actor_kinds = array(preview[@"actor_kinds"], 7, "selection actor kinds");
    const auto positions = array(preview[@"positions_words"], 7, "selection positions");
    for (std::size_t index = 0; index < characters.size(); ++index) {
        result.characters[index] = static_cast<std::uint16_t>(integer(
            imported_characters[index], characters[index], characters[index], "selection character"));
        if (string(actor_kinds[index], "selection actor kind") != kinds[index])
            throw std::runtime_error("invalid imported selection actor identity");
        const auto position = array(positions[index], 3, "selection position");
        result.positions[index] = {dialog_coordinate(position[0]), dialog_coordinate(position[1]),
                                   dialog_coordinate(position[2])};
    }
    result.sonic_aim_y = static_cast<std::int32_t>(
        integer(preview[@"sonic_aim_y"], INT32_MIN, INT32_MAX, "selection Sonic aim"));
    result.question_object = static_cast<character_pack::ResourceId>(integer(
        preview[@"question_object"], 1, pack.record_count(), "selection question object"));
    const auto question = pack.object(result.question_object);
    if (!question.model)
        throw std::runtime_error("imported selection question object has no model");
    (void)pack.model(question.model);
    result.question_menu_texture = static_cast<std::uint32_t>(integer(
        preview[@"question_menu_texture"], 2, 2, "selection question texture"));
    constexpr std::array<std::size_t, 6> counts{3, 3, 3, 3, 4, 3};
    constexpr std::array<std::array<std::uint32_t, 3>, 19> source_keys{{
        {0, 0, 5}, {0, 0, 100}, {1, 0, 0},
        {0, 2, 17}, {0, 2, 39}, {1, 2, 0},
        {0, 3, 81}, {0, 3, 40}, {1, 3, 0},
        {0, 5, 70}, {0, 5, 29}, {1, 5, 0},
        {0, 7, 16}, {0, 7, 29}, {0, 7, 30}, {1, 7, 0},
        {0, 6, 2}, {1, 6, 0}, {1, 6, 1}}};
    const auto sequences = array(preview[@"sequences"], 6, "selection action sequences");
    std::size_t action_index = 0;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        const auto sequence = dictionary(sequences[index], "selection sequence");
        auto &target = result.sequences[index];
        target.character = static_cast<std::uint16_t>(integer(
            sequence[@"character"], characters[index], characters[index], "sequence character"));
        const auto actions = array(sequence[@"actions"], counts[index], "selection actions");
        const auto frames = array(sequence[@"link_frames"], counts[index], "selection link frames");
        for (std::size_t item = 0; item < counts[index]; ++item, ++action_index) {
            SelectionActionStep step{};
            const auto key = array(actions[item], 3, "selection source action key");
            for (std::size_t part = 0; part < step.source_key.size(); ++part)
                step.source_key[part] = static_cast<std::uint32_t>(integer(
                    key[part], source_keys[action_index][part], source_keys[action_index][part],
                    "selection source action identity"));
            step.action = pack.action(static_cast<character_pack::SelectionAction>(action_index));
            (void)pack.resolve(step.action);
            step.link_frames = static_cast<std::int32_t>(
                integer(frames[item], 0, INT32_MAX, "selection action link frames"));
            target.steps.push_back(step);
        }
    }
    const auto menu = read_metadata(directory / "character-select-menu.json");
    const auto widths = array(menu[@"name_width_words"], 7, "selection name widths");
    const auto clear_flags = array(menu[@"clear_flags"], 7, "selection clear flags");
    for (std::size_t index = 0; index < result.name_widths.size(); ++index) {
        result.name_widths[index] = dialog_coordinate(widths[index]);
        if (result.name_widths[index] <= 0)
            throw std::runtime_error("invalid imported selection name width");
        result.clear_flags[index] = static_cast<std::uint32_t>(
            integer(clear_flags[index], 0, 511, "selection clear flag"));
    }
    const auto voices = array(menu[@"selected_voice_cues"], 8, "selected character voice cues");
    for (std::size_t index = 0; index < result.selected_voice_cues.size(); ++index)
        result.selected_voice_cues[index] = static_cast<std::int32_t>(
            integer(voices[index], 0, 2048, "selected character voice"));
    const auto definitions = array(menu[@"definitions"], 4, "character selection dialogs");
    constexpr std::array<std::int32_t, 4> choices{3, 4, 2, 3};
    for (std::size_t index = 0; index < result.definitions.size(); ++index) {
        const auto definition = dictionary(definitions[index], "character selection dialog");
        const auto identity = static_cast<std::int32_t>(index) + 7;
        (void)integer(definition[@"dialog"], identity, identity, "selection dialog identity");
        title_menu::DialogParameters parameters{};
        parameters.style = static_cast<title_menu::DialogStyle>(
            integer(definition[@"style"], 5, 5, "selection dialog style"));
        if (string(definition[@"content"], "selection dialog content") != "none")
            throw std::runtime_error("invalid imported selection dialog callback");
        parameters.content = title_menu::DialogContent::None;
        result.dialog_menu_texture = static_cast<std::uint32_t>(integer(
            definition[@"menu_texture"], 5, 5, "selection dialog menu texture"));
        parameters.choice_count = static_cast<std::int8_t>(integer(
            definition[@"choice_count"], choices[index], choices[index], "selection dialog choices"));
        parameters.cancel_choice = static_cast<std::int8_t>(integer(
            definition[@"cancel_choice"], choices[index] - 1, choices[index] - 1,
            "selection dialog cancel choice"));
        auto panels = std::make_shared<std::vector<title_menu::DialogPanel>>();
        for (id entry in array(definition[@"panels"], choices[index], "selection dialog panels")) {
            const auto panel = dictionary(entry, "selection dialog panel");
            const auto position = array(panel[@"position_words"], 2, "selection panel position");
            panels->push_back({dialog_coordinate(position[0]), dialog_coordinate(position[1]),
                               static_cast<std::uint8_t>(integer(panel[@"texture"], 0, UINT8_MAX,
                                                                 "selection panel texture"))});
        }
        parameters.panels = std::move(panels);
        const auto cues = array(definition[@"sounds"], 5, "selection dialog sounds");
        const auto cue = [&](std::size_t item) {
            return static_cast<std::int32_t>(
                integer(cues[item], INT32_MIN, INT32_MAX, "selection dialog sound"));
        };
        parameters.sounds = std::make_shared<title_menu::DialogSounds>(
            title_menu::DialogSounds{cue(0), cue(1), cue(2), cue(3), cue(4)});
        parameters.upper_color = static_cast<std::uint32_t>(
            integer(definition[@"upper_color"], 0, UINT32_MAX, "selection dialog upper color"));
        parameters.lower_color = static_cast<std::uint32_t>(
            integer(definition[@"lower_color"], 0, UINT32_MAX, "selection dialog lower color"));
        const auto geometry = array(definition[@"geometry_words"], 7, "selection dialog geometry");
        parameters.center_x = dialog_coordinate(geometry[0]);
        parameters.center_y = dialog_coordinate(geometry[1]);
        parameters.depth = dialog_coordinate(geometry[2]);
        parameters.width = dialog_coordinate(geometry[3]);
        parameters.height = dialog_coordinate(geometry[4]);
        parameters.scale_x = dialog_coordinate(geometry[5]);
        parameters.scale_y = dialog_coordinate(geometry[6]);
        result.definitions[index] = std::move(parameters);
    }
    return result;
}
} // namespace sadx::native::app

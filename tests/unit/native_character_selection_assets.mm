#include "apps/native_character_selection_assets.hpp"
#include "apps/native_voice_assets.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 3);
    @autoreleasepool {
        const std::filesystem::path source(argv[1]);
        const auto pack = sadx::character_pack::CharacterPack::load(source / "characters/sonic-selection.actions");
        const auto title = sadx::character_pack::CharacterPack::load(argv[2]);
        assert(pack.record_count() == 3302 && title.record_count() == 1157);
        for (sadx::character_pack::ResourceId id = 1; id <= title.record_count(); ++id) {
            const auto &actual = pack.record(id), &previous = title.record(id);
            assert(actual.kind == previous.kind && actual.payload == previous.payload);
        }
        for (unsigned index = 0; index < 7; ++index)
            assert(pack.action(static_cast<sadx::character_pack::TitleAction>(index)).identity ==
                   title.action(static_cast<sadx::character_pack::TitleAction>(index)).identity);
        bool title_only_rejected = false;
        try { sadx::native::app::load_character_selection_assets(source, title); }
        catch (const std::runtime_error &) { title_only_rejected = true; }
        assert(title_only_rejected);
        const auto assets = sadx::native::app::load_character_selection_assets(source, pack);
        assert((assets.characters == std::array<std::uint16_t, 7>{0, 2, 3, 5, 7, 6, 0}));
        assert(assets.sonic_aim_y == 16384 && assets.question_object == 2942);
        assert(assets.question_menu_texture == 2 && assets.dialog_menu_texture == 5);
        assert((assets.name_widths == std::array<float, 7>{143, 122, 227, 102, 89, 209, 230}));
        assert((assets.clear_flags == std::array<std::uint32_t, 7>{72, 73, 74, 75, 77, 76, 78}));
        assert((assets.selected_voice_cues == std::array<std::int32_t, 8>{1837, 1800, 1787, 1730, 1744, 1767, 396, 2047}));
        constexpr std::array<unsigned, 19> motion_frames{32, 65, 2, 16, 68, 2, 16, 75, 2, 32, 90, 2, 32, 25, 45, 2, 16, 80, 2};
        unsigned count = 0;
        for (const auto &sequence : assets.sequences) {
            for (const auto &step : sequence.steps) {
                assert(step.source_key[1] == sequence.character);
                assert(step.action.identity == pack.action(static_cast<sadx::character_pack::SelectionAction>(count)).identity);
                assert(pack.motion(pack.resolve(step.action).motion).frame_count == motion_frames[count]);
                ++count;
            }
        }
        assert(count == 19);
        for (unsigned id = 7; id <= 10; ++id) {
            const auto &dialog = assets.definition(id);
            assert(dialog.style == sadx::title_menu::DialogStyle::BlueFourth);
            assert(dialog.content == sadx::title_menu::DialogContent::None);
            assert(dialog.cancel_choice == dialog.choice_count - 1);
        }
        for (const auto id : {-1, 0, 2, 6, 11}) {
            bool rejected = false;
            try { assets.definition(id); }
            catch (const sadx::menu_dialog::Error &) { rejected = true; }
            assert(rejected);
        }
        const auto voices = sadx::native::app::load_voice_assets(source);
        assert(voices.clips.size() == 176);
        for (const auto language : {0, 1})
            for (const auto cue : assets.selected_voice_cues) {
                if (cue == 2047) {
                    assert(std::none_of(voices.clips.begin(), voices.clips.end(), [=](const auto &clip) {
                        return clip.language == language && clip.voice == cue;
                    }));
                    continue;
                }
                assert(std::any_of(voices.clips.begin(), voices.clips.end(), [=](const auto &clip) {
                    return clip.language == language && clip.voice == cue && clip.frames > 0;
                }));
            }
        const auto temporary = std::filesystem::temp_directory_path() /
            (std::string("sadx-selection-assets-") + [NSUUID.UUID.UUIDString UTF8String]);
        std::filesystem::create_directories(temporary / "characters");
        const auto original_preview = [NSData dataWithContentsOfFile:
            [NSString stringWithUTF8String:(source / "characters/selection-preview.json").c_str()]];
        const auto original_menu = [NSData dataWithContentsOfFile:
            [NSString stringWithUTF8String:(source / "character-select-menu.json").c_str()]];
        assert(original_preview && original_menu);
        for (unsigned test = 0; test != 28; ++test) {
            auto preview = (NSMutableDictionary *)[NSJSONSerialization JSONObjectWithData:
                original_preview options:NSJSONReadingMutableContainers error:nullptr];
            auto menu = (NSMutableDictionary *)[NSJSONSerialization JSONObjectWithData:
                original_menu options:NSJSONReadingMutableContainers error:nullptr];
            switch (test) {
            case 0: preview[@"pack_schema"] = @"SDXACT02"; break;
            case 1: preview[@"characters"][1] = @1; break;
            case 2: preview[@"actor_kinds"][0] = @"miles"; break;
            case 3: preview[@"positions_words"][0] = @[@0, @0]; break;
            case 4: preview[@"positions_words"][0][0] = @0x7fc00000U; break;
            case 5: preview[@"sequences"][0][@"actions"][0][2] = @6; break;
            case 6: [preview[@"sequences"][0][@"actions"] removeLastObject]; break;
            case 7: preview[@"sequences"][0][@"link_frames"][0] = @(-1); break;
            case 8: preview[@"question_object"] = @0xffffffffU; break;
            case 9: preview[@"question_object"] = @1; break;
            case 10: preview[@"question_menu_texture"] = @3; break;
            case 11: preview[@"sequences"][0][@"character"] = @2; break;
            case 12: preview[@"sonic_aim_y"] = @0.5; break;
            case 13: menu[@"name_width_words"][0] = @0x7f800000U; break;
            case 14: menu[@"name_width_words"][0] = @0; break;
            case 15: menu[@"clear_flags"][0] = @512; break;
            case 16: menu[@"selected_voice_cues"][0] = @2049; break;
            case 17: [menu[@"selected_voice_cues"] removeLastObject]; break;
            case 18: menu[@"definitions"][0][@"dialog"] = @6; break;
            case 19: menu[@"definitions"][0][@"style"] = @4; break;
            case 20: menu[@"definitions"][0][@"content"] = @"file_select_help"; break;
            case 21: menu[@"definitions"][0][@"menu_texture"] = @2; break;
            case 22: menu[@"definitions"][0][@"choice_count"] = @4; break;
            case 23: menu[@"definitions"][0][@"cancel_choice"] = @1; break;
            case 24: menu[@"definitions"][0][@"panels"][0][@"position_words"] = @[@0]; break;
            case 25: menu[@"definitions"][0][@"geometry_words"][0] = @0x7fc00000U; break;
            case 26: [menu[@"definitions"][0][@"sounds"] removeLastObject]; break;
            case 27: [menu[@"definitions"] removeLastObject]; break;
            }
            const auto preview_data = [NSJSONSerialization dataWithJSONObject:preview options:0 error:nullptr];
            const auto menu_data = [NSJSONSerialization dataWithJSONObject:menu options:0 error:nullptr];
            assert([preview_data writeToFile:[NSString stringWithUTF8String:
                (temporary / "characters/selection-preview.json").c_str()] atomically:YES]);
            assert([menu_data writeToFile:[NSString stringWithUTF8String:
                (temporary / "character-select-menu.json").c_str()] atomically:YES]);
            bool rejected = false;
            try { sadx::native::app::load_character_selection_assets(temporary, pack); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::cout << "CharacterSelect DTO: single 3302-record pack, title1157 prefix, 19 actions, 4 sparse dialogs, 176 real voices (cue2047 explicitly absent) and 28 malformed boundaries pass.\n";
    }
}

#include "apps/native_main_menu_assets.hpp"
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 2);
        using namespace sadx::native::app;
        const std::filesystem::path directory(argv[1]);
        const auto assets = load_main_menu_assets(directory);
        const std::array<std::int8_t, 3> counts{6, 5, 3};
        for (std::size_t index = 0; index < counts.size(); ++index) {
            const auto &definition = assets.definition(static_cast<std::int32_t>(index) + 2);
            assert(definition.style == sadx::title_menu::DialogStyle::CyanSecond);
            assert(definition.choice_count == counts[index]);
            assert(definition.cancel_choice == counts[index]);
            assert(definition.panels && definition.panels->size() == std::size_t(counts[index]));
            assert(definition.sounds && definition.sounds->open == 21);
            assert(definition.content == (index == 2 ? sadx::title_menu::DialogContent::TrialMenuBackdrop
                                                    : sadx::title_menu::DialogContent::MainMenuBackdrop));
        }
        assert((assets.variant_mapping == std::array<std::uint8_t, 7>{0, 1, 2, 3, 4, 5, 25}));
        assert(assets.trial_layout.offset_x == 10 && assets.trial_layout.offset_y == 10);
        bool missing = false;
        try { (void)assets.definition(1); }
        catch (const sadx::menu_dialog::Error &) { missing = true; }
        assert(missing);
        const std::string identity = [NSUUID.UUID.UUIDString UTF8String];
        const auto temporary = std::filesystem::temp_directory_path() / ("sadx-main-dialog-" + identity);
        std::filesystem::create_directory(temporary);
        NSData *original = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:
            (directory / "main-menu-dialog.json").c_str()]];
        assert(original);
        for (unsigned kind = 0; kind < 11; ++kind) {
            NSMutableDictionary *bad = [NSJSONSerialization JSONObjectWithData:original
                options:NSJSONReadingMutableContainers error:nullptr];
            assert(bad);
            NSMutableDictionary *first = bad[@"definitions"][0];
            switch (kind) {
            case 0: [bad[@"definitions"] removeLastObject]; break;
            case 1: first[@"style"] = @5; break;
            case 2: first[@"content"] = @"file_select_help"; break;
            case 3: first[@"dialog"] = @0; break;
            case 4: first[@"cancel_choice"] = @0; break;
            case 5: first[@"menu_texture"] = @18; break;
            case 6: first[@"panels"][0][@"position_words"][0] = @2139095040; break;
            case 7: bad[@"variant_mapping"][6] = @0; break;
            case 8: bad[@"trial_backdrop_layout_words"][0] = @4294967295U; break;
            case 9: first[@"sounds"] = @[@0]; break;
            case 10: bad[@"main_background_texture"] = @19; break;
            }
            NSData *data = [NSJSONSerialization dataWithJSONObject:bad options:0 error:nullptr];
            assert(data && [data writeToFile:[NSString stringWithUTF8String:
                (temporary / "main-menu-dialog.json").c_str()] atomically:YES]);
            bool rejected = false;
            try { (void)load_main_menu_assets(temporary); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::cout << "MainMenu source descriptors and11 malformed resource boundaries pass.\n";
    }
}

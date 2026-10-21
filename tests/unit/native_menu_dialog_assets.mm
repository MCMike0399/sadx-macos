#include "apps/native_menu_dialog_assets.hpp"
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        using namespace sadx::native::app;
        const auto assets = load_menu_dialog_assets(argv[1]);
        assert(assets.definitions.size() == 1 && assets.panel_texture_bindings[0] == 5);
        const auto &dialog = assets.definitions[0];
        assert(dialog.center_x == 320 && dialog.center_y == 357 && dialog.depth == 20);
        assert(dialog.width == 568 && dialog.height == 80);
        assert(dialog.choice_count == 3 && dialog.cancel_choice == 1);
        assert(dialog.panels && dialog.panels->size() == 3);
        assert((*dialog.panels)[0].x == -155 && (*dialog.panels)[2].x == 155);
        assert(dialog.sounds && dialog.sounds->open == 21 && dialog.sounds->close == 22);
        assert(assets.cursor_texture_binding == 4);
        // The original pointer identity survives import even for empty strings.
        for (const auto &choice : assets.help) {
            assert(choice[0] == assets.help[0][0]);
            assert(choice[1] == assets.help[0][1]);
            assert(choice[0] == choice[1] && choice[1]->empty());
        }
        const auto rejects = [](const auto &operation) {
            bool rejected = false;
            try { operation(); } catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        };
        rejects([] { (void)dialog_coordinate(@2139095040U); }); // binary32 infinity
        rejects([] { (void)dialog_message(@"0"); });
        rejects([] { (void)dialog_message(@"gg"); });
        rejects([] { (void)dialog_message(@"00"); });
        const auto bytes = dialog_message(@"8140ff");
        assert(bytes.size() == 3 && static_cast<unsigned char>(bytes[0]) == 129);
    }
    std::cout << "Imported dialog geometry, sound cues, source message aliases and invalid data checks pass\n";
}

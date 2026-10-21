#include "apps/native_character_select_label_assets.hpp"
#include "game/character_select_labels.hpp"
#include <cassert>
#include <iostream>
struct Calls final : sadx::character_select_labels::Calls {
    unsigned selects = 0, materials = 0, draws = 0;
    void select_texture(sadx::character_select_labels::Texture) override { ++selects; }
    void material(std::uint32_t) override { ++materials; }
    void draw_texture(std::uint32_t, float, float, float) override { ++draws; }
    void draw_scaled(std::uint32_t, float, float, float, float, float) override { ++draws; }
};
int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        const std::filesystem::path source(argv[1]);
        const auto labels = sadx::native::app::load_character_label_textures(source);
        assert((labels == std::array<std::uint8_t, 7>{0, 1, 2, 3, 4, 5, 6}));
        const std::array<std::uint8_t, 7> names{6, 5, 4, 3, 2, 1, 0};
        const float horizontal = 1, vertical = 1;
        const std::int32_t language = 0;
        const std::uint8_t variant = 0;
        sadx::character_select_labels::State state{horizontal, vertical, language, variant, labels, names};
        for (const auto index : {-1, 7, INT32_MAX}) {
            for (const bool name : {false, true}) {
                sadx::X87Context fp{127}; Calls calls; bool rejected = false;
                try {
                    if (name) sadx::character_select_labels::draw_name(index, 0xffffffffU, 320, 240, 20, 1, 1, state, calls, fp);
                    else sadx::character_select_labels::draw_label(index, 0xffffffffU, 320, 240, 20, state, calls, fp);
                } catch (const sadx::character_select_labels::Error &error) {
                    assert(error.index() == index);
                    assert(error.boundary() == (name ? sadx::character_select_labels::Boundary::NameIndex : sadx::character_select_labels::Boundary::LabelIndex));
                    rejected = true;
                }
                assert(rejected && calls.selects == 1 && calls.materials == 1 && calls.draws == 0);
            }
        }
        const auto temporary = std::filesystem::temp_directory_path() /
            (std::string("sadx-label-assets-") + [NSUUID.UUID.UUIDString UTF8String]);
        std::filesystem::create_directories(temporary);
        const auto original = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:
            (source / "character-select-labels.json").c_str()]];
        assert(original);
        for (unsigned test = 0; test != 5; ++test) {
            auto metadata = (NSMutableDictionary *)[NSJSONSerialization JSONObjectWithData:
                original options:NSJSONReadingMutableContainers error:nullptr];
            if (test == 0) [metadata[@"label_textures"] removeLastObject];
            else if (test == 1) metadata[@"label_textures"][0] = @256;
            else if (test == 2) metadata[@"label_textures"][0] = @(-1);
            else if (test == 3) metadata[@"menu_textures"][@"labels"] = @15;
            else metadata[@"menu_textures"][@"metal_name"] = @14;
            const auto data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nullptr];
            assert([data writeToFile:[NSString stringWithUTF8String:
                (temporary / "character-select-labels.json").c_str()] atomically:YES]);
            bool rejected = false;
            try { sadx::native::app::load_character_label_textures(temporary); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::cout << "Six indexed-read boundaries and five malformed label metadata cases pass.\n";
    }
}

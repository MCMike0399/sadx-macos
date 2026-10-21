#include "apps/native_character_completion_assets.hpp"
#include <cassert>
#include <iostream>
namespace c = sadx::character_completion;
int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        const std::filesystem::path source(argv[1]);
        const auto catalog = sadx::native::app::load_character_completion_assets(source);
        constexpr std::array<std::size_t, 7> counts{19, 14, 10, 10, 11, 10, 1};
        std::array<std::uint8_t, 512> story{}, cutscene{};
        for (std::size_t character = 0; character < catalog.size(); ++character) {
            assert(catalog[character].size() == counts[character]);
            assert(c::value(character, catalog, {story, cutscene}) == 0);
        }
        cutscene[50] = 128;
        assert(c::value(1, catalog, {story, cutscene}) == 5);
        cutscene[50] = 0;
        story[50] = 255;
        assert(c::value(1, catalog, {story, cutscene}) == 0);
        story.fill(1); cutscene.fill(255);
        for (std::size_t character = 0; character < catalog.size(); ++character)
            assert(c::value(character, catalog, {story, cutscene}) == 100);
        const auto before_story = story, before_cutscene = cutscene;
        c::Catalog edited{};
        edited[0] = {{90, c::FlagSource::Story, 0}, {5, c::FlagSource::Cutscene, 0}};
        assert(c::value(0, edited, {story, cutscene}) == 5);
        edited[0].push_back({0, c::FlagSource::Story, 0});
        assert(c::value(0, edited, {story, cutscene}) == 0);
        edited[0].push_back({-7, c::FlagSource::Cutscene, 0});
        assert(c::value(0, edited, {story, cutscene}) == -7);
        edited[0].push_back({100, static_cast<c::FlagSource>(2), -1});
        assert(c::value(0, edited, {story, cutscene}) == -7);
        assert(story == before_story && cutscene == before_cutscene);
        for (const auto character : {-1, 7}) {
            bool rejected = false;
            try { c::value(character, catalog, {story, cutscene}); }
            catch (const c::Error &error) {
                assert(error.boundary() == c::Boundary::Character && error.index() == character);
                rejected = true;
            }
            assert(rejected);
        }
        for (const auto kind : {c::FlagSource::Story, c::FlagSource::Cutscene}) {
            for (const auto index : {-1, 512}) {
                edited[0] = {{100, kind, index}}; bool rejected = false;
                try { c::value(0, edited, {story, cutscene}); }
                catch (const c::Error &error) {
                    assert(error.boundary() == (kind == c::FlagSource::Story ? c::Boundary::StoryFlag : c::Boundary::CutsceneFlag));
                    assert(error.index() == index); rejected = true;
                }
                assert(rejected);
            }
        }
        const auto temporary = std::filesystem::temp_directory_path() /
            (std::string("sadx-completion-assets-") + [NSUUID.UUID.UUIDString UTF8String]);
        std::filesystem::create_directories(temporary);
        const auto original = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:
            (source / "character-completion.json").c_str()]];
        assert(original);
        for (unsigned test = 0; test != 7; ++test) {
            auto metadata = (NSMutableDictionary *)[NSJSONSerialization JSONObjectWithData:
                original options:NSJSONReadingMutableContainers error:nullptr];
            if (test == 0) [metadata[@"catalogs"] removeLastObject];
            else if (test == 1) metadata[@"catalogs"][0][@"selection"] = @1;
            else if (test == 2) [metadata[@"catalogs"][0][@"rows"] removeLastObject];
            else if (test == 3) metadata[@"catalogs"][0][@"rows"][0][@"flag"] = @512;
            else if (test == 4) metadata[@"catalogs"][0][@"rows"][0][@"flag"] = @(-1);
            else if (test == 5) metadata[@"catalogs"][0][@"rows"][0][@"source"] = @"story";
            else metadata[@"catalogs"][0][@"rows"][0][@"value"] = @2147483648LL;
            const auto data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nullptr];
            assert([data writeToFile:[NSString stringWithUTF8String:
                (temporary / "character-completion.json").c_str()] atomically:YES]);
            bool rejected = false;
            try { sadx::native::app::load_character_completion_assets(temporary); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::cout << "Completion source catalogs/last-match/live flags, six indexed-read boundaries and seven malformed metadata cases pass.\n";
    }
}

#include "apps/native_music_assets.hpp"
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 2);
        using namespace sadx::native::app;
        // Keep the exact legacy two-clip judge over a bounded view of production imports.
        const std::filesystem::path imported(argv[1]);
        const std::string fixture_identity = [NSUUID.UUID.UUIDString UTF8String];
        const auto source = std::filesystem::temp_directory_path() /
                            ("sadx-two-music-clips-" + fixture_identity);
        std::filesystem::create_directories(source);
        std::filesystem::create_directory_symlink(std::filesystem::canonical(imported / "music"),
                                                   source / "music");
        NSMutableDictionary *fixture = [NSJSONSerialization JSONObjectWithData:
            [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:
                (imported / "music-assets.json").c_str()]]
            options:NSJSONReadingMutableContainers error:nullptr];
        assert(fixture);
        NSMutableArray *legacy_clips = [NSMutableArray array];
        for (id clip in fixture[@"clips"])
            if ([clip[@"song"] intValue] == 102 || [clip[@"song"] intValue] == 72)
                [legacy_clips addObject:clip];
        fixture[@"clips"] = legacy_clips;
        const auto fixture_data = [NSJSONSerialization dataWithJSONObject:fixture options:0 error:nullptr];
        assert([fixture_data writeToFile:[NSString stringWithUTF8String:
            (source / "music-assets.json").c_str()] atomically:YES]);
        const auto assets = load_music_assets(source);
        assert(assets.catalog.menu_requests.at(0) == 102);
        assert(assets.catalog.songs[102].name == "title2");
        assert(assets.clips.size() == 2 && assets.clips[0].song == 102);
        assert(assets.clips[1].song == 72);
        assert((assets.catalog.menu_requests == std::vector<std::int32_t>{102, 101, 72}));
        assert(assets.catalog.songs[72].name == "option" && assets.catalog.songs[72].loop == 1);
        const std::string identity = [NSUUID.UUID.UUIDString UTF8String];
        const auto temporary = std::filesystem::temp_directory_path() /
                               ("sadx-music-assets-" + identity);
        std::filesystem::create_directories(temporary);
        std::filesystem::create_directory_symlink(std::filesystem::canonical(source / "music"),
                                                   temporary / "music");
        const auto url = [NSURL fileURLWithPath:
            [NSString stringWithUTF8String:(source / "music-assets.json").c_str()]];
        const auto original = [NSData dataWithContentsOfURL:url];
        assert(original);
        for (unsigned test = 0; test != 8; ++test) {
            auto metadata = (NSMutableDictionary *)[NSJSONSerialization
                JSONObjectWithData:original options:NSJSONReadingMutableContainers error:nullptr];
            assert(metadata);
            if (test == 0)
                metadata[@"catalog"][@"songs"][102][@"name"] = @"../title2";
            else if (test == 1)
                metadata[@"clips"][0][@"source_path"] = @"other.wma";
            else if (test == 2)
                metadata[@"catalog"][@"menu_requests"][0] = @125;
            else if (test == 3)
                metadata[@"clips"][0][@"pcm_path"] = @"music/../../outside.wav";
            else if (test == 4)
                metadata[@"catalog"][@"menu_requests"][2] = @125;
            else if (test == 5)
                metadata[@"clips"][1][@"source_path"] = @"other.wma";
            else if (test == 6)
                metadata[@"clips"][1][@"pcm_path"] = @"music/../../outside.wav";
            else
                metadata[@"clips"][1] = metadata[@"clips"][0];
            const auto data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nullptr];
            assert(data);
            assert([data writeToFile:[NSString stringWithUTF8String:
                (temporary / "music-assets.json").c_str()] atomically:YES]);
            bool rejected = false;
            try {
                load_music_assets(temporary);
            } catch (const std::runtime_error &) {
                rejected = true;
            }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::filesystem::remove_all(source);
        std::cout << "Imported title/option music catalog and eight malformed resource boundaries passed.\n";
    }
}

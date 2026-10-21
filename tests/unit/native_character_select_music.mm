#include "apps/native_music_assets.hpp"
#include "platform/mac/mac_audio_playback.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        const std::filesystem::path source(argv[1]);
        const auto assets = sadx::native::app::load_music_assets(source);
        assert((assets.catalog.menu_requests == std::vector<std::int32_t>{102, 101, 72, 24, 103}));
        assert(assets.clips.size() == 5);
        assert(assets.catalog.songs[24].name == "charactr" && assets.catalog.songs[24].loop == 1);
        assert(assets.catalog.songs[103].name == "trial" && assets.catalog.songs[103].loop == 1);
        for (const auto song : {24, 103}) {
            const auto found = std::find_if(assets.clips.begin(), assets.clips.end(),
                                           [song](const auto &clip) { return clip.song == song; });
            assert(found != assets.clips.end());
            assert(found->frames == (song == 24 ? 8380416U : 6834176U));
            std::string error;
            auto player = sadx::audio::mac::StreamPlayer::create({}, error);
            assert(player && player->ready());
            assert(player->prepare_clip((source / found->pcm_path).string(), error));
            assert(player->set_output_gain(0.05f));
            assert(player->play(-3000, true, error));
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
            while ((player->stats().completed_buffers < 4 || !player->stats().nonzero_samples) &&
                   std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            const auto stats = player->stats();
            assert(stats.completed_buffers >= 4 && stats.nonzero_samples && !stats.device_error);
            const auto progress = player->progress();
            assert(progress && progress.running && !progress.complete);
            player->stop();
            player->reset();
            std::cout << "{\"song\":" << song << ",\"frames\":" << found->frames
                      << ",\"nonzero_samples\":" << stats.nonzero_samples
                      << ",\"completed_buffers\":" << stats.completed_buffers
                      << ",\"device_error\":" << stats.device_error << "}\n";
        }
        const auto temporary = std::filesystem::temp_directory_path() /
            (std::string("sadx-character-music-") + [NSUUID.UUID.UUIDString UTF8String]);
        std::filesystem::create_directories(temporary);
        std::filesystem::create_directory_symlink(std::filesystem::canonical(source / "music"),
                                                   temporary / "music");
        const auto original = [NSData dataWithContentsOfFile:
            [NSString stringWithUTF8String:(source / "music-assets.json").c_str()]];
        assert(original);
        for (unsigned test = 0; test != 8; ++test) {
            auto metadata = (NSMutableDictionary *)[NSJSONSerialization
                JSONObjectWithData:original options:NSJSONReadingMutableContainers error:nullptr];
            if (test == 0) metadata[@"catalog"][@"menu_requests"][3] = @125;
            else if (test == 1) metadata[@"catalog"][@"menu_requests"][4] = @(-1);
            else if (test == 2) metadata[@"clips"][3][@"source_path"] = @"other.wma";
            else if (test == 3) metadata[@"clips"][4][@"pcm_path"] = @"music/../../outside.wav";
            else if (test == 4) metadata[@"clips"][4] = metadata[@"clips"][3];
            else if (test == 5) metadata[@"clips"][3][@"frames"] = @0;
            else if (test == 6) metadata[@"clips"][4][@"pcm_path"] = @"music/missing.wav";
            else metadata[@"catalog"][@"menu_requests"] =
                [metadata[@"catalog"][@"menu_requests"] subarrayWithRange:NSMakeRange(0, 4)];
            const auto data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nullptr];
            assert([data writeToFile:[NSString stringWithUTF8String:
                (temporary / "music-assets.json").c_str()] atomically:YES]);
            bool rejected = false;
            try { sadx::native::app::load_music_assets(temporary); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::cout << "CharacterSelect source music and eight malformed boundaries passed.\n";
    }
}

#include "apps/native_music_assets.hpp"
#include "platform/mac/mac_audio_playback.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        const std::filesystem::path imported(argv[1]);
        const std::string fixture_identity = [NSUUID.UUID.UUIDString UTF8String];
        const auto source = std::filesystem::temp_directory_path() /
                            ("sadx-three-music-clips-" + fixture_identity);
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
            if ([clip[@"song"] intValue] == 102 || [clip[@"song"] intValue] == 72 ||
                [clip[@"song"] intValue] == 101)
                [legacy_clips addObject:clip];
        fixture[@"clips"] = legacy_clips;
        fixture[@"catalog"][@"menu_requests"] =
            [fixture[@"catalog"][@"menu_requests"] subarrayWithRange:NSMakeRange(0, 3)];
        const auto fixture_data = [NSJSONSerialization dataWithJSONObject:fixture options:0 error:nullptr];
        assert([fixture_data writeToFile:[NSString stringWithUTF8String:
            (source / "music-assets.json").c_str()] atomically:YES]);
        const auto assets = sadx::native::app::load_music_assets(source);
        assert((assets.catalog.menu_requests == std::vector<std::int32_t>{102, 101, 72}));
        assert(assets.clips.size() == 3);
        assert(assets.clips[0].song == 102 && assets.clips[1].song == 72);
        const auto &clip = assets.clips[2];
        assert(clip.song == 101 && clip.frames == 2541568);
        assert(assets.catalog.songs[101].name == "title" && assets.catalog.songs[101].loop == 0);
        assert(clip.source_path == "SYSTEM\\SoundData\\BGM\\WMA\\title.wma");
        assert(clip.pcm_path == "music/101-title.wav");
        const std::string identity = [NSUUID.UUID.UUIDString UTF8String];
        const auto temporary = std::filesystem::temp_directory_path() /
                               ("sadx-main-menu-music-" + identity);
        std::filesystem::create_directories(temporary);
        std::filesystem::create_directory_symlink(std::filesystem::canonical(source / "music"),
                                                   temporary / "music");
        const auto original = [NSData dataWithContentsOfFile:
            [NSString stringWithUTF8String:(source / "music-assets.json").c_str()]];
        assert(original);
        for (unsigned test = 0; test != 4; ++test) {
            auto metadata = (NSMutableDictionary *)[NSJSONSerialization
                JSONObjectWithData:original options:NSJSONReadingMutableContainers error:nullptr];
            if (test == 0) metadata[@"clips"][2][@"source_path"] = @"other.wma";
            else if (test == 1) metadata[@"clips"][2][@"pcm_path"] = @"music/../../outside.wav";
            else if (test == 2) metadata[@"clips"][2] = metadata[@"clips"][0];
            else metadata[@"clips"][2][@"frames"] = @0;
            const auto data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nullptr];
            assert([data writeToFile:[NSString stringWithUTF8String:
                (temporary / "music-assets.json").c_str()] atomically:YES]);
            bool rejected = false;
            try { sadx::native::app::load_music_assets(temporary); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
        }
        std::filesystem::remove_all(temporary);
        std::string error;
        auto player = sadx::audio::mac::StreamPlayer::create({}, error);
        assert(player && player->ready());
        assert(player->prepare_clip((source / clip.pcm_path).string(), error));
        assert(player->set_output_gain(0.05f));
        assert(player->play(-3000, false, error));
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while ((player->stats().completed_buffers < 4 || !player->stats().nonzero_samples) &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const auto before = player->stats();
        const auto progress = player->progress();
        assert(before.nonzero_samples && before.completed_buffers >= 4 && !before.device_error);
        assert(progress && progress.running && !progress.complete);
        // A failed reopen must keep the active stream and its buffer ownership.
        assert(!player->prepare_clip((source / "music/missing.wav").string(), error));
        assert(player->buffer_available() && player->clip_open());
        const auto preserved = player->progress();
        assert(preserved && preserved.running && !preserved.complete);
        player->stop();
        assert(!player->buffer_available() && player->clip_open());
        player->reset();
        assert(!player->clip_open());
        assert(!player->prepare_clip((source / "music/missing.wav").string(), error));
        assert(!player->buffer_available() && !player->clip_open());
        std::filesystem::remove_all(source);
        std::cout << "{\"song\":101,\"frames\":" << clip.frames
                  << ",\"nonzero_samples\":" << before.nonzero_samples
                  << ",\"completed_buffers\":" << before.completed_buffers
                  << ",\"device_error\":" << before.device_error
                  << ",\"third_clip_rejections\":4,\"output_gain\":0.05}\n";
    }
}

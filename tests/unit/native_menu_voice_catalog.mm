#include "apps/native_file_select_assets.hpp"
#include "apps/native_voice_assets.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        using namespace sadx;
        auto catalog = native::app::load_menu_voice_assets(argv[1]);
        const auto clips = native::app::load_voice_assets(argv[1]);
        // Keep this fixture scoped to all81 original menu requests as other
        // reached source callers add their own voice resources to the install.
        const auto menu_clips = std::count_if(clips.clips.begin(), clips.clips.end(),
            [&](const auto &clip) {
                return std::any_of(catalog.begin(), catalog.end(),
                    [&](const auto &entry) { return entry.voice == clip.voice; });
            });
        assert(catalog.size() == 81 && menu_clips == 162);
        const auto expected = catalog;
        std::reverse(catalog.begin(), catalog.end()); // Lookup must use row/request, not vector position.
        TaskScheduler scheduler;
        TaskHandle canonical;
        menu_voice::TaskServices calls{
            [&] { return scheduler.append_task(4, nullptr, {}).task; },
            [](const TaskHandle &) { return std::make_shared<menu_voice::RequestWork>(); },
            [&](const TaskHandle &task) { assert(scheduler.destroy_task(task)); }
        };
        cleanup::TrackState tracks;
        const std::int32_t enabled = 1, ready = 1;
        for (const auto &entry : expected) {
            assert(menu_voice::request_character_voice(entry.request, entry.character,
                                                        catalog, canonical, calls) == 1);
            const auto task = canonical;
            const auto work = std::static_pointer_cast<menu_voice::RequestWork>(task->auxiliary_native_work);
            assert(work && work->voice == entry.voice);
            if (entry.character == 1 && entry.request == 1) assert(entry.voice == 37);
            for (std::int32_t language = 0; language < 2; ++language) {
                const auto clip = std::find_if(clips.clips.begin(), clips.clips.end(), [&](const auto &item) {
                    return item.language == language && item.voice == entry.voice;
                });
                assert(clip != clips.clips.end() && clip->frames != 0);
                assert(std::filesystem::is_regular_file(std::filesystem::path(argv[1]) / clip->pcm_path));
            }
            menu_voice::update_request(task, canonical, tracks, enabled, ready, calls);
            menu_voice::update_request(task, canonical, tracks, enabled, ready, calls);
            assert(!canonical && tracks.current_voice == entry.voice);
        }
    }
    std::cout << "All81 native menu requests select correctly from the full catalog and bind162 actual clips\n";
}

#include "game/music.hpp"
#include "game/menu_voice.hpp"
#include <cassert>
#include <vector>

namespace sadx {
class TaskSourceProbe {
  public:
    static TaskHandle make() { return TaskHandle(new Task); }
};
}

namespace {
using namespace sadx;
struct ManagerCalls final : title_menu::SoundManagerCalls {
    music::SchedulingState &state;
    std::vector<std::uint32_t> closed;
    explicit ManagerCalls(music::SchedulingState &value) : state(value) {}
    void destroy_sound_manager(const TaskHandle &task) override {
        assert(task == state.manager);
        title_menu::delete_sound_manager(state.manager, state.tracks, *this);
    }
    void close_media(std::uint32_t mode) override {
        assert(state.manager && state.tracks.real_previous_song == 123);
        closed.push_back(mode);
    }
};

void manager_asymmetry() {
    music::SchedulingState state;
    state.tracks = {11, 12, 123, 14, 15, 16};
    state.voice_timing = {17, 18};
    state.manager = TaskSourceProbe::make();
    ManagerCalls calls(state);
    title_menu::close_sound_manager(state.manager, calls);
    assert(!state.manager && (calls.closed == std::vector<std::uint32_t>{0, 1}));
    assert(state.tracks.current_song == -1 && state.tracks.previous_song == -1);
    assert(state.tracks.current_voice == -1 && state.tracks.real_previous_song == 123);
    assert(state.tracks.song_queue_state == 0 && state.tracks.voice_queue_state == 0);
    assert(state.voice_timing.duration == 17 && state.voice_timing.started == 18);
    music::TaskServices services{};
    unsigned creations = 0;
    services.create_manager = [&] {
        ++creations;
        assert(state.tracks.current_song == -1 && state.tracks.previous_song == -1);
        assert(state.tracks.real_previous_song == -1 && state.tracks.current_voice == -1);
        assert(state.tracks.song_queue_state == 0 && state.tracks.voice_queue_state == 0);
        assert(state.voice_timing.duration == 17 && state.voice_timing.started == 18);
        return TaskHandle{};
    };
    state.tracks = {21, 22, 123, 24, 25, 26};
    music::initialize_manager(state.tracks, services);
    assert(creations == 1);
}

void request_publication() {
    music::SchedulingState state;
    title_menu::MenuSoundState menu;
    auto task = TaskSourceProbe::make();
    unsigned creates = 0, allocations = 0, destroys = 0;
    music::TaskServices services{};
    services.close_manager = [] {};
    services.create_manager = [] { return TaskHandle{}; };
    services.create_request = [&] {
        assert(!menu.music_task);
        ++creates;
        return task;
    };
    services.allocate_request = [&](const TaskHandle &value) {
        assert(value == task && menu.music_task == task);
        assert(!task->auxiliary_native_work);
        ++allocations;
        return std::make_shared<music::RequestWork>();
    };
    services.destroy = [&](const TaskHandle &value) {
        assert(value == task && menu.music_task == task);
        assert(state.tracks.current_song == 102 && state.tracks.previous_song == 102);
        ++destroys;
    };
    music::request_song(102, menu, state.tracks, services);
    assert(creates == 1 && allocations == 1);
    const auto work = std::static_pointer_cast<music::RequestWork>(task->auxiliary_native_work);
    assert(work && work->song == 102 && work->elapsed == 0);
    std::int32_t enabled = 1, ready = 1;
    music::update_request(task, menu, state.tracks, enabled, ready, services);
    assert(destroys == 0 && work->elapsed == 1);
    ready = 0;
    music::update_request(task, menu, state.tracks, enabled, ready, services);
    assert(destroys == 0 && work->elapsed == 2);
    ready = 1;
    music::update_request(task, menu, state.tracks, enabled, ready, services);
    assert(destroys == 1 && !menu.music_task);
    // The delayed voice slot and scene music request remain distinct canonical handles.
    assert(!menu.volume_task && !state.manager);
}
} // namespace

int main() {
    manager_asymmetry();
    request_publication();
}

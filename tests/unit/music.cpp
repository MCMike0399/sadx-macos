#include "game/music.hpp"
#include <cassert>
#include <limits>
#include <vector>

namespace sadx {
class TaskSourceProbe {
  public:
    static TaskHandle make() { return TaskHandle(new Task); }
};
}
namespace {
void callback(sadx::TaskScheduler &,const sadx::TaskHandle &) {}
struct Player final : sadx::audio::StreamPlayer {
    std::function<void(bool)> operation;
    explicit Player(std::function<void(bool)> value):operation(std::move(value)) {}
    void stop() override {operation(false);}
    void reset() override {operation(true);}
};
}
int main() {
    using namespace sadx;
    music::Catalog catalog{};
    catalog.songs[102]={"title2",0};catalog.songs[101]={"title",1};
    catalog.menu_requests={102};
    catalog.japanese_voice_durations[2]=41;catalog.english_voice_durations[2]=57;
    cleanup::TrackState tracks{};
    title_menu::MenuSoundState menu{};
    TaskHandle manager;
    auto request=TaskSourceProbe::make(),new_manager=TaskSourceProbe::make();
    unsigned destroyed=0,allocated=0,manager_created=0;
    music::TaskServices tasks{
        [&](const TaskHandle &task) {assert(task==request);++destroyed;},
        [&] {assert(tracks.current_song==-1 && tracks.current_voice==-1);++manager_created;return new_manager;},
        [&](const TaskHandle &task) {assert(manager==task);task->exec=callback;task->destructor=callback;},
        [&] {return request;},
        [&](const TaskHandle &task) {assert(menu.music_task==task);++allocated;return std::make_shared<music::RequestWork>();},
        [&] {manager.reset();}};
    music::request_menu_music(0,catalog,menu,tracks,tasks);
    assert(menu.music_task==request && manager_created==1 && allocated==1);
    auto work=std::static_pointer_cast<music::RequestWork>(request->auxiliary_native_work);
    assert(work && work->song==102 && work->elapsed==0);
    music::initialize_manager_task(new_manager,manager,tasks);
    assert(manager==new_manager && manager->exec==callback && manager->destructor==callback);
    std::int32_t enabled=1,ready=1;
    music::update_request(request,menu,tracks,enabled,ready,tasks);
    assert(work->elapsed==1 && tracks.current_song==-1 && destroyed==0);
    music::update_request(request,menu,tracks,enabled,ready,tasks);
    assert(!menu.music_task && destroyed==1 && tracks.current_song==102 && tracks.previous_song==102);
    work->elapsed=std::numeric_limits<std::int32_t>::max();ready=0;
    music::update_request(request,menu,tracks,enabled,ready,tasks);
    assert(work->elapsed==std::numeric_limits<std::int32_t>::min());
    enabled=0;music::play_song(101,tracks,enabled);assert(tracks.current_song==102);

    music::VoiceTiming voice_timing{};
    bool music_enabled=true,voice_enabled=true;
    std::int32_t language=0;
    music::ManagerState state{tracks,voice_timing,music_enabled,voice_enabled,language};
    std::vector<std::string> paths;
    unsigned children=0,music_calls=0;
    music::ManagerServices manager_calls{
        [] {return 1;},
        [&](std::string_view path,std::int32_t loop) {
            paths.emplace_back(path);
            if(++music_calls==1){assert(loop==0);tracks.current_song=101;return 0;}
            assert(loop==1);return 1;
        },[] {},[] {return 3;},
        [&](std::string_view path) {paths.emplace_back(path);tracks.current_voice=2;language=1;return 1;},
        [] {return 5;},[] {return 100;},[&](const TaskHandle &) {++children;}};
    tracks.real_previous_song=-1;tracks.current_voice=-1;
    music::update_manager(manager,state,catalog,"D:\\",manager_calls);
    assert((paths==std::vector<std::string>{"SYSTEM\\SoundData\\BGM\\WMA\\title2.wma","D:\\SoundData\\BGM\\WMA\\title.wma"}));
    assert(tracks.real_previous_song==101 && tracks.song_queue_state==3 && children==1);
    paths.clear();tracks.current_voice=1;language=0;
    music::update_manager(manager,state,catalog,"D:\\",manager_calls);
    assert(paths[0]=="SYSTEM\\SoundData\\VOICE_JP\\WMA\\0001.wma");
    assert(voice_timing.duration==41 && voice_timing.started==100 && tracks.current_voice==-1 && tracks.voice_queue_state==3);
    assert(music::query_music_state(tracks)==3);

    audio::MediaState media{};
    std::vector<std::string> operations;
    bool buffer=false,clip_open=false;
    std::int32_t attenuation=-7400,reader_state=2;
    media.music_player=std::make_unique<Player>([&](bool reset){operations.push_back(reset?"reset":"stop");buffer=false;if(reset)clip_open=false;});
    music::MediaServices backend{
        [&](std::string_view path) {assert(path=="title2.wma");operations.emplace_back("open");clip_open=true;return 0;},
        [&](std::int32_t volume) {assert(clip_open && volume==attenuation);operations.emplace_back("start");buffer=true;reader_state=1;},
        [&] {return reader_state;},[&] {return buffer;},
        [&](std::int32_t volume) {assert(volume==attenuation);operations.emplace_back("volume");},
        [&] {return reader_state;},[] {assert(false);}};
    assert(music::play_music_file("title2.wma",0,media,attenuation,backend)==1);
    assert((operations==std::vector<std::string>{"stop","reset","open","start"}));
    assert(media.music_open==1 && media.music_status==3);
    operations.clear();reader_state=2;
    assert(music::restart_music(media,attenuation,backend)==0);
    assert((operations==std::vector<std::string>{"stop","reset"}));
    assert(media.music_status==5 && !clip_open && !buffer);
    operations.clear();
    music::play_music_file("title2.wma",1,media,attenuation,backend);
    operations.clear();reader_state=2;
    assert(music::restart_music(media,attenuation,backend)==1);
    assert((operations==std::vector<std::string>{"stop","start"}) && clip_open && buffer);
    operations.clear();attenuation=-3300;
    music::restart_music(media,attenuation,backend);
    assert((operations==std::vector<std::string>{"volume"}));
    assert(music::music_status(media)==3 && music::voice_status(media,backend)==5);
    media.voice_player=std::make_unique<Player>([](bool){});
    assert(music::voice_status(media,backend)==3);reader_state=2;
    assert(music::voice_status(media,backend)==5);
    backend.open_music=[&](std::string_view){return -1;};
    assert(music::play_music_file("title2.wma",0,media,attenuation,backend)==0);
    assert(media.music_open==1 && media.music_looping==1); // failed open preserves source flags
}

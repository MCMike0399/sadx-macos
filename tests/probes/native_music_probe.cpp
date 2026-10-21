#include "game/music.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>

namespace sadx {
class TaskSourceProbe {
  public:
    static TaskHandle make() { return TaskHandle(new Task); }
};
}
namespace {
using namespace sadx;
struct Stop {};
music::Catalog catalog() {
    music::Catalog value{};
    for (std::size_t i=0;i<value.songs.size();++i) value.songs[i]={"song"+std::to_string(i),static_cast<std::int32_t>(i%3)-1};
    for (std::size_t i=0;i<2049;++i) {
        value.japanese_voice_durations[i]=static_cast<std::uint16_t>(i*17+7);
        value.english_voice_durations[i]=static_cast<std::uint16_t>(i*23+11);
    }
    value.menu_requests={102,101,72,24,103};
    return value;
}
void copy_trace(const std::string &value,char *trace,std::size_t capacity) {
    if(value.size()+1>capacity) throw std::runtime_error("probe trace capacity");
    std::memcpy(trace,value.c_str(),value.size()+1);
}
std::array<std::uint32_t,6> track_words(const cleanup::TrackState &t) {
    return {static_cast<std::uint32_t>(t.current_song),static_cast<std::uint32_t>(t.previous_song),
            static_cast<std::uint32_t>(t.real_previous_song),static_cast<std::uint32_t>(t.current_voice),
            t.song_queue_state,t.voice_queue_state};
}
}
extern "C" int sadx_native_music_manager(const std::uint32_t *in,std::uint32_t *out,char *trace,std::size_t capacity) {
    using namespace sadx;
    std::fill_n(out,32,0);
    cleanup::TrackState tracks{static_cast<std::int32_t>(in[0]),static_cast<std::int32_t>(in[1]),
        static_cast<std::int32_t>(in[2]),static_cast<std::int32_t>(in[3]),in[4],in[5]};
    music::VoiceTiming timing{in[6],in[7]};
    bool music_enabled=in[8]!=0,voice_enabled=in[9]!=0;
    auto language=static_cast<std::int32_t>(in[10]);
    music::ManagerState state{tracks,timing,music_enabled,voice_enabled,language};
    unsigned calls=0,clocks=0,music_calls=0,voice_calls=0;
    std::ostringstream events;
    const auto event=[&](unsigned kind,std::string_view text="",std::uint32_t argument=0) {
        events<<kind<<'|'<<text<<'|'<<argument;
        for(auto word:track_words(tracks))events<<'|'<<word;
        events<<'|'<<timing.duration<<'|'<<timing.started<<'|'<<music_enabled<<'|'<<voice_enabled<<'|'<<static_cast<std::uint32_t>(language)<<'\n';
        ++calls;
        const bool resource=kind!=1 && kind!=4;
        if(resource && in[21] && calls==in[21])throw Stop{};
        if(resource && in[22] && calls==in[22]) {
            tracks.current_song=static_cast<std::int32_t>(in[23]);
            tracks.current_voice=static_cast<std::int32_t>(in[24]);
            language=static_cast<std::int32_t>(in[25]);
            music_enabled=in[26]!=0;voice_enabled=in[27]!=0;
            timing.duration=in[28];timing.started=in[29];
        }
    };
    music::ManagerServices services{
        [&] {event(1);return in[11];},
        [&](std::string_view path,std::int32_t loop) {event(2,path,static_cast<std::uint32_t>(loop));return static_cast<std::int32_t>(in[music_calls++?13:12]);},
        [&] {event(3);},[&] {event(4);return in[16];},
        [&](std::string_view path) {event(5,path);return static_cast<std::int32_t>(in[voice_calls++?15:14]);},
        [&] {event(6);return in[17];},[&] {event(7);return in[18]+clocks++*in[19];},
        [&](const TaskHandle &) {event(8);}};
    const std::array<std::string_view,3> prefixes{"","D:\\","X:\\owned\\"};
    try {music::update_manager({},state,catalog(),prefixes.at(in[20]),services);}
    catch(const Stop &) {out[0]=1;}
    catch(const music::Error &error) {out[0]=2+static_cast<unsigned>(error.boundary());}
    const auto words=track_words(tracks);
    std::copy(words.begin(),words.end(),out+1);
    out[7]=timing.duration;out[8]=timing.started;out[9]=music_enabled;out[10]=voice_enabled;
    out[11]=static_cast<std::uint32_t>(language);out[12]=calls;
    copy_trace(events.str(),trace,capacity);return 1;
}
namespace {
struct Player final : sadx::audio::StreamPlayer {
    std::function<void(unsigned)> call;
    explicit Player(std::function<void(unsigned)> callback):call(std::move(callback)) {}
    void stop() override {call(1);}
    void reset() override {call(2);}
};
}
extern "C" int sadx_native_music_media(const std::uint32_t *in,std::uint32_t *out,char *trace,std::size_t capacity) {
    using namespace sadx;
    std::fill_n(out,32,0);
    audio::MediaState media{};
    auto attenuation=static_cast<std::int32_t>(in[7]);
    media.music_open=in[3];media.music_status=in[4];media.shared_status=in[5];media.music_looping=in[6];
    bool buffer=in[9]!=0;
    auto reader_state=static_cast<std::int32_t>(in[10]);
    unsigned pending=in[11],calls=0;
    std::ostringstream events;
    const auto event=[&](unsigned kind,std::string_view text="",std::uint32_t arg=0) {
        events<<kind<<'|'<<text<<'|'<<arg<<'|'<<media.music_open<<'|'<<media.music_status<<'|'<<media.shared_status<<'|'<<media.music_looping<<'|'<<static_cast<std::uint32_t>(attenuation)<<'|'<<buffer<<'|'<<static_cast<std::uint32_t>(reader_state)<<'\n';
        ++calls;
        if(in[13] && calls==in[13])throw Stop{};
        if(in[14] && calls==in[14]) {
            media.music_looping=in[15];media.music_open=in[16];media.music_status=in[17];
            attenuation=static_cast<std::int32_t>(in[18]);
        }
    };
    const auto player_call=[&](unsigned kind) {event(kind);buffer=false;};
    if(in[1])media.music_player=std::make_unique<Player>(player_call);
    if(in[2])media.voice_player=std::make_unique<Player>(player_call);
    music::MediaServices services{
        [&](std::string_view path) {event(3,path);return static_cast<std::int32_t>(in[8]);},
        [&](std::int32_t value) {event(4,"",static_cast<std::uint32_t>(value));buffer=true;reader_state=pending?2:1;},
        [&] {return reader_state;},[&] {return buffer;},
        [&](std::int32_t value) {event(5,"",static_cast<std::uint32_t>(value));},
        [&] {return static_cast<std::int32_t>(in[12]);},
        [&] {event(6);if(pending && --pending==0)reader_state=1;}};
    try {
        switch(in[0]) {
        case 0:out[1]=static_cast<std::uint32_t>(music::play_music_file("SYSTEM\\SoundData\\BGM\\WMA\\title2.wma",static_cast<std::int32_t>(in[6]),media,attenuation,services));break;
        case 1:out[1]=music::restart_music(media,attenuation,services);break;
        case 2:out[1]=music::music_status(media);break;
        case 3:out[1]=music::voice_status(media,services);break;
        }
    } catch(const Stop &) {out[0]=1;}
    catch(const music::Error &error) {out[0]=2+static_cast<unsigned>(error.boundary());}
    out[2]=media.music_open;out[3]=media.music_status;out[4]=media.shared_status;
    out[5]=media.music_looping;out[6]=static_cast<std::uint32_t>(attenuation);
    out[7]=buffer;out[8]=static_cast<std::uint32_t>(reader_state);out[9]=calls;
    copy_trace(events.str(),trace,capacity);return 1;
}
extern "C" int sadx_native_music_tasks(const std::uint32_t *in,std::uint32_t *out,char *trace,std::size_t capacity) {
    using namespace sadx;
    std::fill_n(out,32,0);
    std::array<TaskHandle,8> tasks{};
    for(unsigned i=1;i<tasks.size();++i)tasks[i]=TaskSourceProbe::make();
    const auto id=[&](const TaskHandle &task)->unsigned {for(unsigned i=1;i<tasks.size();++i)if(tasks[i]==task)return i;return 0;};
    title_menu::MenuSoundState menu{tasks[in[1]],tasks[in[2]]};
    TaskHandle manager=tasks[in[3]];
    cleanup::TrackState tracks{static_cast<std::int32_t>(in[4]),static_cast<std::int32_t>(in[5]),static_cast<std::int32_t>(in[6]),static_cast<std::int32_t>(in[7]),in[8],in[9]};
    auto enabled=static_cast<std::int32_t>(in[10]);
    auto ready=static_cast<std::int32_t>(in[11]);
    unsigned calls=0,installed=0;
    auto task=tasks[7];
    auto initial=std::make_shared<music::RequestWork>();
    initial->song=static_cast<std::int32_t>(in[12]);initial->elapsed=static_cast<std::int32_t>(in[13]);
    task->auxiliary_native_work=initial;
    std::ostringstream events;
    const auto event=[&](unsigned kind,unsigned a=0,unsigned b=0,unsigned c=0) {
        events<<kind<<'|'<<a<<'|'<<b<<'|'<<c<<'|'<<id(menu.music_task)<<'|'<<id(menu.volume_task)<<'|'<<id(manager);
        for(auto value:track_words(tracks))events<<'|'<<value;
        events<<'\n';
        ++calls;
        if(in[14] && calls==in[14])throw Stop{};
        if(in[15] && calls==in[15]) {menu.music_task=tasks[in[16]];menu.volume_task=tasks[in[17]];manager=tasks[in[18]];}
    };
    music::TaskServices services{
        [&](const TaskHandle &target) {event(1,id(target));},
        [&] {event(2,0,1);return tasks[in[19]?0:5];},
        [&](const TaskHandle &target) {event(3,id(target));installed=id(target);},
        [&] {event(4,0,4);return tasks[in[20]?0:6];},
        [&](const TaskHandle &target) {event(5,id(target),1,8);return in[21]?std::shared_ptr<music::RequestWork>{}:std::make_shared<music::RequestWork>();},
        [&] {event(6);}};
    try {
        switch(in[0]) {
        case 0:music::initialize_manager(tracks,services);break;
        case 1:music::initialize_manager_task(task,manager,services);break;
        case 2:music::request_song(static_cast<std::int32_t>(in[12]),menu,tracks,services);break;
        case 3:music::request_menu_music(static_cast<std::int32_t>(in[22]),catalog(),menu,tracks,services);break;
        case 4:music::update_request(task,menu,tracks,enabled,ready,services);break;
        case 5:music::play_song(static_cast<std::int32_t>(in[12]),tracks,enabled);break;
        case 6:out[1]=music::query_music_state(tracks);break;
        case 7:music::stop_menu_music(menu,services);break;
        }
    } catch(const Stop &) {out[0]=1;}
    catch(const music::Error &error) {out[0]=2+static_cast<unsigned>(error.boundary());}
    const auto words=track_words(tracks);std::copy(words.begin(),words.end(),out+2);
    out[8]=id(menu.music_task);out[9]=id(menu.volume_task);out[10]=id(manager);out[11]=calls;out[12]=installed;
    const auto work=std::static_pointer_cast<music::RequestWork>(task->auxiliary_native_work);
    out[13]=work?static_cast<std::uint32_t>(work->song):0;out[14]=work?static_cast<std::uint32_t>(work->elapsed):0;
    const auto created=std::static_pointer_cast<music::RequestWork>(tasks[6]->auxiliary_native_work);
    out[15]=created?1:0;out[16]=created?static_cast<std::uint32_t>(created->song):0;out[17]=created?static_cast<std::uint32_t>(created->elapsed):0;
    copy_trace(events.str(),trace,capacity);return 1;
}

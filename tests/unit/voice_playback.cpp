#include "game/voice_playback.hpp"
#include <cassert>
#include <vector>

namespace {
struct Player final : sadx::audio::StreamPlayer {
    std::vector<unsigned> &events;
    explicit Player(std::vector<unsigned> &value):events(value) {}
    void stop() override {events.push_back(1);}
    void reset() override {events.push_back(2);}
};
}
int main() {
    using namespace sadx;
    audio::MediaState media{};
    media.music_open=7;media.music_status=8;media.shared_status=9;media.music_looping=10;
    std::vector<unsigned> events;
    std::int32_t attenuation=-3300,state=2;
    unsigned yields=0;
    voice_playback::Services calls{
        [&](std::string_view path){assert(path=="0001.wma");events.push_back(3);attenuation=-7400;return 0;},
        [&](std::int32_t volume){assert(volume==-7400);events.push_back(4);state=0;},
        [&]{return state;},[&]{events.push_back(5);if(++yields==2)state=1;}};
    assert(voice_playback::play_file("0001.wma",media,attenuation,calls)==0 && events.empty());
    media.voice_player=std::make_unique<Player>(events);
    assert(voice_playback::play_file("0001.wma",media,attenuation,calls)==1);
    assert((events==std::vector<unsigned>{1,2,3,4,5,5}));
    assert(media.music_open==7 && media.music_status==8 && media.shared_status==9 && media.music_looping==10);
    events.clear();calls.open_voice=[&](std::string_view){events.push_back(3);return -1;};
    assert(voice_playback::play_file("0001.wma",media,attenuation,calls)==0);
    assert((events==std::vector<unsigned>{1,2,3}));
    assert(media.shared_status==9);
    try {voice_playback::play_file("\xc3\xa9.wma",media,attenuation,calls);assert(false);}
    catch(const voice_playback::Error &error){assert(error.boundary()==voice_playback::Boundary::Encoding);}
}

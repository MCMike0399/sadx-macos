#include "game/voice_playback.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

namespace {
struct Stop {};
struct Player final : sadx::audio::StreamPlayer {
    std::function<void(unsigned)> operation;
    explicit Player(std::function<void(unsigned)> value):operation(std::move(value)) {}
    void stop() override {operation(1);}
    void reset() override {operation(2);}
};
}
extern "C" int sadx_native_voice_playback(const std::uint32_t *in,std::uint32_t *out,char *trace,std::size_t capacity) {
    using namespace sadx;
    std::fill_n(out,32,0);
    audio::MediaState media{};
    media.music_open=in[9];media.music_status=in[10];media.shared_status=in[11];media.music_looping=in[12];
    auto attenuation=static_cast<std::int32_t>(in[2]);
    auto state=static_cast<std::int32_t>(in[13]);
    bool buffer=in[14]!=0;
    unsigned calls=0,pending=in[4];
    std::vector<std::unique_ptr<audio::StreamPlayer>> retained;
    std::ostringstream events;
    const auto event=[&](unsigned kind,std::string_view path="",std::uint32_t value=0) {
        events<<kind<<'|'<<path<<'|'<<value<<'|'<<static_cast<std::uint32_t>(attenuation)<<'|'<<(media.voice_player?1:0)
              <<'|'<<buffer<<'|'<<static_cast<std::uint32_t>(state)<<'|'<<media.music_open<<'|'<<media.music_status
              <<'|'<<media.shared_status<<'|'<<media.music_looping<<'\n';
        if(++calls==in[5])throw Stop{};
        if(calls==in[6]) {
            attenuation=static_cast<std::int32_t>(in[7]);
            if(in[8])retained.push_back(std::move(media.voice_player));
            media.music_open=in[15];media.music_status=in[16];media.shared_status=in[17];media.music_looping=in[18];
        }
    };
    if(in[0])media.voice_player=std::make_unique<Player>([&](unsigned kind){event(kind);buffer=false;});
    media.music_player=std::make_unique<Player>([&](unsigned kind){event(90+kind);});
    voice_playback::Services services{
        [&](std::string_view path){event(3,path);return static_cast<std::int32_t>(in[1]);},
        [&](std::int32_t volume){event(4,"",static_cast<std::uint32_t>(volume));buffer=true;state=static_cast<std::int32_t>(in[3]);},
        [&]{return state;},
        [&]{event(5);if(pending)--pending;if(!pending)state=1;}};
    try {out[1]=static_cast<std::uint32_t>(voice_playback::play_file("SYSTEM\\SoundData\\VOICE_US\\WMA\\0001.wma",media,attenuation,services));}
    catch(const Stop &){out[0]=1;}
    catch(const voice_playback::Error &error){out[0]=2+static_cast<unsigned>(error.boundary());}
    out[2]=static_cast<std::uint32_t>(attenuation);out[3]=media.voice_player?1:0;out[4]=buffer;
    out[5]=static_cast<std::uint32_t>(state);out[6]=media.music_open;out[7]=media.music_status;
    out[8]=media.shared_status;out[9]=media.music_looping;out[10]=calls;
    const auto text=events.str();if(text.size()+1>capacity)return 0;
    std::memcpy(trace,text.c_str(),text.size()+1);return 1;
}

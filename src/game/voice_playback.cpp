#include "game/voice_playback.hpp"
#include <algorithm>

namespace sadx::voice_playback {
namespace {
// NOTSA: a missing canonical reader is reported at its native resource boundary
// instead of dereferencing the null source object after a reentrant callback.
audio::StreamPlayer &voice_player(audio::MediaState &media) {
    if (!media.voice_player)
        throw Error(Boundary::VoicePlayer, "voice playback lost its canonical native player");
    return *media.voice_player;
}
} // namespace
std::int32_t play_file(std::string_view path, audio::MediaState &media,
                       const std::int32_t &attenuation, Services &services) {
    // NOTSA: imported source paths are ASCII. Other Windows code-page conversion
    // remains an explicit boundary rather than a guessed encoding.
    if (std::any_of(path.begin(), path.end(), [](unsigned char byte) { return byte > 127; }))
        throw Error(Boundary::Encoding, "unrecovered Windows code-page conversion in voice path");
    if (!media.voice_player)
        return 0;
    voice_player(media).stop();
    voice_player(media).reset();
    voice_player(media);
    if (services.open_voice(path) < 0)
        return 0;
    voice_player(media);
    services.start_voice(attenuation);
    for (;;) {
        voice_player(media);
        if (services.reader_state() == 1)
            return 1;
        services.yield();
    }
}
} // namespace sadx::voice_playback

#include "platform/mac/mac_audio.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    using namespace sadx::audio::mac;
    AudioOutput output(1);
    auto clip = std::make_shared<AudioClip>();
    clip->samples = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    const auto token = output.play(clip);
    assert(token);
    std::array<float, 2> frame{};
    assert(output.render(frame.data(), 1));
    assert(frame[0] == 0.1f && frame[1] == 0.2f);
    assert(output.set_gain(token, 0.5f, 0.25f));
    assert(output.render(frame.data(), 1));
    assert(frame[0] == 0.15f && frame[1] == 0.1f);
    assert(!output.set_gain(token, std::numeric_limits<float>::infinity(), 1));
    assert(!output.set_gain(token + 1, 1, 1));
    assert(output.render(frame.data(), 1));
    assert(frame[0] == 0.25f && frame[1] == 0.15f);
    assert(output.set_gain(token, 0, 0));
    assert(output.render(frame.data(), 1));
    assert(frame[0] == 0 && frame[1] == 0);
    assert(!output.set_gain(token, 1, 1));
    assert(output.active_voice_count() == 0);
    std::cout << "Live mixer gains preserve cursor, reject invalid targets and expire at EOF.\n";
}

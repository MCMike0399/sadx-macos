#pragma once
#import <Foundation/Foundation.h>
#include <cmath>
#include <stdexcept>

namespace sadx::native::app {
// NOTSA: optional local listening preferences; game/profile attenuation remains unchanged.
struct AudioMix {
    float music = 1.0f, effects = 1.0f, voice = 1.0f;
};
inline AudioMix load_audio_mix() {
    // The preference writer can be a separate native process; refresh its domain before reading.
    CFPreferencesAppSynchronize(CFSTR("org.sadx-macos.native"));
    const auto gain = [&](NSString *key, double maximum) {
        id value = CFBridgingRelease(CFPreferencesCopyAppValue(
            (__bridge CFStringRef)key, CFSTR("org.sadx-macos.native")));
        if (!value) return 1.0f;
        if (![value isKindOfClass:NSNumber.class])
            throw std::runtime_error("native audio preference must be a number");
        const auto number = [value doubleValue];
        if (!std::isfinite(number) || number < 0 || number > maximum)
            throw std::runtime_error("native audio preference is outside its supported gain range");
        return static_cast<float>(number);
    };
    return {gain(@"MusicVolume", 1), gain(@"EffectsVolume", 1), gain(@"VoiceVolume", 8)};
}
} // namespace sadx::native::app

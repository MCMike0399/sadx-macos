#include "platform/mac/mac_audio.hpp"
#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <fstream>
#include <limits>
#include <mutex>

namespace sadx::audio::mac {
namespace {
// [platform:audio] AudioQueue accepts interleaved linear PCM.
AudioStreamBasicDescription mix_format() {
    AudioStreamBasicDescription format{};
    format.mSampleRate = 44100;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mBytesPerPacket = 8;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 8;
    format.mChannelsPerFrame = 2;
    format.mBitsPerChannel = 32;
    return format;
}
void put_word(std::ostream& file, std::uint32_t value, unsigned count) {
    for (unsigned index = 0; index < count; ++index)
        file.put(static_cast<char>(value >> (index * 8)));
}
void write_header(std::ostream& file, std::uint32_t bytes) {
    file.seekp(0);
    file.write("RIFF", 4);
    put_word(file, bytes + 36, 4);
    file.write("WAVEfmt ", 8);
    put_word(file, 16, 4);
    put_word(file, 3, 2); // IEEE float
    put_word(file, 2, 2);
    put_word(file, 44100, 4);
    put_word(file, 352800, 4);
    put_word(file, 8, 2);
    put_word(file, 32, 2);
    file.write("data", 4);
    put_word(file, bytes, 4);
}
} // namespace

// [platform:audio] Decode imported PCM WAV through Core Audio and resample once at load.
bool load_clip(const std::string& path, AudioClip& clip, std::string& error) {
    error.clear();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        nullptr, reinterpret_cast<const UInt8*>(path.data()), path.size(), false);
    ExtAudioFileRef file = nullptr;
    OSStatus status = url ? ExtAudioFileOpenURL(url, &file) : -1;
    if (url)
        CFRelease(url);
    if (status != noErr) {
        error = "audio open failed: " + path + " status=" + std::to_string(status);
        return false;
    }
    auto format = mix_format();
    status = ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat,
                                     sizeof(format), &format);
    AudioClip decoded;
    std::array<float, 8192> chunk{};
    while (status == noErr) {
        UInt32 frames = 4096;
        AudioBufferList buffers{};
        buffers.mNumberBuffers = 1;
        buffers.mBuffers[0] = {2, static_cast<UInt32>(sizeof(chunk)), chunk.data()};
        status = ExtAudioFileRead(file, &frames, &buffers);
        if (status != noErr || frames == 0)
            break;
        decoded.samples.insert(decoded.samples.end(), chunk.begin(), chunk.begin() + frames * 2);
    }
    ExtAudioFileDispose(file);
    if (status != noErr || decoded.samples.empty()) {
        error = "audio decode failed or empty: " + path + " status=" + std::to_string(status);
        return false;
    }
    clip = std::move(decoded);
    return true;
}

struct AudioOutput::Impl {
    struct Voice {
        std::shared_ptr<const AudioClip> clip;
        std::size_t cursor = 0;
        std::uint64_t token = 0;
        bool loop = false;
        float left = 1;
        float right = 1;
    };
    mutable std::mutex mutex;
    std::vector<Voice> voices;
    std::size_t buffer_frames = 1024;
    std::uint64_t next_token = 1;
    AudioQueueRef queue = nullptr;
    std::atomic<bool> stopping = false;
    bool presentation_clock_seen = false;
    bool started = false;
    std::ofstream capture;
    std::uint32_t capture_bytes = 0;
    AudioStats statistics;

    void mix(float* output, std::size_t frames) {
        std::fill(output, output + frames * 2, 0.0f);
        for (auto& voice : voices) {
            if (!voice.clip)
                continue;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                output[frame * 2] += voice.clip->samples[voice.cursor] * voice.left;
                output[frame * 2 + 1] += voice.clip->samples[voice.cursor + 1] * voice.right;
                voice.cursor += 2;
                if (voice.cursor == voice.clip->samples.size()) {
                    if (voice.loop)
                        voice.cursor = 0;
                    else {
                        voice = {};
                        break;
                    }
                }
            }
        }
        for (std::size_t sample = 0; sample < frames * 2; ++sample)
            output[sample] = std::clamp(output[sample], -1.0f, 1.0f);
    }

    static void callback(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer) {
        auto& self = *static_cast<Impl*>(context);
        std::lock_guard lock(self.mutex);
        if (self.stopping)
            return;
        ++self.statistics.completed_buffers;
        self.fill(queue, buffer);
    }
    void fill(AudioQueueRef target, AudioQueueBufferRef buffer) {
        const auto frames = buffer_frames;
        auto* samples = static_cast<float*>(buffer->mAudioData);
        mix(samples, frames);
        buffer->mAudioDataByteSize = static_cast<UInt32>(frames * 8);
        const auto status = AudioQueueEnqueueBuffer(target, buffer, 0, nullptr);
        if (status != noErr) {
            statistics.device_error = status;
            return;
        }
        statistics.submitted_frames += frames;
        for (std::size_t index = 0; index < frames * 2; ++index)
            statistics.nonzero_samples += samples[index] != 0;
        if (capture.is_open()) {
            capture.write(reinterpret_cast<const char*>(samples), frames * 8);
            capture_bytes += frames * 8;
            if (!capture)
                statistics.device_error = -1;
        }
    }
};

AudioOutput::AudioOutput(std::size_t capacity, std::size_t buffer_frames)
    : impl_(std::make_unique<Impl>()) {
    impl_->buffer_frames = buffer_frames;
    impl_->voices.resize(capacity);
}
AudioOutput::~AudioOutput() { shutdown(); }

// [platform:audio] AudioQueue runs its callback on an internal thread, independently of game frames.
bool AudioOutput::start(const std::string& capture_path, std::string& error) {
    std::unique_lock lock(impl_->mutex);
    error.clear();
    if (impl_->queue) {
        error = "audio output already started";
        return false;
    }
    if (impl_->buffer_frames == 0 ||
        impl_->buffer_frames > std::numeric_limits<UInt32>::max() / 8) {
        error = "audio buffer frame count is outside the native queue range";
        return false;
    }
    if (!capture_path.empty()) {
        impl_->capture.open(capture_path, std::ios::binary | std::ios::trunc);
        if (!impl_->capture) {
            error = "cannot create audio capture: " + capture_path;
            impl_->capture.clear();
            return false;
        }
        write_header(impl_->capture, 0);
        impl_->capture_bytes = 0;
    }
    auto format = mix_format();
    impl_->stopping = false;
    impl_->statistics = {};
    impl_->started = false;
    impl_->presentation_clock_seen = false;
    auto status = AudioQueueNewOutput(&format, Impl::callback, impl_.get(), nullptr,
                                     nullptr, 0, &impl_->queue);
    for (unsigned index = 0; status == noErr && index < 3; ++index) {
        AudioQueueBufferRef buffer = nullptr;
        status = AudioQueueAllocateBuffer(impl_->queue,
                                          static_cast<UInt32>(impl_->buffer_frames * 8), &buffer);
        if (status == noErr)
            impl_->fill(impl_->queue, buffer);
    }
    if (status == noErr && impl_->statistics.device_error == 0)
        status = AudioQueueStart(impl_->queue, nullptr);
    if (status != noErr || impl_->statistics.device_error != 0) {
        error = "audio device start failed: " + std::to_string(
            status != noErr ? status : impl_->statistics.device_error);
        // AudioQueueDispose waits for callbacks, which acquire this mutex.
        // Leave the object stopped even when creation only partially succeeded.
        lock.unlock();
        shutdown();
        return false;
    }
    impl_->started = true;
    return true;
}
void AudioOutput::shutdown() {
    // Dispose waits for callbacks; never hold the callback's mutex while waiting.
    impl_->stopping = true;
    if (impl_->queue) {
        AudioQueueDispose(impl_->queue, true);
        impl_->queue = nullptr;
    }
    std::lock_guard lock(impl_->mutex);
    impl_->started = false;
    if (impl_->capture.is_open()) {
        write_header(impl_->capture, impl_->capture_bytes);
        impl_->capture.close();
    }
    impl_->capture.clear();
}
std::uint64_t AudioOutput::play(std::shared_ptr<const AudioClip> clip, bool loop,
                                float left_gain, float right_gain) {
    if (!clip || clip->samples.empty() || clip->samples.size() % 2 != 0 ||
        !std::isfinite(left_gain) || !std::isfinite(right_gain))
        return 0;
    std::lock_guard lock(impl_->mutex);
    for (auto& voice : impl_->voices) {
        if (!voice.clip) {
            voice = {std::move(clip), 0, impl_->next_token++, loop, left_gain, right_gain};
            return voice.token;
        }
    }
    return 0;
}
void AudioOutput::stop(std::uint64_t token) {
    std::lock_guard lock(impl_->mutex);
    for (auto& voice : impl_->voices)
        if (voice.token == token)
            voice = {};
}
bool AudioOutput::set_gain(std::uint64_t token, float left_gain, float right_gain) {
    if (!token || !std::isfinite(left_gain) || !std::isfinite(right_gain))
        return false;
    std::lock_guard lock(impl_->mutex);
    for (auto &voice : impl_->voices) {
        if (voice.clip && voice.token == token) {
            voice.left = left_gain;
            voice.right = right_gain;
            return true;
        }
    }
    return false;
}
void AudioOutput::stop_all() {
    std::lock_guard lock(impl_->mutex);
    for (auto& voice : impl_->voices)
        voice = {};
}
AudioStats AudioOutput::stats() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->statistics;
}
bool AudioOutput::ready() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->started && !impl_->stopping && impl_->statistics.device_error == 0;
}
std::size_t AudioOutput::capacity() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->voices.size();
}
std::size_t AudioOutput::active_voice_count() const {
    std::lock_guard lock(impl_->mutex);
    return std::count_if(impl_->voices.begin(), impl_->voices.end(),
                         [](const auto& voice) { return voice.clip != nullptr; });
}
std::optional<std::uint64_t> AudioOutput::presentation_frames(std::string& error) const {
    std::lock_guard lock(impl_->mutex);
    error.clear();
    if (!impl_->queue || impl_->stopping) {
        error = "audio presentation clock requires a running output";
        return std::nullopt;
    }
    AudioTimeStamp stamp{};
    const auto status = AudioQueueGetCurrentTime(impl_->queue, nullptr, &stamp, nullptr);
    // AudioQueueStart is asynchronous: before playback begins its sample clock can
    // report InvalidRunState. Expose pending clock readiness instead of inventing zero.
    if (status == kAudioQueueErr_InvalidRunState && !impl_->presentation_clock_seen)
        return std::nullopt;
    if (status != noErr) {
        error = "audio presentation clock failed: " + std::to_string(status);
        return std::nullopt;
    }
    if (!(stamp.mFlags & kAudioTimeStampSampleTimeValid) || stamp.mSampleTime < 0)
        return std::nullopt;
    if (!std::isfinite(stamp.mSampleTime)) {
        error = "audio presentation clock is not finite";
        return std::nullopt;
    }
    impl_->presentation_clock_seen = true;
    return static_cast<std::uint64_t>(stamp.mSampleTime);
}
bool AudioOutput::render(float* stereo, std::size_t frames) {
    std::lock_guard lock(impl_->mutex);
    if (impl_->queue || !stereo)
        return false;
    impl_->mix(stereo, frames);
    return true;
}
} // namespace sadx::audio::mac

#include "platform/mac/mac_movie.hpp"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreVideo/CoreVideo.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace sadx::movie::mac {
std::optional<float> volume_gain(std::int32_t hundredths_db) noexcept {
    if (hundredths_db < -10000 || hundredths_db > 0) return std::nullopt;
    if (hundredths_db == -10000) return 0;
    return static_cast<float>(std::pow(10.0,static_cast<double>(hundredths_db)/2000.0));
}
namespace {
NSArray<AVAssetTrack*>* tracks(AVAsset* asset, AVMediaType type, std::string& error) {
    dispatch_semaphore_t ready = dispatch_semaphore_create(0);
    __block NSArray<AVAssetTrack*>* result = nil;
    __block NSError* problem = nil;
    [asset loadTracksWithMediaType:type completionHandler:^(NSArray<AVAssetTrack*>* value, NSError* failure) {
        result = value;
        problem = failure;
        dispatch_semaphore_signal(ready);
    }];
    if (dispatch_semaphore_wait(ready, dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_SEC)) != 0) {
        error = "native movie track discovery timed out";
        return nil;
    }
    if (problem || result.count != 1) {
        error = problem ? problem.description.UTF8String : "native movie requires one video and one audio track";
        return nil;
    }
    return result;
}
std::string reader_error(AVAssetReader* reader, const char* boundary) {
    return std::string(boundary) + ": " +
           (reader.error ? reader.error.description.UTF8String : "native decoder stopped without end-of-stream");
}
}

struct MoviePlayer::Impl {
    AVURLAsset* asset = nil;
    AVAssetReader* video_reader = nil;
    AVAssetReaderTrackOutput* video_output = nil;
    CMSampleBufferRef pending = nullptr;
    std::shared_ptr<audio::mac::AudioClip> pcm = std::make_shared<audio::mac::AudioClip>();
    audio::mac::AudioOutput audio{1};
    MovieInfo information;
    MovieFrame frame;
    bool started = false, video_ended = false;
    std::uint64_t next_index = 0;
    double previous_pts = -1;
    std::uint64_t previous_audio_frames = 0;

    ~Impl() {
        if (pending) CFRelease(pending);
        [video_reader cancelReading];
    }
    bool read_video(std::string& error) {
        if (pending) {
            CFRelease(pending);
            pending = nullptr;
        }
        pending = [video_output copyNextSampleBuffer];
        if (!pending) {
            if (video_reader.status == AVAssetReaderStatusCompleted) {
                video_ended = true;
                return true;
            }
            error = reader_error(video_reader,"native movie video sample");
            return false;
        }
        const double pts = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(pending));
        if (!std::isfinite(pts) || pts < 0 || pts < previous_pts) {
            error = "native movie video presentation time is invalid or moves backwards";
            return false;
        }
        previous_pts = pts;
        return true;
    }
    bool decode_audio(AVAssetTrack* track, std::string& error) {
        NSError* problem = nil;
        AVAssetReader* reader = [[AVAssetReader alloc] initWithAsset:asset error:&problem];
        if (!reader) {
            error = problem ? problem.description.UTF8String : "native movie audio reader allocation failed";
            return false;
        }
        auto* output = [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:@{
            AVFormatIDKey:@(kAudioFormatLinearPCM),AVSampleRateKey:@44100,AVNumberOfChannelsKey:@2,
            AVLinearPCMBitDepthKey:@32,AVLinearPCMIsFloatKey:@YES,
            AVLinearPCMIsBigEndianKey:@NO,AVLinearPCMIsNonInterleaved:@NO}];
        if (![reader canAddOutput:output]) {
            error = "native movie audio output format is unsupported";
            return false;
        }
        [reader addOutput:output];
        if (![reader startReading]) {
            error = reader_error(reader,"native movie audio start");
            return false;
        }
        while (CMSampleBufferRef sample = [output copyNextSampleBuffer]) {
            const auto* format = CMAudioFormatDescriptionGetStreamBasicDescription(
                CMSampleBufferGetFormatDescription(sample));
            const auto block = CMSampleBufferGetDataBuffer(sample);
            const auto bytes = block ? CMBlockBufferGetDataLength(block) : 0;
            const auto time = CMTimeConvertScale(CMSampleBufferGetPresentationTimeStamp(sample),
                                                 44100,kCMTimeRoundingMethod_RoundHalfAwayFromZero);
            bool valid = format && format->mFormatID == kAudioFormatLinearPCM &&
                         format->mSampleRate == 44100 && format->mChannelsPerFrame == 2 &&
                         format->mBitsPerChannel == 32 && (format->mFormatFlags & kAudioFormatFlagIsFloat) &&
                         !(format->mFormatFlags & kAudioFormatFlagIsNonInterleaved) &&
                         CMTIME_IS_NUMERIC(time) && time.value >= 0 && bytes != 0 && bytes % 8 == 0;
            const auto target = valid ? static_cast<std::uint64_t>(time.value) : 0;
            if (!valid || target < pcm->samples.size()/2 ||
                target > pcm->samples.max_size()/2 - bytes/8) {
                CFRelease(sample);
                [reader cancelReading];
                error = "native movie audio PCM format, timing overlap, or sample size is unsupported";
                return false;
            }
            // Sample timestamps define real leading silence/gaps on the media timeline.
            // Do not shift audio independently from the video stream.
            pcm->samples.resize((target+bytes/8)*2,0);
            const auto status = CMBlockBufferCopyDataBytes(block,0,bytes,pcm->samples.data()+target*2);
            CFRelease(sample);
            if (status != noErr) {
                [reader cancelReading];
                error = "native movie PCM sample copy failed: " + std::to_string(status);
                return false;
            }
        }
        if (reader.status != AVAssetReaderStatusCompleted || pcm->samples.empty()) {
            error = reader_error(reader,"native movie audio end-of-stream");
            return false;
        }
        information.audio_frames = pcm->samples.size()/2;
        return true;
    }
    bool copy_frame(std::string& error) {
        auto image = CMSampleBufferGetImageBuffer(pending);
        if (!image || CVPixelBufferGetPixelFormatType(image) != kCVPixelFormatType_32BGRA ||
            CVPixelBufferGetWidth(image) != information.width ||
            CVPixelBufferGetHeight(image) != information.height) {
            error = "native movie surface format or dimensions changed";
            return false;
        }
        const auto status = CVPixelBufferLockBaseAddress(image,kCVPixelBufferLock_ReadOnly);
        if (status != kCVReturnSuccess) {
            error = "native movie pixel buffer lock failed: " + std::to_string(status);
            return false;
        }
        const auto* source = static_cast<const std::uint8_t*>(CVPixelBufferGetBaseAddress(image));
        const auto stride = CVPixelBufferGetBytesPerRow(image);
        const std::size_t row = information.width*4;
        if (!source || stride < row) {
            CVPixelBufferUnlockBaseAddress(image,kCVPixelBufferLock_ReadOnly);
            error = "native movie pixel buffer storage is invalid";
            return false;
        }
        frame.width = information.width;
        frame.height = information.height;
        frame.index = next_index++;
        frame.presentation_seconds = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(pending));
        frame.bgra.resize(row*frame.height);
        for (unsigned y = 0; y < frame.height; ++y)
            std::memcpy(frame.bgra.data()+y*row,source+y*stride,row);
        CVPixelBufferUnlockBaseAddress(image,kCVPixelBufferLock_ReadOnly);
        return true;
    }
};

MoviePlayer::MoviePlayer(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
MoviePlayer::~MoviePlayer() { stop(); }
std::unique_ptr<MoviePlayer> MoviePlayer::open(const std::string& path, std::string& error) {
    error.clear();
    auto impl = std::make_unique<Impl>();
    NSString* native_path = [[NSString alloc] initWithBytes:path.data() length:path.size()
                                                 encoding:NSUTF8StringEncoding];
    if (!native_path) {
        error = "native movie path is not UTF-8";
        return nullptr;
    }
    impl->asset = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:native_path]
                                    options:@{AVURLAssetPreferPreciseDurationAndTimingKey:@YES}];
    auto video = tracks(impl->asset,AVMediaTypeVideo,error);
    if (!video) return nullptr;
    auto sound = tracks(impl->asset,AVMediaTypeAudio,error);
    if (!sound) return nullptr;
    impl->information.video_end_seconds = CMTimeGetSeconds(CMTimeRangeGetEnd(video[0].timeRange));
    if (!std::isfinite(impl->information.video_end_seconds) || impl->information.video_end_seconds <= 0) {
        error = "native movie video track has no finite end time";
        return nullptr;
    }
    NSError* problem = nil;
    impl->video_reader = [[AVAssetReader alloc] initWithAsset:impl->asset error:&problem];
    if (!impl->video_reader) {
        error = problem ? problem.description.UTF8String : "native movie video reader allocation failed";
        return nullptr;
    }
    impl->video_output = [[AVAssetReaderTrackOutput alloc] initWithTrack:video[0]
        outputSettings:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA)}];
    if (![impl->video_reader canAddOutput:impl->video_output]) {
        error = "native movie video output format is unsupported";
        return nullptr;
    }
    [impl->video_reader addOutput:impl->video_output];
    if (![impl->video_reader startReading] || !impl->read_video(error)) {
        if (error.empty()) error = reader_error(impl->video_reader,"native movie video start");
        return nullptr;
    }
    auto image = impl->pending ? CMSampleBufferGetImageBuffer(impl->pending) : nullptr;
    if (!image) {
        error = "native movie has no decoded video samples";
        return nullptr;
    }
    impl->information.width = static_cast<std::uint32_t>(CVPixelBufferGetWidth(image));
    impl->information.height = static_cast<std::uint32_t>(CVPixelBufferGetHeight(image));
    if (!impl->decode_audio(sound[0],error)) return nullptr;
    return std::unique_ptr<MoviePlayer>(new MoviePlayer(std::move(impl)));
}
const MovieInfo& MoviePlayer::info() const noexcept { return impl_->information; }
bool MoviePlayer::start(std::int32_t hundredths_db, const std::string& capture, std::string& error) {
    error.clear();
    if (impl_->started || !impl_->pending) {
        error = "native movie cannot start twice or after stop";
        return false;
    }
    const auto gain = volume_gain(hundredths_db);
    if (!gain) {
        error = "native movie volume is outside IBasicAudio range -10000..0";
        return false;
    }
    if (impl_->audio.play(impl_->pcm,false,*gain,*gain) == 0 || !impl_->audio.start(capture,error)) {
        if (error.empty()) error = "native movie audio voice was rejected";
        impl_->audio.shutdown();
        impl_->audio.stop_all();
        return false;
    }
    impl_->started = true;
    return true;
}
MovieUpdate MoviePlayer::update() {
    MovieUpdate update;
    if (!impl_->started) {
        update.error = "native movie is not started";
        return update;
    }
    if (impl_->audio.stats().device_error != 0) {
        update.error = "native movie audio delivery failed: " +
                       std::to_string(impl_->audio.stats().device_error);
        return update;
    }
    const auto frames = impl_->audio.presentation_frames(update.error);
    if (!frames) return update;
    update.clock_ready = true;
    update.presented_audio_frames = *frames;
    if (*frames < impl_->previous_audio_frames) {
        update.error = "native movie audio presentation clock moved backwards";
        return update;
    }
    impl_->previous_audio_frames = *frames;
    const double seconds = static_cast<double>(*frames)/44100;
    while (impl_->pending && CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(impl_->pending)) <= seconds) {
        if (!impl_->copy_frame(update.error)) return update;
        update.ready_frame = &impl_->frame;
        if (!impl_->read_video(update.error)) return update;
    }
    // Decoder EOS plus actual queue progress replaces graph EC_COMPLETE. Track end comes from
    // AVFoundation's parsed media timeline, never the game's transition-frame threshold table.
    update.complete = impl_->video_ended && *frames >= impl_->information.audio_frames &&
                      seconds >= impl_->information.video_end_seconds;
    return update;
}
void MoviePlayer::stop() {
    impl_->audio.shutdown();
    impl_->audio.stop_all();
    [impl_->video_reader cancelReading];
    if (impl_->pending) {
        CFRelease(impl_->pending);
        impl_->pending = nullptr;
    }
    impl_->started = false;
}
audio::mac::AudioStats MoviePlayer::audio_stats() const { return impl_->audio.stats(); }
} // namespace sadx::movie::mac

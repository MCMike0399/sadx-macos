#include "platform/mac/mac_movie.hpp"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreVideo/CoreVideo.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
void require(bool ok,const std::string& why) { if(!ok) throw std::runtime_error(why); }
void wait_ready(AVAssetWriterInput* input) {
    const auto start=std::chrono::steady_clock::now();
    while(!input.readyForMoreMediaData) {
        require(std::chrono::steady_clock::now()-start<std::chrono::seconds(5),"fixture writer readiness");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
// Synthetic media is generated at runtime through native APIs, never committed as a binary.
void make_fixture(const std::filesystem::path& path) {
    NSError* problem=nil;
    auto* writer=[[AVAssetWriter alloc] initWithURL:[NSURL fileURLWithPath:@(path.c_str())]
        fileType:AVFileTypeQuickTimeMovie error:&problem];
    require(writer!=nil,"fixture writer creation");
    auto* video=[AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:@{
        AVVideoCodecKey:AVVideoCodecTypeH264,AVVideoWidthKey:@32,AVVideoHeightKey:@32}];
    auto* pixels=[AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:video
        sourcePixelBufferAttributes:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA),
                                      (id)kCVPixelBufferWidthKey:@32,(id)kCVPixelBufferHeightKey:@32}];
    auto* audio=[AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:@{
        AVFormatIDKey:@(kAudioFormatLinearPCM),AVSampleRateKey:@44100,AVNumberOfChannelsKey:@2,
        AVLinearPCMBitDepthKey:@32,AVLinearPCMIsFloatKey:@YES,AVLinearPCMIsBigEndianKey:@NO,
        AVLinearPCMIsNonInterleaved:@NO}];
    require([writer canAddInput:video]&&[writer canAddInput:audio],"fixture writer inputs");
    [writer addInput:video];[writer addInput:audio];
    require([writer startWriting],"fixture writer start");
    [writer startSessionAtSourceTime:kCMTimeZero];
    for(int index=0;index<3;++index) {
        CVPixelBufferRef image=nullptr;
        require(CVPixelBufferCreate(nullptr,32,32,kCVPixelFormatType_32BGRA,nullptr,&image)==kCVReturnSuccess,
                "fixture pixel allocation");
        require(CVPixelBufferLockBaseAddress(image,0)==kCVReturnSuccess,"fixture pixel lock");
        auto* bytes=static_cast<std::uint8_t*>(CVPixelBufferGetBaseAddress(image));
        const auto stride=CVPixelBufferGetBytesPerRow(image);
        for(unsigned y=0;y<32;++y) for(unsigned x=0;x<32;++x) {
            auto* pixel=bytes+y*stride+x*4;
            pixel[0]=static_cast<std::uint8_t>(20+index*40);pixel[1]=60;pixel[2]=180;pixel[3]=255;
        }
        CVPixelBufferUnlockBaseAddress(image,0);
        wait_ready(video);
        require([pixels appendPixelBuffer:image withPresentationTime:CMTimeMake(index*8,100)],"fixture video sample");
        CVPixelBufferRelease(image);
    }
    [video markAsFinished];
    AudioStreamBasicDescription format{};
    format.mSampleRate=44100;format.mFormatID=kAudioFormatLinearPCM;
    format.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked;
    format.mBytesPerPacket=8;format.mFramesPerPacket=1;format.mBytesPerFrame=8;
    format.mChannelsPerFrame=2;format.mBitsPerChannel=32;
    CMAudioFormatDescriptionRef description=nullptr;
    require(CMAudioFormatDescriptionCreate(nullptr,&format,0,nullptr,0,nullptr,nullptr,&description)==noErr,
            "fixture audio format");
    constexpr std::size_t frames=10584; // 0.24 seconds, independent of video encoder duration metadata.
    std::vector<float> samples(frames*2);
    for(std::size_t index=0;index<frames;++index)
        samples[index*2]=samples[index*2+1]=0.002f*std::sin(static_cast<float>(index)*0.02f);
    CMBlockBufferRef block=nullptr;
    require(CMBlockBufferCreateWithMemoryBlock(nullptr,nullptr,samples.size()*4,kCFAllocatorDefault,
        nullptr,0,samples.size()*4,kCMBlockBufferAssureMemoryNowFlag,&block)==noErr,"fixture audio block");
    require(CMBlockBufferReplaceDataBytes(samples.data(),block,0,samples.size()*4)==noErr,"fixture audio data");
    const CMSampleTimingInfo timing={CMTimeMake(1,44100),kCMTimeZero,kCMTimeInvalid};
    const std::size_t sample_bytes=8;
    CMSampleBufferRef sample=nullptr;
    require(CMSampleBufferCreateReady(nullptr,block,description,frames,1,&timing,1,&sample_bytes,&sample)==noErr,
            "fixture audio sample");
    wait_ready(audio);
    require([audio appendSampleBuffer:sample],"fixture audio append");
    [audio markAsFinished];
    CFRelease(sample);CFRelease(block);CFRelease(description);
    dispatch_semaphore_t done=dispatch_semaphore_create(0);
    [writer finishWritingWithCompletionHandler:^{dispatch_semaphore_signal(done);}];
    require(dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_SEC))==0,
            "fixture writer timeout");
    require(writer.status==AVAssetWriterStatusCompleted,"fixture writer completion");
}
}
int main() { @autoreleasepool { try {
    const auto root=std::filesystem::temp_directory_path()/NSUUID.UUID.UUIDString.UTF8String;
    std::filesystem::create_directory(root);
    struct Cleanup {std::filesystem::path root;~Cleanup(){std::filesystem::remove_all(root);}} cleanup{root};
    make_fixture(root/"fixture.mov");
    require(sadx::movie::mac::volume_gain(-10000)==0 && sadx::movie::mac::volume_gain(0)==1,
            "IBasicAudio silence and full volume");
    require(std::abs(*sadx::movie::mac::volume_gain(-2000)-0.1f)<0.000001f &&
            std::abs(*sadx::movie::mac::volume_gain(-6000)-0.001f)<0.000001f,
            "hundredths dB amplitude conversion");
    require(!sadx::movie::mac::volume_gain(-10001) && !sadx::movie::mac::volume_gain(1),
            "invalid attenuation rejected");
    std::string error;
    require(!sadx::movie::mac::MoviePlayer::open((root/"missing.mpg").string(),error)&&!error.empty(),
            "missing media fails explicitly");
    std::ofstream(root/"bad.mpg") << "not a movie";
    require(!sadx::movie::mac::MoviePlayer::open((root/"bad.mpg").string(),error)&&!error.empty(),
            "unsupported media fails explicitly");
    auto movie=sadx::movie::mac::MoviePlayer::open((root/"fixture.mov").string(),error);
    require(movie!=nullptr,error);
    require(movie->info().width==32&&movie->info().height==32&&movie->info().audio_frames==10584,
            "native decoder dimensions and exact PCM length");
    require(!movie->update(),"update before start rejected");
    require(!movie->start(-1000,(root/"missing"/"capture.wav").string(),error),"capture failure reported");
    require(movie->start(-1000,(root/"capture.wav").string(),error),"start after cleanup: "+error);
    std::uint64_t count=0,last_index=0,played=0;
    bool complete=false;
    const auto start=std::chrono::steady_clock::now();
    while(std::chrono::steady_clock::now()-start<std::chrono::seconds(5)) {
        const auto update=movie->update();
        require(bool(update),update.error);
        if(update.ready_frame) {
            require(update.ready_frame->bgra.size()==32*32*4,"decoded BGRA surface");
            require(!count||update.ready_frame->index>last_index,"new-sample-only presentation");
            last_index=update.ready_frame->index;
            ++count;
        }
        played=update.presented_audio_frames;
        if(update.complete) {complete=true;break;}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(complete&&count>0&&last_index==2&&played>=10584,"actual decoder EOS and audio-clock completion");
    movie->stop();
    const auto statistics=movie->audio_stats();
    require(statistics.device_error==0&&statistics.nonzero_samples>0&&statistics.completed_buffers>0,
            "real PCM output delivery");
    require(!movie->update(),"stop cancels playback");
    // A source skip calls stop on an active graph; it must not wait for native decoder EOS.
    auto skipped=sadx::movie::mac::MoviePlayer::open((root/"fixture.mov").string(),error);
    require(skipped && skipped->start(-7400,(root/"skipped.wav").string(),error),"active skip setup: "+error);
    const auto skip_start=std::chrono::steady_clock::now();
    bool ready=false;
    while(std::chrono::steady_clock::now()-skip_start<std::chrono::seconds(5)) {
        const auto update=skipped->update();
        require(bool(update),update.error);
        if(update.ready_frame) {ready=true;break;}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(ready,"skip has an active native decoded frame");
    skipped->stop();
    require(!skipped->update() && !skipped->start(-7400,{},error),"stopped graph cannot silently restart");
    std::ifstream captured(root/"skipped.wav",std::ios::binary);
    captured.seekg(44+100*2*4);
    float delivered=0;
    captured.read(reinterpret_cast<char*>(&delivered),sizeof(delivered));
    require(bool(captured),"stopped PCM capture finalized");
    const float expected=0.002f*std::sin(100.0f*0.02f)*0.00019952623149688788f;
    require(std::abs(delivered-expected)<0.000000000001f,"live -7400 setting reaches actual native PCM delivery");
    // Window abort destroys an active graph, rather than manufacturing normal completion.
    auto aborted=sadx::movie::mac::MoviePlayer::open((root/"fixture.mov").string(),error);
    require(aborted && aborted->start(-7400,(root/"aborted.wav").string(),error),"active abort setup: "+error);
    aborted.reset();
    require(std::filesystem::file_size(root/"aborted.wav")>44,"abort closes and finalizes native PCM output");
    std::cout << "Native movie: generated media, BGRA frames, exact PCM length, real audio clock/EOS, "
                 "live volume, active skip/abort cleanup, failed-start retry and unsupported assets pass\n";
    return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;} } }

#include "platform/mac/mac_movie.hpp"
#import <AppKit/AppKit.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
NSImage* native_image(const sadx::movie::mac::MovieFrame& frame) {
    auto data = CFDataCreate(nullptr,frame.bgra.data(),frame.bgra.size());
    auto provider = CGDataProviderCreateWithCFData(data);
    auto color = CGColorSpaceCreateDeviceRGB();
    auto image = CGImageCreate(frame.width,frame.height,8,32,frame.width*4,color,
        kCGBitmapByteOrder32Little|kCGImageAlphaPremultipliedFirst,provider,nullptr,false,kCGRenderingIntentDefault);
    NSImage* result = image ? [[NSImage alloc] initWithCGImage:image size:NSMakeSize(frame.width,frame.height)] : nil;
    if(image) CGImageRelease(image);
    CGColorSpaceRelease(color);
    CGDataProviderRelease(provider);
    CFRelease(data);
    return result;
}
bool save_frame(const sadx::movie::mac::MovieFrame& frame,const std::string& path) {
    std::ofstream output(path,std::ios::binary);
    output << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
    for(std::size_t pixel=0;pixel<frame.bgra.size();pixel+=4) {
        output.put(static_cast<char>(frame.bgra[pixel+2]));
        output.put(static_cast<char>(frame.bgra[pixel+1]));
        output.put(static_cast<char>(frame.bgra[pixel]));
    }
    return bool(output);
}
}
// NOTSA: Native media/device qualification, not a second implementation of the game movie loop.
// The optional seconds limit aborts this diagnostic; it never claims clip completion or game skip.
int main(int argc,char**argv) { @autoreleasepool { try {
    if(argc<3||argc>5) {
        std::cerr << "usage: movie-qualify imported.MPG output-prefix [diagnostic-seconds-limit] [hundredths-db]\n";
        return 2;
    }
    const double limit=argc>=4?std::stod(argv[3]):0;
    if(limit<0||limit>180) return 2;
    const std::int32_t volume=argc==5?std::stoi(argv[4]):0;
    const std::string prefix=argv[2];
    std::string error;
    auto movie=sadx::movie::mac::MoviePlayer::open(argv[1],error);
    if(!movie) throw std::runtime_error(error);
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,640,480)
        styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
    window.title=@"Native MPEG movie qualification";
    NSImageView* view=[[NSImageView alloc] initWithFrame:NSMakeRect(0,0,640,480)];
    view.imageScaling=NSImageScaleProportionallyUpOrDown;
    window.contentView=view;
    [window center];
    [window makeKeyAndOrderFront:nil];
    [NSApp finishLaunching];
    // Diagnostic output accepts an explicit setting; the game supplies its recovered live setting.
    if(!movie->start(volume,prefix+".wav",error)) throw std::runtime_error(error);
    const auto beginning=std::chrono::steady_clock::now();
    std::uint64_t displayed=0,last_index=0,audio_frames=0;
    bool complete=false,screenshot=false,aborted=false;
    for(;;) { @autoreleasepool {
        while(auto event=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:NSDate.distantPast
                    inMode:NSDefaultRunLoopMode dequeue:YES]) [NSApp sendEvent:event];
        if(!window.visible) { aborted=true;break; }
        auto update=movie->update();
        if(!update) throw std::runtime_error(update.error);
        audio_frames=update.presented_audio_frames;
        if(update.ready_frame) {
            view.image=native_image(*update.ready_frame);
            if(!view.image) throw std::runtime_error("native movie image presentation failed");
            [view displayIfNeeded];
            [window displayIfNeeded];
            ++displayed;
            last_index=update.ready_frame->index;
            if(!screenshot && update.ready_frame->presentation_seconds>=2) {
                if(!save_frame(*update.ready_frame,prefix+".ppm")) throw std::runtime_error("movie frame capture failed");
                screenshot=true;
            }
        }
        if(update.complete) { complete=true;break; }
        const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-beginning).count();
        if(limit>0&&seconds>=limit) { aborted=true;break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }}
    movie->stop();
    const auto stats=movie->audio_stats();
    [window orderOut:nil];
    std::cout << "{\"diagnostic_volume_hundredths_db\":" << volume
              << ",\"native_video_frames_presented\":" << displayed
              << ",\"last_video_index\":" << last_index << ",\"audio_presented_frames\":" << audio_frames
              << ",\"audio_submitted_frames\":" << stats.submitted_frames
              << ",\"audio_nonzero_samples\":" << stats.nonzero_samples
              << ",\"audio_completed_buffers\":" << stats.completed_buffers
              << ",\"device_error\":" << stats.device_error
              << ",\"movie_complete\":" << (complete?"true":"false")
              << ",\"diagnostic_aborted\":" << (aborted?"true":"false") << "}\n";
    return displayed>0&&stats.nonzero_samples>0&&stats.device_error==0?0:1;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;} } }

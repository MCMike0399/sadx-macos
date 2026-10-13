// sadx-app: a window on the game.
//   sadx-app --guest GAME_DIR WINDOWS_SYSTEM_DIR [--exit-after N] [--screenshot out.ppm] [--keys FRAME:VK[:FRAMES],...]
//     (the keyboard is forwarded to the guest as Windows keys; --keys scripts presses by guest frame)
//     The guest (M1): sonic.exe runs from its entry point in the in-process x86 guest with the mod loader and
//     mods (src/hle); every IDirect3DDevice8::Present pauses it, the frame it issued (Clears and
//     DrawPrimitiveUP draws with their textures and states) is drawn with Metal (platform/mac/metal_guest_frame),
//     and the guest resumes for the next frame. The overlay states the guest frame count and any stop.
//   sadx-app SONIC.EXE INPUT_MOD.DLL [--frames N] [--survey] [--exit-after N] [--screenshot out.ppm]
//     The native boot: the NativeGame composition of sadx-boot, then what it issued to Direct3D 8 (the Clear,
//     BeginScene's screen polygon) with the boot report as an overlay.
// Neither mode draws anything the game did not ask for.
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <map>
#include "runtime/game_session.hpp"
#include "platform/mac/mac_clock.hpp"
#include "platform/mac/metal_guest_frame.hpp"
#include "guest/fpu_shims.hpp"
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
namespace {
const char* kShader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Out {float4 position [[position]];float4 color;};
// XYZRHW|DIFFUSE (0x44) vertices, 20 bytes: x y z rhw colour(ARGB). D3D8 integer pixel centres -> Metal half-integer.
vertex Out polygon_vertex(uint index [[vertex_id]],constant uint* bytes [[buffer(0)]],constant float4& viewport [[buffer(1)]]){
    uint at=index*5;float x=as_type<float>(bytes[at]),y=as_type<float>(bytes[at+1]);uint c=bytes[at+4];
    float2 xy=(float2(x,y)+0.5f-viewport.xy)/viewport.zw;
    Out o;o.position=float4(xy.x*2.0f-1.0f,1.0f-xy.y*2.0f,0.0f,1.0f);
    o.color=float4(float((c>>16)&255),float((c>>8)&255),float(c&255),float(c>>24))/255.0f;return o;
}
fragment float4 polygon_fragment(Out in [[stage_in]]){return in.color;}
)METAL";
struct Options {std::string exe,dll,screenshot,game_dir,system_dir,keys;std::uint32_t frames=5;bool survey=true,guest=false;int exit_after=-1;};
bool parse(int argc,char** argv,Options& o){
    if(argc<3)return false;int first=3;
    if(!std::strcmp(argv[1],"--guest")){if(argc<4)return false;o.guest=true;o.game_dir=argv[2];o.system_dir=argv[3];first=4;}else{o.exe=argv[1];o.dll=argv[2];}
    for(int i=first;i<argc;++i){
        if(!std::strcmp(argv[i],"--frames")&&i+1<argc)o.frames=static_cast<std::uint32_t>(std::strtoul(argv[++i],nullptr,10));
        else if(!std::strcmp(argv[i],"--survey"))o.survey=true;
        else if(!std::strcmp(argv[i],"--stop"))o.survey=false;
        else if(!std::strcmp(argv[i],"--exit-after")&&i+1<argc)o.exit_after=std::atoi(argv[++i]);
        else if(!std::strcmp(argv[i],"--screenshot")&&i+1<argc)o.screenshot=argv[++i];
        else if(!std::strcmp(argv[i],"--keys")&&i+1<argc)o.keys=argv[++i];
        else return false;
    }
    return true;
}
// --keys "FRAME:VK[:FRAMES],...": press Windows virtual key VK at guest Present FRAME for FRAMES frames (default 5).
struct ScriptedKey {std::uint64_t frame=0;std::uint32_t vk=0,hold=5;};
std::vector<ScriptedKey> parse_keys(const std::string& text){
    std::vector<ScriptedKey> out;std::size_t at=0;
    while(at<text.size()){auto end=text.find(',',at);if(end==std::string::npos)end=text.size();const auto item=text.substr(at,end-at);at=end+1;
        ScriptedKey k;const auto a=item.find(':'),b=item.find(':',a+1);k.frame=std::stoull(item.substr(0,a));k.vk=static_cast<std::uint32_t>(std::stoul(item.substr(a+1,b-a-1),nullptr,0));
        if(b!=std::string::npos)k.hold=static_cast<std::uint32_t>(std::stoul(item.substr(b+1)));out.push_back(k);}
    return out;}
// macOS virtual key codes (Carbon kVK_*) to Windows virtual keys (US layout); 0: not forwarded.
std::uint32_t windows_key(unsigned short code){
    static const std::map<unsigned short,std::uint32_t> table={
        {0x00,'A'},{0x0b,'B'},{0x08,'C'},{0x02,'D'},{0x0e,'E'},{0x03,'F'},{0x05,'G'},{0x04,'H'},{0x22,'I'},{0x26,'J'},{0x28,'K'},{0x25,'L'},{0x2e,'M'},
        {0x2d,'N'},{0x1f,'O'},{0x23,'P'},{0x0c,'Q'},{0x0f,'R'},{0x01,'S'},{0x11,'T'},{0x20,'U'},{0x09,'V'},{0x0d,'W'},{0x07,'X'},{0x10,'Y'},{0x06,'Z'},
        {0x1d,'0'},{0x12,'1'},{0x13,'2'},{0x14,'3'},{0x15,'4'},{0x17,'5'},{0x16,'6'},{0x1a,'7'},{0x1c,'8'},{0x19,'9'},
        {0x24,0x0d},{0x35,0x1b},{0x31,0x20},{0x33,0x08},{0x30,0x09},{0x7b,0x25},{0x7e,0x26},{0x7c,0x27},{0x7d,0x28},
        {0x38,0xa0},{0x3c,0xa1},{0x3b,0xa2},{0x3e,0xa3},{0x3a,0xa4},{0x3d,0xa5},
        {0x52,0x60},{0x53,0x61},{0x54,0x62},{0x55,0x63},{0x56,0x64},{0x57,0x65},{0x58,0x66},{0x59,0x67},{0x5b,0x68},{0x5c,0x69}};
    const auto it=table.find(code);return it==table.end()?0:it->second;}
// Triangle fan -> triangle list indices.
std::vector<std::uint32_t> fan_indices(std::uint32_t vertex_count){std::vector<std::uint32_t> out;for(std::uint32_t i=1;i+1<vertex_count;++i){out.push_back(0);out.push_back(i);out.push_back(i+1);}return out;}
}
@interface SADXAppView : MTKView
@property(nonatomic) std::vector<sadx::NinjaScreenPolygon>* polygons;
@property(nonatomic) sadx::HleProcess* guest;              // guest mode
@property(nonatomic) sadx::MetalGuestFrameRenderer* guestRenderer;
@property(nonatomic) BOOL guestStopped;
@property(nonatomic) std::string* guestStop;
@property(nonatomic) id<MTLRenderPipelineState> pipeline;
@property(nonatomic) id<MTLCommandQueue> queue;
@property(nonatomic) int framesDrawn;
@property(nonatomic) int exitAfter;
@property(nonatomic) NSString* screenshot;
@property(nonatomic,weak) NSTextField* status;
@end
@implementation SADXAppView
// Keyboard: forwarded to the guest as Windows keys (the Input mod reads WM_KEYDOWN/WM_KEYUP).
- (BOOL)acceptsFirstResponder{return YES;}
- (void)keyDown:(NSEvent*)event{if(!self.guest)return;if(const auto vk=windows_key(event.keyCode))self.guest->host_key(vk,true);}
- (void)keyUp:(NSEvent*)event{if(!self.guest)return;if(const auto vk=windows_key(event.keyCode))self.guest->host_key(vk,false);}
- (void)flagsChanged:(NSEvent*)event{if(!self.guest)return;const auto vk=windows_key(event.keyCode);if(!vk)return;
    const NSEventModifierFlags flag=vk==0xa0||vk==0xa1?NSEventModifierFlagShift:vk==0xa2||vk==0xa3?NSEventModifierFlagControl:NSEventModifierFlagOption;
    self.guest->host_key(vk,(event.modifierFlags&flag)!=0);}
- (void)drawRect:(NSRect)rect{
    (void)rect;id<CAMetalDrawable> drawable=self.currentDrawable;MTLRenderPassDescriptor* pass=self.currentRenderPassDescriptor;if(!drawable||!pass)return;
    id<MTLCommandBuffer> commands=[self.queue commandBuffer];
    if(self.guest){[self drawGuest:commands pass:pass drawable:drawable];return;}
    pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,1); // the game's Clear: colour 0
    id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];
    // The game's viewport is 640x480; the window scales it to fit.
    [encoder setRenderPipelineState:self.pipeline];
    float viewport[4]={0,0,640,480};
    for(const auto& p:*self.polygons){
        const auto vertices=static_cast<std::uint32_t>(p.vertex_bytes.size()/p.stride);if(vertices<3)continue;
        const auto indices=fan_indices(vertices);
        id<MTLBuffer> vb=[self.device newBufferWithBytes:p.vertex_bytes.data() length:p.vertex_bytes.size() options:MTLResourceStorageModeShared];
        id<MTLBuffer> ib=[self.device newBufferWithBytes:indices.data() length:indices.size()*4 options:MTLResourceStorageModeShared];
        [encoder setVertexBuffer:vb offset:0 atIndex:0];[encoder setVertexBytes:viewport length:sizeof viewport atIndex:1];
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:indices.size() indexType:MTLIndexTypeUInt32 indexBuffer:ib indexBufferOffset:0];
    }
    [encoder endEncoding];
    const bool capture=self.screenshot!=nil&&self.framesDrawn+1==(self.exitAfter>0?self.exitAfter:1);
    if(capture){ // read the drawable back after the GPU finishes, write a binary PPM (golden-frame input)
        id<MTLTexture> texture=drawable.texture;NSString* path=self.screenshot;
        [commands addCompletedHandler:^(id<MTLCommandBuffer>){
            const auto w=texture.width,h=texture.height;std::vector<std::uint8_t> bgra(w*h*4);
            [texture getBytes:bgra.data() bytesPerRow:w*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0];
            std::ofstream f(path.UTF8String,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";
            for(std::size_t i=0;i<w*h;++i){f.put(static_cast<char>(bgra[4*i+2]));f.put(static_cast<char>(bgra[4*i+1]));f.put(static_cast<char>(bgra[4*i]));}
        }];
    }
    [commands presentDrawable:drawable];[commands commit];
    if(capture)[commands waitUntilCompleted];
    ++self.framesDrawn;
    if(self.exitAfter>0&&self.framesDrawn>=self.exitAfter)[NSApp terminate:nil];
}
// Guest mode: draw the frame the guest closed with its last Present, then let it run to the next one.
- (void)drawGuest:(id<MTLCommandBuffer>)commands pass:(MTLRenderPassDescriptor*)pass drawable:(id<CAMetalDrawable>)drawable{
    const auto stats=self.guestRenderer->encode(self.guest->last_frame,self.guest->guest(),commands,pass);
    const bool capture=self.screenshot!=nil&&self.framesDrawn+1==(self.exitAfter>0?self.exitAfter:1);
    if(capture){id<MTLTexture> texture=drawable.texture;NSString* path=self.screenshot;
        [commands addCompletedHandler:^(id<MTLCommandBuffer>){
            const auto w=texture.width,h=texture.height;std::vector<std::uint8_t> bgra(w*h*4);
            [texture getBytes:bgra.data() bytesPerRow:w*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0];
            std::ofstream f(path.UTF8String,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";
            for(std::size_t i=0;i<w*h;++i){f.put(static_cast<char>(bgra[4*i+2]));f.put(static_cast<char>(bgra[4*i+1]));f.put(static_cast<char>(bgra[4*i]));}}];}
    [commands presentDrawable:drawable];[commands commit];if(capture)[commands waitUntilCompleted];
    ++self.framesDrawn;
    std::string unsupported;for(const auto& [why,n]:stats.unsupported)unsupported+=" "+why+" x"+std::to_string(n);
    if(!self.guestStopped){const auto r=self.guest->guest().resume();if(!r.paused){self.guestStopped=YES;*self.guestStop=r.completed?"the guest returned from its entry point":r.error;}}
    std::printf("frame %d: guest present %llu, %zu draws (%zu textured), %zu clears%s%s\n",self.framesDrawn,static_cast<unsigned long long>(self.guest->d3d8.presents),stats.draws,stats.textured,stats.clears,
        unsupported.empty()?"":(", not drawn:"+unsupported).c_str(),self.guestStopped?(", guest stopped: "+*self.guestStop).c_str():"");
    self.status.stringValue=[NSString stringWithFormat:@"Guest: sonic.exe + mod loader + mods, frame %llu (%zu draws, %zu textured)%@",static_cast<unsigned long long>(self.guest->d3d8.presents),stats.draws,stats.textured,
        self.guestStopped?[NSString stringWithFormat:@" — guest stopped: %s",self.guestStop->c_str()]:@""];
    if(self.exitAfter>0&&self.framesDrawn>=self.exitAfter)[NSApp terminate:nil];
}
@end
namespace {
// Guest mode: boot the guest to its first Present, then hand it to the view.
int run_guest(const Options& o){
    std::ifstream f(o.game_dir+"/sonic.exe",std::ios::binary);if(!f){std::fprintf(stderr,"cannot read %s/sonic.exe\n",o.game_dir.c_str());return 2;}
    std::string error;const auto image=sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);if(!image){std::fprintf(stderr,"%s\n",error.c_str());return 2;}
    auto process=sadx::HleProcess::create(o.game_dir,"build/sadx-app-overlay",&error);if(!process){std::fprintf(stderr,"%s\n",error.c_str());return 2;}
    process->use_real_cpp_runtime(o.system_dir);process->hle().set_logging(false);
    if(!process->map_main(*image,&error)){std::fprintf(stderr,"%s\n",error.c_str());return 2;}
    sadx::FpuShimStats shims;if(!sadx::install_fpu_shims(process->guest(),sadx::load_fpu_sites("reference/inventory/fpu-sites.json"),shims,&error)){std::fprintf(stderr,"%s\n",error.c_str());return 2;}
    auto* P=process.get();const auto script=parse_keys(o.keys);
    process->on_present=[P,script](sadx::X86Guest& g){
        for(const auto& k:script){if(P->d3d8.presents==k.frame)P->host_key(k.vk,true);if(P->d3d8.presents==k.frame+k.hold)P->host_key(k.vk,false);}
        g.pause();};
    const auto first=process->guest().call(0x643f96,{});
    std::string stop;if(!first.paused){std::fprintf(stderr,"the guest did not reach its first Present: %s\n",first.error.c_str());return 4;}
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();if(!device){std::fprintf(stderr,"no Metal device\n");return 3;}
    auto renderer=sadx::MetalGuestFrameRenderer::create(device,&error);if(!renderer){std::fprintf(stderr,"shader: %s\n",error.c_str());return 3;}
    NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,1280,960) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    window.title=@"sadx-macos — guest: sonic.exe with the speedrun profile";
    SADXAppView* view=[[SADXAppView alloc] initWithFrame:NSMakeRect(0,0,1280,960) device:device];
    view.colorPixelFormat=MTLPixelFormatBGRA8Unorm;view.depthStencilPixelFormat=MTLPixelFormatDepth32Float;view.framebufferOnly=NO;view.preferredFramesPerSecond=63;view.drawableSize=CGSizeMake(640,480);view.autoResizeDrawable=NO;
    view.queue=[device newCommandQueue];view.exitAfter=o.exit_after;view.screenshot=o.screenshot.empty()?nil:[NSString stringWithUTF8String:o.screenshot.c_str()];
    view.guest=process.get();view.guestRenderer=renderer.get();view.guestStop=&stop;window.contentView=view;
    NSTextField* status=[NSTextField labelWithString:@"Guest"];status.font=[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    status.textColor=[NSColor colorWithWhite:0.9 alpha:1];status.backgroundColor=[NSColor colorWithWhite:0 alpha:0.55];status.drawsBackground=YES;
    status.frame=NSMakeRect(8,8,1264,36);[view addSubview:status];view.status=status;
    [window center];[window makeKeyAndOrderFront:nil];[window makeFirstResponder:view];[NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
    std::printf("guest assumptions (%zu):\n",process->assumptions.size());for(const auto& a:process->assumptions)std::printf("  %s\n",a.c_str());
    return view.guestStopped&&o.exit_after>0&&view.framesDrawn<o.exit_after?5:0;
}
}
int main(int argc,char** argv){
    Options o;if(!parse(argc,argv,o)){std::fprintf(stderr,"usage: %s --guest GAME_DIR WINDOWS_SYSTEM_DIR [--exit-after N] [--screenshot out.ppm] [--keys FRAME:VK[:FRAMES],...]\n       %s SONIC.EXE INPUT_MOD.DLL [--frames N] [--survey|--stop] [--exit-after N] [--screenshot out.ppm]\n",argv[0],argv[0]);return 2;}
    if(o.guest){@autoreleasepool{return run_guest(o);}}
    @autoreleasepool{
        sadx::MacPerformanceCounter counter;
        const auto policy=o.survey?sadx::OwnerPolicy::survey_unported_as_noop:sadx::OwnerPolicy::stop_at_unported;
        auto loaded=sadx::load_boot_config(o.exe,o.dll,o.frames,policy,counter.limiter_clock());
        if(!loaded){std::fprintf(stderr,"%s\n",loaded.error.c_str());return 2;}
        sadx::NativeGame game(*loaded.config);const auto report=game.run();
        const auto text=sadx::describe_report(report,game.d3d8(),policy);std::fputs(text.c_str(),stdout);
        auto polygons=game.d3d8().polygons; // copies the D3D8 stream the frames issued
        [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();if(!device){std::fprintf(stderr,"no Metal device\n");return 3;}
        NSError* error=nil;id<MTLLibrary> library=[device newLibraryWithSource:[NSString stringWithUTF8String:kShader] options:nil error:&error];
        if(!library){std::fprintf(stderr,"shader: %s\n",error.localizedDescription.UTF8String);return 3;}
        MTLRenderPipelineDescriptor* desc=[MTLRenderPipelineDescriptor new];desc.vertexFunction=[library newFunctionWithName:@"polygon_vertex"];desc.fragmentFunction=[library newFunctionWithName:@"polygon_fragment"];
        desc.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
        id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:desc error:&error];
        if(!pipeline){std::fprintf(stderr,"pipeline: %s\n",error.localizedDescription.UTF8String);return 3;}
        NSWindow* window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,1280,960) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
        window.title=[NSString stringWithFormat:@"sadx-macos — native boot, %u frame(s), %s",report.frames_completed,o.survey?"survey":"stop at unported"];
        SADXAppView* view=[[SADXAppView alloc] initWithFrame:NSMakeRect(0,0,1280,960) device:device];
        view.colorPixelFormat=MTLPixelFormatBGRA8Unorm;view.framebufferOnly=NO;view.preferredFramesPerSecond=60;view.drawableSize=CGSizeMake(640,480);view.autoResizeDrawable=NO;
        view.polygons=&polygons;view.pipeline=pipeline;view.queue=[device newCommandQueue];view.exitAfter=o.exit_after;view.screenshot=o.screenshot.empty()?nil:[NSString stringWithUTF8String:o.screenshot.c_str()];
        window.contentView=view;
        // Overlay: the boot report, so the window says exactly what it is showing.
        std::string overlay="Native boot report (what the game issued to D3D8 in these frames is drawn behind this text):\n"+text;
        NSTextField* status=[NSTextField wrappingLabelWithString:[NSString stringWithUTF8String:overlay.c_str()]];
        status.font=[NSFont monospacedSystemFontOfSize:10 weight:NSFontWeightRegular];status.textColor=[NSColor colorWithWhite:0.9 alpha:1];status.backgroundColor=[NSColor colorWithWhite:0 alpha:0.55];status.drawsBackground=YES;
        status.frame=NSMakeRect(8,8,1264,944);status.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;[view addSubview:status];view.status=status;
        [window center];[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

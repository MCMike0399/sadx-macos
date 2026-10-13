// sadx-app: a window on the native boot. It runs the same NativeGame composition as
// sadx-boot, then shows what the game actually issued to Direct3D 8 in those frames
// (the Clear, BeginScene's screen polygon, any sprite draws) in a Metal view, with
// the boot report as an overlay. It renders nothing the game did not ask for: before
// the SEGA logo's texture loads natively, the game's own frame is a black screen.
//   sadx-app SONIC.EXE INPUT_MOD.DLL [--frames N] [--survey] [--exit-after N] [--screenshot out.ppm]
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "runtime/game_session.hpp"
#include "platform/mac/mac_clock.hpp"
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
struct Options {std::string exe,dll,screenshot;std::uint32_t frames=5;bool survey=true;int exit_after=-1;};
bool parse(int argc,char** argv,Options& o){
    if(argc<3)return false;o.exe=argv[1];o.dll=argv[2];
    for(int i=3;i<argc;++i){
        if(!std::strcmp(argv[i],"--frames")&&i+1<argc)o.frames=static_cast<std::uint32_t>(std::strtoul(argv[++i],nullptr,10));
        else if(!std::strcmp(argv[i],"--survey"))o.survey=true;
        else if(!std::strcmp(argv[i],"--stop"))o.survey=false;
        else if(!std::strcmp(argv[i],"--exit-after")&&i+1<argc)o.exit_after=std::atoi(argv[++i]);
        else if(!std::strcmp(argv[i],"--screenshot")&&i+1<argc)o.screenshot=argv[++i];
        else return false;
    }
    return true;
}
// Triangle fan -> triangle list indices.
std::vector<std::uint32_t> fan_indices(std::uint32_t vertex_count){std::vector<std::uint32_t> out;for(std::uint32_t i=1;i+1<vertex_count;++i){out.push_back(0);out.push_back(i);out.push_back(i+1);}return out;}
}
@interface SADXAppView : MTKView
@property(nonatomic) std::vector<sadx::NinjaScreenPolygon>* polygons;
@property(nonatomic) id<MTLRenderPipelineState> pipeline;
@property(nonatomic) id<MTLCommandQueue> queue;
@property(nonatomic) int framesDrawn;
@property(nonatomic) int exitAfter;
@property(nonatomic) NSString* screenshot;
@property(nonatomic,weak) NSTextField* status;
@end
@implementation SADXAppView
- (void)drawRect:(NSRect)rect{
    (void)rect;id<CAMetalDrawable> drawable=self.currentDrawable;MTLRenderPassDescriptor* pass=self.currentRenderPassDescriptor;if(!drawable||!pass)return;
    pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,1); // the game's Clear: colour 0
    id<MTLCommandBuffer> commands=[self.queue commandBuffer];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];
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
@end
int main(int argc,char** argv){
    Options o;if(!parse(argc,argv,o)){std::fprintf(stderr,"usage: %s SONIC.EXE INPUT_MOD.DLL [--frames N] [--survey|--stop] [--exit-after N] [--screenshot out.ppm]\n",argv[0]);return 2;}
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

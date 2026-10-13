#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <GameController/GameController.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

// This executable exercises the future game's native rendering boundary with
// real land geometry. It has no gameplay loop and does not load Windows DLLs.
struct Vertex { float position[4]; float color[4]; };
using Vec = std::array<float, 3>;
using Matrix = std::array<float, 16>;
static_assert(sizeof(Vertex) == 32);

static Vec sub(Vec a, Vec b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
static float dot(Vec a, Vec b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
static Vec cross(Vec a, Vec b) { return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}; }
static Vec unit(Vec a) {
    float length = std::sqrt(dot(a, a));
    if (!(length > 0)) throw std::runtime_error("Invalid camera vector");
    return {a[0]/length, a[1]/length, a[2]/length};
}
static Matrix multiply(const Matrix& a, const Matrix& b) {
    Matrix c{};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k) c[col*4+row] += a[k*4+row]*b[col*4+k];
    return c;
}

static Matrix camera(const std::vector<Vertex>& vertices, float aspect) {
    Vec low{INFINITY, INFINITY, INFINITY}, high{-INFINITY, -INFINITY, -INFINITY};
    for (const auto& v : vertices) for (int i=0; i<3; ++i) {
        low[i] = std::min(low[i], v.position[i]); high[i] = std::max(high[i], v.position[i]);
    }
    Vec center{}, span{};
    for (int i=0; i<3; ++i) { center[i] = (low[i]+high[i])/2; span[i]=high[i]-low[i]; }
    float radius=std::sqrt(dot(span, span))/2;
    if (!(radius > 0) || !std::isfinite(radius)) throw std::runtime_error("Empty geometry bounds");
    // Frustum fits the complete stage sphere. This is an inspection camera,
    // independent of the game's camera code and its state.
    float fov = 0.8f;
    float distance = radius / std::sin(fov/2) * 1.12f;
    Vec direction = unit({0.12f, 0.92f, -0.36f});
    Vec eye{};
    for(int i=0;i<3;++i) eye[i]=center[i]+direction[i]*distance;
    Vec z=unit(sub(eye,center)), x=unit(cross({0,1,0},z)), y=cross(z,x);
    Matrix view{x[0],y[0],z[0],0, x[1],y[1],z[1],0, x[2],y[2],z[2],0, -dot(x,eye),-dot(y,eye),-dot(z,eye),1};
    float nearPlane = std::max(1.0f, distance-radius*1.1f), farPlane=distance+radius*1.1f;
    float sy=1/std::tan(fov/2), sx=sy/aspect;
    Matrix projection{sx,0,0,0, 0,sy,0,0, 0,0,farPlane/(nearPlane-farPlane),-1, 0,0,nearPlane*farPlane/(nearPlane-farPlane),0};
    return multiply(projection,view);
}

static std::vector<Vertex> loadMesh(const char* path) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    if(!stream) throw std::runtime_error("Cannot open mesh");
    auto size=stream.tellg();
    if(size<12) throw std::runtime_error("Truncated mesh header");
    stream.seekg(0);
    char magic[8]; uint32_t count;
    stream.read(magic,8); stream.read(reinterpret_cast<char*>(&count),4);
    if(std::memcmp(magic,"SDXMESH1",8) || count == 0 || count % 3 ||
       static_cast<uint64_t>(size) != 12+static_cast<uint64_t>(count)*sizeof(Vertex))
        throw std::runtime_error("Invalid mesh shape or length");
    std::vector<Vertex> vertices(count);
    stream.read(reinterpret_cast<char*>(vertices.data()),count*sizeof(Vertex));
    if(!stream) throw std::runtime_error("Mesh read failed");
    for(const auto& vertex: vertices) {
        for(float value: vertex.position) if(!std::isfinite(value)) throw std::runtime_error("Non-finite vertex");
        for(float value: vertex.color) if(!std::isfinite(value)) throw std::runtime_error("Non-finite color");
    }
    return vertices;
}

static const char* shader=R"(
#include <metal_stdlib>
using namespace metal;
struct Vertex { float4 position; float4 color; };
struct Output { float4 position [[position]]; float4 color; };
vertex Output stage_vertex(uint id [[vertex_id]], constant Vertex* vertices [[buffer(0)]],
                          constant float4x4& camera [[buffer(1)]]) {
    return {camera * vertices[id].position, vertices[id].color};
}
fragment float4 stage_fragment(Output in [[stage_in]]) { return in.color; }
)";

static int render(const char* meshPath, const char* outputPath) {
    auto vertices=loadMesh(meshPath);
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();
    if(!device) throw std::runtime_error("No Metal device available to this process");
    NSError* error=nil;
    MTLCompileOptions* options=[MTLCompileOptions new]; options.fastMathEnabled=NO;
    id<MTLLibrary> library=[device newLibraryWithSource:[NSString stringWithUTF8String:shader] options:options error:&error];
    if(!library) throw std::runtime_error([[error localizedDescription] UTF8String]);
    MTLRenderPipelineDescriptor* pipelineDescriptor=[MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction=[library newFunctionWithName:@"stage_vertex"];
    pipelineDescriptor.fragmentFunction=[library newFunctionWithName:@"stage_fragment"];
    pipelineDescriptor.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm;
    pipelineDescriptor.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if(!pipeline) throw std::runtime_error([[error localizedDescription] UTF8String]);
    id<MTLCommandQueue> queue=[device newCommandQueue];
    id<MTLBuffer> vertexBuffer=[device newBufferWithBytes:vertices.data() length:vertices.size()*sizeof(Vertex) options:MTLResourceStorageModeShared];
    constexpr NSUInteger width=1280, height=720;
    auto textureDescriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
    textureDescriptor.usage=MTLTextureUsageRenderTarget;
    textureDescriptor.storageMode=MTLStorageModeShared;
    id<MTLTexture> target=[device newTextureWithDescriptor:textureDescriptor];
    auto depthDescriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:width height:height mipmapped:NO];
    depthDescriptor.usage=MTLTextureUsageRenderTarget;
    depthDescriptor.storageMode=MTLStorageModePrivate;
    id<MTLTexture> depth=[device newTextureWithDescriptor:depthDescriptor];
    if(!queue || !vertexBuffer || !target || !depth) throw std::runtime_error("Metal resource allocation failed");
    auto depthStateDescriptor=[MTLDepthStencilDescriptor new];
    depthStateDescriptor.depthCompareFunction=MTLCompareFunctionLess;
    depthStateDescriptor.depthWriteEnabled=YES;
    id<MTLDepthStencilState> depthState=[device newDepthStencilStateWithDescriptor:depthStateDescriptor];
    if(!depthState) throw std::runtime_error("Depth state allocation failed");
    auto pass=[MTLRenderPassDescriptor new];
    pass.colorAttachments[0].texture=target;
    pass.colorAttachments[0].loadAction=MTLLoadActionClear;
    pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    pass.colorAttachments[0].clearColor=MTLClearColorMake(0.035,0.09,0.15,1);
    pass.depthAttachment.texture=depth;
    pass.depthAttachment.loadAction=MTLLoadActionClear;
    pass.depthAttachment.storeAction=MTLStoreActionDontCare;
    pass.depthAttachment.clearDepth=1;
    id<MTLCommandBuffer> command=[queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
    if(!command || !encoder) throw std::runtime_error("Metal encoder unavailable");
    auto matrix=camera(vertices,static_cast<float>(width)/height);
    [encoder setRenderPipelineState:pipeline];
    [encoder setDepthStencilState:depthState];
    [encoder setCullMode:MTLCullModeNone];
    [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    [encoder setVertexBytes:matrix.data() length:sizeof(matrix) atIndex:1];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertices.size()];
    [encoder endEncoding];
    [command commit]; [command waitUntilCompleted];
    if(command.status!=MTLCommandBufferStatusCompleted)
        throw std::runtime_error(command.error ? [[command.error localizedDescription] UTF8String] : "Metal command did not complete");
    std::vector<uint8_t> pixels(width*height*4);
    [target getBytes:pixels.data() bytesPerRow:width*4 fromRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0];
    std::ofstream image(outputPath,std::ios::binary);
    if(!image) throw std::runtime_error("Cannot open output image");
    image<<"P6\n"<<width<<" "<<height<<"\n255\n";
    uint64_t checksum=14695981039346656037ull;
    size_t changed=0;
    for(size_t i=0;i<pixels.size();i+=4) {
        image.write(reinterpret_cast<const char*>(&pixels[i]),3);
        for(int c=0;c<3;++c) { checksum^=pixels[i+c]; checksum*=1099511628211ull; }
        if(pixels[i]!=9 || pixels[i+1]!=23 || pixels[i+2]!=38) ++changed;
    }
    image.close();
    if(!image) throw std::runtime_error("Output image write failed");
    if(!changed) throw std::runtime_error("Render contains only the clear color");
    std::cout<<"{\"device\":\""<<[[device name] UTF8String]<<"\",\"architecture\":\"arm64\","
             <<"\"vertices\":"<<vertices.size()<<",\"width\":"<<width<<",\"height\":"<<height
             <<",\"non_background_pixels\":"<<changed<<",\"rgb_fnv1a64\":\""<<std::hex<<checksum<<std::dec
             <<"\",\"gpu_seconds\":"<<(command.GPUEndTime-command.GPUStartTime)<<",\"gameplay\":false}\n";
    return 0;
}

static int controllers() {
    // The run loop lets Apple's framework publish currently connected devices.
    // No discovery, pairing or synthetic controller is initiated.
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
    NSMutableArray* devices=[NSMutableArray array];
    for(GCController* controller in [GCController controllers]) {
        NSMutableDictionary* info=[NSMutableDictionary dictionary];
        info[@"vendor"]=controller.vendorName ?: [NSNull null];
        info[@"category"]=controller.productCategory ?: [NSNull null];
        info[@"extended_gamepad"]=@(controller.extendedGamepad != nil);
        info[@"physical_input"]=@(controller.physicalInputProfile != nil);
        if(controller.extendedGamepad) {
            auto pad=controller.extendedGamepad;
            info[@"left_stick"]=@[@(pad.leftThumbstick.xAxis.value),@(pad.leftThumbstick.yAxis.value)];
            info[@"start_pressed"]=@(pad.buttonMenu.isPressed);
        }
        [devices addObject:info];
    }
    NSDictionary* result=@{@"backend":@"Apple GameController", @"controllers":devices,
                           @"hardware_support_verified":@NO, @"gameplay_mapping_implemented":@NO};
    NSError* error=nil;
    NSData* json=[NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted error:&error];
    if(!json) throw std::runtime_error([[error localizedDescription] UTF8String]);
    std::cout<<std::string(static_cast<const char*>(json.bytes),json.length)<<"\n";
    return 0;
}

int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            if(argc==2 && std::strcmp(argv[1],"--controllers")==0) return controllers();
            if(argc==4 && std::strcmp(argv[1],"--render")==0) return render(argv[2],argv[3]);
            std::cerr<<"Usage: sadx-metal --render emerald-coast.mesh output.ppm | --controllers\n";
            return 2;
        } catch(const std::exception& error) {
            std::cerr<<error.what()<<"\n";
            return 1;
        }
    }
}

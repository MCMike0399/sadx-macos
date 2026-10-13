#include "platform/mac/metal_texture.hpp"
#import <Foundation/Foundation.h>
#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <iomanip>
namespace sadx {
namespace {
const char* shader=R"METAL(
#include <metal_stdlib>
using namespace metal;
kernel void decode_source_texture(texture2d<ushort, access::read> input [[texture(0)]],
    texture2d<float, access::write> output [[texture(1)]],constant uint2& state [[buffer(0)]],
    uint2 point [[thread_position_in_grid]]) {
    uint level=state.y;
    if(point.x>=output.get_width(level)||point.y>=output.get_height(level))return;
    uint p=input.read(point,level).r;
    float4 color;
    if(state.x==0)color=float4(float((p>>10)&31)/31.0f,float((p>>5)&31)/31.0f,float(p&31)/31.0f,float(p>>15));
    else if(state.x==1)color=float4(float((p>>11)&31)/31.0f,float((p>>5)&63)/63.0f,float(p&31)/31.0f,1.0f);
    else color=float4(float((p>>8)&15)/15.0f,float((p>>4)&15)/15.0f,float(p&15)/15.0f,float((p>>12)&15)/15.0f);
    output.write(color,point,level);
}
)METAL";
std::string message(NSError* error,const char* fallback){return error?std::string(error.localizedDescription.UTF8String):fallback;}
std::string hash(const std::vector<std::uint8_t>& bytes){
    CC_SHA256_CTX context;CC_SHA256_Init(&context);std::size_t offset=0;
    while(offset<bytes.size()){const auto n=std::min(bytes.size()-offset,std::size_t(std::numeric_limits<CC_LONG>::max()));CC_SHA256_Update(&context,bytes.data()+offset,static_cast<CC_LONG>(n));offset+=n;}
    std::array<unsigned char,CC_SHA256_DIGEST_LENGTH> digest;CC_SHA256_Final(digest.data(),&context);
    std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto b:digest)out<<std::setw(2)<<unsigned(b);return out.str();
}
}
struct MetalTextureResource::Impl {
    std::shared_ptr<const NinjaTextureArchive> archive;std::size_t index;
    NinjaTextureDecodeResult decoded;std::string digest;bool completed=false;
    id<MTLTexture> packed=nil,normalized=nil;
};
struct MetalTextureUploader::Impl {
    id<MTLDevice> device;id<MTLCommandQueue> queue;id<MTLComputePipelineState> pipeline;
    std::string name;bool filtering;
};
MetalTextureResource::MetalTextureResource(std::unique_ptr<Impl> state):impl(std::move(state)){}
MetalTextureResource::~MetalTextureResource()=default;
const NinjaTextureArchive& MetalTextureResource::archive()const noexcept{return *impl->archive;}
const NinjaTextureDecodeResult& MetalTextureResource::decoded()const noexcept{return impl->decoded;}
std::size_t MetalTextureResource::record_index()const noexcept{return impl->index;}
const std::string& MetalTextureResource::archive_sha256()const noexcept{return impl->digest;}
bool MetalTextureResource::ready()const noexcept{return impl->completed;}
id<MTLTexture> MetalTextureResource::packed_texture()const noexcept{return impl->packed;}
id<MTLTexture> MetalTextureResource::normalized_texture()const noexcept{return impl->normalized;}
MetalTextureUploader::MetalTextureUploader(std::unique_ptr<Impl> state):impl(std::move(state)){}
MetalTextureUploader::~MetalTextureUploader()=default;
const std::string& MetalTextureUploader::device_name()const noexcept{return impl->name;}
bool MetalTextureUploader::supports_float_filtering()const noexcept{return impl->filtering;}
const char* metal_texture_shader_source()noexcept{return shader;}
MetalTextureContextResult MetalTextureUploader::create(id<MTLDevice> device,id<MTLCommandQueue> queue){
    if(!device)return {MetalTextureFailure::missing_device,"Caller supplied no device",{}};
    if(!queue)return {MetalTextureFailure::missing_queue,"Caller supplied no queue",{}};
    if(queue.device!=device)return {MetalTextureFailure::device_mismatch,"Queue and device owners differ",{}};
    // This implementation deliberately targets the Apple Silicon shared-memory
    // profile. Other storage/synchronization models remain explicit boundaries.
    if(!device.hasUnifiedMemory)return {MetalTextureFailure::unsupported_memory_profile,"Shared Apple Silicon texture profile required",{}};
    MTLCompileOptions* options=[MTLCompileOptions new];
    if(@available(macOS 15.0,*)){options.mathMode=MTLMathModeSafe;options.mathFloatingPointFunctions=MTLMathFloatingPointFunctionsPrecise;}
    else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        options.fastMathEnabled=NO;
#pragma clang diagnostic pop
    }
    NSError* error=nil;id<MTLLibrary> library=[device newLibraryWithSource:[NSString stringWithUTF8String:shader] options:options error:&error];
    if(!library)return {MetalTextureFailure::shader_compile_failed,message(error,"Metal library unavailable"),{}};
    id<MTLFunction> function=[library newFunctionWithName:@"decode_source_texture"];
    id<MTLComputePipelineState> pipeline=function?[device newComputePipelineStateWithFunction:function error:&error]:nil;
    if(!pipeline)return {MetalTextureFailure::pipeline_failed,message(error,"Metal pipeline unavailable"),{}};
    auto state=std::make_unique<Impl>();state->device=device;state->queue=queue;state->pipeline=pipeline;state->name=device.name.UTF8String;state->filtering=device.supports32BitFloatFiltering;
    return {MetalTextureFailure::none,{},std::shared_ptr<MetalTextureUploader>(new MetalTextureUploader(std::move(state)))};
}
MetalTextureUploadResult MetalTextureUploader::upload(std::shared_ptr<const NinjaTextureArchive> archive,std::size_t index){
    MetalTextureUploadResult result;if(!archive){result.failure=MetalTextureFailure::missing_archive;return result;}
    auto state=std::make_unique<MetalTextureResource::Impl>();state->archive=std::move(archive);state->index=index;
    state->decoded=decode_ninja_texture(*state->archive,index);state->digest=hash(state->archive->bytes);
    result.resource=std::shared_ptr<MetalTextureResource>(new MetalTextureResource(std::move(state)));auto& resource=*result.resource->impl;
    if(!resource.decoded){result.failure=MetalTextureFailure::decode_failed;result.decode_failure=resource.decoded.failure;return result;}
    const auto fail=[&](MetalTextureFailure why,const std::string& detail){result.failure=why;result.detail=detail;return result;};
    const auto& base=resource.decoded.levels[0];const auto make=[&](MTLPixelFormat format,MTLTextureUsage usage){
        auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:base.width height:base.height mipmapped:resource.decoded.levels.size()>1];descriptor.mipmapLevelCount=resource.decoded.levels.size();descriptor.storageMode=MTLStorageModeShared;descriptor.usage=usage;
        return [impl->device newTextureWithDescriptor:descriptor];
    };
    resource.packed=make(MTLPixelFormatR16Uint,MTLTextureUsageShaderRead);
    resource.normalized=make(MTLPixelFormatRGBA32Float,MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite);
    if(!resource.packed||!resource.normalized)return fail(MetalTextureFailure::allocation_failed,"Metal texture allocation failed");
    for(std::size_t level=0;level<resource.decoded.levels.size();++level){const auto& data=resource.decoded.levels[level];[resource.packed replaceRegion:MTLRegionMake2D(0,0,data.width,data.height) mipmapLevel:level withBytes:data.packed16.data() bytesPerRow:std::size_t(data.width)*2];}
    id<MTLCommandBuffer> command=[impl->queue commandBuffer];id<MTLComputeCommandEncoder> encoder=[command computeCommandEncoder];
    if(!command||!encoder)return fail(MetalTextureFailure::encoder_failed,"Metal command/encoder unavailable");
    [encoder setComputePipelineState:impl->pipeline];[encoder setTexture:resource.packed atIndex:0];[encoder setTexture:resource.normalized atIndex:1];
    for(std::size_t level=0;level<resource.decoded.levels.size();++level){const auto& data=resource.decoded.levels[level];std::array<std::uint32_t,2> parameters{{resource.archive->records[index].pixel_format,static_cast<std::uint32_t>(level)}};
        [encoder setBytes:parameters.data() length:sizeof(parameters) atIndex:0];
        [encoder dispatchThreads:MTLSizeMake(data.width,data.height,1) threadsPerThreadgroup:MTLSizeMake(impl->pipeline.threadExecutionWidth,1,1)];
    }
    [encoder endEncoding];[command commit];[command waitUntilCompleted];
    if(command.status!=MTLCommandBufferStatusCompleted)return fail(MetalTextureFailure::command_failed,message(command.error,"GPU command did not complete"));
    resource.completed=true;return result;
}
MetalTextureReadbackResult read_metal_texture(const std::shared_ptr<const MetalTextureResource>& resource,std::size_t level){
    MetalTextureReadbackResult result;if(!resource||!resource->ready()){result.failure=MetalTextureFailure::not_ready;return result;}
    if(level>=resource->decoded().levels.size()){result.failure=MetalTextureFailure::invalid_level;return result;}
    const auto& metadata=resource->decoded().levels[level];result.width=metadata.width;result.height=metadata.height;result.rgba32f.resize(std::size_t(metadata.width)*metadata.height*4);
    [resource->normalized_texture() getBytes:result.rgba32f.data() bytesPerRow:std::size_t(metadata.width)*16 fromRegion:MTLRegionMake2D(0,0,metadata.width,metadata.height) mipmapLevel:level];return result;
}
} // namespace sadx

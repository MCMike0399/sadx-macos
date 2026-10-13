#include "platform/mac/metal_sprite.hpp"
#import <Foundation/Foundation.h>
#include <array>
#include <cmath>
#include <cstring>
namespace sadx {
namespace {
const char* shader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Parameters {float4 viewport;uint color_op,color_a,color_b,alpha_op,alpha_a,alpha_b,alpha_test,alpha_func,alpha_ref;};
struct Output {float4 position [[position]];float2 uv;float4 diffuse;};
vertex Output source_screen_sprite(uint index [[vertex_id]],constant uint* bytes [[buffer(0)]],constant Parameters& p [[buffer(1)]]) {
    uint at=index*7;float x=as_type<float>(bytes[at]),y=as_type<float>(bytes[at+1]);uint c=bytes[at+4];
    Output result;
    // D3D8 integer pixel centers become Metal half-integer pixel centers.
    float2 xy=(float2(x,y)+0.5f-p.viewport.xy)/p.viewport.zw;
    result.position=float4(xy.x*2.0f-1.0f,1.0f-xy.y*2.0f,as_type<float>(bytes[at+2]),1.0f);
    result.uv=float2(as_type<float>(bytes[at+5]),as_type<float>(bytes[at+6]));
    result.diffuse=float4(float((c>>16)&255),float((c>>8)&255),float(c&255),float(c>>24))/255.0f;
    return result;
}
float4 argument(uint which,float4 diffuse,float4 texel){return which==2?texel:diffuse;}
float4 operation(uint op,float4 a,float4 b){if(op==2)return a;if(op==3)return b;return a*b;}
bool alpha_pass(uint function,float value,float reference){
    if(function==1)return false;if(function==2)return value<reference;if(function==3)return value==reference;
    if(function==4)return value<=reference;if(function==5)return value>reference;if(function==6)return value!=reference;
    if(function==7)return value>=reference;return true;
}
fragment float4 source_sprite_fragment(Output input [[stage_in]],texture2d<float> texture [[texture(0)]],sampler sampling [[sampler(0)]],constant Parameters& p [[buffer(1)]]) {
    float4 texel=texture.sample(sampling,input.uv);
    float4 color=operation(p.color_op,argument(p.color_a,input.diffuse,texel),argument(p.color_b,input.diffuse,texel));
    color.a=operation(p.alpha_op,argument(p.alpha_a,input.diffuse,texel),argument(p.alpha_b,input.diffuse,texel)).a;
    if(p.alpha_test&&!alpha_pass(p.alpha_func,color.a,float(p.alpha_ref)/255.0f))discard_fragment();
    return saturate(color);
}
)METAL";
std::string detail(NSError* e,const char* fallback){return e?e.localizedDescription.UTF8String:fallback;}
struct alignas(16) Parameters {std::array<float,4> viewport;std::uint32_t color_op,color_a,color_b,alpha_op,alpha_a,alpha_b,alpha_test,alpha_func,alpha_ref;};
struct StateBoundary {MetalSpriteFailure failure;std::uint32_t state;bool stage;};
std::uint32_t need(const MetalSpriteSourceState& state,std::uint32_t key,bool stage=false){
    const auto& values=stage?state.stage_zero:state.render;const auto found=values.find(key);
    if(found==values.end())throw StateBoundary{MetalSpriteFailure::missing_source_state,key,stage};return found->second;
}
std::uint32_t bounded(const MetalSpriteSourceState& state,std::uint32_t key,std::uint32_t lo,std::uint32_t hi,bool stage=false){auto value=need(state,key,stage);if(value<lo||value>hi)throw StateBoundary{MetalSpriteFailure::unsupported_source_state,key,stage};return value;}
void exact(const MetalSpriteSourceState& state,std::uint32_t key,std::uint32_t value,bool stage=false){if(need(state,key,stage)!=value)throw StateBoundary{MetalSpriteFailure::unsupported_source_state,key,stage};}
MTLBlendFactor blend(std::uint32_t v){switch(v){case 1:return MTLBlendFactorZero;case 2:return MTLBlendFactorOne;case 3:return MTLBlendFactorSourceColor;case 4:return MTLBlendFactorOneMinusSourceColor;case 5:return MTLBlendFactorSourceAlpha;case 6:return MTLBlendFactorOneMinusSourceAlpha;case 7:return MTLBlendFactorDestinationAlpha;case 8:return MTLBlendFactorOneMinusDestinationAlpha;case 9:return MTLBlendFactorDestinationColor;case 10:return MTLBlendFactorOneMinusDestinationColor;default:return MTLBlendFactorSourceAlphaSaturated;}}
MTLBlendFactor alpha_blend(std::uint32_t v){if(v==3)v=5;else if(v==4)v=6;else if(v==9)v=7;else if(v==10)v=8;else if(v==11)v=2;return blend(v);}
MTLCompareFunction compare(std::uint32_t v){return static_cast<MTLCompareFunction>(v-1);}
std::uint32_t word(const NinjaSpriteDrawCommand& command,std::size_t at){std::uint32_t value;std::memcpy(&value,command.vertex_bytes.data()+at,4);return value;}
bool same_owner(const std::shared_ptr<const void>& a,const std::shared_ptr<const MetalTextureResource>& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);}
std::uint32_t texture_argument(const MetalSpriteSourceState& state,std::uint32_t key){return bounded(state,key,0,2,true);}
void stage_operation(const MetalSpriteSourceState& state,std::uint32_t key,std::uint32_t& op,std::uint32_t& a,std::uint32_t& b){
    op=bounded(state,key,2,4,true);a=0;b=0;
    if(op!=3)a=texture_argument(state,key+1);if(op!=2)b=texture_argument(state,key+2);
}
}
struct MetalSpriteSubmission::Impl {
    id<MTLCommandBuffer> command;
    std::vector<MetalSpriteDraw> draws;
    std::vector<id<MTLBuffer>> vertices;
    std::vector<id<MTLRenderPipelineState>> pipelines;
    std::vector<id<MTLDepthStencilState>> depths;
    std::vector<id<MTLSamplerState>> samplers;
    MTLRenderPassDescriptor* pass;
    std::size_t count=0;
};
struct MetalSpriteRenderer::Impl {NinjaRenderDeviceHandle source_device;id<MTLDevice> device;id<MTLCommandQueue> queue;id<MTLFunction> vertex,fragment;};
MetalSpriteSubmission::MetalSpriteSubmission(std::shared_ptr<Impl> state):impl(std::move(state)){}
MetalSpriteSubmission::~MetalSpriteSubmission()=default;
bool MetalSpriteSubmission::completed()const noexcept{return impl->command.status==MTLCommandBufferStatusCompleted;}
MetalSpriteFailure MetalSpriteSubmission::wait(){if(!impl->command||impl->command.status<MTLCommandBufferStatusCommitted)return MetalSpriteFailure::not_completed;[impl->command waitUntilCompleted];return completed()?MetalSpriteFailure::none:MetalSpriteFailure::command_failed;}
std::size_t MetalSpriteSubmission::draw_count()const noexcept{return impl->count;}
MetalSpriteRenderer::MetalSpriteRenderer(std::unique_ptr<Impl> state):impl(std::move(state)){}
MetalSpriteRenderer::~MetalSpriteRenderer()=default;
const char* metal_sprite_shader_source()noexcept{return shader;}
MetalSpriteContextResult MetalSpriteRenderer::create(NinjaRenderDeviceHandle source_device,id<MTLDevice> device,id<MTLCommandQueue> queue){
    if(!device||!source_device)return {MetalSpriteFailure::missing_device,"No caller device identity",{}};
    if(!queue)return {MetalSpriteFailure::missing_queue,"No caller queue",{}};
    if(queue.device!=device)return {MetalSpriteFailure::device_mismatch,"Device and queue differ",{}};
    if(!device.hasUnifiedMemory)return {MetalSpriteFailure::unsupported_memory_profile,"Shared Apple Silicon profile required",{}};
    MTLCompileOptions* options=[MTLCompileOptions new];
    if(@available(macOS 15.0,*)){options.mathMode=MTLMathModeSafe;options.mathFloatingPointFunctions=MTLMathFloatingPointFunctionsPrecise;}
    else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        options.fastMathEnabled=NO;
#pragma clang diagnostic pop
    }
    NSError* error=nil;auto library=[device newLibraryWithSource:[NSString stringWithUTF8String:shader] options:options error:&error];
    if(!library)return {MetalSpriteFailure::shader_failed,detail(error,"Shader compilation failed"),{}};
    auto state=std::make_unique<Impl>();state->source_device=std::move(source_device);state->device=device;state->queue=queue;state->vertex=[library newFunctionWithName:@"source_screen_sprite"];state->fragment=[library newFunctionWithName:@"source_sprite_fragment"];
    if(!state->vertex||!state->fragment)return {MetalSpriteFailure::shader_failed,"Shader entry missing",{}};
    return {MetalSpriteFailure::none,{},std::shared_ptr<MetalSpriteRenderer>(new MetalSpriteRenderer(std::move(state)))};
}
MetalSpriteResult MetalSpriteRenderer::submit(std::vector<MetalSpriteDraw> draws,MTLRenderPassDescriptor* pass,MTLViewport viewport){
    MetalSpriteResult result;
    if(draws.empty()){result.failure=MetalSpriteFailure::empty_draws;return result;}
    if(!pass||!pass.colorAttachments[0].texture){result.failure=MetalSpriteFailure::missing_target;return result;}
    auto target=pass.colorAttachments[0].texture;
    if(target.device!=impl->device){result.failure=MetalSpriteFailure::device_mismatch;return result;}
    if(target.sampleCount!=1||target.textureType!=MTLTextureType2D||viewport.znear!=0||viewport.zfar!=1||!std::isfinite(viewport.originX)||!std::isfinite(viewport.originY)||!std::isfinite(viewport.width)||!std::isfinite(viewport.height)||viewport.width<=0||viewport.height<=0){result.failure=MetalSpriteFailure::unsupported_target;return result;}
    for(unsigned i=1;i<8;++i)if(pass.colorAttachments[i].texture){result.failure=MetalSpriteFailure::unsupported_target;return result;}
    auto retained=std::make_shared<MetalSpriteSubmission::Impl>();retained->draws=std::move(draws);retained->pass=[pass copy];retained->command=[impl->queue commandBuffer];
    auto encoder=[retained->command renderCommandEncoderWithDescriptor:retained->pass];
    if(!retained->command||!encoder){result.failure=MetalSpriteFailure::encoder_failed;result.submission=std::shared_ptr<MetalSpriteSubmission>(new MetalSpriteSubmission(retained));return result;}
    [encoder setViewport:viewport];
    for(std::size_t index=0;index<retained->draws.size();++index){
        auto& draw=retained->draws[index];result.pending_draw=index;
        auto stop=[&](MetalSpriteFailure why,const char* text){result.failure=why;result.detail=text;};
        const auto& binding=draw.texture_binding;
        if(binding.device!=impl->source_device){stop(MetalSpriteFailure::device_mismatch,"Source device identity differs");break;}
        if(!binding.memory||!draw.texture){stop(MetalSpriteFailure::missing_texture,"Required selected texture resource absent");break;}
        if(binding.stage!=0||binding.surface_is_null||!binding.surface_owner||!*binding.surface_owner||!same_owner(*binding.surface_owner,draw.texture)||!binding.memory->archive||binding.memory->archive.get()!=&draw.texture->archive()||binding.memory->record_ordinal!=draw.texture->record_index()) {stop(MetalSpriteFailure::texture_identity_conflict,"Selected texture snapshot and typed upload owner differ");break;}
        if(!draw.texture->ready()){stop(MetalSpriteFailure::texture_not_ready,"Texture upload has not completed");break;}
        if(draw.texture->normalized_texture().device!=impl->device){stop(MetalSpriteFailure::device_mismatch,"Texture device differs");break;}
        if(draw.command.vertex_format!=0x144||draw.command.vertex_stride!=28||draw.command.primitive_type!=5||draw.command.primitive_count!=2){stop(MetalSpriteFailure::unsupported_vertices,"Only source XYZRHW/FVF144 strip is closed");break;}
        bool finite=true;
        for(std::size_t v=0;v<4;++v){for(auto at:{0U,4U,8U,20U,24U}){float value;auto raw=word(draw.command,v*28+at);std::memcpy(&value,&raw,4);finite=finite&&std::isfinite(value);}if(word(draw.command,v*28+12)!=0x3f800000)finite=false;}
        if(!finite){stop(MetalSpriteFailure::nonfinite_vertices,"Nonfinite or unsupported RHW bytes retained");break;}
        try{
            const auto& state=draw.state;exact(state,22,1);exact(state,28,0);exact(state,52,0);exact(state,168,15);
            exact(state,13,3,true);exact(state,14,3,true);
            const auto mag=bounded(state,16,1,2,true),min=bounded(state,17,1,2,true),mip=bounded(state,18,0,2,true);
            if((mag==2||min==2||mip==2)&&!impl->device.supports32BitFloatFiltering){stop(MetalSpriteFailure::unsupported_memory_profile,"Device cannot filter retained FP32 source channels");break;}
            auto sampler=[MTLSamplerDescriptor new];sampler.sAddressMode=MTLSamplerAddressModeClampToEdge;sampler.tAddressMode=MTLSamplerAddressModeClampToEdge;
            sampler.minFilter=min==1?MTLSamplerMinMagFilterNearest:MTLSamplerMinMagFilterLinear;sampler.magFilter=mag==1?MTLSamplerMinMagFilterNearest:MTLSamplerMinMagFilterLinear;sampler.mipFilter=static_cast<MTLSamplerMipFilter>(mip);
            if(mip){exact(state,19,0,true);exact(state,20,0,true);exact(state,21,1,true);}
            auto sampling=[impl->device newSamplerStateWithDescriptor:sampler];
            auto depth=[MTLDepthStencilDescriptor new];const auto z=bounded(state,7,0,1);depth.depthCompareFunction=MTLCompareFunctionAlways;depth.depthWriteEnabled=NO;
            if(z){if(!pass.depthAttachment.texture){stop(MetalSpriteFailure::missing_depth_target,"Depth enabled without owned attachment");break;}if(pass.depthAttachment.texture.device!=impl->device){stop(MetalSpriteFailure::device_mismatch,"Depth device differs");break;}depth.depthCompareFunction=compare(bounded(state,23,1,8));depth.depthWriteEnabled=bounded(state,14,0,1)!=0;}
            auto depth_state=[impl->device newDepthStencilStateWithDescriptor:depth];
            auto pipeline=[MTLRenderPipelineDescriptor new];pipeline.vertexFunction=impl->vertex;pipeline.fragmentFunction=impl->fragment;pipeline.colorAttachments[0].pixelFormat=target.pixelFormat;
            if(pass.depthAttachment.texture)pipeline.depthAttachmentPixelFormat=pass.depthAttachment.texture.pixelFormat;
            if(pass.stencilAttachment.texture)pipeline.stencilAttachmentPixelFormat=pass.stencilAttachment.texture.pixelFormat;
            auto color=pipeline.colorAttachments[0];color.blendingEnabled=bounded(state,27,0,1)!=0;
            color.writeMask=MTLColorWriteMaskAll;
            if(color.blendingEnabled){exact(state,171,1);const auto src=bounded(state,19,1,11),dst=bounded(state,20,1,11);color.rgbBlendOperation=MTLBlendOperationAdd;color.alphaBlendOperation=MTLBlendOperationAdd;color.sourceRGBBlendFactor=blend(src);color.destinationRGBBlendFactor=blend(dst);color.sourceAlphaBlendFactor=alpha_blend(src);color.destinationAlphaBlendFactor=alpha_blend(dst);}
            Parameters parameters{{static_cast<float>(viewport.originX),static_cast<float>(viewport.originY),static_cast<float>(viewport.width),static_cast<float>(viewport.height)},0,0,0,0,0,0,0,8,0};
            stage_operation(state,1,parameters.color_op,parameters.color_a,parameters.color_b);stage_operation(state,4,parameters.alpha_op,parameters.alpha_a,parameters.alpha_b);
            parameters.alpha_test=bounded(state,15,0,1);if(parameters.alpha_test){parameters.alpha_func=bounded(state,25,1,8);parameters.alpha_ref=need(state,24)&255U;}
            NSError* error=nil;auto pipeline_state=[impl->device newRenderPipelineStateWithDescriptor:pipeline error:&error];
            if(!pipeline_state){result.failure=MetalSpriteFailure::pipeline_failed;result.detail=detail(error,"Pipeline unavailable");break;}
            auto vertices=[impl->device newBufferWithBytes:draw.command.vertex_bytes.data() length:112 options:MTLResourceStorageModeShared];
            if(!vertices||!sampling||!depth_state){stop(MetalSpriteFailure::allocation_failed,"Native vertex/sampler/depth allocation failed");break;}
            retained->vertices.push_back(vertices);retained->pipelines.push_back(pipeline_state);retained->samplers.push_back(sampling);retained->depths.push_back(depth_state);
            [encoder setRenderPipelineState:pipeline_state];[encoder setDepthStencilState:depth_state];[encoder setCullMode:MTLCullModeNone];[encoder setVertexBuffer:vertices offset:0 atIndex:0];[encoder setVertexBytes:&parameters length:sizeof(parameters) atIndex:1];
            [encoder setFragmentBytes:&parameters length:sizeof(parameters) atIndex:1];[encoder setFragmentTexture:draw.texture->normalized_texture() atIndex:0];[encoder setFragmentSamplerState:sampling atIndex:0];[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];++retained->count;
        }catch(const StateBoundary& e){result.failure=e.failure;result.source_state=e.state;result.source_state_is_stage_zero=e.stage;break;}
    }
    [encoder endEncoding];result.encoded_draws=retained->count;
    // The block owns exact source commands, records and texture metadata even
    // if the caller releases its ticket before the actual GPU finishes.
    if(retained->count){[retained->command addCompletedHandler:^(id<MTLCommandBuffer> command){(void)command;(void)retained;}];[retained->command commit];}
    result.submission=std::shared_ptr<MetalSpriteSubmission>(new MetalSpriteSubmission(retained));
    if(result.failure==MetalSpriteFailure::none)result.pending_draw.reset();return result;
}
} // namespace sadx

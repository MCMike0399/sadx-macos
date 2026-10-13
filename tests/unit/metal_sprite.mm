#include "platform/mac/metal_sprite.hpp"
#import <Foundation/Foundation.h>
#include <CommonCrypto/CommonDigest.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <sstream>
using namespace sadx;
namespace {
void check(bool v,const char* text){if(!v)throw std::runtime_error(text);}
std::vector<std::uint8_t> bytes(const char* path){std::ifstream file(path,std::ios::binary);check(bool(file),"Archive missing");return {std::istreambuf_iterator<char>(file),{}};}
void put(NinjaSpriteDrawCommand& command,std::size_t offset,float value){std::memcpy(command.vertex_bytes.data()+offset,&value,4);}
MetalSpriteDraw draw(const NinjaRenderDeviceHandle& device,const std::shared_ptr<const NinjaTextureArchive>& archive,const std::shared_ptr<const MetalTextureResource>& texture,float z=0.25F){
    MetalSpriteDraw d;d.command.vertex_format=0x144;d.command.vertex_stride=28;
    const float x[]={-0.5F,63.5F,-0.5F,63.5F},y[]={-0.5F,-0.5F,63.5F,63.5F},u[]={0,1,0,1},v[]={0,0,1,1};
    for(unsigned i=0;i<4;++i){put(d.command,i*28,x[i]);put(d.command,i*28+4,y[i]);put(d.command,i*28+8,z);put(d.command,i*28+12,1);const std::uint32_t white=0xffffffff;std::memcpy(d.command.vertex_bytes.data()+i*28+16,&white,4);put(d.command,i*28+20,u[i]);put(d.command,i*28+24,v[i]);}
    auto memory=std::make_shared<NinjaTextureMemoryReference>();memory->archive=archive;memory->record_ordinal=130;memory->runtime_owner=texture;
    d.texture_binding.device=device;d.texture_binding.memory=memory;d.texture_binding.surface_owner=texture;d.texture=texture;
    // Explicit inspection fixture, not inferred game/D3D defaults.
    d.state.render={{7,0},{15,0},{22,1},{27,0},{28,0},{52,0},{168,15}};
    d.state.stage_zero={{1,4},{2,2},{3,0},{4,3},{6,2},{13,3},{14,3},{16,1},{17,1},{18,0}};return d;
}
std::array<float,4> expected(std::uint16_t p,std::uint8_t format){if(format==0)return {{float((p>>10)&31)/31,float((p>>5)&31)/31,float(p&31)/31,float(p>>15)}};if(format==1)return {{float((p>>11)&31)/31,float((p>>5)&63)/63,float(p&31)/31,1}};return {{float((p>>8)&15)/15,float((p>>4)&15)/15,float(p&15)/15,float((p>>12)&15)/15}};}
void tint(MetalSpriteDraw& draw,std::uint32_t color){for(unsigned v=0;v<4;++v)std::memcpy(draw.command.vertex_bytes.data()+v*28+16,&color,4);draw.state.stage_zero[1]=2;draw.state.stage_zero[2]=0;draw.state.stage_zero[4]=2;draw.state.stage_zero[5]=0;}
std::string hash(const void* p,std::size_t n){std::array<unsigned char,32> digest;CC_SHA256(p,static_cast<CC_LONG>(n),digest.data());std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto c:digest)out<<std::setw(2)<<unsigned(c);return out.str();}
void test(int argc,char** argv){
    check(argc==3,"usage: metal-sprite-test EFF_REGULAR.PVM REPORT.json");
    auto parsed=parse_ninja_texture_resource(bytes(argv[1]));check(bool(parsed),"PVM parse");auto archive=std::make_shared<const NinjaTextureArchive>(std::move(parsed.archive));check(archive->records.size()>130,"Actual bubble record absent");
    auto device=MTLCreateSystemDefaultDevice();check(device!=nil,"Actual Metal device required");auto queue=[device newCommandQueue];auto uploader=MetalTextureUploader::create(device,queue);check(bool(uploader),"Texture upload context");auto uploaded=uploader.uploader->upload(archive,130);check(bool(uploaded),"Actual source texture upload");
    auto source=std::make_shared<NinjaRenderDevice>();source->identity=1;auto renderer=MetalSpriteRenderer::create(source,device,queue);check(bool(renderer),renderer.detail.c_str());
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:64 height:64 mipmapped:NO];descriptor.storageMode=MTLStorageModeShared;descriptor.usage=MTLTextureUsageRenderTarget;auto target=[device newTextureWithDescriptor:descriptor];check(target!=nil,"Target allocation");
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
    const MTLViewport viewport{0,0,64,64,0,1};auto fixture=draw(source,archive,uploaded.resource);
    auto record=std::make_shared<NinjaQueueRecord>();std::weak_ptr<NinjaQueueRecord> weak=record;fixture.source_record=record;auto result=renderer.renderer->submit({fixture},pass,viewport);check(bool(result)&&result.encoded_draws==1,"Actual sprite encoded");record.reset();fixture.source_record.reset();check(!weak.expired(),"Submission retains source record");check(result.submission->wait()==MetalSpriteFailure::none&&result.submission->completed(),"Actual GPU completion required");
    std::vector<float> pixels(64*64*4);[target getBytes:pixels.data() bytesPerRow:64*16 fromRegion:MTLRegionMake2D(0,0,64,64) mipmapLevel:0];const auto& texels=uploaded.resource->decoded().levels[0].packed16;check(texels.size()==4096,"Bubble dimensions");std::size_t different=0;
    for(std::size_t p=0;p<texels.size();++p){auto want=expected(texels[p],archive->records[130].pixel_format);for(unsigned c=0;c<4;++c)if(std::memcmp(&want[c],&pixels[p*4+c],4))++different;}
    auto missing=fixture;missing.state.stage_zero.erase(17);auto stopped=renderer.renderer->submit({missing},pass,viewport);check(stopped.failure==MetalSpriteFailure::missing_source_state&&stopped.source_state==17&&stopped.source_state_is_stage_zero&&stopped.encoded_draws==0,"Unknown filter remains explicit");check(stopped.submission->wait()==MetalSpriteFailure::not_completed,"Uncommitted failed submission cannot claim completion");
    auto identity=fixture;identity.texture_binding.surface_owner=std::shared_ptr<const void>{};check(renderer.renderer->submit({identity},pass,viewport).failure==MetalSpriteFailure::texture_identity_conflict,"Known-null upload cannot become loaded texture");
    auto world=fixture;world.command.vertex_format=0x142;world.command.vertex_stride=24;check(renderer.renderer->submit({world},pass,viewport).failure==MetalSpriteFailure::unsupported_vertices,"No world-matrix fallback");
    auto depth=fixture;depth.state.render[7]=1;check(renderer.renderer->submit({depth},pass,viewport).failure==MetalSpriteFailure::missing_depth_target,"No fake depth attachment");
    auto partial=renderer.renderer->submit({fixture,missing},pass,viewport);check(partial.failure==MetalSpriteFailure::missing_source_state&&partial.encoded_draws==1&&partial.pending_draw==1,"Later failure retains actual encoded prefix");check(partial.submission->wait()==MetalSpriteFailure::none,"Encoded prefix actually completes");
    auto red=fixture,green=draw(source,archive,uploaded.resource,0.75F);tint(red,0xffff0000);tint(green,0xff00ff00);
    auto read_pixel=[&](){std::array<float,4> pixel;[target getBytes:pixel.data() bytesPerRow:16 fromRegion:MTLRegionMake2D(32,32,1,1) mipmapLevel:0];return pixel;};
    auto ordered=renderer.renderer->submit({red,green},pass,viewport);check(ordered&&ordered.submission->wait()==MetalSpriteFailure::none,"Ordered real draw completion");check(read_pixel()==std::array<float,4>{0,1,0,1},"Later opaque source draw wins");
    auto depth_descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:64 height:64 mipmapped:NO];depth_descriptor.storageMode=MTLStorageModeShared;depth_descriptor.usage=MTLTextureUsageRenderTarget;pass.depthAttachment.texture=[device newTextureWithDescriptor:depth_descriptor];pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;
    red.state.render[7]=1;red.state.render[14]=1;red.state.render[23]=2;green.state.render[7]=1;green.state.render[14]=1;green.state.render[23]=2;
    auto tested=renderer.renderer->submit({red,green},pass,viewport);check(tested&&tested.submission->wait()==MetalSpriteFailure::none,"Depth test actual GPU completion");check(read_pixel()==std::array<float,4>{1,0,0,1},"Later farther source draw fails less comparison");
    pass.depthAttachment.texture=nil;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,1,1);red.state.render[7]=0;red.state.render[27]=1;red.state.render[19]=5;red.state.render[20]=6;red.state.render[171]=1;tint(red,0x80ff0000);
    auto blended=renderer.renderer->submit({red},pass,viewport);check(blended&&blended.submission->wait()==MetalSpriteFailure::none,"Actual blend completion");const auto blend_pixel=read_pixel();const float alpha=128.0F/255.0F;const std::array<float,4> blend_expected{alpha,0,1-alpha,alpha*alpha+(1-alpha)};float blend_error=0;for(unsigned c=0;c<4;++c)blend_error=std::fmax(blend_error,std::fabs(blend_pixel[c]-blend_expected[c]));check(blend_error<=1e-6F,"Native source-alpha blend contract");
    const auto shader_hash=hash(metal_sprite_shader_source(),std::strlen(metal_sprite_shader_source()));const auto vertex_hash=hash(fixture.command.vertex_bytes.data(),112);
    NSDictionary* report=@{@"device":device.name,@"registry_id":@(device.registryID),@"os":NSProcessInfo.processInfo.operatingSystemVersionString,@"archive_sha256":[NSString stringWithUTF8String:uploaded.resource->archive_sha256().c_str()],@"shader_sha256":[NSString stringWithUTF8String:shader_hash.c_str()],@"vertex_fixture_sha256":[NSString stringWithUTF8String:vertex_hash.c_str()],@"record_ordinal":@130,@"vertex_format":@324,@"stride":@28,@"source_pixel_center":@"D3D8 integer to Metal half-integer",@"inspection_profile":@"explicit nearest/clamp, white diffuse, alpha texture, no blend/depth",@"channels":@(pixels.size()),@"channel_mismatches":@(different),@"gpu_completed":@YES,@"partial_prefix_completed":@YES,@"opaque_order_test":@YES,@"less_depth_test":@YES,@"source_alpha_blend_max_abs_error":@(blend_error),@"source_alpha_blend_gpu":@[@(blend_pixel[0]),@(blend_pixel[1]),@(blend_pixel[2]),@(blend_pixel[3])],@"limits":@[@"Real Apple GPU rendering/readback of supplied packed16 bubble texture and explicit screen command",@"Inspection vertices/state are declared fixtures; no full queue/game startup inferred",@"No original Windows GPU/filter/blend/hardware parity claimed",@"World FVF142, unknown state, nonfinite vertices and unresolved projected-depth production remain explicit boundaries"]};
    NSError* error=nil;NSData* json=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:&error];check(json!=nil,"Report encoding");check([json writeToFile:[NSString stringWithUTF8String:argv[2]] atomically:YES],"Report writing");check(different==0,"Actual nearest sprite readback differs");std::cout<<"Metal sprite: 16,384 channels match; actual GPU completion, resource identity and partial-prefix boundaries passed.\n";
}
}
int main(int argc,char** argv){@autoreleasepool{try{test(argc,argv);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}

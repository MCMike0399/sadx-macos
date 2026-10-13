#include "platform/mac/metal_sprite.hpp"
#include "engine/ninja_queue_consumer.hpp"
#import <Foundation/Foundation.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
// CPU→Metal composition: native 404760 submission and 4086f0 consumption emit
// the actual D3D8 command stream; this test folds it into a D3D8 state snapshot
// and hands each source draw to metal_sprite on a real GPU. States that no
// reconstructed owner has produced yet (device creation defaults, sampler
// filters) are an explicit declared fixture, never inferred D3D defaults.
using namespace sadx;
namespace {
void check(bool v,const std::string& text){if(!v)throw std::runtime_error(text);}
std::vector<std::uint8_t> bytes(const char* path,std::size_t size=0){std::ifstream f(path,std::ios::binary);check(bool(f),std::string("missing ")+path);std::vector<std::uint8_t> d{std::istreambuf_iterator<char>(f),{}};if(size)check(d.size()==size,std::string("size ")+path);return d;}
void test(int argc,char** argv){
    check(argc==9,"usage: metal-sprite-composition-test EFF_REGULAR.PVM sprite40 blend56 depth32 trig unit returns consumer");
    auto parsed=parse_ninja_texture_resource(bytes(argv[1]));check(bool(parsed),"PVM parse");auto archive=std::make_shared<const NinjaTextureArchive>(std::move(parsed.archive));
    const auto sprite=bytes(argv[2],40),blend_only=bytes(argv[3],56),depth_bytes=bytes(argv[4],32),trig=bytes(argv[5],4100),unit_bytes=bytes(argv[6],64),returns_bytes=bytes(argv[7],16),consumer_bytes=bytes(argv[8],8);
    std::vector<std::uint8_t> blend_bytes(blend_only);blend_bytes.insert(blend_bytes.end(),depth_bytes.begin(),depth_bytes.end()); // contiguous .rdata 88ad64..88adbb
    const auto frame=*decode_ninja_sprite_frame(sprite.data(),20);const auto constants=*decode_ninja_sprite_constants(sprite.data()+20,20);const auto blend=*decode_ninja_blend_tables(blend_bytes.data(),88);
    const auto consumer_constants=*decode_ninja_queue_consumer_constants(consumer_bytes.data(),8);
    std::array<std::uint32_t,8> depth;std::memcpy(depth.data(),depth_bytes.data(),32);std::array<std::uint32_t,4> returns;std::memcpy(returns.data(),returns_bytes.data(),16);
    NinjaMath::SineTable table;std::memcpy(table.data(),trig.data(),4100);NinjaMatrix unit;std::memcpy(&unit,unit_bytes.data(),64);
    auto device=MTLCreateSystemDefaultDevice();check(device!=nil,"Actual Metal device required");auto queue=[device newCommandQueue];
    auto uploader=MetalTextureUploader::create(device,queue);check(bool(uploader),"upload context");
    const std::size_t ordinal=static_cast<std::size_t>(frame.texture_index);check(ordinal<archive->records.size(),"source frame texture ordinal outside archive");
    auto uploaded=uploader.uploader->upload(archive,ordinal);check(bool(uploaded),"actual texture upload");
    auto source_device=std::make_shared<NinjaRenderDevice>();source_device->identity=1;
    auto renderer=MetalSpriteRenderer::create(source_device,device,queue);check(bool(renderer),renderer.detail);
    // Declared fixture for states not produced by any reconstructed owner yet.
    // BLENDOP(171): sonic.exe has 14 candidate `push 0xab` sites in other owners
    // (e.g. 53e643, 573c2a, 5f7e5e); its live value at sprite time depends on
    // their history, so ADD here is a declared fixture, not a source fact.
    MetalSpriteSourceState state;state.render={{7,0},{15,0},{28,0},{52,0},{168,15},{171,1}};state.stage_zero={{1,4},{2,2},{3,0},{5,2},{6,0},{16,1},{17,1},{18,0}};
    // Stage0 ALPHAARG1/2 (5/6) = TEXTURE/DIFFUSE (D3D8 CURRENT at stage0): declared, no producer reconstructed yet.
    const auto declared=state;
    NinjaTextureBindCommand binding;bool bound=false;std::vector<MetalSpriteDraw> draws;std::size_t render_commands=0,stage_commands=0,bind_commands=0;
    NinjaRenderState render;render.device_at_3d128b0=source_device;render.packed_blend_at_3ab9890=0xa8;render.blend_tables=&blend;
    render.set_render_state=[&](const auto& c){state.render[c.state]=c.value;++render_commands;return NinjaRenderCallResult{true,{}};};
    render.set_texture_stage_state=[&](const auto& c){check(c.stage==0,"only stage0 is consumed");state.stage_zero[c.state]=c.value;++stage_commands;return NinjaRenderCallResult{true,{}};};
    NinjaTextureSelectionState selection;NinjaTextureSelectionRuntime texture_runtime{render,&returns,nullptr,{}};
    texture_runtime.set_texture=[&](const auto& c){binding=c;bound=true;++bind_commands;return NinjaRenderCallResult{true,{}};};
    auto memory=std::make_shared<NinjaTextureMemoryReference>();memory->archive=archive;memory->record_ordinal=ordinal;memory->runtime_owner=std::shared_ptr<const void>(uploaded.resource);
    memory->global_id_at_0=7;memory->palette_id_at_4=0xffffffffU;memory->format_flags_at_20=0;memory->source_surface_word_at_34=0x11007000;
    auto list=std::make_shared<NinjaTextureListOwner>();list->count=1;list->entries=std::make_shared<NinjaTextureListOwner::Entries>(ordinal+1);
    (**list->entries)[0]=std::shared_ptr<const NinjaTextureMemoryReference>(memory);(**list->entries)[ordinal]=std::shared_ptr<const NinjaTextureMemoryReference>(memory);list->first_filename_is_invalid_sentinel=false;
    selection.current_list_at_3d0fa24=std::shared_ptr<NinjaTextureListOwner>{};selection.requested_memory_at_3ce7128=std::shared_ptr<const NinjaTextureMemoryReference>{};
    selection.active_memory_at_3d129a0=std::shared_ptr<const NinjaTextureMemoryReference>{};selection.global_id_at_3d0fdf4=99;
    NinjaMatrixStack stack({unit},1,0);NinjaMatrixPrimitives coordinates(table,unit,stack);X87Context fp(0x7f); // capture-tied main-thread CW candidate
    NinjaSpriteProjection projection{&coordinates,1e-6F,32,32,{}};
    NinjaSpriteDisplayRuntime display{projection,selection,texture_runtime,constants,{},{},{}};
    display.set_vertex_format=[&](const auto& c){check(c.format==0x144,"screen FVF");return NinjaRenderCallResult{true,{}};};
    display.draw=[&](const NinjaSpriteIssueCommand& c){check(bound,"draw before any texture bind");MetalSpriteDraw d;d.command=c.vertices;d.texture_binding=binding;d.texture=uploaded.resource;d.state=state;d.source_record=c.source_record;draws.push_back(d);return NinjaRenderCallResult{true,{}};};
    std::optional<std::int32_t> display_switch=0;std::optional<float> depth_bias=0;
    auto arena=std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(65536,0xcd),0,65536,1);NinjaSpriteQueue sprite_queue(arena);sprite_queue.initialize_sort_buckets();
    NinjaSpriteQueueRuntime q;q.display_switch_at_3b1117c=&display_switch;q.depth_bias_at_3abd9b8=&depth_bias;q.render_state=&render;q.mode_at_3ab98ac=0;q.flag_at_3abdcfe=0;
    q.captured_word_at_3abd950=0;q.captured_word_at_3d0f9c8=11;q.captured_word_at_3d0f840=22;q.captured_word_at_3d0f9c4=33;q.color_at_3abd9a8=std::array<float,4>{1,1,1,1};q.color_at_3ab9864=std::array<float,4>{1,1,1,1};q.flag_at_3b12208=6;q.sort_bias_at_3abd9c0=0;q.bucket_bias_at_7dcb60=2048;
    NinjaQueueConsumerState s;s.special_flags_at_909eb4=0;s.fog_profile_at_3abdc6c=0;s.count_at_3abd9d4=3;s.maximum_at_3abd9d8=2;s.count_at_3abd9dc=5;s.maximum_at_3abd9e0=4;s.fog_enabled_at_3d12798=0;s.fog_supported_at_3d128a4=0;
    s.cached_fog_start_at_90a098=consumer_constants.fog_sentinel_at_7dce50;s.cached_fog_end_at_90a09c=consumer_constants.fog_sentinel_at_7dce50;
    const std::optional<std::array<float,4>> initial=std::array<float,4>{1,1,1,1};
    NinjaQueueConsumerRuntime runtime{sprite_queue,q,s,display,consumer_constants,initial,&depth};
    auto packet=std::make_shared<NinjaQueuedSpriteInput>();packet->sprite={{0,0,1e-6F},16,16,0};packet->texture_list=list;packet->animation=std::make_shared<const std::vector<NinjaSpriteFrame>>(1,frame);
    check(bool(sprite_queue.submit(std::shared_ptr<const NinjaQueuedSpriteInput>(packet),true,0,0x30,0,q,&coordinates,fp)),"native 404760 submission");
    const auto consumed=ninja_consume_queue(runtime,fp);check(bool(consumed),"native 4086f0 consumption");check(draws.size()==1,"one source sprite draw");
    std::size_t produced=0;for(const auto& [k,v]:draws[0].state.render)if(!declared.render.count(k)||declared.render.at(k)!=v)++produced;
    for(const auto& [k,v]:draws[0].state.stage_zero)if(!declared.stage_zero.count(k)||declared.stage_zero.at(k)!=v)++produced;
    auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:64 height:64 mipmapped:NO];descriptor.storageMode=MTLStorageModeShared;descriptor.usage=MTLTextureUsageRenderTarget;auto target=[device newTextureWithDescriptor:descriptor];
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);
    auto result=renderer.renderer->submit(draws,pass,MTLViewport{0,0,64,64,0,1});
    if(!result){std::cerr<<"metal_sprite stopped: failure "<<static_cast<int>(result.failure)<<" state "<<(result.source_state?std::to_string(*result.source_state):"-")<<(result.source_state_is_stage_zero?" (stage0) ":" ")<<result.detail<<'\n';}
    check(bool(result)&&result.submission->wait()==MetalSpriteFailure::none,"actual GPU completion of CPU-produced draw");
    std::vector<float> pixels(64*64*4);[target getBytes:pixels.data() bytesPerRow:64*16 fromRegion:MTLRegionMake2D(0,0,64,64) mipmapLevel:0];
    std::size_t covered=0;double alpha_sum=0;for(std::size_t p=0;p<64*64;++p){if(pixels[p*4+3]>0){++covered;alpha_sum+=pixels[p*4+3];}}
    float x0=0,y0=0,x3=0,y3=0;std::memcpy(&x0,draws[0].command.vertex_bytes.data(),4);std::memcpy(&y0,draws[0].command.vertex_bytes.data()+4,4);std::memcpy(&x3,draws[0].command.vertex_bytes.data()+84,4);std::memcpy(&y3,draws[0].command.vertex_bytes.data()+88,4);
    check(covered>0,"composited sprite covers pixels");
    std::cout<<"Metal composition: native 404760+4086f0 produced "<<render_commands<<" render, "<<stage_commands<<" stage and "<<bind_commands<<" texture commands ("<<produced<<" state entries beyond the declared fixture); record "<<ordinal<<" drawn on "<<device.name.UTF8String
             <<", screen quad ("<<x0<<","<<y0<<")-("<<x3<<","<<y3<<"), "<<covered<<" covered pixels, alpha sum "<<alpha_sum<<".\n";
}
}
int main(int argc,char** argv){@autoreleasepool{try{test(argc,argv);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}

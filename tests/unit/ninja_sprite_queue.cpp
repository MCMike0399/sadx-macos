#include "ninja/ninja_sprite_queue.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
template<class T>void check(const T& v){if(!static_cast<bool>(v))throw std::runtime_error("queue assertion");}
NinjaSpriteQueueRuntime runtime(NinjaRenderState& render){NinjaSpriteQueueRuntime r;r.render_state=&render;r.mode_at_3ab98ac=0;r.flag_at_3abdcfe=3;r.captured_word_at_3abd950=0x12345678;r.captured_word_at_3d0f9c8=11;r.captured_word_at_3d0f840=22;r.captured_word_at_3d0f9c4=33;r.color_at_3abd9a8=std::array<float,4>{1,2,3,4};r.color_at_3ab9864=std::array<float,4>{5,6,7,8};r.flag_at_3b12208=2;r.sort_bias_at_3abd9c0=0;r.bucket_bias_at_7dcb60=2048;return r;}
void arena_semantics(){
    NinjaRenderQueueArena a(std::vector<std::uint8_t>(320,0xcd),0,320,0);
    auto x=a.allocate(152,{}),y=a.allocate(1,{});check(x&&y&&x.block->offset==0&&x.block->aligned==160&&y.block->offset==160&&y.block->aligned==32&&a.cursor()==192);
    std::uint32_t v;check(a.read(*x.block,0,&v,4)==NinjaQueueFailure::unobserved_bytes);v=0x12345678;check(a.write(*x.block,0,&v,4)==NinjaQueueFailure::none);
    check(a.allocate(160,{}).failure==NinjaQueueFailure::missing_logger && a.cursor()==192 && a.overflow_reported()==0);
    unsigned calls=0;const auto logged=a.allocate(160,[&](std::string_view s){check(s=="late_alloca() : not enough memory!\n");++calls;return NinjaQueueLoggerResult{true};});check(logged&&!logged.block&&a.overflow_reported()==1);
    check(a.allocate(160,{}).failure==NinjaQueueFailure::none&&calls==1&&a.cursor()==192);
    check(a.allocate(0xffffffff,{}).failure==NinjaQueueFailure::size_wrap);
    NinjaRenderQueueArena foreign(std::vector<std::uint8_t>(320),0,320,1);check(foreign.read(*x.block,0,&v,4)==NinjaQueueFailure::foreign_record);
    NinjaRenderQueueArena unknown(std::vector<std::uint8_t>(32),0,32,{});check(unknown.allocate(64,{}).failure==NinjaQueueFailure::unknown_overflow_flag);
}
void records_and_sort(){
    NinjaRenderState render;render.packed_blend_at_3ab9890=0xa8;auto r=runtime(render);auto a=std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(4096,0xcd),0,4096,1);NinjaSpriteQueue q(a);
    check(!q.bucket_records(0));auto first=q.allocate_record(152,3,3,0x12345678,r);check(first&&first.record);std::uint8_t flags;check(a->read(first.record->block,8,&flags,1)==NinjaQueueFailure::none&&flags==0xf3);
    std::uint16_t attribute;check(a->read(first.record->block,10,&attribute,2)==NinjaQueueFailure::none&&attribute==0x5678);
    X87Context fp(0x7f);auto unavailable=q.insert(first.record,-100.5F,0,r,fp);check(unavailable.failure==NinjaQueueFailure::unknown_bucket_state);
    q.initialize_sort_buckets();auto inserted=q.insert(first.record,-100.5F,0,r,fp);check(inserted&&inserted.bucket==1947); // flag bit10 selects unshifted bucket
    auto second=q.allocate_record(152,3,3,9,r).record,third=q.allocate_record(152,3,3,10,r).record,equal=q.allocate_record(152,3,3,11,r).record;
    check(q.insert(second,-100.1F,0,r,fp));check(q.insert(third,-100.9F,0,r,fp));check(q.insert(equal,-100.5F,0,r,fp));
    const auto ordered=q.bucket_records(1947);check(ordered&&ordered.records==std::vector<NinjaQueueRecordHandle>({third,first.record,equal,second}));
    check(q.insert(first.record,0,0,r,fp).failure==NinjaQueueFailure::already_linked);
    auto high=q.allocate_record(152,3,0,0,r).record;check(q.insert(high,20000,1,r,fp).bucket==2047);
    auto low=q.allocate_record(152,3,0,0,r).record;check(q.insert(low,-50000,0,r,fp).bucket==0);
    q.initialize_sort_buckets();check(q.bucket_records(1947).records.empty());float retained;check(a->read(first.record->block,4,&retained,4)==NinjaQueueFailure::none&&retained==-100.5F);
    render.packed_blend_at_3ab9890.reset();const auto pending=q.allocate_record(152,3,0,0,r);check(pending.failure==NinjaQueueFailure::unknown_header_fact&&pending.source_boundary==0x403fa6&&pending.record);check(a->read(pending.record->block,8,&flags,1)==NinjaQueueFailure::unobserved_bytes);
}
void texture_gate(){
    check(ninja_texture_list_gate({}).failure==NinjaQueueFailure::unknown_texture_list);
    check(*ninja_texture_list_gate(std::shared_ptr<NinjaTextureListOwner>{}).source_not_ready);
    auto list=std::make_shared<NinjaTextureListOwner>();list->count=0;check(*ninja_texture_list_gate(list).source_not_ready);list->count=1;
    check(ninja_texture_list_gate(list).failure==NinjaQueueFailure::unknown_texture_array);
    list->entries=std::shared_ptr<NinjaTextureListOwner::Entries>{};check(ninja_texture_list_gate(list).failure==NinjaQueueFailure::null_texture_array);
    list->entries=std::make_shared<NinjaTextureListOwner::Entries>(1);check(ninja_texture_list_gate(list).failure==NinjaQueueFailure::unknown_texture_reference);
    (**list->entries)[0]=std::shared_ptr<const NinjaTextureMemoryReference>{};check(*ninja_texture_list_gate(list).source_not_ready);
    auto resource=std::make_shared<NinjaTextureMemoryReference>();(**list->entries)[0]=resource;check(ninja_texture_list_gate(list).failure==NinjaQueueFailure::unknown_texture_filename);
    list->first_filename_is_invalid_sentinel=false;check(!*ninja_texture_list_gate(list).source_not_ready);list->first_filename_is_invalid_sentinel=true;check(*ninja_texture_list_gate(list).source_not_ready);
}
void header_texture_ownership(){
    NinjaRenderState render;render.packed_blend_at_3ab9890=0xa8;auto r=runtime(render);
    auto arena=std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(2048,0xcd),0,2048,1);NinjaSpriteQueue queue(arena);
    auto unmapped=queue.allocate_record(152,3,0,0,r);check(unmapped&&!unmapped.record->texture_list_at_c);
    r.captured_word_at_3abd950=0;auto null=queue.allocate_record(152,3,0,0,r);check(null&&null.record->texture_list_at_c&&!*null.record->texture_list_at_c);
    auto list=std::make_shared<NinjaTextureListOwner>();std::weak_ptr<NinjaTextureListOwner> lifetime=list;
    r.captured_word_at_3abd950=0x12345678;r.captured_texture_list_at_3abd950=list;
    auto bound=queue.allocate_record(152,3,0,0,r);check(bound&&bound.record->texture_list_at_c&&*bound.record->texture_list_at_c==list);
    r.captured_texture_list_at_3abd950.reset();list.reset();check(!lifetime.expired());
    r.captured_word_at_3abd950=0;r.captured_texture_list_at_3abd950=*bound.record->texture_list_at_c;
    auto conflict=queue.allocate_record(152,3,0,7,r);check(conflict.failure==NinjaQueueFailure::texture_reference_conflict&&conflict.source_boundary==0x403fb5&&conflict.record);
    std::uint32_t raw;check(arena->read(conflict.record->block,12,&raw,4)==NinjaQueueFailure::none&&raw==0);
    std::uint16_t attribute;check(arena->read(conflict.record->block,10,&attribute,2)==NinjaQueueFailure::unobserved_bytes);
}
template<class T>T asset(const char* path){T result{};std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(result.data()),sizeof(result));check(f.gcount()==sizeof(result)&&f.peek()==EOF);return result;}
void submission_and_immediate(const NinjaMath::SineTable& table,const NinjaMatrix& unit,const std::array<std::uint32_t,8>& depth_table){
    NinjaRenderState render;render.packed_blend_at_3ab9890=0xa8;auto r=runtime(render);std::optional<std::int32_t> display=0;std::optional<float> depth_bias=1;r.display_switch_at_3b1117c=&display;r.depth_bias_at_3abd9b8=&depth_bias;
    auto arena=std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(2048,0xcd),0,2048,1);NinjaSpriteQueue queue(arena);queue.initialize_sort_buckets();X87Context fp(0x7f);
    auto resource=std::make_shared<NinjaTextureMemoryReference>();auto texture=std::make_shared<NinjaTextureListOwner>();texture->count=1;texture->first_filename_is_invalid_sentinel=false;texture->entries=std::make_shared<NinjaTextureListOwner::Entries>(1);(**texture->entries)[0]=resource;
    auto packet=std::make_shared<NinjaQueuedSpriteInput>();packet->sprite={{3,4,5},2,3,123};packet->texture_list=texture;packet->animation=std::make_shared<const std::vector<NinjaSpriteFrame>>();
    NinjaMatrixStack stack({unit},1,0);PlayerCoordinates coordinates(table,unit,stack);
    auto submitted=queue.submit(packet,true,7,0x31,0,r,&coordinates,fp);check(submitted&&submitted.route==NinjaSpriteSubmissionRoute::queued&&submitted.queue.record->sprite);
    auto record=submitted.queue.record;float depth;check(arena->read(record->block,4,&depth,4)==NinjaQueueFailure::none&&depth==-4);
    const auto retained=record->sprite->texture_list;packet->sprite.position.x=99;check(record->sprite->sprite.position.x==3&&retained==texture);
    std::weak_ptr<NinjaTextureListOwner> lifetime=texture;texture.reset();packet.reset();check(!lifetime.expired());
    auto input=std::make_shared<NinjaQueuedSpriteInput>();input->sprite={{1,2,3},1,1,0};input->texture_list=*retained;
    r.mode_at_3ab98ac=1;auto direct=queue.submit(input,true,0,0x31,0,r,nullptr,fp);check(direct&&direct.route==NinjaSpriteSubmissionRoute::immediate_required&&!direct.queue.record);
    r.mode_at_3ab98ac=0;auto missing_matrix=queue.submit(input,true,0,0x31,0,r,nullptr,fp);check(missing_matrix.queue.failure==NinjaQueueFailure::missing_matrix&&missing_matrix.queue.record&&missing_matrix.queue.record->sprite);
    display=1;auto guarded=queue.submit({}, {},0,0x31,0,r,nullptr,fp);check(guarded&&guarded.route==NinjaSpriteSubmissionRoute::not_drawn);display.reset();check(queue.submit(input,true,0,0x31,0,r,&coordinates,fp).queue.failure==NinjaQueueFailure::unknown_display_switch);display=0;
    auto device=std::make_shared<NinjaRenderDevice>();device->identity=1;render.device_at_3d128b0=device;render.set_render_state=[](const auto&){return NinjaRenderCallResult{true,{}};};NinjaSpriteProjection geometry;
    auto prepared=ninja_prepare_immediate_sprite(16,r,&depth_table,geometry);check(prepared&&prepared.render->commands.size()==4&&geometry.color_argb_at_3d0f7f0==r.color_at_3ab9864);
    check(prepared.render->commands[0].state==23&&prepared.render->commands[0].value==depth_table[3]&&prepared.render->commands[1].state==14&&prepared.render->commands[2].value==depth_table[7]);
    unsigned count=0;render.set_render_state=[&](const auto&){++count;return NinjaRenderCallResult{count<2,{}};};auto partial=ninja_prepare_immediate_sprite(16,r,&depth_table,geometry);check(partial.failure==NinjaQueueFailure::render_failed&&partial.render->commands.size()==2);
}

}
void arena_reset(){
    auto arena=std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(256,0xcd),0,256,1);NinjaSpriteQueue queue(arena);queue.initialize_sort_buckets();NinjaSpriteQueueRuntime runtime;
    auto block=arena->allocate(40,{}).block;check(block.has_value());const std::uint32_t word=0x12345678;check(arena->write(*block,0,&word,4)==NinjaQueueFailure::none);
    check(arena->reset_407ab0(0x01230008)==NinjaQueueFailure::none&&arena->cursor()==24);
    std::uint32_t read=1;check(arena->read(*block,0,&read,4)==NinjaQueueFailure::foreign_record);
    auto fresh=arena->allocate(40,{}).block;check(fresh&&fresh->offset==24);check(arena->read(*fresh,0,&read,4)==NinjaQueueFailure::none&&read==0);
    check(arena->read(*block,0,&read,4)==NinjaQueueFailure::foreign_record); // an old id never matches a new allocation
    check(arena->reset_407ab0(0x01230000)==NinjaQueueFailure::none&&arena->cursor()==0);
    queue.forget_records_after_arena_reset();
}
int main(int argc,char** argv){try{check(argc==4);const auto table=asset<NinjaMath::SineTable>(argv[1]);const auto unit=asset<NinjaMatrix>(argv[2]);const auto depth_table=asset<std::array<std::uint32_t,8>>(argv[3]);arena_semantics();arena_reset();records_and_sort();texture_gate();header_texture_ownership();submission_and_immediate(table,unit,depth_table);std::cout<<"Sprite queue: aligned ownership/log boundaries, exact headers, stable source sort and nullable texture facts passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

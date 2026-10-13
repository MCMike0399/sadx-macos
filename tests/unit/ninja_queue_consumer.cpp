#include "ninja_sprite_test_fixture.hpp"
#include <fstream>
#include <iostream>
using namespace sadx;using namespace sadx_test;
namespace {
template<class T>void require(const T& v,unsigned line){if(!static_cast<bool>(v))throw std::runtime_error("queue consumer assertion at line "+std::to_string(line));}
#define check(...) require((__VA_ARGS__),__LINE__)
std::vector<std::uint8_t> bytes(const char* path,std::size_t size){std::ifstream f(path,std::ios::binary);std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(f),{}};check(data.size()==size);return data;}
void test(const SpriteAssets& assets){
    SpriteFixture f(assets,0x7f);auto node=f.enqueue();check(node);auto consumed=ninja_consume_queue(f.consumer_runtime,f.fp);check(consumed&&consumed.records.size()==1&&consumed.records[0].sprite_display);
    const auto& displayed=*consumed.records[0].sprite_display;check(displayed&&displayed.issue&&displayed.format&&displayed.depth&&displayed.depth->value);check(!displayed.reciprocal);
    check(f.consumer_state.maximum_at_3abd9d8==3&&f.consumer_state.maximum_at_3abd9e0==5);check(f.queue_runtime.flag_at_3b12208==3&&f.consumer_state.mode_byte_at_3abdbc0==3);check(f.coordinates.current_matrix()==assets.unit);check(f.queue_runtime.mode_at_3ab98ac==1);
    check(displayed.stage.commands.size()==3&&displayed.stage.commands[2].state==4&&displayed.stage.commands[2].value==4);check(displayed.ignored_draw_hresult==-1);
    // Standalone display has no consumer copy at77ff90, so the fixture states the
    // projection colour explicitly; without it77e53b correctly stops first.
    SpriteFixture uncoloured(assets,0x37f);uncoloured.packet->sprite.position.z=assets.sprite.minimum_depth;auto colourless=ninja_display_sprite(uncoloured.packet,0,0x32,{},uncoloured.display_runtime,uncoloured.fp);
    check(colourless.failure==NinjaSpriteDisplayFailure::geometry_failed&&colourless.source_boundary==0x77e53b&&colourless.geometry&&colourless.geometry->failure==NinjaSpriteFailure::missing_color&&!colourless.reciprocal&&!colourless.vertices);
    SpriteFixture pending(assets,0x37f);pending.projection.color_argb_at_3d0f7f0=std::array<float,4>{1,1,1,1};pending.packet->sprite.position.z=assets.sprite.minimum_depth;auto missing=ninja_display_sprite(pending.packet,0,0x32,{},pending.display_runtime,pending.fp);
    check(missing.failure==NinjaSpriteDisplayFailure::depth_unavailable&&missing.source_boundary==0x77e637&&missing.reciprocal&&!missing.vertices);check(missing.render.commands.size()==4&&missing.stage.commands.size()==3);
    SpriteFixture late(assets,0x7f);auto first=late.enqueue();check(first);
    auto extra=late.queue.allocate_record(152,3,0,0,late.queue_runtime).record;check(extra);extra->sprite=*late.packet;
    auto store=[&](std::uint32_t at,const auto& value){check(late.arena->write(extra->block,at,&value,sizeof(value))==NinjaQueueFailure::none);};const std::uint32_t flags=0x32;store(0x50,flags);store(0x58,assets.unit);
    bool inserted=false;late.display_runtime.draw=[&](const auto&){if(!inserted){inserted=true;float depth;check(late.arena->read(first.queue.record->block,4,&depth,4)==NinjaQueueFailure::none);check(late.queue.insert(extra,depth,0,late.queue_runtime,late.fp));}return NinjaRenderCallResult{true,{}};};
    auto followed=ninja_consume_queue(late.consumer_runtime,late.fp);check(followed&&followed.records.size()==2&&followed.records[1].record==extra);
    SpriteFixture dangling(assets,0x7f);auto d=dangling.enqueue();check(d);dangling.display_runtime.draw=[&](const auto&){d.queue.record->next_is_sentinel=false;d.queue.record->next.reset();return NinjaRenderCallResult{true,{}};};auto stopped=ninja_consume_queue(dangling.consumer_runtime,dangling.fp);check(stopped.failure==NinjaQueueConsumerFailure::missing_next&&stopped.source_boundary==0x408f24&&stopped.records.size()==1&&stopped.records[0].sprite_display->returned);
    SpriteFixture unknown(assets,0x7f);unknown.queue_runtime.captured_word_at_3abd950=0x12345678;check(unknown.enqueue());auto unbound=ninja_consume_queue(unknown.consumer_runtime,unknown.fp);check(unbound.failure==NinjaQueueConsumerFailure::unknown_fact&&unbound.source_boundary==0x4088e7&&unbound.required_fact==0x3abd950&&unbound.records.size()==1);
    SpriteFixture empty(assets,0x7f);empty.initial_color=std::array<float,4>{-2,2,0.5F,-0.5F};auto none=ninja_consume_queue(empty.consumer_runtime,empty.fp);check(none&&none.records.empty()&&empty.queue_runtime.color_at_3ab9864==std::array<float,4>{-1,1,0.5F,-0.5F});
    SpriteFixture passes(assets,0x7f);passes.consumer_state.special_flags_at_909eb4=3;passes.consumer_state.fog_profile_at_3abdc6c=1;passes.queue_runtime.flag_at_3abdcfe=1;check(passes.enqueue());auto two=ninja_consume_queue(passes.consumer_runtime,passes.fp);check(two&&two.records.size()==2&&!two.records[0].skipped_by_pass&&two.records[1].skipped_by_pass&&passes.consumer_state.special_flags_at_909eb4==3);
}
}
int main(int argc,char** argv){try{check(argc==8);SpriteAssets assets;auto sprite=bytes(argv[1],40),blend=bytes(argv[2],56),depth=bytes(argv[3],32),table=bytes(argv[4],4100),unit=bytes(argv[5],64),returns=bytes(argv[6],16),consumer=bytes(argv[7],8);assets.frame=*decode_ninja_sprite_frame(sprite.data(),20);assets.sprite=*decode_ninja_sprite_constants(sprite.data()+20,20);assets.blend=*decode_ninja_blend_tables(blend.data(),56);std::memcpy(assets.depth.data(),depth.data(),32);std::memcpy(assets.table.data(),table.data(),4100);std::memcpy(assets.unit.data(),unit.data(),64);std::memcpy(assets.returns.data(),returns.data(),16);assets.consumer=*decode_ninja_queue_consumer_constants(consumer.data(),8);test(assets);std::cout<<"Queue consumer: real sprite zero-depth path, stable/lazy dispatch, typed resources, mode transform and partial failures passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

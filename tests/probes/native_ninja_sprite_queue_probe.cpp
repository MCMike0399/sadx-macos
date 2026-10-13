#include "ninja/ninja_sprite_queue.hpp"
#include <cstring>
namespace {
float f(std::uint32_t w){float value;std::memcpy(&value,&w,4);return value;}
std::uint32_t bits(float f){std::uint32_t w;std::memcpy(&w,&f,4);return w;}
void put(std::uint8_t* p,std::uint32_t w){for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(w>>(i*8));}
}

// Source-bound known list facts only; malformed/unknown native resources have
// separate owner tests. Entries retain their real native reference identities.
extern "C" int sadx_native_ninja_sprite_queue_texture_gate(std::uint32_t present,std::uint32_t count,std::uint32_t null_mask,std::uint32_t sentinel,std::uint32_t* out){
    if(!out||count>8)return 1;
    std::optional<std::shared_ptr<sadx::NinjaTextureListOwner>> list=std::shared_ptr<sadx::NinjaTextureListOwner>{};
    if(present){
        auto owner=std::make_shared<sadx::NinjaTextureListOwner>();owner->count=count;
        auto entries=std::make_shared<sadx::NinjaTextureListOwner::Entries>();
        for(std::uint32_t i=0;i<count;++i){std::shared_ptr<const sadx::NinjaTextureMemoryReference> memory;if(!(null_mask&(1U<<i)))memory=std::make_shared<sadx::NinjaTextureMemoryReference>();entries->emplace_back(memory);}
        owner->entries=entries;owner->first_filename_is_invalid_sentinel=sentinel!=0;list=owner;
    }
    const auto result=sadx::ninja_texture_list_gate(list);if(!result||!result.source_not_ready)return 2;*out=*result.source_not_ready;return 0;
}
// Five words per record:headerflags,eax,rawdepth,policy,recordtype. Declared
// resource fixture has65536 physical bytes and source overflow warning known1.
extern "C" int sadx_native_ninja_sprite_queue(const std::uint32_t* commands,std::size_t words,std::uint16_t cw,
    std::uint32_t bias,std::uint32_t* trace,std::size_t trace_words,std::uint8_t* arena_bytes,std::size_t arena_size,std::uint8_t* bucket_bytes,std::size_t bucket_size){
    if(!commands||words%5||!trace||trace_words!=words/5*8||!arena_bytes||arena_size!=65536||!bucket_bytes||bucket_size!=16384)return 1;
    try{
        auto arena=std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(65536,0xcd),0,65536,1);sadx::NinjaSpriteQueue queue(arena);queue.initialize_sort_buckets();sadx::NinjaRenderState render;render.packed_blend_at_3ab9890=0xa8;
        sadx::NinjaSpriteQueueRuntime runtime;runtime.render_state=&render;runtime.mode_at_3ab98ac=0;runtime.flag_at_3abdcfe=3;runtime.captured_word_at_3abd950=0x12345678;runtime.captured_word_at_3d0f9c8=11;runtime.captured_word_at_3d0f840=22;runtime.captured_word_at_3d0f9c4=33;runtime.color_at_3abd9a8=std::array<float,4>{1,2,3,4};runtime.color_at_3ab9864=std::array<float,4>{5,6,7,8};runtime.flag_at_3b12208=2;runtime.sort_bias_at_3abd9c0=f(bias);runtime.bucket_bias_at_7dcb60=2048;sadx::X87Context fp(cw);
        std::memset(trace,0,trace_words*4);
        for(std::size_t i=0;i<words;i+=5){auto result=queue.allocate_record(152,static_cast<std::uint8_t>(commands[i+4]),static_cast<std::uint8_t>(commands[i]),commands[i+1],runtime);if(result&&result.record)result=queue.insert(result.record,f(commands[i+2]),commands[i+3],runtime,fp);const auto at=i/5*8;
            trace[at]=static_cast<std::uint32_t>(result.failure);trace[at+1]=result.source_boundary;trace[at+2]=result.record?0x20000000+result.record->block.offset:0;trace[at+3]=arena->cursor();trace[at+4]=result.bucket?static_cast<std::uint32_t>(*result.bucket):0xffffffffU;
            if(result.record){float depth;if(arena->read(result.record->block,4,&depth,4)==sadx::NinjaQueueFailure::none)trace[at+5]=bits(depth);}trace[at+6]=fp.exception_flags();trace[at+7]=fp.control_word();}
        std::memcpy(arena_bytes,arena->bytes().data(),65536);std::memset(bucket_bytes,0,16384);
        for(std::size_t i=0;i<2048;++i){auto list=queue.bucket_records(i);if(!list)return 2;const auto sentinel=0x21000000+static_cast<std::uint32_t>(i*8);
            put(bucket_bytes+i*8,list.records.empty()?sentinel:0x20000000+list.records.front()->block.offset);put(bucket_bytes+i*8+4,list.records.empty()?sentinel:0x20000000+list.records.back()->block.offset);
            for(std::size_t j=0;j<list.records.size();++j)put(arena_bytes+list.records[j]->block.offset,j+1<list.records.size()?0x20000000+list.records[j+1]->block.offset:sentinel);}
        return 0;
    }catch(const sadx::X87Error&){return 3;}
}

extern "C" int sadx_native_ninja_sprite_queue_immediate(const std::uint32_t* table,std::size_t table_words,std::uint32_t flags,std::uint32_t* out,std::size_t words){
    if(!table||table_words!=8||!out||words!=24)return 1;
    std::array<std::uint32_t,8> depth;std::memcpy(depth.data(),table,32);sadx::NinjaRenderState render;auto device=std::make_shared<sadx::NinjaRenderDevice>();device->identity=1;render.device_at_3d128b0=device;render.set_render_state=[](const auto&){return sadx::NinjaRenderCallResult{true,-1};};
    sadx::NinjaSpriteQueueRuntime runtime;runtime.render_state=&render;runtime.color_at_3abd9a8=std::array<float,4>{1,2,3,4};runtime.color_at_3ab9864=std::array<float,4>{5,6,7,8};sadx::NinjaSpriteProjection geometry;auto result=sadx::ninja_prepare_immediate_sprite(static_cast<std::uint8_t>(flags),runtime,&depth,geometry);std::memset(out,0,words*4);
    out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=result.render?result.render->commands.size():0;out[3]=geometry.color_argb_at_3d0f7f0.has_value();
    if(result.render)for(unsigned i=0;i<result.render->commands.size();++i){out[4+i*3]=result.render->commands[i].state;out[5+i*3]=result.render->commands[i].value;out[6+i*3]=result.render->commands[i].source_call;}
    if(geometry.color_argb_at_3d0f7f0)for(unsigned i=0;i<4;++i)out[16+i]=bits((*geometry.color_argb_at_3d0f7f0)[i]);return 0;
}

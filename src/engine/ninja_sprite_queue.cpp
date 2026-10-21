#include "engine/ninja_sprite_queue.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>
namespace sadx {
namespace {
std::int32_t signed_word(std::uint32_t v){std::int32_t result;std::memcpy(&result,&v,4);return result;}
NinjaQueueResult failed(NinjaQueueFailure failure,std::uint32_t source,NinjaQueueRecordHandle record={}){NinjaQueueResult r;r.failure=failure;r.source_boundary=source;r.record=std::move(record);return r;}
}
NinjaRenderQueueArena::NinjaRenderQueueArena(std::vector<std::uint8_t> bytes,std::uint32_t cursor,std::uint32_t end,std::optional<std::uint32_t> reported)
    :bytes_(std::move(bytes)),known_(bytes_.size(),0),cursor_(cursor),end_(end),overflow_reported_(reported){
    if(bytes_.size()>std::numeric_limits<std::uint32_t>::max()||cursor>end||end>bytes_.size())throw std::invalid_argument("Invalid owned render arena interval");
}
NinjaQueueAllocationResult NinjaRenderQueueArena::allocate(std::uint32_t size,const NinjaQueueLogger& logger){
    NinjaQueueAllocationResult result;
    if(size>std::numeric_limits<std::uint32_t>::max()-31){result.failure=NinjaQueueFailure::size_wrap;result.source_boundary=0x403d3a;return result;}
    const auto aligned=(size+31)&~31U;
    if(aligned>std::numeric_limits<std::uint32_t>::max()-cursor_){result.failure=NinjaQueueFailure::size_wrap;result.source_boundary=0x403d40;return result;}
    const auto next=cursor_+aligned;
    if(next<=end_){NinjaQueueBlock block;block.offset=cursor_;block.requested=size;block.aligned=aligned;block.owner_=this;block.allocation_=generation_base_+allocations_.size()+1;allocations_.push_back(block);cursor_=next;result.block=block;return result;}
    if(!overflow_reported_){result.failure=NinjaQueueFailure::unknown_overflow_flag;result.source_boundary=0x403d59;return result;}
    if(*overflow_reported_==0){
        if(!logger){result.failure=NinjaQueueFailure::missing_logger;result.source_boundary=0x403d67;return result;}
        if(!logger("late_alloca() : not enough memory!\n").completed){result.failure=NinjaQueueFailure::logger_unavailable;result.source_boundary=0x401000;return result;}
        overflow_reported_=1;
    }
    return result; // actual source null return, not an invented allocation
}
NinjaQueueFailure NinjaRenderQueueArena::write(const NinjaQueueBlock& block,std::uint32_t at,const void* input,std::uint32_t size){
    if(block.owner_!=this||block.allocation_<=generation_base_||block.allocation_-generation_base_>allocations_.size())return NinjaQueueFailure::foreign_record;
    const auto& owned=allocations_[block.allocation_-generation_base_-1];
    if(block.offset!=owned.offset||block.requested!=owned.requested||block.aligned!=owned.aligned)return NinjaQueueFailure::foreign_record;
    if(at>block.aligned||size>block.aligned-at||std::uint64_t(block.offset)+at+size>bytes_.size()||(!input&&size))return NinjaQueueFailure::out_of_bounds;
    if(size){std::memcpy(bytes_.data()+block.offset+at,input,size);std::memset(known_.data()+block.offset+at,1,size);}return NinjaQueueFailure::none;
}
NinjaQueueFailure NinjaRenderQueueArena::read(const NinjaQueueBlock& block,std::uint32_t at,void* output,std::uint32_t size)const{
    if(block.owner_!=this||block.allocation_<=generation_base_||block.allocation_-generation_base_>allocations_.size())return NinjaQueueFailure::foreign_record;
    const auto& owned=allocations_[block.allocation_-generation_base_-1];
    if(block.offset!=owned.offset||block.requested!=owned.requested||block.aligned!=owned.aligned)return NinjaQueueFailure::foreign_record;
    if(at>block.aligned||size>block.aligned-at||std::uint64_t(block.offset)+at+size>bytes_.size()||(!output&&size))return NinjaQueueFailure::out_of_bounds;
    for(std::uint32_t i=0;i<size;++i)if(!known_[block.offset+at+i])return NinjaQueueFailure::unobserved_bytes;
    if(size)std::memcpy(output,bytes_.data()+block.offset+at,size);return NinjaQueueFailure::none;
}
std::uint8_t *NinjaRenderQueueArena::mutable_block(const NinjaQueueBlock &block){
    if(read(block,0,nullptr,0)!=NinjaQueueFailure::none)throw std::invalid_argument("foreign queue allocation view");
    return bytes_.data()+block.offset;
}
std::uint8_t *NinjaRenderQueueArena::borrow_from_end(std::uint32_t size){
    if(size>end_||end_>bytes_.size())throw std::out_of_range("queue end borrow exceeds the canonical arena");
    return bytes_.data()+end_-size;
}
NinjaQueueFailure NinjaRenderQueueArena::reset_407ab0(std::uint32_t base){
    const auto start=((base+0x1fU)&~0x1fU)-base;
    if(start>end_)return NinjaQueueFailure::out_of_bounds;
    std::fill(bytes_.begin(),bytes_.end(),0);std::fill(known_.begin(),known_.end(),1);
    generation_base_+=allocations_.size()+1;allocations_.clear();cursor_=start;overflow_reported_=0;return NinjaQueueFailure::none;
}
NinjaTextureListResult ninja_texture_list_gate(const std::optional<std::shared_ptr<NinjaTextureListOwner>>& list){
    NinjaTextureListResult result;auto stop=[&](NinjaQueueFailure f,std::uint32_t at){result.failure=f;result.source_boundary=at;return result;};
    if(!list)return stop(NinjaQueueFailure::unknown_texture_list,0x403250);
    if(!*list){result.source_not_ready=true;return result;}
    const auto& value=**list;if(!value.count)return stop(NinjaQueueFailure::unknown_texture_count,0x40325a);
    if(*value.count==0){result.source_not_ready=true;return result;}
    if(!value.entries)return stop(NinjaQueueFailure::unknown_texture_array,0x403274);
    if(!*value.entries)return stop(NinjaQueueFailure::null_texture_array,0x403274);
    for(std::uint32_t i=0;i<*value.count;++i){
        if(i>=(**value.entries).size())return stop(NinjaQueueFailure::texture_array_short,0x403274);
        const auto& entry=(**value.entries)[i];if(!entry)return stop(NinjaQueueFailure::unknown_texture_reference,0x403274);
        if(!*entry){result.source_not_ready=true;return result;}
    }
    if(!value.first_filename_is_invalid_sentinel)return stop(NinjaQueueFailure::unknown_texture_filename,0x403281);
    result.source_not_ready=*value.first_filename_is_invalid_sentinel;return result;
}
NinjaSpriteQueue::NinjaSpriteQueue(std::shared_ptr<NinjaRenderQueueArena> arena):arena_(std::move(arena)){}
void NinjaSpriteQueue::forget_records_after_arena_reset(){records_.clear();}
void NinjaSpriteQueue::rebind_arena(std::shared_ptr<NinjaRenderQueueArena> arena){records_.clear();arena_=std::move(arena);} // buckets keep stale links until 407330
void NinjaSpriteQueue::initialize_sort_buckets(){for(auto& bucket:buckets_){bucket.initialized=true;bucket.head.reset();bucket.tail.reset();}for(auto& record:records_)record->linked=false;}
NinjaQueueResult NinjaSpriteQueue::allocate_record(std::uint32_t size,std::uint8_t type,std::uint8_t flags,std::uint32_t eax,NinjaSpriteQueueRuntime& runtime){
    if(!runtime.mode_at_3ab98ac)return failed(NinjaQueueFailure::unknown_queue_mode,0x403f68);
    if(*runtime.mode_at_3ab98ac!=0||(flags&8))return {};
    if(!arena_)return failed(NinjaQueueFailure::missing_arena,0x403f7b);
    const auto allocated=arena_->allocate(size,runtime.logger);
    if(!allocated)return failed(allocated.failure,allocated.source_boundary);
    if(!allocated.block)return {};
    auto record=std::make_shared<NinjaQueueRecord>();record->block=*allocated.block;record->arena=arena_;records_.push_back(record);
    auto store=[&](std::uint32_t offset,const void* data,std::uint32_t count,std::uint32_t site){const auto f=arena_->write(record->block,offset,data,count);return f==NinjaQueueFailure::none?NinjaQueueResult{}:failed(f,site,record);};
    if(!runtime.flag_at_3abdcfe)return failed(NinjaQueueFailure::unknown_header_fact,0x403f8c,record);
    auto byte=static_cast<std::uint8_t>(((flags&1)<<4)|(*runtime.flag_at_3abdcfe<<5)|type);
    if(!runtime.render_state||!runtime.render_state->packed_blend_at_3ab9890)return failed(NinjaQueueFailure::unknown_header_fact,0x403fa6,record);
    const auto packed=*runtime.render_state->packed_blend_at_3ab9890;
    auto written=store(8,&byte,1,0x403fac);if(!written)return written;
    if(!runtime.captured_word_at_3abd950)return failed(NinjaQueueFailure::unknown_header_fact,0x403faf,record);
    written=store(12,&*runtime.captured_word_at_3abd950,4,0x403fb5);if(!written)return written;
    const auto& captured=runtime.captured_texture_list_at_3abd950;
    if(captured && ((*runtime.captured_word_at_3abd950==0)!=!*captured))return failed(NinjaQueueFailure::texture_reference_conflict,0x403fb5,record);
    if(*runtime.captured_word_at_3abd950==0)record->texture_list_at_c=std::shared_ptr<NinjaTextureListOwner>{};
    else record->texture_list_at_c=captured; // nonnull raw/unmapped stays unknown
    std::uint16_t half=static_cast<std::uint16_t>(eax);written=store(10,&half,2,0x403fb8);if(!written)return written;
    written=store(9,&packed,1,0x403fbc);if(!written)return written;
    if(!runtime.captured_word_at_3d0f9c8)return failed(NinjaQueueFailure::unknown_header_fact,0x403fbf,record);
    written=store(0x20,&*runtime.captured_word_at_3d0f9c8,4,0x403fc5);if(!written)return written;
    if(!runtime.captured_word_at_3d0f840)return failed(NinjaQueueFailure::unknown_header_fact,0x403fc8,record);
    written=store(0x24,&*runtime.captured_word_at_3d0f840,4,0x403fce);if(!written)return written;
    if(!runtime.captured_word_at_3d0f9c4)return failed(NinjaQueueFailure::unknown_header_fact,0x403fd1,record);
    written=store(0x28,&*runtime.captured_word_at_3d0f9c4,4,0x403fd7);if(!written)return written;
    const auto& color=(flags&2)?runtime.color_at_3abd9a8:runtime.color_at_3ab9864;
    if(!color)return failed(NinjaQueueFailure::unknown_header_fact,0x403fea,record);
    const std::uint32_t color_sites[]={0x403fef,0x403ff4,0x403ffa,0x404000};
    for(unsigned i=0;i<4;++i){written=store(16+i*4,&(*color)[i],4,color_sites[i]);if(!written)return written;}
    if(!runtime.flag_at_3b12208)return failed(NinjaQueueFailure::unknown_header_fact,0x404003,record);
    if(flags&4){half=static_cast<std::uint16_t>(eax|0x8000);written=store(10,&half,2,0x404012);if(!written)return written;}
    byte=static_cast<std::uint8_t>(byte|(*runtime.flag_at_3b12208<<6));written=store(8,&byte,1,0x40401f);if(!written)return written;
    NinjaQueueResult result;result.record=record;return result;
}
NinjaQueueResult NinjaSpriteQueue::insert(const NinjaQueueRecordHandle& record,float depth,std::uint32_t policy,NinjaSpriteQueueRuntime& runtime,X87Context& fp){
    if(!record||record->arena!=arena_)return failed(NinjaQueueFailure::foreign_record,0x403e84,record);
    bool owned=false;for(const auto& r:records_)if(r==record){owned=true;break;}
    if(!owned)return failed(NinjaQueueFailure::foreign_record,0x403e84,record);
    if(record->linked)return failed(NinjaQueueFailure::already_linked,0x403e84,record);
    auto write_depth=[&](float value,std::uint32_t site){const auto f=arena_->write(record->block,4,&value,4);return f==NinjaQueueFailure::none?NinjaQueueResult{}:failed(f,site,record);};
    auto written=write_depth(depth,0x403e84);if(!written)return written;
    if(!runtime.sort_bias_at_3abd9c0)return failed(NinjaQueueFailure::missing_sort_bias,0x403e87,record);
    std::uint32_t at=0x403e87;
    try {
        auto retained=fp.from_float(*runtime.sort_bias_at_3abd9c0);at=0x403e8d;retained=fp.add_float_memory(retained,depth);
        if(!runtime.bucket_bias_at_7dcb60)return failed(NinjaQueueFailure::missing_constants,0x403e91,record);
        at=0x403e91;auto conversion=fp.from_float(*runtime.bucket_bias_at_7dcb60);at=0x403e97;conversion=fp.add(conversion,retained);
        at=0x403e99;auto index=crt_ftol_low32(conversion,fp);
        std::uint8_t flags;const auto read=arena_->read(record->block,8,&flags,1);if(read!=NinjaQueueFailure::none)return failed(read,0x403e9e,record);
        if(!(flags&16))index+=8;
        auto signed_index=signed_word(index);
        if(signed_index>2046){at=0x403eb2;written=write_depth(fp.to_float(retained),at);if(!written)return written;index=policy==1&&signed_index>=10000?2047:2046;}
        else if(signed_index<5){at=0x403ed9;written=write_depth(fp.to_float(retained),at);if(!written)return written;
            if(signed_index>-7952)index=4;else if(signed_index>-17952)index=3;else if(signed_index>-27952)index=2;else index=signed_index>-37952?1:0;
        }
        const auto bucket_index=static_cast<std::size_t>(index);auto& bucket=buckets_[bucket_index];
        if(!bucket.initialized)return failed(NinjaQueueFailure::unknown_bucket_state,0x403f17,record);
        float key;auto read_depth=arena_->read(record->block,4,&key,4);if(read_depth!=NinjaQueueFailure::none)return failed(read_depth,0x403f24,record);
        NinjaQueueRecordHandle previous,current=bucket.head;
        std::size_t visited=0;
        while(current){
            if(++visited>records_.size())return failed(NinjaQueueFailure::unknown_bucket_state,0x403f33,record);
            float existing;read_depth=arena_->read(current->block,4,&existing,4);if(read_depth!=NinjaQueueFailure::none)return failed(read_depth,0x403f27,record);
            at=0x403f24;auto loaded=fp.from_float(key);at=0x403f27;const auto comparison=fp.compare_float_memory(loaded,existing);
            // TEST AH,05 / JNP breaks only for LESS. Equal/unordered continue,
            // retaining stable tail insertion and source NaN behavior.
            if(comparison==X87Comparison::less)break;
            previous=current;if(current->next_is_sentinel){current.reset();break;}
            auto next=current->next.lock();if(!next)return failed(NinjaQueueFailure::unknown_bucket_state,0x403f33,record);current=std::move(next);
        }
        record->next=current;record->next_is_sentinel=!current;record->bucket=bucket_index;record->linked=true;
        if(previous){previous->next=record;previous->next_is_sentinel=false;}else bucket.head=record;
        if(!current)bucket.tail=record;
        NinjaQueueResult result;result.record=record;result.bucket=bucket_index;return result;
    }catch(const X87Error& e){auto result=failed(NinjaQueueFailure::fp_failed,at,record);result.fp_failure=e.failure();result.fp_exceptions=e.exceptions();return result;}
}

NinjaSpriteSubmissionResult NinjaSpriteQueue::submit(const std::optional<std::shared_ptr<const NinjaQueuedSpriteInput>>& input,std::optional<bool> domain,std::int32_t frame,std::uint32_t flags,std::uint32_t queue_flags,NinjaSpriteQueueRuntime& runtime,NinjaMatrixPrimitives* coordinates,X87Context& fp){
    NinjaSpriteSubmissionResult result;
    auto stop=[&](NinjaQueueFailure failure,std::uint32_t site,NinjaQueueRecordHandle record={}){result.queue=failed(failure,site,std::move(record));return result;};
    if(!runtime.display_switch_at_3b1117c||!*runtime.display_switch_at_3b1117c)return stop(NinjaQueueFailure::unknown_display_switch,0x404760);
    if(**runtime.display_switch_at_3b1117c!=0)return result;
    if(!input)return stop(NinjaQueueFailure::unknown_input,0x40476f);
    if(!*input)return result;
    if(!domain)return stop(NinjaQueueFailure::unknown_pointer_domain,0x404773);
    if(!*domain)return result;
    result.texture_gate=ninja_texture_list_gate((*input)->texture_list);
    if(!*result.texture_gate)return stop(result.texture_gate->failure,result.texture_gate->source_boundary);
    if(*result.texture_gate->source_not_ready)return result;
    result.queue=allocate_record(0x98,3,static_cast<std::uint8_t>(queue_flags),static_cast<std::uint32_t>(frame),runtime);
    if(!result.queue)return result;
    const auto record=result.queue.record;
    if(!record){result.route=NinjaSpriteSubmissionRoute::immediate_required;return result;}
    auto write=[&](std::uint32_t offset,const void* data,std::uint32_t bytes,std::uint32_t site){const auto failure=arena_->write(record->block,offset,data,bytes);if(failure==NinjaQueueFailure::none)return true;result.queue=failed(failure,site,record);return false;};
    if(!write(0x50,&flags,4,0x4047b6))return result;
    const auto& sprite=(*input)->sprite;
    if(!write(0x30,&sprite.position,12,0x4047c3)||!write(0x3c,&sprite.scale_x,4,0x4047c3)||!write(0x40,&sprite.scale_y,4,0x4047c3)||!write(0x44,&sprite.angle,4,0x4047c3))return result;
    // Source8-dword copy includes two resource pointers. Native references are
    // authoritative and retained here; no fake pointer bytes are written.
    record->sprite=**input;
    if(!coordinates)return stop(NinjaQueueFailure::missing_matrix,0x4047c8,record);
    std::uint32_t site=0x4047c8;
    try{
        const auto matrix=coordinates->current_matrix();if(!write(0x58,matrix.data(),64,0x4047c8))return result;
        site=0x4047cf;auto z=fp.from_float(matrix[10]);site=0x4047d6;z=fp.multiply_float_memory(z,sprite.position.z);
        site=0x4047db;auto y=fp.from_float(matrix[6]);site=0x4047de;y=fp.multiply_float_memory(y,sprite.position.y);
        site=0x4047e1;auto sum=fp.add(z,y);
        site=0x4047e3;auto x=fp.from_float(matrix[2]);site=0x4047e6;x=fp.multiply_float_memory(x,sprite.position.x);
        site=0x4047e9;sum=fp.add(sum,x);site=0x4047eb;sum=fp.add_float_memory(sum,matrix[14]);sum=sum.negated();
        if(!runtime.depth_bias_at_3abd9b8||!*runtime.depth_bias_at_3abd9b8)return stop(NinjaQueueFailure::missing_depth_bias,0x4047f3,record);
        site=0x4047f3;sum=fp.add_float_memory(sum,**runtime.depth_bias_at_3abd9b8);site=0x4047f9;const auto depth=fp.to_float(sum);
        result.queue=insert(record,depth,0,runtime,fp);if(result.queue)result.route=NinjaSpriteSubmissionRoute::queued;return result;
    }catch(const CoordinateError& e){result.coordinate_failure=e.failure();return stop(NinjaQueueFailure::coordinate_failed,site,record);}
    catch(const X87Error& e){result.queue=failed(NinjaQueueFailure::fp_failed,site,record);result.queue.fp_failure=e.failure();result.queue.fp_exceptions=e.exceptions();return result;}
}

NinjaQueueBucketResult NinjaSpriteQueue::bucket_records(std::size_t index)const{
    NinjaQueueBucketResult result;
    if(index>=buckets_.size()||!buckets_[index].initialized){result.failure=NinjaQueueFailure::unknown_bucket_state;return result;}
    auto current=buckets_[index].head;
    while(current){if(result.records.size()>=records_.size()){result.failure=NinjaQueueFailure::unknown_bucket_state;return result;}result.records.push_back(current);if(current->next_is_sentinel)break;current=current->next.lock();if(!current){result.failure=NinjaQueueFailure::unknown_bucket_state;return result;}}return result;
}
NinjaQueueBucketFrontResult NinjaSpriteQueue::bucket_front(std::size_t index)const{
    if(index>=buckets_.size()||!buckets_[index].initialized)return {NinjaQueueFailure::unknown_bucket_state,{}};
    return {NinjaQueueFailure::none,buckets_[index].head};
}
bool NinjaSpriteQueue::owns_record(const NinjaQueueRecordHandle& record)const noexcept{
    if(!record||record->arena!=arena_)return false;for(const auto& owned:records_)if(owned==record)return true;return false;
}
NinjaQueueResult ninja_prepare_immediate_sprite(std::uint8_t flags,NinjaSpriteQueueRuntime& runtime,const std::array<std::uint32_t,8>* table,NinjaSpriteProjection& geometry){
    NinjaQueueResult result;result.render=NinjaRenderResult{};
    auto call=[&](NinjaRenderResult r){auto& aggregate=*result.render;aggregate.commands.insert(aggregate.commands.end(),r.commands.begin(),r.commands.end());aggregate.ignored_hresults.insert(aggregate.ignored_hresults.end(),r.ignored_hresults.begin(),r.ignored_hresults.end());if(r)return true;aggregate.failure=r.failure;aggregate.source_boundary=r.source_boundary;aggregate.pending_command=r.pending_command;result.failure=NinjaQueueFailure::render_failed;result.source_boundary=r.source_boundary;return false;};
    if((!(flags&1)||(flags&16))&&!runtime.render_state)return failed(NinjaQueueFailure::render_failed,!(flags&1)?0x40424c:0x404262);
    if(!(flags&1)){if(!call(ninja_set_depth_comparison(3,table,*runtime.render_state)))return result;if(!call(ninja_set_depth_write(0,*runtime.render_state)))return result;}
    if(flags&16){if(!call(ninja_set_depth_comparison(7,table,*runtime.render_state)))return result;if(!call(ninja_set_depth_write(0,*runtime.render_state)))return result;}
    const auto& color=(flags&2)?runtime.color_at_3abd9a8:runtime.color_at_3ab9864;
    if(!color){result.failure=NinjaQueueFailure::missing_color_owner;result.source_boundary=0x77ff98;return result;}
    geometry.color_argb_at_3d0f7f0=*color;return result;
}
} // namespace sadx

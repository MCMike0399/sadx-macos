#include "engine/ninja_queue_consumer.hpp"
#include <cstring>
#include <set>
namespace sadx {
namespace {
struct Boundary{};
struct Operation {
    NinjaQueueConsumerRuntime& runtime;X87Context& fp;NinjaQueueConsumerResult result;
    std::uint32_t fp_site=0;
    [[noreturn]]void stop(NinjaQueueConsumerFailure failure,std::uint32_t at,std::uint32_t callee=0){result.failure=failure;result.source_boundary=at;result.callee_boundary=callee;throw Boundary{};}
    template<class T>T need(const std::optional<T>& value,std::uint32_t at,std::uint32_t fact){if(!value){result.required_fact=fact;stop(NinjaQueueConsumerFailure::unknown_fact,at);}return *value;}
    NinjaRenderState& render(){if(!runtime.queue_runtime.render_state)stop(NinjaQueueConsumerFailure::render_failed,0x77ed04);return *runtime.queue_runtime.render_state;}
    template<class T>T read(const NinjaQueueRecordHandle& record,std::uint32_t offset,std::uint32_t at){T value;auto failure=record->arena->read(record->block,offset,&value,sizeof(value));if(failure!=NinjaQueueFailure::none){result.queue_failure=failure;stop(NinjaQueueConsumerFailure::queue_failed,at);}return value;}
    void append(NinjaRenderResult commands){auto& all=result.render;all.commands.insert(all.commands.end(),commands.commands.begin(),commands.commands.end());all.ignored_hresults.insert(all.ignored_hresults.end(),commands.ignored_hresults.begin(),commands.ignored_hresults.end());if(!commands){all.failure=commands.failure;all.source_boundary=commands.source_boundary;all.pending_command=commands.pending_command;stop(NinjaQueueConsumerFailure::render_failed,commands.source_boundary);}}
    void source_state(std::uint32_t state,std::uint32_t value,NinjaRenderSourceSites sites){if(!ninja_issue_render_state(result.render,render(),state,value,sites))stop(NinjaQueueConsumerFailure::render_failed,result.render.source_boundary);}
    void color(const std::optional<std::array<float,4>>& input,std::uint32_t call){
        result.colors.emplace_back();
        NinjaQueueColorOwners owners{runtime.queue_runtime.color_at_3ab9864,runtime.sprite.projection.color_argb_at_3d0f7f0,runtime.sprite.constants.one,runtime.constants.negative_one_at_7dcb0c};
        if(!ninja_queue_color_402f40(input,owners,result.colors.back(),fp,fp_site)){result.required_fact=call==0x4128f0?0x3b18230:0x3ab9864;stop(NinjaQueueConsumerFailure::unknown_fact,0x402f44);}
    }
    std::int32_t display(std::uint32_t at){auto pointer=runtime.queue_runtime.display_switch_at_3b1117c;if(!pointer){result.required_fact=0x3b1117c;stop(NinjaQueueConsumerFailure::unknown_fact,at);}return need(*pointer,at,0x3b1117c);}
    void mode(std::uint32_t value){runtime.queue_runtime.flag_at_3b12208=value;runtime.state.mode_byte_at_3abdbc0=static_cast<std::uint8_t>(value);}//412160→401440
    void fog(bool enabled){
        if(!update_queue_fog(enabled,runtime,fp,result,fp_site))
            stop(result.failure,result.source_boundary,result.callee_boundary);
    }
    NinjaMatrixPrimitives& coordinates(std::uint32_t at){if(!runtime.sprite.projection.coordinates)stop(NinjaQueueConsumerFailure::missing_coordinates,at);return *runtime.sprite.projection.coordinates;}
    void body(){
        auto& q=runtime.queue_runtime;auto& state=runtime.state;
        const auto saved_word20=q.captured_word_at_3d0f9c8,saved_word24=q.captured_word_at_3d0f840,saved_word28=q.captured_word_at_3d0f9c4,saved_mode=q.flag_at_3b12208;
        const auto saved_fog=q.flag_at_3abdcfe,saved_special=state.special_flags_at_909eb4;
        // The supported3/default/observed-null branches never change the local
        // FOV register. Its final comparison with the same saved value is equal
        // even when that value is unknown; do not fabricate an angle or require
        // an unused field. Other selected bodies stop before their FOV effects.
        std::int32_t fog_class=-1;
        color(runtime.initial_color_at_3b18230,0x4128f0);
        const auto disabled=display(0x40874f);const auto queue_mode=need(q.mode_at_3ab98ac,0x408756,0x3ab98ac);
        if(disabled||queue_mode){if(queue_mode==2){append(ninja_set_depth_comparison(7,runtime.depth_comparison_table,render()));stop(NinjaQueueConsumerFailure::unlifted_callee,0x40901c,0x4071c0);}q.mode_at_3ab98ac=1;result.returned=true;return;}
        const auto count=need(state.count_at_3abd9d4,0x408769,0x3abd9d4);const auto maximum=need(state.maximum_at_3abd9d8,0x40876e,0x3abd9d8);q.mode_at_3ab98ac=1;if(maximum<count)state.maximum_at_3abd9d8=count;
        const auto count2=need(state.count_at_3abd9dc,0x408789,0x3abd9dc);if(need(state.maximum_at_3abd9e0,0x40878e,0x3abd9e0)<count2)state.maximum_at_3abd9e0=count2;
        if(need(q.flag_at_3b12208,0x40879b,0x3b12208)!=0)mode(0);
        fp_site=0x4087b2;const auto saved_matrix=coordinates(0x4087b2).current_matrix();
        auto branch_flags=need(state.special_flags_at_909eb4,0x4087b7,0x909eb4);
        auto pass=(branch_flags&3)==3?1:0;
        for(;pass>=0;--pass)for(std::size_t bucket=0;bucket<2048;++bucket){
            const auto front=runtime.queue.bucket_front(bucket);if(!front){result.queue_failure=front.failure;stop(NinjaQueueConsumerFailure::queue_failed,0x4087ea);}auto current=front.record;std::set<const NinjaQueueRecord*> visited;
            while(current){if(!runtime.queue.owns_record(current))stop(NinjaQueueConsumerFailure::foreign_record,0x408800);if(!visited.insert(current.get()).second)stop(NinjaQueueConsumerFailure::cycle,0x408800);
                result.records.push_back({current,bucket,pass,false,{},{},{},{},{},{}});auto& row=result.records.back();
                const auto flags=read<std::uint8_t>(current,8,0x408800);row.flags=flags;
                q.captured_word_at_3d0f9c8=read<std::uint32_t>(current,0x20,0x408804);q.captured_word_at_3d0f840=read<std::uint32_t>(current,0x24,0x40880d);q.captured_word_at_3d0f9c4=read<std::uint32_t>(current,0x28,0x408816)|0x800000U;
                const auto attribute=read<std::uint16_t>(current,10,0x408829);row.attribute=attribute;
                const bool selected_fog=(flags&32)&&!(attribute&0x8000);const bool two_pass=(branch_flags&3)==3;
                if(two_pass && selected_fog!=(pass==1)){row.skipped_by_pass=true;}else{
                    if(selected_fog){if(fog_class==0)fog(true);fog_class=1;}else {if(fog_class!=0)fog(false);fog_class=0;}
                    auto next_mode=std::uint32_t(flags>>6);if(attribute&0x8000){q.captured_word_at_3d0f9c4=*q.captured_word_at_3d0f9c4|0x2080000U;next_mode=0;}
                    if(next_mode!=need(q.flag_at_3b12208,0x4088a8,0x3b12208))mode(next_mode);
                    //411f30's argument is ignored and its actual51f680 is RET.
                    q.captured_word_at_3abd950=read<std::uint32_t>(current,12,0x4088c6);q.captured_texture_list_at_3abd950=current->texture_list_at_c;
                    const auto header=need(current->texture_list_at_c,0x4088e7,0x3abd950);
                    bool usable_header=static_cast<bool>(header),texture_valid=true;
                    if(header){row.header_texture_gate=ninja_texture_list_gate(header);if(!*row.header_texture_gate){result.queue_failure=row.header_texture_gate->failure;stop(NinjaQueueConsumerFailure::queue_failed,row.header_texture_gate->source_boundary);}
                        if(*row.header_texture_gate->source_not_ready){usable_header=false;texture_valid=false;}
                        if(!*row.header_texture_gate->source_not_ready){row.header_texture_selection=ninja_select_texture_list(header,runtime.sprite.textures,runtime.sprite.texture_runtime);if(!*row.header_texture_selection)stop(NinjaQueueConsumerFailure::texture_failed,0x40891b,0x77f3d0);}
                    }
                    append(ninja_set_depth_comparison(bucket==2047?7:3,runtime.depth_comparison_table,render()));append(ninja_set_depth_write(bucket==2047?0:std::uint32_t((flags&16)!=0),render()));
                    std::array<float,4> color_words;for(unsigned i=0;i<4;++i)color_words[i]=read<float>(current,16+i*4,0x408962);runtime.sprite.projection.color_argb_at_3d0f7f0=color_words;
                    const auto packed=read<std::uint8_t>(current,9,0x408969);append(ninja_set_blend(0,packed&15,render()));append(ninja_set_blend(1,packed>>4,render()));
                    const auto live_special=need(state.special_flags_at_909eb4,0x408986,0x909eb4);if((live_special&3)==3)state.special_flags_at_909eb4=std::uint8_t(live_special&~2U);
                    const auto type=flags&15;
                    if(type==3){if(!current->sprite)stop(NinjaQueueConsumerFailure::missing_sprite_owner,0x408ab6);
                        row.sprite_texture_gate=ninja_texture_list_gate(current->sprite->texture_list);if(!*row.sprite_texture_gate){result.queue_failure=row.sprite_texture_gate->failure;stop(NinjaQueueConsumerFailure::queue_failed,row.sprite_texture_gate->source_boundary);}
                        if(!*row.sprite_texture_gate->source_not_ready){
                            fp_site=0x408b0d;coordinates(0x408b0d).current_matrix()=read<NinjaMatrix>(current,0x58,0x408b0d);
                            const auto sprite_flags=read<std::uint32_t>(current,0x50,0x408b12);
                            auto packet=std::make_shared<const NinjaQueuedSpriteInput>(*current->sprite);
                            row.sprite_display=ninja_display_sprite(packet,attribute&0x7fff,sprite_flags,current,runtime.sprite,fp);
                            if(!*row.sprite_display)stop(NinjaQueueConsumerFailure::display_failed,0x408b1f,0x77e390);
                        }
                    }else if(type!=0){
                        const bool observed_skip=(!usable_header&&(type==1||type==8||type==10||type==11||type==15))||(!texture_valid&&type==9)||(type==12&&read<std::uint32_t>(current,0x70,0x408e94)==0);
                        if(!observed_skip){static constexpr std::uint32_t body_entries[]={0,0x4089b5,0x408a1b,0,0x408baa,0x408c00,0x408cd4,0x408d2a,0x408d70,0x408db6,0x408e01,0x408e3b,0x408e94,0x408eda,0x408b2c,0x408e70};stop(NinjaQueueConsumerFailure::unlifted_dispatch,body_entries[type]);}
                    }
                    branch_flags=need(saved_special,0x408f17,0x909eb4);state.special_flags_at_909eb4=branch_flags;
                }
                // Source reads NEXT after callbacks. Do not pre-snapshot the
                // list or silently turn a missing edge into an empty bucket.
                if(current->next_is_sentinel){if(current->bucket!=bucket)stop(NinjaQueueConsumerFailure::sentinel_mismatch,0x408f24);current.reset();}else{auto next=current->next.lock();if(!next)stop(NinjaQueueConsumerFailure::missing_next,0x408f24);current=std::move(next);}
            }
        }
        fp_site=0x408f53;coordinates(0x408f53).current_matrix()=saved_matrix;q.captured_word_at_3abd950=0;q.captured_texture_list_at_3abd950=std::shared_ptr<NinjaTextureListOwner>{};
        color(q.color_at_3ab9864,0x402f40);const auto prior_mode=need(saved_mode,0x408f70,0x3b12208);mode((prior_mode>>1)&63U); //actual412400, not exact restore
        append(ninja_set_depth_comparison(1,runtime.depth_comparison_table,render()));append(ninja_set_depth_write(1,render()));
        q.captured_word_at_3d0f9c8=saved_word20;q.captured_word_at_3d0f840=saved_word24;q.captured_word_at_3d0f9c4=saved_word28;
        fog(need(saved_fog,0x408fbe,0x3abdcfe)!=0);result.returned=true;
    }
};
}
NinjaFovResult ninja_set_fov_7815c0(std::int32_t angle,const NinjaMath::SineTable& table,NinjaSpriteProjection& projection,const std::optional<float>& width_at_3d0f9e4,X87Context& fp){
    NinjaFovResult r;std::uint32_t site=0x7815cb;
    try{
        const NinjaMath math(table,fp);const auto half=angle/2; // CDQ/SUB/SAR: truncating halve
        site=0x787e34;const auto sine=math.sin_extended(half);site=0x787e39;const auto stored=fp.to_float(sine);
        site=0x787e3f;auto cosine=math.cos_extended(half);site=0x787e44;auto tangent=fp.reverse_divide_float_memory(cosine,stored);
        site=0x7815d0;tangent=fp.add(tangent,tangent);
        if(!width_at_3d0f9e4){r.source_boundary=0x7815d2;return r;}
        site=0x7815d2;const auto scale=fp.reverse_divide_float_memory(tangent,*width_at_3d0f9e4);site=0x7815d8;projection.scale_at_3d0f9e0=fp.to_float(scale);
        r.completed=true;
    }catch(const X87Error& e){r.fp_failure=e.failure();r.source_boundary=site;}
    return r;
}
bool ninja_queue_color_402f40(const std::optional<std::array<float,4>>& input,NinjaQueueColorOwners& o,NinjaQueueColorResult& partial,X87Context& fp,std::uint32_t& fp_site){
    if(!input)return false;
    const auto& source=*input;auto& owned=o.color_at_3ab9864;
    if(owned)for(unsigned i=0;i<4;++i)partial.source_stores[i]=(*owned)[i];
    auto store=[&](unsigned i,float value){partial.source_stores[i]=value;if(owned)(*owned)[i]=value;bool known=true;for(const auto& v:partial.source_stores)known=known&&v.has_value();if(known)owned=std::array<float,4>{*partial.source_stores[0],*partial.source_stores[1],*partial.source_stores[2],*partial.source_stores[3]};};
    store(0,source[0]);fp_site=0x402f4c;auto loaded=fp.from_float(source[0]);fp_site=0x402f55;const auto first=fp.compare_float_memory(loaded,o.one_at_7dcb10);
    store(1,source[1]);store(2,source[2]);store(3,source[3]);
    const std::uint32_t load[]={0x402f4c,0x402fa3,0x402fdf,0x40301b},compare_sites[]={0x402f55,0x402fa9,0x402fe5,0x403021},lower_load[]={0x402f86,0x402fc2,0x402ffe,0x403045},lower_compare[]={0x402f8c,0x402fc8,0x403004,0x40304b};
    for(unsigned i=0;i<4;++i){auto compared=first;if(i){fp_site=load[i];loaded=fp.from_float(*partial.source_stores[i]);fp_site=compare_sites[i];compared=fp.compare_float_memory(loaded,o.one_at_7dcb10);}
        if(compared==X87Comparison::greater)store(i,1.0F);else{fp_site=lower_load[i];loaded=fp.from_float(*partial.source_stores[i]);fp_site=lower_compare[i];if(fp.compare_float_memory(loaded,o.negative_one_at_7dcb0c)==X87Comparison::less)store(i,-1.0F);}
    }
    o.projection_color_at_3d0f7f0=*owned; // actual 77ff90 raw copy
    return true;
}
std::optional<NinjaQueueConsumerConstants> decode_ninja_queue_consumer_constants(const std::uint8_t* bytes,std::size_t size){if(!bytes||size!=8)return {};NinjaQueueConsumerConstants result;std::memcpy(&result,bytes,8);return result;}
NinjaQueueConsumerResult ninja_consume_queue(NinjaQueueConsumerRuntime& runtime,X87Context& fp){Operation op{runtime,fp,{},0};if(runtime.queue_runtime.render_state!=&runtime.sprite.texture_runtime.render){op.result.failure=NinjaQueueConsumerFailure::owner_mismatch;op.result.source_boundary=0x4086f0;return op.result;}try{op.body();}catch(Boundary){}catch(const CoordinateError& e){op.result.failure=NinjaQueueConsumerFailure::coordinate_failed;op.result.coordinate_failure=e.failure();op.result.source_boundary=op.fp_site;}catch(const X87Error& e){op.result.failure=NinjaQueueConsumerFailure::fp_failed;op.result.fp_failure=e.failure();op.result.fp_exceptions=e.exceptions();op.result.source_boundary=op.fp_site;}return std::move(op.result);}
} // namespace sadx

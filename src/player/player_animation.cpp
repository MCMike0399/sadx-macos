#include "player/player_animation.hpp"
#include <cstring>
namespace sadx { namespace {
struct Stop {PlayerAnimationResult result;};
[[noreturn]] void stop(PlayerAnimationFailure failure,std::uint32_t at,std::uint32_t callee=0){PlayerAnimationResult r;r.failure=failure;r.source_boundary=at;r.callee_address=callee;throw Stop{r};}
PlayerAnimationResult complete(){PlayerAnimationResult r;r.completed=true;return r;}
template<class T> const T& known(const std::optional<T>& v,std::uint32_t at){if(!v)stop(PlayerAnimationFailure::unknown_field,at);return *v;}
const PlayerAnimationTable& table(const PlayerAnimationView& v,std::uint32_t at){const auto& ptr=known(v.tail.table_at_140,at);if(!ptr)stop(PlayerAnimationFailure::null_resource,at);return *ptr;}
const PlayerAnimationRecord& record(const PlayerAnimationView& v,std::uint16_t index,std::uint32_t at){const auto& t=table(v,at);auto r=t.records.find(index);if(r==t.records.end())stop(PlayerAnimationFailure::unresolved_resource,at);return r->second;}
const PlayerAnimationAction& action(const PlayerAnimationRecord& r,const PlayerAnimationRuntime& rt,std::uint32_t at){const auto& ref=known(r.action,at);if(!ref.id)stop(PlayerAnimationFailure::null_resource,at);auto a=rt.resources.actions.find(*ref.id);if(a==rt.resources.actions.end())stop(PlayerAnimationFailure::unresolved_resource,at);return a->second;}
const PlayerAnimationMotion& motion(const PlayerAnimationRecord& r,const PlayerAnimationRuntime& rt,std::uint32_t at){const auto& a=action(r,rt,at);const auto& ref=known(a.motion,at);if(!ref.id)stop(PlayerAnimationFailure::null_resource,at);auto m=rt.resources.motions.find(*ref.id);if(m==rt.resources.motions.end())stop(PlayerAnimationFailure::unresolved_resource,at);return m->second;}
X87Value unsigned_frame(const PlayerAnimationRecord& r,const PlayerAnimationRuntime& rt,std::uint32_t at,X87Context& fp){auto raw=known(motion(r,rt,at).frame_count,at);std::int32_t signed_value;std::memcpy(&signed_value,&raw,4);auto value=fp.from_int64(signed_value);if(signed_value<0)value=fp.add_float_memory(value,rt.constants.unsigned_correction);return value;}
float scalar(PlayerAnimationView& v,const PlayerAnimationRuntime& rt,std::uint32_t offset,std::uint32_t at){
    if(!rt.arena)stop(PlayerAnimationFailure::missing_arena,at);
    if(!v.player_allocation)stop(PlayerAnimationFailure::unresolved_resource,at);
    auto ref=rt.arena->reference_view(v.player_allocation,offset);
    if(!ref){PlayerAnimationResult r;r.failure=PlayerAnimationFailure::work_failed;r.source_boundary=at;r.work_failure=ref.failure;throw Stop{r};}
    if(!ref.work)stop(PlayerAnimationFailure::null_resource,at);
    if(ref.work!=v.player_allocation)stop(PlayerAnimationFailure::unresolved_resource,at);
    if(ref.byte_offset==0x38)return v.player.motion.speed.x;
    if(ref.byte_offset==0)return known(v.player.source_charge_at_0,at);
    stop(PlayerAnimationFailure::unresolved_resource,at);
}
std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
bool greater_or_unordered(X87Comparison c){return c==X87Comparison::greater||c==X87Comparison::unordered;}
void arena_result(const TaskWorkResult& result,std::uint32_t at){if(!result){PlayerAnimationResult r;r.failure=PlayerAnimationFailure::work_failed;r.source_boundary=at;r.work_failure=result.failure;throw Stop{r};}}
PlayerAnimationAllocatedAction& buffer(PlayerAnimationView& v,std::uint32_t at){const auto& ref=known(v.tail.buffer_at_148,at);if(!ref)stop(PlayerAnimationFailure::null_resource,at);return *ref;}
PlayerAnimationAllocatedMotion& allocated_motion(PlayerAnimationView& v,std::uint32_t at){auto& a=buffer(v,at);const auto& m=known(a.motion,at);if(!m)stop(PlayerAnimationFailure::null_resource,at);return *m;}
void require_arena(const PlayerAnimationRuntime& rt,std::uint32_t at){if(!rt.arena)stop(PlayerAnimationFailure::missing_arena,at);}
void allocate_initial(PlayerAnimationView& v,PlayerAnimationRuntime& rt,std::uint16_t req){
    v.player.current_animation_at_126=req;v.player.animation_mode_at_120=1;v.tail.start_at_134=0;v.tail.frame_at_130=0;
    require_arena(rt,0x44a868);auto allocation=rt.arena->allocate(8);if(!allocation&&allocation.failure!=TaskWorkFailure::exhausted)arena_result(allocation,0x44a868);
    (void)table(v,0x44a86d);std::shared_ptr<PlayerAnimationAllocatedAction> a;
    if(allocation.work){a=std::make_shared<PlayerAnimationAllocatedAction>();a->allocation=allocation.work;}v.tail.buffer_at_148=a;
    const auto& selected=action(record(v,req,0x44a879),rt,0x44a87c);const auto& object=known(selected.object,0x44a87c);
    if(!a)stop(PlayerAnimationFailure::null_resource,0x44a880);a->object=object;
    auto m_alloc=rt.arena->allocate(12);if(!m_alloc&&m_alloc.failure!=TaskWorkFailure::exhausted)arena_result(m_alloc,0x44a882);
    std::shared_ptr<PlayerAnimationAllocatedMotion> m;if(m_alloc.work){m=std::make_shared<PlayerAnimationAllocatedMotion>();m->allocation=m_alloc.work;}a->motion=m;
    if(!m)stop(PlayerAnimationFailure::null_resource,0x44a898);m->frame_count=2;m->type=3;m->input_function=2;
    auto data=rt.arena->allocate(111*24);if(!data&&data.failure!=TaskWorkFailure::exhausted)arena_result(data,0x44a8b5);if(data.work)arena_result(rt.arena->fill(data.work,0,0,111*24),0x44a8b5);m->data=data.work;
}
void wrap_frame(PlayerAnimationView& v,const PlayerAnimationRuntime& rt,const PlayerAnimationRecord& selected,std::uint32_t boundary,X87Context& fp,PlayerAnimationResult& result){
    auto period=unsigned_frame(selected,rt,boundary,fp);
    while(!greater_or_unordered(fp.compare_float_memory(period,known(v.tail.frame_at_130,boundary)))){
        auto before=known(v.tail.frame_at_130,boundary);v.tail.frame_at_130=fp.to_float(fp.subtract(fp.from_float(before),period));++result.wrap_iterations;
        period=unsigned_frame(selected,rt,boundary,fp);
        if(bits(before)==bits(*v.tail.frame_at_130)&&!greater_or_unordered(fp.compare_float_memory(period,*v.tail.frame_at_130)))stop(PlayerAnimationFailure::source_loop_no_progress,boundary);
    }
    while(fp.compare_float_memory(fp.from_float(rt.constants.zero),known(v.tail.frame_at_130,boundary))==X87Comparison::greater){
        auto before=*v.tail.frame_at_130;auto p=unsigned_frame(selected,rt,boundary,fp);v.tail.frame_at_130=fp.to_float(fp.add_float_memory(p,before));++result.wrap_iterations;
        if(bits(before)==bits(*v.tail.frame_at_130)&&fp.compare_float_memory(fp.from_float(rt.constants.zero),*v.tail.frame_at_130)==X87Comparison::greater)stop(PlayerAnimationFailure::source_loop_no_progress,boundary);
    }
}
}
PlayerAnimationResult reset_player_animation_frame(PlayerAnimationView& v,const PlayerAnimationRuntime& rt,X87Context& fp){try{const auto mode=known(v.player.animation_mode_at_120,0x43f231);if(mode==6||mode==7){const auto current=known(v.player.current_animation_at_126,0x43f270);auto frame=unsigned_frame(record(v,current,0x43f27a),rt,0x43f280,fp);v.tail.frame_at_130=fp.to_float(fp.subtract_float_memory(frame,rt.constants.one));}else if(mode==3||mode==9||mode==10||mode==12){v.tail.frame_at_130=known(v.tail.start_at_134,0x43f261);v.tail.start_at_134=0;}else v.tail.frame_at_130=0;return complete();}catch(const Stop& s){return s.result;}}
PlayerAnimationResult next_player_animation(PlayerAnimationView& v,const PlayerAnimationRuntime& rt,std::uint16_t from,X87Context& fp){try{auto next=static_cast<std::uint8_t>(known(record(v,from,0x446919).next,0x446919));v.player.requested_animation_at_124=next;v.player.current_animation_at_126=next;v.player.animation_mode_at_120=known(record(v,next,0x44692d).mode,0x44692d);const auto nextnext=static_cast<std::uint8_t>(known(record(v,next,0x44693d).next,0x44693d));auto flags=known(v.tail.flags_at_12e,0x446943);v.tail.flags_at_12e=flags|1;v.tail.next_at_12a=nextnext;return reset_player_animation_frame(v,rt,fp);}catch(const Stop& s){return s.result;}}
PlayerAnimationResult blend_player_animation(PlayerAnimationView& v,PlayerAnimationRuntime& rt,X87Context& fp){try{const auto current=known(v.player.current_animation_at_126,0x446950),req=known(v.player.requested_animation_at_124,0x446958);const auto old_type=known(motion(record(v,current,0x44695f),rt,0x446962).type,0x446965);const auto new_type=known(motion(record(v,req,0x44696c),rt,0x44696f).type,0x446972);const auto site=((old_type|new_type)&4)?0x4469a0U:0x446996U;auto result=build_player_animation_transition(v,rt,fp);if(!result){result.nested_source_boundary=result.source_boundary;result.source_boundary=site;result.callee_address=0x43f2a0;if(result.failure==PlayerAnimationFailure::none)result.failure=PlayerAnimationFailure::blend_failed;}return result;}catch(const Stop& s){return s.result;}}
PlayerAnimationResult release_animation_keys(PlayerAnimationView& v,PlayerAnimationRuntime& rt){try{
    auto req=known(v.player.requested_animation_at_124,0x43f9f0);auto& m=allocated_motion(v,0x43f9fa);const auto count=known(record(v,req,0x43fa00).object_count,0x43fa00);auto data=known(m.data,0x43fa08);const auto channels=known(m.input_function,0x43fa0b);if(!count)return complete();if(!data)stop(PlayerAnimationFailure::null_resource,0x43fa14);require_arena(rt,0x43fa1b);
    std::uint32_t cursor=0;for(std::uint32_t node=0;node<count;++node){const auto stride=channels==3?24U:16U;
        for(std::uint32_t channel=0;channel<(channels==3?3U:2U);++channel){const auto slot=cursor+4*channel;const std::uint32_t load_sites[3]={0x43fa14,0x43fa33,0x43fa5f},free_sites[3]={0x43fa1b,0x43fa3e,0x43fa66},clear_sites[3]={0x43fa23,0x43fa46,0x43fa6b},count_sites[3]={0x43fa2c,0x43fa4f,0x43fa74};auto ref=rt.arena->reference_view(data,slot);if(!ref){PlayerAnimationResult r;r.failure=PlayerAnimationFailure::work_failed;r.source_boundary=load_sites[channel];r.work_failure=ref.failure;throw Stop{r};}if(ref.byte_offset)stop(PlayerAnimationFailure::unresolved_resource,free_sites[channel]);if(ref.work){arena_result(rt.arena->release(ref.work),free_sites[channel]);arena_result(rt.arena->set_reference(data,slot,{}),clear_sites[channel]);arena_result(rt.arena->set_reference(data,slot+4*channels,{}),count_sites[channel]);}}
        cursor+=stride;
    }return complete();}catch(const Stop& s){return s.result;}}
PlayerAnimationResult update_player_animation(PlayerAnimationView& v,PlayerAnimationRuntime& rt,X87Context& fp){PlayerAnimationResult result;try{
    const auto current=known(v.player.current_animation_at_126,0x44a808);const auto requested=known(v.player.requested_animation_at_124,0x44a80c);v.player.previous_animation_at_128=current;v.tail.flags_at_12e=0;
    auto mode=known(v.player.animation_mode_at_120,0x44a825);
    if(current!=requested&&mode!=2&&mode!=0&&mode!=14){mode=1;v.player.animation_mode_at_120=1;v.tail.timer_at_12c=0;}else{const auto timer=known(v.tail.timer_at_12c,0x44a842);v.tail.timer_at_12c=static_cast<std::uint16_t>(timer+1);}
    if(mode==0){allocate_initial(v,rt,requested);return complete();}
    if(mode<0||mode>12){auto released=release_animation_keys(v,rt);if(!released)return released;const auto& ref=known(v.tail.buffer_at_148,0x44adf6);if(ref){auto& m=allocated_motion(v,0x44adfd);auto data=known(m.data,0x44ae00);require_arena(rt,0x44ae07);if(data){arena_result(rt.arena->release(data),0x44ae07);m.data=TaskWorkHandle{};}arena_result(rt.arena->release(m.allocation),0x44ae22);ref->motion=std::shared_ptr<PlayerAnimationAllocatedMotion>{};arena_result(rt.arena->release(ref->allocation),0x44ae34);v.tail.buffer_at_148=std::shared_ptr<PlayerAnimationAllocatedAction>{};}return complete();}
    std::uint16_t selected_index=requested;
    auto transition=[&](const PlayerAnimationRecord& selected,std::uint32_t at){auto step=known(selected.transition_step,at);auto frame=fp.add_float_memory(fp.from_float(step),known(v.tail.frame_at_130,at));v.tail.frame_at_130=fp.to_float(frame);auto c=fp.compare_float_memory(frame,rt.constants.one);if(c==X87Comparison::less||c==X87Comparison::unordered)return complete();v.player.animation_mode_at_120=known(selected.mode,0x44a93b);auto released=release_animation_keys(v,rt);if(!released)return released;return reset_player_animation_frame(v,rt,fp);};
    if(mode==1){const auto& selected=record(v,requested,0x44a8d9);const auto rate=known(selected.transition_step,0x44a8d9);bool same=false;if(bits(rate)!=0x3f800000){const auto& old=action(record(v,current,0x44a8e5),rt,0x44a8e5);const auto& next=action(selected,rt,0x44a8e8);const auto& next_object=known(next.object,0x44a8eb);const auto& old_object=known(old.object,0x44a8ed);same=next_object.id==old_object.id;}if(same){auto blended=blend_player_animation(v,rt,fp);if(!blended)return blended;v.tail.frame_at_130=0;}else v.tail.frame_at_130=1;v.player.current_animation_at_126=requested;v.player.animation_mode_at_120=2;v.tail.next_at_12a=static_cast<std::uint8_t>(known(selected.next,0x44a90c));return transition(selected,0x44a921);}
    const auto& selected=record(v,requested,mode==2?0x44a919:0x44a948);
    if(mode==2)return transition(selected,0x44a921);
    if(mode==3||mode==9||mode==10){auto step=known(selected.frame_step,0x44a955);auto increment=fp.from_float(step);if(mode==9)increment=fp.multiply_float_memory(fp.from_float(scalar(v,rt,0x138,0x44aa00)),step);if(mode==10)increment=fp.multiply_float_memory(fp.from_float(scalar(v,rt,0x13c,0x44aaa5)),step);v.tail.frame_at_130=fp.to_float(fp.add_float_memory(increment,known(v.tail.frame_at_130,0x44a958)));wrap_frame(v,rt,selected,0x44a975,fp,result);result.completed=true;return result;}
    if(mode==12){wrap_frame(v,rt,selected,0x44ad78,fp,result);result.completed=true;return result;}
    X87Value advanced=fp.from_float(known(v.tail.frame_at_130,0x44ab63));if(mode==6||mode==7)advanced=fp.subtract_float_memory(advanced,known(selected.frame_step,0x44abce));else if(mode==11)advanced=fp.add_float_memory(fp.from_float(scalar(v,rt,0x13c,0x44ab52)),*v.tail.frame_at_130);else advanced=fp.add_float_memory(fp.from_float(known(selected.frame_step,0x44a955)),*v.tail.frame_at_130);v.tail.frame_at_130=fp.to_float(advanced);
    if(mode==6||mode==7){auto c=fp.compare(fp.from_float(rt.constants.zero),advanced);if(c!=X87Comparison::greater)return complete();if(mode==6)return next_player_animation(v,rt,requested,fp);v.tail.frame_at_130=0;v.tail.flags_at_12e=1;return complete();}
    auto end=fp.subtract_float_memory(unsigned_frame(selected,rt,0x44ab6c,fp),rt.constants.one);std::optional<float> stored_end;if(mode==5)stored_end=fp.to_float(end);auto comparison=fp.compare(end,advanced);if(greater_or_unordered(comparison))return complete();
    if(mode==4||mode==11)return next_player_animation(v,rt,requested,fp);
    if(mode==5){v.tail.frame_at_130=*stored_end;v.tail.flags_at_12e=1;return complete();}
    if(mode==8){v.tail.timer_at_12c=0;auto rate=known(selected.transition_step,0x44ac42);bool same=false;std::uint16_t next=0;if(bits(rate)!=0x3f800000){next=known(v.tail.next_at_12a,0x44ac4b);const auto& a=action(selected,rt,0x44ac4f);const auto& b=action(record(v,next,0x44ac59),rt,0x44ac59);same=known(a.object,0x44ac51).id==known(b.object,0x44ac5c).id;}if(same){selected_index=next;v.player.requested_animation_at_124=static_cast<std::uint8_t>(next);auto blended=blend_player_animation(v,rt,fp);if(!blended)return blended;v.tail.frame_at_130=0;}else v.tail.frame_at_130=1;const auto& now=record(v,selected_index,0x44ac7d);v.player.current_animation_at_126=selected_index;v.tail.next_at_12a=static_cast<std::uint8_t>(known(now.next,0x44ac8c));v.player.animation_mode_at_120=2;return transition(now,0x44ac9a);}
    return complete();
}catch(const Stop& s){auto r=s.result;r.wrap_iterations=result.wrap_iterations;return r;}}
} // namespace sadx

#include "player/player_auxiliary_effect.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>

namespace sadx {
namespace {
std::uint32_t word(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
PlayerAuxiliaryEffectResult fail(PlayerAuxiliaryEffectFailure f,std::uint32_t source,std::uint32_t nested=0){PlayerAuxiliaryEffectResult r;r.failure=f;r.source_boundary=source;r.nested_source_boundary=nested;return r;}
PlayerAuxiliaryEffectResult work_fail(const TaskWorkResult& w,std::uint32_t source){auto r=fail(PlayerAuxiliaryEffectFailure::work_failed,source);r.work_failure=w.failure;r.pending_node=w.work;return r;}
PlayerAuxiliaryEffectResult fp_fail(const X87Error& e,std::uint32_t source){auto r=fail(PlayerAuxiliaryEffectFailure::fp_failed,source);r.fp_failure=e.failure();r.fp_exceptions=e.exceptions();return r;}
PlayerAuxiliaryEffectResult bind_auxiliary_effect_work(TaskScheduler& s,const TaskHandle& task,std::uint32_t read_site,std::uint32_t null_site){
    if(!task)return fail(PlayerAuxiliaryEffectFailure::missing_task,read_site);
    if(!task->primary_work)return fail(PlayerAuxiliaryEffectFailure::missing_primary,null_site);
    if(task->primary_state)return fail(PlayerAuxiliaryEffectFailure::conflicting_view,read_site);
    if(!s.work_arena())return fail(PlayerAuxiliaryEffectFailure::missing_arena,null_site);
    if(task->auxiliary_effect_work && task->auxiliary_effect_work->allocation!=task->primary_work)return fail(PlayerAuxiliaryEffectFailure::conflicting_view,read_site);
    if(!task->auxiliary_effect_work){task->auxiliary_effect_work=std::make_shared<PlayerAuxiliaryEffectTaskWork>();task->auxiliary_effect_work->allocation=task->primary_work;}
    return {};
}
std::shared_ptr<PlayerAuxiliaryEffectRuntime> context(TaskScheduler& s){return std::dynamic_pointer_cast<PlayerAuxiliaryEffectRuntime>(s.callback_context(&player_auxiliary_effect_initialize_callback));}
void propagate(const TaskHandle& task,PlayerAuxiliaryEffectResult result){
    if(result)return;
    TaskResult failure{TaskFailure::callback_failed,task,result.work_failure,result.source_boundary,std::make_shared<PlayerAuxiliaryEffectDiagnostic>(std::move(result))};
    TaskScheduler::raise_callback_failure(std::move(failure));
}
PlayerAuxiliaryEffectResult character(PlayerAuxiliaryEffectRuntime& runtime){
    if(!runtime.players)return fail(PlayerAuxiliaryEffectFailure::missing_context,0x4a2b21,0x441975);
    const auto& entry=runtime.players->task_work[0];
    if(!entry)return fail(PlayerAuxiliaryEffectFailure::unknown_registration,0x4a2b21,0x441975);
    PlayerAuxiliaryEffectResult r;
    if(*entry)r.character=(*entry)->character_id;
    else if(runtime.current_character_at_3b22dc0&&*runtime.current_character_at_3b22dc0)r.character=**runtime.current_character_at_3b22dc0;
    else return fail(PlayerAuxiliaryEffectFailure::unknown_current_character,0x4a2b21,0x4144e0);
    return r;
}
}

std::uint32_t PlayerAuxiliaryEffectAssetTable::character(std::size_t group)const {
    if(group>=5)throw std::out_of_range("Auxiliary-effect asset group outside actual source table");
    return word(bytes.data()+group*216);
}
std::shared_ptr<const PlayerAuxiliaryEffectAssetTable> decode_player_auxiliary_effect_table(const std::uint8_t* data,std::size_t size){
    if(!data||size!=0x438)return {};
    auto table=std::make_shared<PlayerAuxiliaryEffectAssetTable>();std::memcpy(table->bytes.data(),data,size);return table;
}
TaskCallbackContextResult install_player_auxiliary_effect_context(TaskScheduler& s,const std::shared_ptr<PlayerAuxiliaryEffectRuntime>& runtime){
    if(!runtime)return {TaskCallbackContextFailure::null_context};
    const TaskCallback callbacks[]={&player_auxiliary_effect_initialize_callback,&player_auxiliary_effect_update_callback,&player_auxiliary_effect_display_callback,&player_auxiliary_effect_destroy_callback};
    for(auto callback:callbacks){const auto old=s.callback_context(callback);if(old && old!=runtime)return {TaskCallbackContextFailure::conflicting_owner};}
    for(auto callback:callbacks){const auto installed=s.register_callback_context(callback,runtime);if(!installed)return installed;}
    return {};
}

PlayerAuxiliaryEffectResult initialize_player_auxiliary_effect(TaskScheduler& s,const TaskHandle& task,PlayerAuxiliaryEffectRuntime& runtime){
    if(!task)return fail(PlayerAuxiliaryEffectFailure::missing_task,0x4a2ae2);
    if(task->primary_state)return fail(PlayerAuxiliaryEffectFailure::conflicting_view,0x4a2ae2);
    // Source stores callbacks before taskwork initialization. All are real
    // native owners; their missing dependencies return tagged pending results.
    task->display=&player_auxiliary_effect_display_callback;
    task->exec=&player_auxiliary_effect_update_callback;
    task->destructor=&player_auxiliary_effect_destroy_callback;
    const auto accepted=bind_auxiliary_effect_work(s,task,0x4a2ae2,0x4a2afd);if(!accepted)return accepted;
    auto& arena=*s.work_arena();const auto& work=task->primary_work;
    auto stored=arena.set_reference(work,0x10,{});if(!stored)return work_fail(stored,0x4a2afd);
    stored=arena.fill(work,1,0,1);if(!stored)return work_fail(stored,0x4a2b00);
    stored=arena.fill(work,6,0,2);if(!stored)return work_fail(stored,0x4a2b03);
    // Actual4f88a0 XOR EAX,EAX;RET is a proven zero-return body.
    stored=arena.fill(work,3,0,1);if(!stored)return work_fail(stored,0x4a2b17);
    stored=arena.fill(work,8,0,4);if(!stored)return work_fail(stored,0x4a2b1a);
    task->auxiliary_effect_work->selection_at_8=std::shared_ptr<const PlayerAuxiliaryEffectSelection>{};
    PlayerAuxiliaryEffectResult result;
    for(std::size_t group=0;group<5;++group){
        auto queried=character(runtime);if(!queried)return queried;
        result.character=queried.character;
        if(!runtime.table)return fail(PlayerAuxiliaryEffectFailure::missing_table,0x4a2b26);
        if(*queried.character!=runtime.table->character(group))continue;
        auto selection=std::make_shared<PlayerAuxiliaryEffectSelection>();selection->table=runtime.table;selection->group=group;
        task->auxiliary_effect_work->selection_at_8=std::move(selection);result.selected_group=group;return result;
    }
    return result;
}

PlayerAuxiliaryEffectResult destroy_player_auxiliary_effect(TaskScheduler& s,const TaskHandle& task){
    const auto accepted=bind_auxiliary_effect_work(s,task,0x4a0725,0x4a0728);if(!accepted)return accepted;
    auto& arena=*s.work_arena();const auto& work=task->primary_work;
    for(;;){
        const auto head=arena.reference(work,0x10);if(!head)return work_fail(head,0x4a0728);
        if(!head.work)return {};
        const auto previous=arena.reference(head.work,0x2c);if(!previous)return work_fail(previous,0x4a0733);
        const auto next=arena.reference(head.work,0x30);if(!next)return work_fail(next,0x4a0738);
        auto stored=previous.work?arena.set_reference(previous.work,0x30,next.work):arena.set_reference(work,0x10,next.work);
        if(!stored)return work_fail(stored,previous.work?0x4a073d:0x4a0742);
        if(next.work){stored=arena.set_reference(next.work,0x2c,previous.work);if(!stored)return work_fail(stored,0x4a0749);}
        const auto released=arena.release(head.work);if(!released)return work_fail(released,0x4a074d);
        // Head is reloaded AFTER release, as in the original. Native allocation
        // generations prevent freed-node memory from becoming a host pointer.
    }
}

PlayerAuxiliaryEffectResult display_player_auxiliary_effect(TaskScheduler& s,const TaskHandle& task,PlayerAuxiliaryEffectRuntime& runtime){
    if(!task)return fail(PlayerAuxiliaryEffectFailure::missing_task,0x4a0768);
    if(!runtime.display_switch_at_3b1117c||!*runtime.display_switch_at_3b1117c)return fail(PlayerAuxiliaryEffectFailure::unknown_display_switch,0x4a076b);
    if(**runtime.display_switch_at_3b1117c>0)return {};
    if(!runtime.render_state_4030d0)return fail(PlayerAuxiliaryEffectFailure::missing_render_state,0x4a0788,0x4030d0);
    auto state=runtime.render_state_4030d0(0,8);if(!state.available)return fail(PlayerAuxiliaryEffectFailure::render_state_failed,0x4a0788,state.source_boundary);
    state=runtime.render_state_4030d0(1,10);if(!state.available)return fail(PlayerAuxiliaryEffectFailure::render_state_failed,0x4a0791,state.source_boundary);
    const auto accepted=bind_auxiliary_effect_work(s,task,0x4a0768,0x4a0796);if(!accepted)return accepted;
    auto& arena=*s.work_arena();auto node=arena.reference(task->primary_work,0x10);if(!node)return work_fail(node,0x4a0796);
    while(node.work){
        std::array<std::uint8_t,12> bytes{};
        auto read=arena.read(node.work,0x1c,bytes.data(),2);if(!read)return work_fail(read,0x4a07a0);
        const auto a=static_cast<std::uint16_t>(bytes[0]|(std::uint16_t(bytes[1])<<8));
        PlayerAuxiliaryEffectSprite packet{};packet.angle=a<0x8000?a:std::int32_t(a)-0x10000;
        read=arena.read(node.work,4,&packet.position,12);if(!read)return work_fail(read,0x4a07ab);
        read=arena.read(node.work,0x24,&packet.scale_x,4);if(!read)return work_fail(read,0x4a07bf);
        if(!runtime.fp)return fail(PlayerAuxiliaryEffectFailure::missing_fp,0x4a07c8);
        float factor;read=arena.read(node.work,0x20,&factor,4);if(!read)return work_fail(read,0x4a07c8);
        auto operation_site=std::uint32_t{0x4a07c8};
        try {
            const auto loaded=runtime.fp->from_float(factor);operation_site=0x4a07cf;
            const auto product=runtime.fp->multiply_float_memory(loaded,packet.scale_x);operation_site=0x4a07d5;
            packet.scale_y=runtime.fp->to_float(product);
        }catch(const X87Error& e){auto r=fp_fail(e,operation_site);r.pending_node=node.work;return r;}
        // Actual4072f0 ignores every nonzero gate, whereas the display's entry
        // gate ignores only positive values. Negative still mutates blend state.
        if(!runtime.display_switch_at_3b1117c||!*runtime.display_switch_at_3b1117c){auto r=fail(PlayerAuxiliaryEffectFailure::unknown_display_switch,0x4a07d9,0x4072f0);r.pending_sprite=packet;r.pending_node=node.work;return r;}
        if(**runtime.display_switch_at_3b1117c==0){
            if(!runtime.sprite_4072f0){auto r=fail(PlayerAuxiliaryEffectFailure::missing_sprite_owner,0x4a07d9,0x4072f0);r.pending_sprite=packet;r.pending_node=node.work;return r;}
            const auto draw=runtime.sprite_4072f0(packet,0,0x31);
            if(!draw.available){auto r=fail(PlayerAuxiliaryEffectFailure::sprite_failed,0x4a07d9,draw.source_boundary);r.pending_sprite=packet;r.pending_node=node.work;return r;}
        }
        node=arena.reference(node.work,0x30);if(!node)return work_fail(node,0x4a07de);
    }
    return {};
}

PlayerAuxiliaryEffectResult update_player_auxiliary_effect(TaskScheduler& s,const TaskHandle& task,PlayerAuxiliaryEffectRuntime& runtime){
    const auto accepted=bind_auxiliary_effect_work(s,task,0x4a257f,0x4a25aa);if(!accepted)return accepted;
    // Real4f88a0 returns0. Source next loads source scale.z and calls actual
    // CRT_ftol before signed-byte indexing of the registered primary table.
    if(!runtime.fp)return fail(PlayerAuxiliaryEffectFailure::missing_fp,0x4a25aa);
    float index_value;auto read=s.work_arena()->read(task->primary_work,0x34,&index_value,4);if(!read)return work_fail(read,0x4a25aa);
    std::uint32_t index_bits;
    try {index_bits=crt_ftol_low32(runtime.fp->from_float(index_value),*runtime.fp)&0xffU;}
    catch(const X87Error& e){return fp_fail(e,0x4a25aa);}
    const auto index=index_bits<128?static_cast<int>(index_bits):static_cast<int>(index_bits)-256;
    if(!runtime.players)return fail(PlayerAuxiliaryEffectFailure::missing_context,0x4a25b5);
    if(index<0||index>=8)return fail(PlayerAuxiliaryEffectFailure::unknown_registration,0x4a25b5);
    const auto& registered=runtime.players->task_work[static_cast<std::size_t>(index)];
    if(!registered)return fail(PlayerAuxiliaryEffectFailure::unknown_registration,0x4a25b5);
    bool retire=!*registered;
    if(!retire){
        std::uint8_t variant;read=s.work_arena()->read(task->primary_work,3,&variant,1);if(!read)return work_fail(read,0x4a25c4);
        if(!task->auxiliary_effect_work->selection_at_8)return fail(PlayerAuxiliaryEffectFailure::unknown_selection,0x4a25cb);
        if(!*task->auxiliary_effect_work->selection_at_8){
            if(variant)return fail(PlayerAuxiliaryEffectFailure::unsupported_variant_pointer,0x4a25ce);
            retire=true;
        }
    }
    if(retire){const auto scheduled=s.free_task(task);if(!scheduled){auto r=fail(PlayerAuxiliaryEffectFailure::work_failed,0x4a29f0,scheduled.source_boundary);r.work_failure=scheduled.work_failure;return r;}return {};}
    const auto& player=**registered;
    if(!player.secondary_mode_at_1)return fail(PlayerAuxiliaryEffectFailure::missing_secondary_mode,0x4a25d9);
    std::uint8_t secondary;read=s.work_arena()->read(task->primary_work,1,&secondary,1);if(!read)return work_fail(read,0x4a25df);
    const std::uint8_t variant=*player.secondary_mode_at_1==0x27?static_cast<std::uint8_t>((secondary==0?1:0)+1):0;
    const auto stored=s.work_arena()->write(task->primary_work,3,&variant,1);if(!stored)return work_fail(stored,0x4a25e9);
    // Real source prefix has committed variant before these required calls.
    // No host rand, false probability, fake130 field or geometry is supplied.
    if(secondary){
        if(variant)return fail(PlayerAuxiliaryEffectFailure::missing_update_owner,0x4a262c,0x4a04c0);
        return fail(PlayerAuxiliaryEffectFailure::missing_update_owner,0x4a2600,0x6443bf);
    }
    if(variant)return fail(PlayerAuxiliaryEffectFailure::missing_update_owner,0x4a264d,0x6443bf);
    return fail(PlayerAuxiliaryEffectFailure::missing_update_owner,0x4a2687,0x441b00);
}

void player_auxiliary_effect_initialize_callback(TaskScheduler& s,const TaskHandle& task){auto owner=context(s);PlayerAuxiliaryEffectRuntime unknown;propagate(task,initialize_player_auxiliary_effect(s,task,owner?*owner:unknown));}
void player_auxiliary_effect_update_callback(TaskScheduler& s,const TaskHandle& task){auto owner=context(s);PlayerAuxiliaryEffectRuntime unknown;propagate(task,update_player_auxiliary_effect(s,task,owner?*owner:unknown));}
void player_auxiliary_effect_display_callback(TaskScheduler& s,const TaskHandle& task){auto owner=context(s);PlayerAuxiliaryEffectRuntime unknown;propagate(task,display_player_auxiliary_effect(s,task,owner?*owner:unknown));}
void player_auxiliary_effect_destroy_callback(TaskScheduler& s,const TaskHandle& task){propagate(task,destroy_player_auxiliary_effect(s,task));}

} // namespace sadx

#include "player/player_hold.hpp"
namespace sadx {
namespace {
PlayerHoldResult failure(PlayerHoldFailure why,std::uint32_t at,
                         TaskWorkFailure work=TaskWorkFailure::none){return {why,at,work,{}};}
PlayerHoldResult returned(std::uint32_t flags){return {PlayerHoldFailure::none,0,TaskWorkFailure::none,flags};}
PlayerHoldResult actor_flags(std::uint8_t character,std::uint8_t actor){
    // Exact character map bytes at43bb10/43bb24. Branches1,4 and >7 return
    // the initial EAX=0, regardless of actor category.
    constexpr std::uint8_t classes[8]={0,2,0,0,2,0,0,1};
    if(character>7 || classes[character]==2)return returned(0);
    if(actor==4)return returned(0x200);
    if(actor==5)return returned(classes[character]==1?0x600:0x400);
    if(actor==6)return returned(classes[character]==1?0x600:0);
    return returned(actor==7?0x800:0);
}
}
PlayerHoldResult query_player_hold(const PlayerTaskState& task,
        const PlayerPositionCollisionScene& scene,const PlayerHoldRuntime& runtime){
    if(task.orientation.flags&0x800U){
        if(!runtime.player_lookup)return failure(PlayerHoldFailure::missing_player_lookup,0x43b9e3);
        const auto player=runtime.player_lookup(task.player_index);
        if(!player)return failure(PlayerHoldFailure::unknown_player_reference,0x43b9e3);
        if(!*player)return failure(PlayerHoldFailure::null_required_player_work,0x43b9ea);
        if(!(*player)->held_task_at_68)return failure(PlayerHoldFailure::unknown_held_task,0x43b9ea);
        const auto& held=*(*player)->held_task_at_68;
        if(!held)return returned(0);
        if(held->exec==&TaskScheduler::destroy_callback || !held->exec)return returned(0);
        if(!held->motion_work)return returned(0);
        if(!held->primary_work)return returned(0);
        if(!runtime.work_arena)return failure(PlayerHoldFailure::missing_work_arena,0x43ba22);
        const auto collision=runtime.work_arena->reference(held->primary_work,0x38);
        if(!collision)return failure(PlayerHoldFailure::work_read_failed,0x43ba22,collision.failure);
        if(!collision.work)return returned(0);
        // Source reads actor ID before its out-of-range character branch.
        std::uint8_t actor;
        if(held->primary_state){
            if(!held->primary_state->task_id_at_2)return failure(PlayerHoldFailure::unknown_actor_id,0x43ba34);
            actor=*held->primary_state->task_id_at_2;
        }else{
            const auto read=runtime.work_arena->read(held->primary_work,2,&actor,1);
            if(!read)return failure(PlayerHoldFailure::work_read_failed,0x43ba34,read.failure);
        }
        return actor_flags(task.character_id,actor);
    }
    if(!scene.task_collision)return failure(PlayerHoldFailure::unknown_collision_reference,0x43ba61);
    if(!scene.task_collision->id)return failure(PlayerHoldFailure::null_required_collision,0x43ba64);
    const auto own=scene.works.find(*scene.task_collision->id);
    if(own==scene.works.end())return failure(PlayerHoldFailure::unresolved_collision,0x43ba64);
    if(!own->second.flags)return failure(PlayerHoldFailure::unknown_collision_flags,0x43ba64);
    const auto flags=*own->second.flags;
    if(!(flags&1) || (task.orientation.flags&4) || !(flags&4))return returned(0);
    if(!own->second.hit_work)return failure(PlayerHoldFailure::unknown_hit_reference,0x43ba7f);
    if(!own->second.hit_work->id)return returned(0);
    const auto hit=scene.works.find(*own->second.hit_work->id);
    if(hit==scene.works.end())return failure(PlayerHoldFailure::unresolved_collision,0x43ba89);
    if(!hit->second.owning_task)return failure(PlayerHoldFailure::unknown_owning_task,0x43ba89);
    if(!hit->second.owning_task->id)return failure(PlayerHoldFailure::null_required_owning_task,0x43ba8f);
    const auto owner=scene.tasks.find(*hit->second.owning_task->id);
    if(owner==scene.tasks.end())return failure(PlayerHoldFailure::unresolved_owning_task,0x43ba8f);
    if(!owner->second.work)return failure(PlayerHoldFailure::unknown_actor_reference,0x43ba8f);
    if(!owner->second.work->id)return failure(PlayerHoldFailure::null_required_actor,0x43ba92);
    const auto actor=scene.actors.find(*owner->second.work->id);
    if(actor==scene.actors.end())return failure(PlayerHoldFailure::unresolved_actor,0x43ba92);
    const auto& state=actor->second;
    const auto low=position_actor_flag_byte(state,0);
    if(!low)return failure(PlayerHoldFailure::unknown_actor_flags,0x43ba92);
    if(!(*low&0x40U))return returned(0);
    const auto id=position_actor_task_id(state);
    if(!id)return failure(PlayerHoldFailure::unknown_actor_id,0x43ba98);
    const auto high=position_actor_flag_byte(state,1);
    if(!high)return failure(PlayerHoldFailure::unknown_actor_flags,0x43ba9b);
    if(*high&0x10U)return returned(0);
    return actor_flags(task.character_id,*id);
}
decltype(PlayerPositionCallees::hold) bind_player_hold(const PlayerHoldRuntime& runtime){
    return [&runtime](PlayerTaskState& task,PlayerPhysicsState&,
            PlayerPositionCollisionScene& scene,X87Context&){
        const auto result=std::make_shared<PlayerHoldResult>(query_player_hold(task,scene,runtime));
        return PositionHoldResult{*result?PlayerPositionFailure::none:PlayerPositionFailure::hold_owner_failed,
                                  result->flags,result->source_boundary,result};
    };
}
} // namespace sadx

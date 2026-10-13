#include "player/player_startup.hpp"

#include <cstring>

namespace sadx {
namespace {
std::uint32_t word(const std::uint8_t* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
           (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
std::int32_t signed_word(std::uint32_t w) {std::int32_t v;std::memcpy(&v,&w,4);return v;}
float float_word(std::uint32_t w) {float v;std::memcpy(&v,&w,4);return v;}
TaskWorkResult read_bytes(const std::shared_ptr<TaskWorkArena>& arena,
    const TaskWorkHandle& work,std::uint32_t offset,void* out,std::uint32_t count) {
    return arena ? arena->read(work,offset,out,count)
                 : TaskWorkResult{TaskWorkFailure::foreign_work,work};
}
TaskWorkResult allocate_zero(TaskWorkArena& arena,std::uint32_t count,std::uint32_t size) {
    // This owner supplies only the actual constant call arguments. The source
    // calloc-like helper40b2d0 rounds the per-element size to4 then multiplies.
    const auto bytes=((size+3)&~std::uint32_t{3})*count;
    auto result=arena.allocate(bytes);
    if(result)result=arena.fill(result.work,0,0,bytes);
    return result;
}
PlayerContactWork zero_contact() {
    PlayerContactWork result;const NinjaVector zero{0,0,0};
    result.direction_at_1c=zero;result.radius_at_0=0;result.center_at_4=zero;
    result.displacement_at_10=zero;result.angle_x_at_34=0;result.angle_z_at_38=0;
    result.contact_count_at_28=0;result.selected_contact_at_2a=0;
    result.vector_at_3c=zero;result.vector_at_48=zero;result.vector_at_54=zero;
    result.vector_at_60=zero;result.vector_at_6c=zero;result.vector_at_78=zero;
    return result;
}
struct EventStore {std::uint16_t index,mask;std::uint32_t call;};
const EventStore sonic[]={{0x8d,2,0x44281a},{0x8e,1,0x44282f},{0x9f,4,0x442844},
                         {0xcd,8,0x442859},{0xdb,16,0x44286e}};
const EventStore tails[]={{0xcd,8,0x442859},{0xdb,16,0x44286e}};
const EventStore knuckles[]={{0x119,0x40,0x44288c},{0x11a,0x20,0x4428a1}};
const EventStore amy[]={{0x152,0x100,0x4428bf},{0x15e,0x80,0x4428d4}};
const EventStore gamma[]={{0x189,0x400,0x4428eb},{0x18a,0x200,0x442900},
                         {0x1d0,0x1000,0x442915},{0x1d1,0x800,0x44292a}};
const EventStore big[]={{0x1d0,0x1000,0x442915},{0x1d1,0x800,0x44292a}};
}

TaskWorkResult bind_player_task_primary(TaskScheduler& scheduler,const TaskHandle& task) {
    if(!task)return {TaskWorkFailure::foreign_work,{}};
    if(!task->primary_work)return {TaskWorkFailure::foreign_work,{}};
    if(task->auxiliary_effect_work)return {TaskWorkFailure::conflicting_live_view,task->primary_work};
    if(task->primary_state)return {TaskWorkFailure::none,task->primary_work};
    const auto arena=scheduler.work_arena();
    std::array<std::uint8_t,12> bytes{};
    auto state=std::make_shared<PlayerTaskState>();
    const auto action=read_bytes(arena,task->primary_work,0,bytes.data(),1);
    if(action)state->action_at_0=bytes[0]; // omission stays unknown, never guessed
    const auto secondary=read_bytes(arena,task->primary_work,1,bytes.data(),1);
    if(secondary)state->secondary_mode_at_1=bytes[0];
    const auto timer=read_bytes(arena,task->primary_work,6,bytes.data(),2);
    if(timer)state->control_timer_at_6=static_cast<std::uint16_t>(bytes[0]|(std::uint16_t(bytes[1])<<8));
    const auto counter=read_bytes(arena,task->primary_work,0x0c,bytes.data(),4);
    if(counter)state->source_counter_at_c=signed_word(word(bytes.data()));
    auto result=read_bytes(arena,task->primary_work,2,bytes.data(),1);if(!result)return result;
    state->task_id_at_2=bytes[0];
    result=read_bytes(arena,task->primary_work,4,bytes.data(),2);if(!result)return result;
    state->orientation.flags=static_cast<std::uint16_t>(bytes[0]|(std::uint16_t(bytes[1])<<8));
    result=read_bytes(arena,task->primary_work,8,bytes.data(),2);if(!result)return result;
    state->player_index=bytes[0];state->character_id=bytes[1];
    result=read_bytes(arena,task->primary_work,0x14,bytes.data(),12);if(!result)return result;
    for(unsigned i=0;i<3;++i)state->orientation.angles[i]=signed_word(word(bytes.data()+i*4));
    result=read_bytes(arena,task->primary_work,0x20,bytes.data(),12);if(!result)return result;
    state->position=NinjaVector{float_word(word(bytes.data())),float_word(word(bytes.data()+4)),float_word(word(bytes.data()+8))};
    result=read_bytes(arena,task->primary_work,0x2c,bytes.data(),12);if(!result)return result;
    state->scale_at_2c=NinjaVector{float_word(word(bytes.data())),float_word(word(bytes.data()+4)),float_word(word(bytes.data()+8))};
    // A generic known-zero pointer can establish null. Nonzero or unknown
    // path/event data never becomes a guessed native resource or decoded event.
    const auto pointer=read_bytes(arena,task->primary_work,0x10,bytes.data(),4);
    if(pointer && word(bytes.data())==0)
        state->path_resource=std::shared_ptr<const PlayerPathResource>{};
    task->primary_state=std::move(state);return {TaskWorkFailure::none,task->primary_work};
}

std::optional<const PlayerPhysicsState*> PlayerStartupRegistry::lookup_player(std::uint8_t index) const {
    if(index>=observed_slots || !player_work[index])return {};
    const auto& record=*player_work[index];
    return record && record->player ? record->player.get() : nullptr;
}

std::shared_ptr<PlayerStartupPlayer> PlayerStartupRegistry::lookup_player_work(const TaskWorkHandle& work) const {
    if(!work || work->released())return {};
    const auto found=allocations.find(work.get());
    return found==allocations.end()?std::shared_ptr<PlayerStartupPlayer>{}:found->second;
}

PlayerStartupResult initialize_player_startup(std::int32_t index,const TaskHandle& task,
    std::shared_ptr<PlayerMotionState> motion,TaskScheduler& scheduler,
    PlayerStartupRegistry& registry,const PlayerStartupRuntime& runtime,X87Context& fp) {
    PlayerStartupResult result;result.partial=std::make_shared<PlayerStartupPlayer>();
    auto& record=*result.partial;record.task=task;record.motion=std::move(motion);
    auto fail=[&](PlayerStartupFailure failure,std::uint32_t source,std::uint32_t callee=0){
        result.failure=failure;result.source_boundary=source;result.callee_boundary=callee;return result;
    };
    auto work_failed=[&](const TaskWorkResult& work,std::uint32_t source,std::uint32_t callee=0){
        result.work_failure=work.failure;return fail(PlayerStartupFailure::work_failure,source,callee);
    };
    if(!task)return fail(PlayerStartupFailure::missing_task,0x442756);
    const auto arena=scheduler.work_arena();if(!arena)return fail(PlayerStartupFailure::missing_arena,0x44276a,0x40b2d0);
    auto allocated=allocate_zero(*arena,1,0x200);
    record.player_work=allocated.work;
    if(!allocated)return work_failed(allocated,0x442781); // source null destination copy
    record.stage=PlayerStartupStage::player_allocated;
    record.player=std::make_shared<PlayerPhysicsState>();auto& player=*record.player;
    registry.allocations.emplace(record.player_work.get(),result.partial);
    player.floor_normal=NinjaVector{0,0,0};player.source_vector_at_44=NinjaVector{0,0,0};
    player.prior_attributes=0;player.path_distance=0;player.held_task_at_68=TaskHandle{};
    player.flags_at_10=0;player.no_control_time_at_a=0;
    player.source_flags_at_6=0;player.timer_at_7e=0;
    player.source_charge_at_0=0;player.jump_timer_at_8=0;player.idle_timer_at_e=0;
    player.timer_at_80=0;player.timer_at_82=0;
    player.requested_animation_at_124=0;player.current_animation_at_126=0;player.previous_animation_at_128=0;
    // The actual 0x200-byte calloc includes these mtnjvwk fields. Keep them
    // unknown in generic state; only this source producer establishes zero/null.
    player.animation_tail.next_at_12a=0;player.animation_tail.timer_at_12c=0;player.animation_tail.flags_at_12e=0;
    player.animation_tail.frame_at_130=0;player.animation_tail.start_at_134=0;
    player.animation_tail.table_at_140=std::shared_ptr<const PlayerAnimationTable>{};
    player.animation_tail.buffer_at_148=std::shared_ptr<PlayerAnimationAllocatedAction>{};
    player.moving_task_at_6c=TaskHandle{};player.cached_land_object_at_74=SceneResourceReference{};
    player.equipment_at_4=0;player.island_number_at_12=0;player.animation_mode_at_120=0;
    player.join_vertex_mode_at_122=0;player.shadow_scale_at_1fc=0;
    if(!task->primary_work)return fail(PlayerStartupFailure::missing_primary_work,0x44276f);
    const auto bound=bind_player_task_primary(scheduler,task);
    if(!bound)return work_failed(bound,0x44276f);
    auto& primary=*task->primary_state;
    if(primary.character_id>=8)return fail(PlayerStartupFailure::unsupported_character_row,0x44278c);
    if(!runtime.parameters)return fail(PlayerStartupFailure::missing_parameters,0x44278c);
    const auto& raw=(*runtime.parameters)[primary.character_id];
    const auto parsed=decode_player_parameters(raw.data(),raw.size());
    if(!parsed)return fail(PlayerStartupFailure::missing_parameters,0x44278c);
    const auto copied=arena->write(record.player_work,0x9c,raw.data(),raw.size());
    if(!copied)return work_failed(copied,0x44278c);
    record.parameters=*parsed.parameters;record.stage=PlayerStartupStage::parameters_copied;
    if(!task->motion_work)return fail(PlayerStartupFailure::missing_motion_work,0x442793);
    const auto motion_bound=arena->set_reference(task->motion_work,0,record.player_work);
    if(!motion_bound)return work_failed(motion_bound,0x442793);
    record.stage=PlayerStartupStage::motion_bound;
    auto object=arena->reference(task->primary_work,0x3c);
    if(!object)return work_failed(object,0x442795,0x417e85);
    if(!object.work) {
        object=allocate_zero(*arena,1,0x7c);
        const auto stored=arena->set_reference(task->primary_work,0x3c,object.work);
        if(!stored)return work_failed(stored,0x442795,0x417e95);
        if(!object)return work_failed(object,0x442795,0x417e9b);
        const std::uint8_t mode=3;const auto initialized=arena->write(object.work,0,&mode,1);
        if(!initialized)return work_failed(initialized,0x442795,0x417e9b);
    }
    record.stage=PlayerStartupStage::object_bound;
    auto contact=allocate_zero(*arena,1,0x84);record.contact_work=contact.work;
    const auto contact_stored=arena->set_reference(record.player_work,0x64,record.contact_work);
    if(!contact_stored)return work_failed(contact_stored,0x4427aa);
    if(contact.work)player.contact_work=zero_contact(); // a failed nullable allocation stays null
    record.stage=PlayerStartupStage::contact_bound;
    if(index<0 || static_cast<std::uint32_t>(index)>=PlayerStartupRegistry::observed_slots)
        return fail(PlayerStartupFailure::unsupported_registration_slot,0x4427b1);
    const auto slot=static_cast<std::size_t>(index);
    registry.tasks[slot]=task;record.stage=PlayerStartupStage::registration_task;
    registry.task_work[slot]=task->primary_state;record.stage=PlayerStartupStage::registration_primary;
    if(!record.motion)return fail(PlayerStartupFailure::missing_live_motion,0x4427bf);
    registry.motion_work[slot]=record.motion;registry.player_work[slot]=result.partial;
    record.stage=PlayerStartupStage::registered;
    const auto shadow=player_initial_shadow_scale(record.parameters->rad,fp);
    player.animation_mode_at_120=0;player.shadow_scale_at_1fc=shadow;player.join_vertex_mode_at_122=0;
    const auto speed_alias=arena->set_reference_view(record.player_work,0x138,record.player_work,0x38);
    if(!speed_alias)return work_failed(speed_alias,0x4427f2);
    const auto self_alias=arena->set_reference(record.player_work,0x13c,record.player_work);
    if(!self_alias)return work_failed(self_alias,0x4427f8);
    // Typed speed_at_138/self_at_13c expose the same authoritative live state;
    // validated allocation aliases are identity facts, not raw vector reads.
    record.stage=PlayerStartupStage::shadow_stored;
    const EventStore* events=nullptr;std::size_t event_count=0;
    switch(primary.character_id) {
        case 0:events=sonic;event_count=5;break;case 2:events=tails;event_count=2;break;
        case 3:events=knuckles;event_count=2;break;case 5:events=amy;event_count=2;break;
        case 6:events=gamma;event_count=4;break;case 7:events=big;event_count=2;break;
        default:break;
    }
    record.stage=PlayerStartupStage::event_flags;
    for(std::size_t i=0;i<event_count;++i) {
        if(!runtime.events)return fail(PlayerStartupFailure::missing_events,events[i].call,0x412d20);
        if((*runtime.events)[events[i].index])
            player.equipment_at_4=static_cast<std::uint16_t>(*player.equipment_at_4|events[i].mask);
    }
    player.motion.speed={0,0,0};record.stage=PlayerStartupStage::speed_cleared;
    if(!record.motion)return fail(PlayerStartupFailure::missing_live_motion,0x442949);
    record.motion->friction_scale=1;
    record.stage=PlayerStartupStage::task_id;
    if(!primary.task_id_at_2)return fail(PlayerStartupFailure::missing_task_id,0x44294c);
    if(*primary.task_id_at_2==0)primary.task_id_at_2=1;
    auto extra=allocate_zero(*arena,15,0x20);record.auxiliary_work=extra.work;
    const auto extra_stored=arena->set_reference(record.player_work,0x78,record.auxiliary_work);
    if(!extra_stored)return work_failed(extra_stored,0x442965);
    player.island_number_at_12=0;record.stage=PlayerStartupStage::auxiliary_work;
    primary.scale_at_2c=NinjaVector{1,1,1};record.stage=PlayerStartupStage::task_scale;
    if(!runtime.auxiliary_effect_initialize_4a2b60)return fail(PlayerStartupFailure::missing_auxiliary_effect_callback,0x44297b,0x4a2b60);
    auto created=scheduler.create_element(2,2,runtime.auxiliary_effect_initialize_4a2b60);
    record.auxiliary_task=created.task;
    if(!created){result.task_failure=created;return fail(PlayerStartupFailure::task_creation_failed,0x44297b,created.source_boundary);}
    record.stage=PlayerStartupStage::auxiliary_task;
    // The auxiliary callback uses a different taskwk union: +8 becomes a
    // table pointer in4a2ae0, not player_index/character_id. Keep its work in
    // the generic native byte owner until its real shadow view is recovered.
    const float encoded_index=fp.to_float(fp.from_int64(index));
    const auto stored_index=arena->write(record.auxiliary_task->primary_work,0x34,&encoded_index,4);
    if(!stored_index)return work_failed(stored_index,0x44298b);
    record.stage=PlayerStartupStage::complete;return result;
}

} // namespace sadx

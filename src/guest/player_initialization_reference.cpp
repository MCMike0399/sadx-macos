#include "player/player_startup.hpp"
#include "player/player_initialization.hpp"

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

namespace {
namespace native = player_initialization;
struct ReferenceAccess final : native::Access {
    TaskScheduler &scheduler;
    const PlayerStartupRuntime &runtime;
    std::array<PlayerParameters,8> decoded{};
    ReferenceAccess(TaskScheduler &scheduler, const PlayerStartupRuntime &runtime)
        : scheduler(scheduler), runtime(runtime) {}
    std::shared_ptr<PlayerTaskState> primary(const TaskHandle &task) override {
        const auto bound=bind_player_task_primary(scheduler,task);
        if(!bound)throw native::Error(native::Boundary::Primary,bound.failure);
        return task->primary_state;
    }
    const PlayerParameters &parameters(std::uint8_t character) override {
        if(character>=8)throw native::Error(native::Boundary::ParameterRow);
        if(!runtime.parameters)throw native::Error(native::Boundary::Parameters);
        const auto &row=(*runtime.parameters)[character];
        const auto result=decode_player_parameters(row.data(),row.size());
        if(!result)throw native::Error(native::Boundary::Parameters);
        decoded[character]=*result.parameters;
        return decoded[character];
    }
    bool event_flag(std::uint16_t index) override {
        if(!runtime.events)throw native::Error(native::Boundary::Events);
        return (*runtime.events)[index]!=0;
    }
    std::shared_ptr<PlayerMotionState> live_motion(
        const TaskHandle &, const std::shared_ptr<PlayerMotionState> &candidate) override {
        if(!candidate)throw native::Error(native::Boundary::LiveMotion);
        return candidate;
    }
    std::uint8_t &task_id(const TaskHandle &task) override {
        if(!task->primary_state->task_id_at_2)throw native::Error(native::Boundary::TaskId);
        return *task->primary_state->task_id_at_2;
    }
    void store_scale(const TaskHandle &task,NinjaVector value) override {task->primary_state->scale_at_2c=value;}
    TaskCallback auxiliary_initializer() override {
        if(!runtime.auxiliary_effect_initialize_4a2b60)throw native::Error(native::Boundary::AuxiliaryCallback);
        return runtime.auxiliary_effect_initialize_4a2b60;
    }
};
std::shared_ptr<PlayerStartupPlayer> legacy(const native::PlayerHandle &source) {
    if(!source)return {};
    auto record=std::make_shared<PlayerStartupPlayer>();
    record->task=source->task;record->player_work=source->allocation;record->contact_work=source->contact;
    record->auxiliary_work=source->auxiliary;record->motion=source->motion;record->player=source->physics;
    if(source->stage>=native::Stage::ParametersCopied)record->parameters=source->parameters;
    record->auxiliary_task=source->auxiliary_task;record->stage=static_cast<PlayerStartupStage>(source->stage);
    return record;
}
}
PlayerStartupResult initialize_player_startup(std::int32_t index,const TaskHandle &task,
    std::shared_ptr<PlayerMotionState> motion,TaskScheduler &scheduler,
    PlayerStartupRegistry &registry,const PlayerStartupRuntime &runtime,X87Context &fp) {
    native::Registry state;
    ReferenceAccess access(scheduler,runtime);
    native::PlayerHandle record;
    PlayerStartupResult result;
    try {native::initialize(index,task,std::move(motion),scheduler,state,access,fp,record);}
    catch(const native::Error &error) {
        using B=native::Boundary; using F=PlayerStartupFailure;
        result.work_failure=error.work_failure;
        switch(error.boundary) {
        case B::Task:result.failure=F::missing_task;result.source_boundary=0x442756;break;
        case B::Arena:result.failure=F::missing_arena;result.source_boundary=0x44276a;result.callee_boundary=0x40b2d0;break;
        case B::Primary:result.failure=error.work_failure==TaskWorkFailure::none?F::missing_primary_work:F::work_failure;result.source_boundary=0x44276f;break;
        case B::PlayerAllocation:result.failure=F::work_failure;result.source_boundary=0x442781;break;
        case B::ParameterRow:result.failure=F::unsupported_character_row;result.source_boundary=0x44278c;break;
        case B::Parameters:result.failure=F::missing_parameters;result.source_boundary=0x44278c;break;
        case B::MotionWork:result.failure=F::missing_motion_work;result.source_boundary=0x442793;break;
        case B::LiveMotion:result.failure=F::missing_live_motion;result.source_boundary=record->stage==native::Stage::SpeedCleared?0x442949:0x4427bf;break;
        case B::Events: {
            result.failure=F::missing_events;result.callee_boundary=0x412d20;
            const std::uint32_t first[]={0x44281a,0,0x442859,0x44288c,0,0x4428bf,0x4428eb,0x442915};
            result.source_boundary=first[task->primary_state->character_id];break;
        }
        case B::RegistrationSlot:result.failure=F::unsupported_registration_slot;result.source_boundary=0x4427b1;break;
        case B::TaskId:result.failure=F::missing_task_id;result.source_boundary=0x44294c;break;
        case B::AuxiliaryCallback:result.failure=F::missing_auxiliary_effect_callback;result.source_boundary=0x44297b;result.callee_boundary=0x4a2b60;break;
        case B::TaskCreation:result.failure=F::task_creation_failed;result.source_boundary=0x44297b;result.callee_boundary=error.task_failure.source_boundary;result.task_failure=error.task_failure;break;
        default: {
            result.failure=F::work_failure;
            switch(error.boundary) {
            case B::ObjectReference:result.source_boundary=0x442795;result.callee_boundary=0x417e85;break;
            case B::ObjectStore:result.source_boundary=0x442795;result.callee_boundary=0x417e95;break;
            case B::ObjectInitialization:result.source_boundary=0x442795;result.callee_boundary=0x417e9b;break;
            case B::ParameterStore:result.source_boundary=0x44278c;break;
            case B::MotionBinding:result.source_boundary=0x442793;break;
            case B::ContactBinding:result.source_boundary=0x4427aa;break;
            case B::SpeedAlias:result.source_boundary=0x4427f2;break;
            case B::SelfAlias:result.source_boundary=0x4427f8;break;
            case B::AuxiliaryBinding:result.source_boundary=0x442965;break;
            case B::AuxiliaryIndex:result.source_boundary=0x44298b;break;
            default:throw;
            }
        }
        }
    }
    result.partial=legacy(record);
    if(record&&record->allocation)registry.allocations[record->allocation.get()]=result.partial;
    for(std::size_t slot=0;slot<state.tasks.size();++slot) {
        if(state.tasks[slot])registry.tasks[slot]=state.tasks[slot];
        if(state.primary[slot])registry.task_work[slot]=state.primary[slot];
        if(state.motion[slot])registry.motion_work[slot]=state.motion[slot];
        if(state.players[slot])registry.player_work[slot]=result.partial;
    }
    return result;
}
} // namespace sadx

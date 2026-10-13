#include "game/game_scene_logic.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Visit {int index;std::vector<std::uint32_t>* trace;};
void task_visit(sadx::TaskScheduler&,const sadx::TaskHandle& task){auto v=std::static_pointer_cast<Visit>(task->field30);v->trace->push_back(0x80000000U|unsigned(v->index));}
void test(){
    std::optional<std::uint16_t> stage=15;std::optional<std::int32_t> mode=9;sadx::GameSceneLogicState state{stage,&mode,7,8};
    sadx::TaskScheduler scheduler;sadx::X87Context fp(0x7f);std::vector<std::uint32_t> calls;
    for(int i=0;i<8;++i){auto result=scheduler.append_task(i,&task_visit,std::make_shared<Visit>(Visit{i,&calls}));require(bool(result),"Fixture task creation");}
    sadx::GameSceneLogicRuntime runtime{state,scheduler,fp,{}};
    runtime.invoke=[&](const sadx::GameSceneCall& call,sadx::X87Context& arithmetic){
        require(&arithmetic==&fp,"All scene owners share the real caller arithmetic context");calls.push_back(call.callee);
        if(call.operation==sadx::GameSceneOperation::conditional_426070)stage=35;
        return sadx::GameSceneOwnerResult{{sadx::GameLogicOwnerState::completed,0},call.operation==sadx::GameSceneOperation::query_42fb00?std::optional<std::int32_t>{0}:std::optional<std::int32_t>{}};
    };
    auto result=sadx::run_game_scene_logic(runtime);
    require(bool(result)&&result.tasks_completed&&state.byte_at_3b0ef40==0&&state.byte_at_3b0f150==8,"Successful source branch reloads stage after conditional owner");
    require(calls.size()==15&&calls[2]==0x80000007U&&calls[3]==0x80000000U&&calls[9]==0x80000006U&&calls[10]==0x420700&&calls[12]==0x4511d0&&calls[14]==0x426070,"Actual task-list ordering precedes collision/registry owners");
    mode=9;stage=25;runtime.invoke=[](const sadx::GameSceneCall& call,sadx::X87Context&){return sadx::GameSceneOwnerResult{{sadx::GameLogicOwnerState::completed,0},call.operation==sadx::GameSceneOperation::query_42fb00?std::optional<std::int32_t>{1}:std::optional<std::int32_t>{}};};
    result=sadx::run_game_scene_logic(runtime);require(bool(result)&&state.byte_at_3b0ef40==0xff&&state.byte_at_3b0f150==0xff,"Nonzero query and signed stage branch set source bytes");
    runtime.invoke=[](const sadx::GameSceneCall&,sadx::X87Context&){return sadx::GameSceneOwnerResult{{sadx::GameLogicOwnerState::completed,0},{}};};
    state.byte_at_3b0ef40=7;result=sadx::run_game_scene_logic(runtime);
    require(result.failure==sadx::GameSceneLogicFailure::missing_return&&result.source_boundary==0x413d71&&state.byte_at_3b0ef40==7,"Absent query return never selects a zero branch");
    runtime.invoke={};result=sadx::run_game_scene_logic(runtime);
    require(result.failure==sadx::GameSceneLogicFailure::missing_owner&&result.source_boundary==0x413d40&&!result.tasks_completed,"Missing input owner stops before any task dispatch");
    runtime.invoke=[](const sadx::GameSceneCall&,sadx::X87Context&){return sadx::GameSceneOwnerResult{{sadx::GameLogicOwnerState::failed,0x123456},{}};};
    result=sadx::run_game_scene_logic(runtime);require(result.failure==sadx::GameSceneLogicFailure::owner_failed&&result.nested_source_boundary==0x123456&&result.pending_call.has_value(),"Nested source failure retained");
    std::cout<<"Scene logic: real task ordering, shared arithmetic, stage reloads and required-owner failures passed.\n";
}
}
int main(){try{test();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include "game/game_scene_logic.hpp"
#include <cstring>
namespace sadx {
GameSceneLogicResult run_game_scene_logic(GameSceneLogicRuntime& runtime){
    GameSceneLogicResult result;auto& state=runtime.state;std::optional<std::int32_t> query;
    const auto fail=[&](GameSceneLogicFailure why,std::uint32_t boundary){result.failure=why;result.source_boundary=boundary;return false;};
    const auto call=[&](GameSceneOperation operation,std::uint32_t boundary,std::uint32_t callee,bool needs_return=false){
        result.pending_call=GameSceneCall{operation,boundary,callee};
        if(!runtime.invoke)return fail(GameSceneLogicFailure::missing_owner,boundary);
        const auto receipt=runtime.invoke(*result.pending_call,runtime.arithmetic);
        result.diagnostic=receipt.owner.diagnostic;result.nested_source_boundary=receipt.owner.source_boundary;
        if(receipt.owner.state!=GameLogicOwnerState::completed)
            return fail(receipt.owner.state==GameLogicOwnerState::cancelled?GameSceneLogicFailure::cancelled:
                receipt.owner.state==GameLogicOwnerState::failed?GameSceneLogicFailure::owner_failed:GameSceneLogicFailure::missing_owner,boundary);
        if(needs_return){if(!receipt.return_value)return fail(GameSceneLogicFailure::missing_return,0x413d71);query=receipt.return_value;}
        result.pending_call.reset();result.diagnostic.reset();result.nested_source_boundary=0;return true;
    };
    if(!call(GameSceneOperation::input_40fdc0,0x413d40,0x40fdc0)||
       !call(GameSceneOperation::context_4014b0,0x413d45,0x4014b0))return result;
    if(!state.mode_at_3abdc7c||!*state.mode_at_3abdc7c){fail(GameSceneLogicFailure::unknown_mode,0x413d4a);return result;}
    if(**state.mode_at_3abdc7c==10&&!call(GameSceneOperation::conditional_5919c0,0x413d53,0x5919c0))return result;
    const auto tasks=runtime.scheduler.run_game_tasks();
    if(!tasks){result.task_failure=tasks;result.nested_source_boundary=tasks.source_boundary;fail(GameSceneLogicFailure::task_failed,0x413d58);return result;}
    result.tasks_completed=true;
    if(!call(GameSceneOperation::collision_420700,0x413d5d,0x420700)||
       !call(GameSceneOperation::collision_456ec0,0x413d62,0x456ec0)||
       !call(GameSceneOperation::registry_4511d0,0x413d67,0x4511d0)||
       !call(GameSceneOperation::query_42fb00,0x413d6c,0x42fb00,true))return result;
    if(!state.stage_at_3b22dcc){fail(GameSceneLogicFailure::unknown_stage,0x413d73);return result;}
    auto bits=*state.stage_at_3b22dcc;std::int16_t stage;std::memcpy(&stage,&bits,2);
    if(*query==0){
        if(stage<=25||(stage>34&&stage<=38)){
            if(!call(GameSceneOperation::conditional_426070,0x413d93,0x426070))return result;
            // This branch reloads AX; the other branch retains its prior stage.
            if(!state.stage_at_3b22dcc){fail(GameSceneLogicFailure::unknown_stage,0x413d98);return result;}
            bits=*state.stage_at_3b22dcc;std::memcpy(&stage,&bits,2);
        }
        state.byte_at_3b0ef40=0;
    }else state.byte_at_3b0ef40=0xff;
    if(!state.mode_at_3abdc7c||!*state.mode_at_3abdc7c){fail(GameSceneLogicFailure::unknown_mode,0x413dae);return result;}
    if(**state.mode_at_3abdc7c==9&&stage>14&&stage<=25)state.byte_at_3b0f150=0xff;
    result.completed=true;return result;
}
} // namespace sadx

#include "engine/game_logic.hpp"
#include <cstring>

namespace sadx {
namespace {
std::int32_t decrement(std::int32_t value){
    std::uint32_t bits;std::memcpy(&bits,&value,4);--bits;std::memcpy(&value,&bits,4);return value;
}
}
GameLogicResult run_game_logic(GameLogicRuntime& runtime){
    GameLogicResult result;auto& state=runtime.state;
    std::array<std::uint32_t,4> presses{};
    const auto fail=[&](GameLogicFailure failure,std::uint32_t boundary){result.failure=failure;result.source_boundary=boundary;return false;};
    const auto controller=[&](std::size_t index,std::uint32_t boundary,std::uint32_t load_boundary=0)->std::shared_ptr<GameLogicController>{
        const auto& reference=runtime.controllers[index];
        if(!reference){fail(GameLogicFailure::unknown_controller,load_boundary?load_boundary:boundary);return {};}
        if(!*reference){fail(GameLogicFailure::null_controller,boundary);return {};}
        return *reference;
    };
    const auto read_press=[&](std::size_t index,std::uint32_t boundary,bool combine,std::uint32_t load_boundary=0){
        auto pad=controller(index,boundary,load_boundary);if(!pad)return false;
        const auto pressed=pad->read_u32(GameLogicController::pressed_offset);
        if(!pressed)return fail(GameLogicFailure::unknown_pressed,boundary);
        if(combine)presses[index]|=*pressed;else presses[index]=*pressed;
        return true;
    };
    const auto call=[&](GameLogicOperation operation,std::uint32_t boundary,std::uint32_t callee,
                        std::optional<std::int32_t> first={},std::optional<std::int32_t> second={}){
        result.pending_call=GameLogicCall{operation,boundary,callee,first,second};
        if(runtime.cancellation_requested&&runtime.cancellation_requested())return fail(GameLogicFailure::cancelled,boundary);
        if(!runtime.invoke)return fail(GameLogicFailure::missing_owner,boundary);
        const auto receipt=runtime.invoke(*result.pending_call);result.nested_source_boundary=receipt.source_boundary;result.diagnostic=receipt.diagnostic;
        if(receipt.state!=GameLogicOwnerState::completed){
            return fail(receipt.state==GameLogicOwnerState::cancelled?GameLogicFailure::cancelled:
                        receipt.state==GameLogicOwnerState::failed?GameLogicFailure::owner_failed:GameLogicFailure::missing_owner,boundary);
        }
        result.pending_call.reset();result.nested_source_boundary=0;result.diagnostic.reset();return true;
    };
    // Pointer-word loads precede dereferences; an unknown word and a loaded
    // null pointer therefore stop at different source instructions.
    constexpr std::array<std::uint32_t,3> first_loads{{0x413dd1,0x413dd6,0x413ddc}};
    for(std::size_t i=0;i<3;++i)if(!runtime.controllers[i]){
        fail(GameLogicFailure::unknown_controller,first_loads[i]);return result;
    }
    // Exact first dereference order from the source register loads.
    if(!read_press(2,0x413de3,false)||!read_press(0,0x413de8,false))return result;
    if(!runtime.controllers[3]){fail(GameLogicFailure::unknown_controller,0x413deb);return result;}
    if(!read_press(3,0x413df0,false)||!read_press(1,0x413df4,false))return result;
    if(!call(GameLogicOperation::input_edges,0x413df9,0x40ec80,0))return result;
    // The source loads3b11198 before its unconditional zero store.
    if(!state.timing.word_at_3b11198){fail(GameLogicFailure::unknown_state,0x413dfe);return result;}
    const auto active=*state.timing.word_at_3b11198;state.word_at_3abdf70=0;
    if(active!=0)state.word_at_3b11194=1;
    if(!state.word_at_3b11194){fail(GameLogicFailure::unknown_state,0x413e1e);return result;}
    if(*state.word_at_3b11194==0){
        state.timing.word_at_3b1118c=1;
        if(!call(GameLogicOperation::scene_logic,0x413ea4,0x413d40))return result;
        ++result.completed_scene_ticks;
    }else{
        if(!call(GameLogicOperation::calculate_ticks,0x413e27,0x413920))return result;
        std::uint32_t iteration=0;
        for(;;){
            if(!state.timing.word_at_3b1117c){fail(GameLogicFailure::unknown_state,iteration?0x413e86:0x413e2c);return result;}
            state.timing.word_at_3b1117c=decrement(*state.timing.word_at_3b1117c);
            if(*state.timing.word_at_3b1117c<0)break;
            if(iteration!=0){
                if(!call(GameLogicOperation::poll_input,0x413e49,0x40f460))return result;
                if(!read_press(0,0x413e54,true,0x413e4e)||!read_press(1,0x413e5d,true,0x413e57)||
                   !read_press(2,0x413e65,true,0x413e60)||!read_press(3,0x413e6e,true,0x413e68))return result;
                if(!call(GameLogicOperation::input_edges,0x413e75,0x40ec80,1))return result;
            }
            if(!call(GameLogicOperation::scene_logic,0x413e7d,0x413d40))return result;
            ++result.completed_scene_ticks;++iteration;
        }
    }
    // CMP's operand read precedes the zero store in this shared tail.
    if(!state.timing.word_at_3b11178){fail(GameLogicFailure::unknown_state,0x413ea9);return result;}
    const auto mode=*state.timing.word_at_3b11178;state.timing.word_at_3b1117c=0;
    if(mode==2&&!call(GameLogicOperation::set_mode,0x413ec0,0x411e30,1,1))return result;
    constexpr std::array<std::uint32_t,4> boundaries{{0x413ecd,0x413ed5,0x413edd,0x413ee5}};
    constexpr std::array<std::uint32_t,4> loads{{0x413ec8,0x413ed0,0x413ed8,0x413ee0}};
    for(std::size_t i=0;i<4;++i){
        auto pad=controller(i,boundaries[i],loads[i]);if(!pad)return result;
        const auto pressed=pad->read_u32(GameLogicController::pressed_offset);
        if(!pressed){fail(GameLogicFailure::unknown_pressed,boundaries[i]);return result;}
        pad->write_u32(GameLogicController::pressed_offset,*pressed|presses[i]);
    }
    if(!call(GameLogicOperation::input_edges,0x413eef,0x40ec80,2))return result;
    result.completed=true;return result;
}
} // namespace sadx

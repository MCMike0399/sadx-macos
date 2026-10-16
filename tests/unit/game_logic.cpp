#include "guest/frame_control_reference.hpp"
#include "engine/game_logic.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    sadx::GameLogicControllers pads;
    sadx::GameLogicTimingState timing;
    sadx::GameLogicState state{timing,{},{}};
    std::vector<sadx::GameLogicCall> calls;
    sadx::GameLogicRuntime runtime;
    unsigned ticks=0,polls=0;
    Fixture():runtime{pads,state,{},{} }{
        for(unsigned i=0;i<4;++i){auto pad=std::make_shared<sadx::GameLogicController>();pad->write_u32(0x10,1U<<i);pads[i]=pad;}
        state.word_at_3abdf70=99;state.timing.word_at_3b11198=1;state.word_at_3b11194=0;
        state.timing.word_at_3b1118c=55;state.timing.pending_logic_ticks=55;state.timing.logic_increment=2;
        runtime.invoke=[&](const sadx::GameLogicCall& call){
            calls.push_back(call);
            if(call.operation==sadx::GameLogicOperation::calculate_ticks){state.timing.pending_logic_ticks=3;state.timing.word_at_3b1118c=3;}
            if(call.operation==sadx::GameLogicOperation::poll_input){++polls;for(unsigned i=0;i<4;++i)(*pads[i])->write_u32(0x10,1U<<(i+4*polls));}
            if(call.operation==sadx::GameLogicOperation::scene_logic){++ticks;for(auto& pad:pads)(*pad)->write_u32(0x10,0);}
            return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0};
        };
    }
};
struct SupportDiagnostic final:sadx::GameLogicOwnerDiagnostic {
    sadx::GameLogicSupportResult result;
    explicit SupportDiagnostic(sadx::GameLogicSupportResult value):result(std::move(value)){}
};
void test(){
    Fixture normal;auto result=sadx::run_game_logic(normal.runtime);
    require(bool(result)&&result.completed_scene_ticks==3&&normal.ticks==3&&normal.polls==2,"Three actual scene invocations with two input polls");
    require(normal.state.word_at_3abdf70==0&&normal.state.word_at_3b11194==1&&normal.state.timing.pending_logic_ticks==0,"Source stores and final pending-tick reset");
    for(unsigned i=0;i<4;++i)require((*normal.pads[i])->read_u32(0x10)==((1U<<i)|(1U<<(i+4))|(1U<<(i+8))),"Presses survive each extra tick and scene consumption");
    const auto& c=normal.calls;
    require(c.size()==11&&c.front().first_argument==0&&c.back().first_argument==2&&c[9].operation==sadx::GameLogicOperation::set_mode&&c[9].first_argument==1&&c[9].second_argument==1,"Exact edge/mode call arguments and order");
    Fixture simple;simple.state.timing.word_at_3b11198=0;simple.state.word_at_3b11194=0;simple.state.timing.logic_increment=0;
    result=sadx::run_game_logic(simple.runtime);
    require(bool(result)&&result.completed_scene_ticks==1&&simple.calls.size()==3&&simple.polls==0&&simple.state.timing.word_at_3b1118c==1,"No tick calculation or extra polls in the source inactive path");
    Fixture absent;absent.runtime.invoke={};result=sadx::run_game_logic(absent.runtime);
    require(result.failure==sadx::GameLogicFailure::missing_owner&&result.source_boundary==0x413df9&&absent.state.word_at_3abdf70==99,"Missing first owner cannot cause later stores");
    Fixture failed;failed.runtime.invoke=[](const sadx::GameLogicCall&){return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::failed,0x123456};};result=sadx::run_game_logic(failed.runtime);
    require(result.failure==sadx::GameLogicFailure::owner_failed&&result.nested_source_boundary==0x123456&&result.pending_call.has_value(),"Nested failure keeps required call context");
    Fixture unknown;unknown.pads[0].reset();result=sadx::run_game_logic(unknown.runtime);
    require(result.failure==sadx::GameLogicFailure::unknown_controller&&result.source_boundary==0x413dd1&&unknown.calls.empty(),"Unknown pointer word precedes pad2 dereference");
    Fixture null;null.pads[0]=std::shared_ptr<sadx::GameLogicController>{};result=sadx::run_game_logic(null.runtime);
    require(result.failure==sadx::GameLogicFailure::null_controller&&result.source_boundary==0x413de8,"Observed null fails on its actual dereference");
    Fixture partial;(*partial.pads[0])->forget(0x13,1);result=sadx::run_game_logic(partial.runtime);
    require(result.failure==sadx::GameLogicFailure::unknown_pressed&&result.source_boundary==0x413de8&&partial.calls.empty()&&(*partial.pads[0])->read_byte(0x10)==1,"One unknown pressed byte stops the whole dword read without losing known bytes");
    Fixture later;later.state.timing.logic_increment.reset();result=sadx::run_game_logic(later.runtime);
    require(result.failure==sadx::GameLogicFailure::unknown_state&&result.source_boundary==0x413ea9&&result.completed_scene_ticks==3&&later.state.timing.pending_logic_ticks==-1,"Tail mode read precedes pending-counter zero store");
    Fixture live;auto invoke=live.runtime.invoke;live.runtime.invoke=[&](const sadx::GameLogicCall& call){auto receipt=invoke(call);if(call.operation==sadx::GameLogicOperation::set_mode){auto pad=std::make_shared<sadx::GameLogicController>();pad->write_u32(0x10,0x80000000U);live.pads[0]=pad;}return receipt;};
    result=sadx::run_game_logic(live.runtime);require(bool(result)&&(*live.pads[0])->read_u32(0x10)==0x80000111U,"Tail aggregation rereads a changed live controller owner");
    Fixture cancelled;unsigned probes=0;cancelled.runtime.cancellation_requested=[&](){return ++probes==4;};result=sadx::run_game_logic(cancelled.runtime);
    require(result.failure==sadx::GameLogicFailure::cancelled&&result.completed_scene_ticks==1&&result.pending_call->operation==sadx::GameLogicOperation::poll_input,"Cancellation preserves partial tick state and pending owner");
    Fixture bound;sadx::GameLogicEdgeState edges;
    for(auto& row:edges.raw_at_3b0e3e2)row[0]=0;
    edges.raw_at_3b0e3e2[42][0]=1;edges.special_at_3b0e3ba=1;
    bound.timing.logic_increment=1;bound.timing.word_at_3b11180=1;
    bound.timing.frame_multiplier=1;bound.timing.word_at_3b2a2e4=0;
    auto external=bound.runtime.invoke;
    bound.runtime.invoke=[&](const sadx::GameLogicCall& call){
        if(call.operation==sadx::GameLogicOperation::input_edges||call.operation==sadx::GameLogicOperation::calculate_ticks){
            const auto receipt=call.operation==sadx::GameLogicOperation::input_edges?
                sadx::aggregate_game_logic_edges(*call.first_argument,edges):sadx::calculate_game_logic_ticks(bound.state.timing);
            return sadx::GameLogicOwnerResult{receipt?sadx::GameLogicOwnerState::completed:sadx::GameLogicOwnerState::failed,
                receipt.source_boundary,std::make_shared<SupportDiagnostic>(receipt)};
        }
        auto receipt=external(call);
        if(call.operation==sadx::GameLogicOperation::poll_input)edges.raw_at_3b0e3e2[42][0]=2;
        if(call.operation==sadx::GameLogicOperation::scene_logic)edges.raw_at_3b0e3e2[42][0]=0;
        return receipt;
    };
    result=sadx::run_game_logic(bound.runtime);
    require(bool(result)&&result.completed_scene_ticks==2&&bound.timing.word_at_3b1118c==2&&
        edges.raw_at_3b0e3e2[42][0]==3,"Real tick/byte-aggregation owners share the same live state through orchestration");
    bound.timing.frame_multiplier=0;result=sadx::run_game_logic(bound.runtime);
    const auto diagnostic=std::dynamic_pointer_cast<const SupportDiagnostic>(result.diagnostic);
    require(result.failure==sadx::GameLogicFailure::owner_failed&&result.source_boundary==0x413e27&&diagnostic&&
        diagnostic->result.failure==sadx::GameLogicSupportFailure::divide_by_zero,"A real leaf division failure survives the orchestration boundary");
    std::cout<<"Game logic: source order, multi-tick press aggregation, live owner replacement and partial failure/cancellation passed.\n";
}
}
int main(){try{test();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

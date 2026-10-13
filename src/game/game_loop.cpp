#include "game/game_loop.hpp"
#include "engine/speedrun.hpp"

namespace sadx {
namespace {
struct Stop{};
struct Body {
    GameLoopRuntime& runtime;GameLoopResult result;
    GameLoopState& s(){return runtime.state;}
    [[noreturn]]void fail(GameLoopFailure failure,std::uint32_t at){result.failure=failure;result.source_boundary=at;throw Stop{};}
    template<class T>T need(const std::optional<T>& value,std::uint32_t at){if(!value)fail(GameLoopFailure::unknown_state,at);return *value;}
    std::optional<std::uint32_t> call(GameLoopOperation operation,std::uint32_t site,std::uint32_t callee,
                                      std::optional<std::uint32_t> first={},std::optional<std::uint32_t> second={},bool returns=false){
        result.pending_call=GameLoopCall{operation,site,callee,{first,second}};
        if(runtime.cancellation_requested&&runtime.cancellation_requested())fail(GameLoopFailure::cancelled,site);
        if(!runtime.invoke)fail(GameLoopFailure::missing_owner,site);
        const auto receipt=runtime.invoke(*result.pending_call);
        result.nested_source_boundary=receipt.owner.source_boundary;result.diagnostic=receipt.owner.diagnostic;
        if(receipt.owner.state!=GameLogicOwnerState::completed)
            fail(receipt.owner.state==GameLogicOwnerState::cancelled?GameLoopFailure::cancelled:
                 receipt.owner.state==GameLogicOwnerState::failed?GameLoopFailure::owner_failed:GameLoopFailure::missing_owner,site);
        if(returns&&!receipt.return_value)fail(GameLoopFailure::missing_return,site);
        result.pending_call.reset();result.nested_source_boundary=0;result.diagnostic.reset();
        return receipt.return_value;
    }
    bool fog_pass(std::uint32_t flags_at,std::uint32_t fog_at){
        const auto borrowed=[](const std::optional<std::uint8_t>* owner){return owner?*owner:std::optional<std::uint8_t>{};};
        const auto flags=need(borrowed(s().special_flags_at_909eb4),flags_at);
        if(!(flags&2)||!(flags&1))return false;
        return need(borrowed(s().fog_flag_at_3abdcfe),fog_at)!=0;
    }
    void exit_test(){result.next=need(s().exit_at_3abdf7c,0x40cc9b)==0?GameLoopNext::iteration:GameLoopNext::shutdown;}
    void iteration(){
        auto& st=s();
        runtime.logic.timing.word_at_3b11198=1;
        call(GameLoopOperation::message_77f090,0x40cb06,0x77f090);
        call(GameLoopOperation::input_40f460,0x40cb0b,0x40f460);
        if(need(st.focus_at_8929d4,0x40cb10)==0){
            call(GameLoopOperation::idle_781010,0x40cb18,0x781010);call(GameLoopOperation::idle_781060,0x40cb1d,0x781060);
            exit_test();return;
        }
        call(GameLoopOperation::work_420830,0x40cb27,0x420830);call(GameLoopOperation::queue_reset_407ab0,0x40cb2c,0x407ab0);
        call(GameLoopOperation::depth_77ed00,0x40cb33,0x77ed00,3);call(GameLoopOperation::work_40c090,0x40cb3b,0x40c090);
        call(GameLoopOperation::mode_4250d0,0x40cb40,0x4250d0);
        if(fog_pass(0x40cb45,0x40cb52)){
            call(GameLoopOperation::fog_411b60,0x40cb5b,0x411b60,0);
            // 411b60 may replace the range; both words are read after it returns.
            const auto far=need(st.fog_range_at_3abdca4,0x40cb60);const auto near=need(st.fog_range_at_3abdca0,0x40cb65);
            call(GameLoopOperation::fog_range_403180,0x40cb6d,0x403180,near,far);
        }
        call(GameLoopOperation::consume_4086f0,0x40cb75,0x4086f0);
        if(fog_pass(0x40cb7a,0x40cb87)){
            const auto far=need(st.fog_range_at_3abdc74,0x40cb8f);const auto near=need(st.fog_range_at_3abdc70,0x40cb95);
            call(GameLoopOperation::fog_range_403180,0x40cb9c,0x403180,near,far);
        }
        if(need(st.request_at_3b291ad,0x40cba4)!=0){
            st.counter_at_3b291b2=static_cast<std::uint8_t>(need(st.counter_at_3b291b2,0x40cbac)+1U);
            st.save_phase_at_3b291a4=1;st.pending_at_3abdf76=1;
        }
        const auto phase=need(st.save_phase_at_3b291a4,0x40cbbf);
        if(phase==1||phase==-1)call(GameLoopOperation::save_40be40,0x40cbed,0x40be40);
        else if(need(st.pending_at_3abdf79,0x40cbd0)!=0){
            call(GameLoopOperation::pending_40e2c0,0x40cbdd,0x40e2c0,0x3abdca8);st.pending_at_3abdf79=0;
        }
        if(need(st.word_at_3abdf80,0x40cbf2)!=0)call(GameLoopOperation::work_4326f0,0x40cbfa,0x4326f0);
        call(GameLoopOperation::work_40d4d0,0x40cbff,0x40d4d0);call(GameLoopOperation::work_40eab0,0x40cc04,0x40eab0);
        call(GameLoopOperation::work_42f990,0x40cc09,0x42f990);call(GameLoopOperation::work_40d3c0,0x40cc0e,0x40d3c0);
        call(GameLoopOperation::work_420840,0x40cc13,0x420840);call(GameLoopOperation::frame_411d00,0x40cc18,0x411d00);
        if(need(st.pending_at_3abdf76,0x40cc1d)!=0){st.pending_at_3abdf76=0;call(GameLoopOperation::work_421fd0,0x40cc2b,0x421fd0);}
        if(need(st.save_phase_at_3b291a4,0x40cc30)==0){
            if(need(st.pending_at_3abdf79,0x40cc38)!=0){call(GameLoopOperation::pending_40e2c0,0x40cc45,0x40e2c0,0x3abdca8);st.pending_at_3abdf79=0;}
            if(need(st.mode_at_3b28114,0x40cc53)==2)st.mode_at_3b28114=1;
        }
        // The counter is loaded and incremented before the game-mode compare,
        // but stored after it; an unknown mode leaves the counter unchanged.
        const auto frames=need(st.frames_at_3abdf58,0x40cc62)+1U;
        const auto game_mode=need(st.game_mode_at_3b22de4,0x40cc69);st.frames_at_3abdf58=frames;
        if(game_mode!=0x10)st.frames_at_3abdf5c=need(st.frames_at_3abdf5c,0x40cc79)+1U;
        call(GameLoopOperation::ret_51f680,0x40cc7f,0x51f680);
        const auto keys=*call(GameLoopOperation::keys_40ef30,0x40cc84,0x40ef30,{},{},true);
        if(keys&0x44){
            if(*call(GameLoopOperation::key_40ef20,0x40cc8f,0x40ef20,0x3d,{},true)!=0){st.exit_at_3abdf7c=1;result.next=GameLoopNext::shutdown;return;}
        }
        exit_test();
    }
};
template<class F>GameLoopResult run(GameLoopRuntime& runtime,F&& body){
    Body b{runtime,{}};try{body(b);b.result.completed=true;}catch(Stop){}return std::move(b.result);
}
}
GameLoopResult run_game_loop_startup(GameLoopRuntime& runtime){
    return run(runtime,[](Body& b){
        b.call(GameLoopOperation::init_40c030,0x40cad2,0x40c030);b.s().word_at_3abdcf8=0;
        b.call(GameLoopOperation::ret_51f680,0x40cadf,0x51f680);b.call(GameLoopOperation::set_401410,0x40caea,0x401410,1);
        b.result.next=b.need(b.s().exit_at_3abdf7c,0x40caef)!=0?GameLoopNext::shutdown:GameLoopNext::iteration;
    });
}
GameLoopResult run_game_loop_iteration(GameLoopRuntime& runtime){return run(runtime,[](Body& b){b.iteration();});}
GameLoopResult run_game_loop_shutdown(GameLoopRuntime& runtime){
    return run(runtime,[](Body& b){
        b.call(GameLoopOperation::ret_51f680,0x40ccb4,0x51f680,0x7dcc30);b.call(GameLoopOperation::ret_51f680,0x40ccbc,0x51f680);
        b.call(GameLoopOperation::shutdown_7853c0,0x40ccc1,0x7853c0);
        if(b.need(b.s().word_at_3c85804,0x40ccc6)!=0)b.call(GameLoopOperation::shutdown_6fbf10,0x40ccd1,0x6fbf10);
        b.call(GameLoopOperation::shutdown_410630,0x40ccd6,0x410630);b.call(GameLoopOperation::shutdown_420f40,0x40ccdb,0x420f40);
        b.call(GameLoopOperation::shutdown_420ef0,0x40cce0,0x420ef0);b.call(GameLoopOperation::shutdown_77fc50,0x40cce5,0x77fc50);
        b.result.next=GameLoopNext::finished;
    });
}
FrameCompletionResult run_frame_completion(FrameCompletionRuntime& runtime){
    FrameCompletionResult r;auto& timing=runtime.logic.timing;
    const auto stop=[&](FrameCompletionFailure failure,std::uint32_t at){r.failure=failure;r.source_boundary=at;return r;};
    const auto call=[&](FrameCompletionOperation operation,std::uint32_t site,std::uint32_t callee){
        r.pending_call=FrameCompletionCall{operation,site,callee};
        if(!runtime.invoke){stop(FrameCompletionFailure::missing_owner,site);return false;}
        const auto receipt=runtime.invoke(*r.pending_call);r.nested_source_boundary=receipt.source_boundary;
        if(receipt.state!=GameLogicOwnerState::completed){stop(receipt.state==GameLogicOwnerState::cancelled?FrameCompletionFailure::cancelled:
            receipt.state==GameLogicOwnerState::failed?FrameCompletionFailure::owner_failed:FrameCompletionFailure::missing_owner,site);return false;}
        r.pending_call.reset();r.nested_source_boundary=0;return true;};
    if(!runtime.loop.resync_at_3abdf68)return stop(FrameCompletionFailure::unknown_state,0x411d01);
    std::int32_t extra;
    if(*runtime.loop.resync_at_3abdf68!=0){
        if(!runtime.renderer.render_factor_at_89295c)return stop(FrameCompletionFailure::unknown_state,0x411d0a);
        extra=static_cast<std::int32_t>(static_cast<std::uint32_t>(*runtime.renderer.render_factor_at_89295c)-1U);runtime.loop.resync_at_3abdf68=0;
    }else{
        if(runtime.policy!=FrameCompletionPolicy::speedrun_2_0)return stop(FrameCompletionFailure::unported_original_policy,0x411d1c);
        if(!timing.word_at_3b11188)return stop(FrameCompletionFailure::unknown_state,0x411d1c);
        extra=speedrun::extra_game_ticks(*timing.word_at_3b11188); // DLL +0x11c0
    }
    r.extra_ticks=extra;
    // 411da3: load 3b11198 before the 3b11180 store, then 3b28114.
    if(!timing.word_at_3b11198)return stop(FrameCompletionFailure::unknown_state,0x411da3);
    const auto active=*timing.word_at_3b11198;timing.word_at_3b11180=extra;
    if(!runtime.loop.mode_at_3b28114)return stop(FrameCompletionFailure::unknown_state,0x411dae);
    const auto mode=*runtime.loop.mode_at_3b28114;runtime.logic.word_at_3b11194=active;timing.word_at_3b11198=0;
    using O=FrameCompletionOperation;
    if(mode!=0&&mode!=3){
        if(!call(O::thunk_781010,0x411dcc,0x781010)||!call(O::thunk_781020,0x411dd1,0x781020)||!call(O::thunk_781040,0x411dd6,0x781040)||!call(O::thunk_781050,0x411ddb,0x781050)||!call(O::limiter_781060,0x411de3,0x781060))return r;
    }else{
        if(!call(O::thunk_781010,0x411de8,0x781010)||!call(O::thunk_781020,0x411ded,0x781020)||!call(O::thunk_781030,0x411df2,0x781030)||!call(O::thunk_781040,0x411df7,0x781040)||!call(O::thunk_781050,0x411dfc,0x781050))return r;
        // Reread: the owners above may change 3b28114.
        if(!runtime.loop.mode_at_3b28114)return stop(FrameCompletionFailure::unknown_state,0x411e01);
        if(*runtime.loop.mode_at_3b28114!=0)runtime.loop.mode_at_3b28114=*runtime.loop.mode_at_3b28114-1U;
        if(!call(O::limiter_781060,0x411e13,0x781060))return r;
    }
    r.completed=true;return r;
}
} // namespace sadx

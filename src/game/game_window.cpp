#include "game/game_window.hpp"
namespace sadx {
GameWindowResult run_message_pump_789bd0(GameMessagePumpRuntime& rt){
    GameWindowResult r;const auto stop=[&](GameWindowFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;return r;};
    if(rt.profile==GameMessagePumpProfile::mod_loader){ // the loader's HandleWindowMessages_r (loader code: sites are its hook entry 789bd0)
        if(!rt.peek)return stop(GameWindowFailure::host_unavailable,0x789bd0);
        auto message=rt.peek(0);if(!message)return stop(GameWindowFailure::host_unavailable,0x789bd0);
        if(!*message){rt.state.last_wparam_at_3d08534.reset();return r;}
        for(;;){
            if(!rt.translate_accelerator)return stop(GameWindowFailure::host_unavailable,0x789bd0);
            const auto handled=rt.translate_accelerator(**message);if(!handled)return stop(GameWindowFailure::host_unavailable,0x789bd0);
            if(!*handled){
                if(!rt.dispatch)return stop(GameWindowFailure::host_unavailable,0x789bd0);
                if(rt.dispatch(**message).state!=GameLogicOwnerState::completed)return stop(GameWindowFailure::dispatch_failed,0x789bd0);
                ++r.dispatched;}
            const auto next=rt.peek(0);if(!next)return stop(GameWindowFailure::host_unavailable,0x789bd0);
            if(!*next)break;
            message=next;}
        rt.state.last_wparam_at_3d08534=(*message)->wparam;return r;
    }
    if(!rt.window_at_3d0fd30)return stop(GameWindowFailure::unknown_state,0x789bd0,0x3d0fd30);
    if(!rt.peek)return stop(GameWindowFailure::host_unavailable,0x789beb);
    auto message=rt.peek(*rt.window_at_3d0fd30);if(!message)return stop(GameWindowFailure::host_unavailable,0x789beb);
    if(!*message){rt.state.last_wparam_at_3d08534.reset();return r;} // 789c36: an uninitialised MSG.wParam
    for(;;){
        if(!rt.dispatch)return stop(GameWindowFailure::host_unavailable,0x789c05);
        const auto d=rt.dispatch(**message);if(d.state!=GameLogicOwnerState::completed)return stop(GameWindowFailure::dispatch_failed,0x789c0c);
        ++r.dispatched;
        if(!rt.window_at_3d0fd30)return stop(GameWindowFailure::unknown_state,0x789c0e,0x3d0fd30);
        const auto next=rt.peek(*rt.window_at_3d0fd30);if(!next)return stop(GameWindowFailure::host_unavailable,0x789c20);
        if(!*next)break; // the MSG buffer keeps the last message
        message=next;
    }
    rt.state.last_wparam_at_3d08534=(*message)->wparam;return r;
}
} // namespace sadx

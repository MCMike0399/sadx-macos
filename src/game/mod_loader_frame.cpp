#include "game/mod_loader_frame.hpp"
namespace sadx {
SpeedrunModState speedrun_mod_after_init(bool quick_save_enabled){
    SpeedrunModState s;s.quick_save_at_100107e9=quick_save_enabled?1:0;s.chao_viewer_at_100107e8=0;
    s.timer_frames_at_10010818=0;s.timer_seconds_at_10010819=0;s.timer_minutes_at_1001081a=0;return s;
}
ModFrameResult run_speedrun_on_frame(SpeedrunFrameRuntime& rt){
    ModFrameResult r;auto& m=rt.mod;
    const auto stop=[&](ModFrameFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;return r;};
    const auto call=[&](std::uint32_t site,std::uint32_t callee){
        r.pending_callee=callee;
        if(!rt.invoke){stop(ModFrameFailure::missing_owner,site);return false;}
        const auto receipt=rt.invoke(site,callee);
        if(receipt.state!=GameLogicOwnerState::completed){stop(receipt.state==GameLogicOwnerState::cancelled?ModFrameFailure::cancelled:
            receipt.state==GameLogicOwnerState::failed?ModFrameFailure::owner_failed:ModFrameFailure::missing_owner,site);return false;}
        r.pending_callee.reset();return true;};
    if(!m.quick_save_at_100107e9)return stop(ModFrameFailure::unknown_state,0x10002e64,0x100107e9);
    if(*m.quick_save_at_100107e9!=0&&!call(0x10002e6d,0x10004f80))return r;
    if(!m.chao_viewer_at_100107e8)return stop(ModFrameFailure::unknown_state,0x10002e72,0x100107e8);
    if(*m.chao_viewer_at_100107e8!=0){
        if(!call(0x10002e7f,0x10002e7f))return r; // the stat viewer body (7808c0, 7808e0, 780b30 grade text)
        if(!rt.stage_at_3b22dcc)return stop(ModFrameFailure::unknown_state,0x10002f4f,0x3b22dcc);
        if(*rt.stage_at_3b22dcc!=0x27)m.chao_viewer_at_100107e8=0;
    }
    if(!rt.game_mode_at_3b22de4)return stop(ModFrameFailure::unknown_state,0x10002f60,0x3b22de4);
    bool counting=false;
    if(*rt.game_mode_at_3b22de4!=7){
        if(!rt.mode_at_3abdc7c)return stop(ModFrameFailure::unknown_state,0x10002f6a,0x3abdc7c);
        counting=*rt.mode_at_3abdc7c==4||*rt.mode_at_3abdc7c==9;
    }
    if(!counting){m.timer_frames_at_10010818=0;m.timer_seconds_at_10010819=0;m.timer_minutes_at_1001081a=0;return r;}
    // Signed byte compares (JL); ADD AL,0xc4 / SUB AL,0x3c wrap in 8 bits.
    if(!m.timer_frames_at_10010818)return stop(ModFrameFailure::unknown_state,0x10002f79,0x10010818);
    const auto frames=*m.timer_frames_at_10010818;if(static_cast<std::int8_t>(frames)<0x3c)return r;
    m.timer_frames_at_10010818=static_cast<std::uint8_t>(frames+0xc4);
    if(!m.timer_seconds_at_10010819)return stop(ModFrameFailure::unknown_state,0x10002f89,0x10010819);
    const auto seconds=static_cast<std::uint8_t>(*m.timer_seconds_at_10010819+1);m.timer_seconds_at_10010819=seconds;
    if(static_cast<std::int8_t>(seconds)<0x3c)return r;
    m.timer_seconds_at_10010819=static_cast<std::uint8_t>(seconds-0x3c);
    if(!m.timer_minutes_at_1001081a)return stop(ModFrameFailure::unknown_state,0x10002fa0,0x1001081a);
    const auto minutes=static_cast<std::uint8_t>(*m.timer_minutes_at_1001081a+1);m.timer_minutes_at_1001081a=minutes;
    if(static_cast<std::int8_t>(minutes)<0x64)return r;
    m.timer_frames_at_10010818=0x3b;m.timer_seconds_at_10010819=0x3b;m.timer_minutes_at_1001081a=0x63;return r;
}
ModFrameResult run_mod_loader_process_codes(SpeedrunFrameRuntime& rt){return run_speedrun_on_frame(rt);}
} // namespace sadx

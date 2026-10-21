#include "game/game_frame_owners.hpp"
namespace sadx {
namespace {
struct Stop{};
struct Operation {
    GameFrameOwnersRuntime& rt;GameFrameOwnerResult r;
    [[noreturn]]void fail(GameFrameOwnerFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(GameFrameOwnerFailure::unknown_state,at,fact);return *v;}
    void call(std::uint32_t site,std::uint32_t callee,std::array<std::optional<std::uint32_t>,2> arguments={}){
        r.pending_call=GameFrameOwnerCall{site,callee,arguments};
        if(!rt.invoke)fail(GameFrameOwnerFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);
        if(receipt.state!=GameLogicOwnerState::completed)fail(receipt.state==GameLogicOwnerState::failed?GameFrameOwnerFailure::owner_failed:GameFrameOwnerFailure::missing_owner,site);
        r.pending_call.reset();}
    void render(const NinjaRenderResult& result){if(!result)fail(GameFrameOwnerFailure::render_failed,result.source_boundary);}
    void depth(std::uint32_t comparison,std::uint32_t write){render(ninja_set_depth_comparison(comparison,rt.depth_table,rt.render));render(ninja_set_depth_write(write,rt.render));}
    // Actual 402ed0: 7815c0 only on a change, then both FOV words.
    void fov(std::uint32_t angle,std::uint32_t at){
        if(need(rt.view.fov_at_3aad0bc,at,0x3aad0bc)!=angle){
            if(!rt.sine_table)fail(GameFrameOwnerFailure::unknown_state,at+0xf,0x88af78);
            const auto f=ninja_set_fov_7815c0(static_cast<std::int32_t>(angle),*rt.sine_table,rt.projection,rt.width_at_3d0f9e4,rt.fp);
            if(!f)fail(f.fp_failure?GameFrameOwnerFailure::fp_failed:GameFrameOwnerFailure::owner_failed,f.source_boundary);}
        rt.view.fov_copy_at_3ab98ec=angle;rt.view.fov_at_3aad0bc=angle;}
    // NOTSA: caption owns the timer/release algorithm; its adapter validates source facts in order.
    void complete_caption(){
        if(!rt.complete_caption_frame)fail(GameFrameOwnerFailure::missing_owner,0x40bcf0);
        const auto result=rt.complete_caption_frame(rt.invoke);
        if(!result){r=result;throw Stop{};}}

};
}
GameFrameOwnerResult run_game_frame_owner(std::uint32_t callee,std::optional<std::uint32_t> argument,GameFrameOwnersRuntime& rt){
    Operation op{rt,{}};auto& s=rt.state;
    try{
        switch(callee){
        case 0x40d4d0:{
            rt.view.word_at_3b2cbb4=op.need(rt.view.fov_copy_at_3ab98ec,0x40d4d4,0x3ab98ec); // 402f00
            op.depth(7,0);op.fov(0x31c7,0x40d4f2);
            const auto entries=static_cast<std::int32_t>(op.need(rt.draw_count_at_3b0e338,0x40d4f7,0x3b0e338));
            if(entries-1>=0)op.call(0x40d520,0x40d520,{static_cast<std::uint32_t>(entries-1)}); // the 3abe13c draw-list body
            if(const auto saved=op.need(rt.view.word_at_3b2cbb4,0x40d819,0x3b2cbb4);saved!=0)op.fov(saved,0x40d823);
            op.depth(1,1);rt.draw_count_at_3b0e338=0;op.complete_caption();break;}
        case 0x40eab0:
            if(static_cast<std::int32_t>(op.need(rt.effect_at_3b0e998,0x40eab0,0x3b0e998))>0)op.call(0x40eac0,0x40eac0); // the effect body
            break;
        case 0x42f990:{
            const auto list=op.need(s.list_at_3b2c584,0x42f9b6,0x3b2c584);if(list==0)break;
            s.list_at_3b2c584=0;op.call(0x42f9cc,0x42f9cc,{list});break;} // the list walk (403290 per texture entry)
        case 0x40d3c0:{
            if(static_cast<std::int32_t>(op.need(s.count_at_3b0e334,0x40d3ed,0x3b0e334))>0)op.call(0x40d3f5,0x40d3f5); // the loop body over 3abe150 pairs (403290 per nonzero entry)
            s.count_at_3b0e334=0;break;}
        case 0x420840:{
            rt.view_reset_at_3b280a8=0;op.depth(3,1);
            NinjaQueueColorResult partial;std::uint32_t site=0;
            if(!ninja_queue_color_402f40(rt.color_at_3b18230,rt.colors,partial,rt.fp,site))op.fail(GameFrameOwnerFailure::unknown_state,0x402f44,0x3b18230);
            break;}
        case 0x40ef30:op.r.eax=op.need(rt.keys_at_3b0e3b8,0x40ef30,0x3b0e3b8);break;
        case 0x40ef20:{
            if(!argument)op.fail(GameFrameOwnerFailure::unknown_state,0x40ef20);
            const auto k=*argument;
            if(k==0)op.r.eax=op.need(rt.edges.bytes_at_3b0e3e0[0],0x40ef24,0x3b0e3e0);
            else if(k-1<256)op.r.eax=op.need(rt.edges.raw_at_3b0e3e2[k-1][1],0x40ef24,0x3b0e3e0+3*k);
            else op.fail(GameFrameOwnerFailure::unknown_state,0x40ef24,0x3b0e3e0+3*k);
            break;}
        default:op.r.failure=GameFrameOwnerFailure::not_ported;break;
        }
    }catch(Stop){}catch(const X87Error&){op.r.failure=GameFrameOwnerFailure::fp_failed;op.r.source_boundary=callee;}
    return op.r;
}
} // namespace sadx

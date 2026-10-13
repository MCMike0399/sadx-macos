#include "game/game_view.hpp"
#include <cstring>

namespace sadx {
namespace {
struct Stop{};
std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
}
GameViewResult run_game_view_4208e0(GameViewRuntime& rt,X87Context& fp){
    GameViewResult r;auto& v=rt.view;auto& q=rt.queue;std::uint32_t site=0x4208e0;
    const auto fail=[&](GameViewFailure f,std::uint32_t at){r.failure=f;r.source_boundary=at;throw Stop{};};
    const auto need=[&](const auto& x,std::uint32_t at){if(!x)fail(GameViewFailure::unknown_state,at);return *x;};
    const auto call=[&](std::uint32_t at,std::uint32_t callee,std::array<std::optional<std::uint32_t>,6> args={}){
        r.pending_call=GameViewCall{at,callee,args};if(!rt.invoke)fail(GameViewFailure::missing_owner,at);
        const auto receipt=rt.invoke(*r.pending_call);if(receipt.state!=GameLogicOwnerState::completed)fail(receipt.state==GameLogicOwnerState::failed?GameViewFailure::owner_failed:GameViewFailure::missing_owner,at);
        r.pending_call.reset();};
    const auto render=[&]()->NinjaRenderState&{if(!q.queue_runtime.render_state)fail(GameViewFailure::render_failed,0x77ec68);return *q.queue_runtime.render_state;};
    const auto append=[&](const NinjaRenderResult& c){r.render.commands.insert(r.render.commands.end(),c.commands.begin(),c.commands.end());if(!c)fail(GameViewFailure::render_failed,c.source_boundary);};
    const auto product=[&](std::optional<float> a,std::uint32_t load,float k,std::uint32_t mul){site=load;const auto x=fp.from_float(need(a,load));site=mul;return fp.to_float(fp.multiply_float_memory(x,k));};
    const auto color=[&](const std::optional<std::array<float,4>>& input,std::uint32_t fact){
        NinjaQueueColorOwners owners{q.queue_runtime.color_at_3ab9864,q.projection.color_argb_at_3d0f7f0,q.one_at_7dcb10,q.negative_one_at_7dcb0c};NinjaQueueColorResult partial;
        if(!ninja_queue_color_402f40(input,owners,partial,fp,site)){(void)fact;fail(GameViewFailure::unknown_state,0x402f44);}};
    try{
        q.queue_runtime.mode_at_3ab98ac=2; // 403e60
        // 420860
        if(need(v.reset_pending_at_3b280a8,0x420860)!=0){
            const auto reset=ninja_queue_reset_407ab0(q,rt.queue_source_base,fp);if(!reset)fail(GameViewFailure::queue_failed,reset.source_boundary);
            v.clear_colors_at_3ce7174={0xff000000U,0xff000000U,0xff000000U,0xff000000U}; // 402f10(0,0,0) -> 780c00
            append(ninja_set_blend(0,8,render()));append(ninja_set_blend(1,6,render()));
            color(q.initial_color_at_3b18230,0x3b18230); // 4128f0
            const auto y=product(v.screen_height_at_8928c4,0x420893,v.scale_y_at_7dcb24,0x420899);
            const auto x=product(v.screen_width_at_8928c0,0x4208b0,v.scale_x_at_7dcb28,0x4208b6);
            call(0x4208c3,0x4071c0,{0,0,bits(x),bits(y),0xbf800000U,0xff000000U});
            v.mode_at_3b28114=0;
        }
        const auto w=product(v.screen_width_at_8928c0,0x4208ed,v.scale_x_at_7dcb28,0x4208f3);v.word_at_3abd9cc=0;v.view_at_3b28098=w;
        const auto h=product(v.screen_height_at_8928c4,0x42090e,v.scale_y_at_7dcb24,0x420914);v.view_at_3b2809c=h;
        v.view_at_3b280a0=product(v.view_at_3b28098,0x420920,v.half_at_7dcc98,0x420926);
        v.view_at_3b280a4=product(v.view_at_3b2809c,0x420932,v.half_at_7dcc98,0x420938);
        // 788240(&3b28094)
        q.projection.scale_at_3d0f9e0=[&]{const auto s=need(v.scale_word_at_3b28094,0x420944);float f;std::memcpy(&f,&s,4);return f;}();
        v.projection_words_at_3d0f9ec=bits(*v.view_at_3b280a0);v.projection_words_at_3d0f9f0=bits(*v.view_at_3b280a4);
        // 402ed0(0x31c7)
        if(need(v.fov_at_3aad0bc,0x402ed0)!=0x31c7U)call(0x402edf,0x7815c0,{0x31c7});
        v.fov_copy_at_3ab98ec=0x31c7;v.fov_at_3aad0bc=0x31c7;v.word_at_3b2cbb4=0x31c7;
        // Local {0,0,w*fov_x,h*fov_y} passed to the 78c5a0 RET: computed for the x87 state only.
        (void)product(v.screen_width_at_8928c0,0x420953,v.fov_x_at_7dd594,0x420959);
        (void)product(v.screen_height_at_8928c4,0x420972,v.fov_y_at_7dd590,0x420980);
        v.clear_colors_at_3ce7174={0xff000000U,0xff000000U,0xff000000U,0xff000000U}; // 402f10(0,0,0)
        q.queue_runtime.captured_word_at_3d0f9c8=0x36c;                              // 77ff60
        q.queue_runtime.captured_word_at_3d0f840=0xffffffffU;q.queue_runtime.captured_word_at_3d0f9c4=0x2000; // 77ff70(-1,0x2000)
        color(v.color_at_3b280ac,0x3b280ac);                                       // 402f40(&3b280ac)
        call(0x4209e1,0x403180,{0xbf800000U,0xc53b8000U});                         // fog range(-1.0f, -3000.0f)
        // 77ed50(1): no case for 1
        r.completed=true;
    }catch(Stop){}catch(const X87Error&){r.failure=GameViewFailure::fp_failed;r.source_boundary=site;}
    return r;
}
} // namespace sadx

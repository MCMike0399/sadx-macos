#include "guest/frame_control_reference.hpp"
#include "game/game_loop.hpp"
#include <algorithm>
#include <vector>
// Native 40cad0 phases with the same scripted owner fixtures as
// tools/verify_original_game_loop.py; owners are fixtures, not subsystems.
// Words: 0..21 initial globals, 22 fail-at-call, 24+i*10 per-iteration script.
extern "C" int sadx_native_game_loop(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=64||!out||capacity<512)return 0;
    sadx::GameLoopState st;std::optional<std::uint8_t> special=static_cast<std::uint8_t>(in[1]),fog=static_cast<std::uint8_t>(in[2]);
    st.exit_at_3abdf7c=in[0];st.special_flags_at_909eb4=&special;st.fog_flag_at_3abdcfe=&fog;
    st.fog_range_at_3abdca0=in[3];st.fog_range_at_3abdca4=in[4];st.fog_range_at_3abdc70=in[5];st.fog_range_at_3abdc74=in[6];
    st.request_at_3b291ad=static_cast<std::uint8_t>(in[7]);st.counter_at_3b291b2=static_cast<std::uint8_t>(in[8]);st.save_phase_at_3b291a4=static_cast<std::int32_t>(in[9]);
    st.pending_at_3abdf76=static_cast<std::uint8_t>(in[10]);st.pending_at_3abdf79=static_cast<std::uint8_t>(in[11]);st.word_at_3abdf80=in[12];st.mode_at_3b28114=in[13];
    st.frames_at_3abdf58=in[14];st.frames_at_3abdf5c=in[15];st.game_mode_at_3b22de4=static_cast<std::uint16_t>(in[16]);st.word_at_3c85804=in[17];st.focus_at_8929d4=in[18];
    st.word_at_3abdcf8=in[19];sadx::GameLogicTimingState timing;timing.word_at_3b11198=static_cast<std::int32_t>(in[20]);sadx::GameLogicState logic{timing,{},{}};
    std::vector<std::array<std::uint32_t,4>> trace;std::uint32_t iteration=0;
    sadx::GameLoopRuntime runtime{st,logic,{}};
    runtime.invoke=[&](const sadx::GameLoopCall& c)->sadx::GameLoopOwnerResult{
        trace.push_back({c.callee,c.source_call,c.arguments[0].value_or(0),c.arguments[1].value_or(0)});
        if(in[22]&&trace.size()==in[22])return {{sadx::GameLogicOwnerState::failed,0x123},{}};
        const auto* w=in+24+(iteration?std::min<std::uint32_t>(iteration-1,3):0)*10;
        switch(c.operation){
        case sadx::GameLoopOperation::message_77f090:
            ++iteration;w=in+24+std::min<std::uint32_t>(iteration-1,3)*10;
            st.focus_at_8929d4=iteration>=4?1U:(w[0]&1);special=static_cast<std::uint8_t>(w[0]>>1&3);fog=static_cast<std::uint8_t>(w[0]>>3&1);
            st.request_at_3b291ad=static_cast<std::uint8_t>(w[0]>>4&1);st.pending_at_3abdf79=static_cast<std::uint8_t>(w[0]>>5&1);st.word_at_3abdf80=w[0]>>6&1;
            st.save_phase_at_3b291a4=static_cast<std::int32_t>(w[1]);st.mode_at_3b28114=w[2];st.game_mode_at_3b22de4=static_cast<std::uint16_t>(w[3]);break;
        case sadx::GameLoopOperation::mode_4250d0:if(w[6])st.exit_at_3abdf7c=1;break;
        case sadx::GameLoopOperation::fog_411b60:st.fog_range_at_3abdca0=w[7];break;
        case sadx::GameLoopOperation::keys_40ef30:return {{sadx::GameLogicOwnerState::completed,0},iteration>=4?0x44U:w[4]};
        case sadx::GameLoopOperation::key_40ef20:return {{sadx::GameLogicOwnerState::completed,0},iteration>=4?1U:w[5]};
        default:break;
        }
        return {{sadx::GameLogicOwnerState::completed,0},{}};
    };
    std::uint32_t phase=0;sadx::GameLoopResult r=sadx::run_game_loop_startup(runtime);
    while(r&&r.next==sadx::GameLoopNext::iteration&&iteration<8){phase=1;r=sadx::run_game_loop_iteration(runtime);}
    if(r&&r.next==sadx::GameLoopNext::shutdown){phase=2;r=sadx::run_game_loop_shutdown(runtime);}
    std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.nested_source_boundary;out[3]=r.completed&&r.next==sadx::GameLoopNext::finished;out[4]=iteration;
    const std::uint32_t values[]={st.exit_at_3abdf7c.value_or(0xdeadbeef),special.value_or(0xef),fog.value_or(0xef),st.fog_range_at_3abdca0.value_or(0xdeadbeef),
        st.request_at_3b291ad.value_or(0xef),st.counter_at_3b291b2.value_or(0xef),static_cast<std::uint32_t>(st.save_phase_at_3b291a4.value_or(0)),st.pending_at_3abdf76.value_or(0xef),st.pending_at_3abdf79.value_or(0xef),
        st.mode_at_3b28114.value_or(0xdeadbeef),st.frames_at_3abdf58.value_or(0xdeadbeef),st.frames_at_3abdf5c.value_or(0xdeadbeef),st.word_at_3abdcf8.value_or(0xdeadbeef),static_cast<std::uint32_t>(timing.word_at_3b11198.value_or(static_cast<std::int32_t>(0xdeadbeef)))};
    std::copy(std::begin(values),std::end(values),out+5);
    out[19]=static_cast<std::uint32_t>(std::min<std::size_t>(trace.size(),120));
    for(std::size_t i=0;i<trace.size()&&i<120;++i)std::copy(trace[i].begin(),trace[i].end(),out+20+i*4);
    (void)phase;return 1;
}

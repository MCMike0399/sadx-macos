// Actual legacy loop routing compared to the native split, including completion.
#include "guest/frame_control_reference.hpp"
#include "guest/game_loop_reference.hpp"
#include "game/save_frame.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
namespace {
using Words = std::array<std::uint32_t,6>;
using Event = std::array<std::uint32_t,7>;
struct Fault {};
std::ostringstream output;
void print(const Event &row) { for (auto x : row) output << x << ','; output << '\n'; }
Words mutate(Words w,int id) {
    if (id==1) { w[3]=0; w[4]=1; w[2]=2; w[5]=2; }
    if (id==4) { w[0]=0; w[1]=static_cast<std::uint8_t>(w[1]+1); w[2]=0; w[3]=1; w[4]=1; w[5]=2; }
    if (id==3) { w[2]=0; w[3]=9; w[4]=1; w[5]=2; }
    if (id==2) { w[2]=1; w[3]=3; w[4]=7; w[5]=7; }
    return w;
}
Event event(int id,Words w) { return {static_cast<unsigned>(id),w[0],w[1],w[2],w[3],w[4],w[5]}; }
struct Native final : sadx::save::FrameCalls {
    std::uint8_t ready,count,exec,pending;
    std::int32_t phase;
    std::uint32_t pause;
    sadx::menu_caption::Context context;
    int mutation,fail;
    std::vector<Event> trace;
    Native(Words w,int m,int f):ready(w[0]),count(w[1]),exec(w[3]),pending(w[4]),phase(static_cast<std::int32_t>(w[2])),pause(w[5]),mutation(m),fail(f) {}
    Words words() const { return {ready,count,static_cast<std::uint32_t>(phase),exec,pending,pause}; }
    void store(Words w) { ready=w[0];count=w[1];phase=static_cast<std::int32_t>(w[2]);exec=w[3];pending=w[4];pause=w[5]; }
    sadx::save::FrameStateView view() { return {ready,count,phase,exec,pending,pause,context}; }
    void call(int id) {
        trace.push_back(event(id,words()));
        if (mutation==id || (mutation==5 && (id==4 || id==2))) store(mutate(words(),id));
        if (fail==id) throw Fault{};
    }
    void display_saving_message(sadx::menu_caption::Context &c) override { assert(&c==&context);call(1); }
    void close_saving_message(sadx::menu_caption::Context &c) override { assert(&c==&context);call(2); }
    void write_save_file() override { assert(exec==0);call(3); }
    bool run() {
        try { sadx::save::before_frame_completion(view(),*this);call(4);sadx::save::after_frame_completion(view(),*this);return true; }
        catch (const Fault &) { return false; }
    }
};
struct Reference {
    sadx::GameLoopState state;
    std::optional<std::uint8_t> special=0,fog=0;
    sadx::GameLogicTimingState timing;
    sadx::GameLogicState logic{timing,{},{}};
    sadx::GameLoopRuntime runtime{state,logic,{}};
    std::vector<Event> trace;
    int mutation,fail,missing;
    Reference(Words w,int m,int f,int u):mutation(m),fail(f),missing(u) {
        state.exit_at_3abdf7c=0;state.focus_at_8929d4=1;
        state.special_flags_at_909eb4=&special;state.fog_flag_at_3abdcfe=&fog;
        state.frame_message=0;state.frames_at_3abdf58=0;state.frames_at_3abdf5c=0;state.game_mode_at_3b22de4=4;
        store(w);
        runtime.invoke=[this](const sadx::GameLoopCall &c) {
            output << "route:" << static_cast<unsigned>(c.operation) << ',' << c.source_call << ',' << c.callee << '\n';
            using Op=sadx::GameLoopOperation;
            int id=0;
            if(c.operation==Op::save_40be40) id=1;
            if(c.operation==Op::pending_40e2c0) { id=2; assert(c.arguments[0]==0x3abdca8U); }
            if(c.operation==Op::work_421fd0) id=3;
            if(c.operation==Op::frame_411d00) id=4;
            if(id) {
                // Unknown metadata stays unknown; only known fixtures use this observation.
                if(!missing) trace.push_back(event(id,words()));
                if(mutation==id || (mutation==5 && (id==4 || id==2))) {
                    if(!missing) store(mutate(words(),id));
                    else erase();
                }
                if(fail==id) return sadx::GameLoopOwnerResult{{sadx::GameLogicOwnerState::failed,0x12345678},{}};
            }
            return sadx::GameLoopOwnerResult{{sadx::GameLogicOwnerState::completed,0},0};
        };
    }
    Words words() const { return {*state.request_at_3b291ad,*state.counter_at_3b291b2,static_cast<std::uint32_t>(*state.save_phase_at_3b291a4),*state.pending_at_3abdf76,*state.pending_at_3abdf79,*state.mode_at_3b28114}; }
    void store(Words w) { state.request_at_3b291ad=w[0];state.counter_at_3b291b2=w[1];state.save_phase_at_3b291a4=static_cast<std::int32_t>(w[2]);state.pending_at_3abdf76=w[3];state.pending_at_3abdf79=w[4];state.mode_at_3b28114=w[5]; }
    void erase() {
        switch(missing) {
        case 1:state.request_at_3b291ad.reset();break;
        case 2:state.counter_at_3b291b2.reset();break;
        case 3:state.save_phase_at_3b291a4.reset();break;
        case 4:state.pending_at_3abdf76.reset();break;
        case 5:state.pending_at_3abdf79.reset();break;
        case 6:state.mode_at_3b28114.reset();break;
        }
    }
    void snapshot_optional() {
        const auto show=[](const auto &v) { if(v) output << static_cast<std::uint32_t>(*v); else output << '?'; output << ','; };
        show(state.request_at_3b291ad);show(state.counter_at_3b291b2);show(state.save_phase_at_3b291a4);
        show(state.pending_at_3abdf76);show(state.pending_at_3abdf79);show(state.mode_at_3b28114);output << '\n';
    }
};
}
int main(int argc,char **argv) {
    const bool reserved=argc==3&&std::string(argv[1])=="--root-cohort";
    std::uint32_t seed=reserved?static_cast<std::uint32_t>(std::stoul(argv[2])):0;
    const auto next=[&]() { seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed; };
    unsigned cases=0;
    for (std::uint32_t ready : {0,1}) for (std::uint32_t count : {4,255})
    for (std::uint32_t phase : {0U,1U,0xffffffffU,2U}) for (std::uint32_t exec : {0,1})
    for (std::uint32_t pending : {0,1}) for (int mutation=0;mutation<=5;++mutation)
    for (int failure=0;failure<=4;++failure) {
        Words initial{ready,count,phase,exec,pending,2};
        if(reserved) { initial[0]=next()&255;initial[1]=next()&255;initial[3]=next()&255;initial[4]=next()&255;initial[5]=next()%4; }
        output << "case:" << cases++ << ',' << mutation << ',' << failure << '\n';print(event(0,initial));
        Native native(initial,mutation,failure);bool ok=native.run();
        Reference reference(initial,mutation,failure,0);auto result=sadx::run_game_loop_iteration(reference.runtime);
        assert(ok==bool(result));assert(native.trace==reference.trace);assert(native.words()==reference.words());
        output << "result:" << ok << ',' << static_cast<unsigned>(result.failure) << ',' << result.source_boundary << ',' << result.nested_source_boundary << '\n';
        for(const auto &row:native.trace)print(row);
        print(event(99,native.words()));
    }
    for (int missing=1;missing<=6;++missing) for (int at : {0,1,2,3,4}) {
        output << "missing:" << missing << ',' << at << '\n';
        Reference reference({1,255,1,1,1,2},at,0,missing);
        if(!at) reference.erase();
        auto result=sadx::run_game_loop_iteration(reference.runtime);
        output << "prefix:" << static_cast<unsigned>(result.failure) << ',' << result.source_boundary << ',' << result.nested_source_boundary << '\n';
        reference.snapshot_optional();
    }
    const auto text=output.str();
    if(reserved||(argc==2&&std::string(argv[1])=="--capture")) { std::cout << text;return 0; }
    std::uint64_t hash=14695981039346656037ULL;
    for(unsigned char c:text) { hash^=c;hash*=1099511628211ULL; }
    std::cout << cases << ' ' << text.size() << ' ' << hash << '\n';

    assert(text.size() == 948217U && hash == 8729241527267754014ULL);
    return 0;
}

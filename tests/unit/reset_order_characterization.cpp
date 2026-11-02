#include "game/game_reset.hpp"
#include "game/game_reset_leaves.hpp"
#include "game/game_cleanup.hpp"
#include "game/audio_playback.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
struct Fixture {
    std::optional<std::uint32_t> depth=8, queue=9, ready=0;
    std::optional<std::int32_t> timing=7;
    std::array<std::optional<std::uint32_t>,36> records{};
    sadx::ResetLeafState state{{},{},depth,queue,6,5,timing,0,records,{},{},ready,
                              10,11,1,12,13,14,1,1,15,{},16,17,18,{},{},{}};
    Fixture() {
        records.fill(99); state.sound_record_flags_at_3b29304.fill(2);
        state.words_at_3b29b90.fill(3);
        state.set_files_at_3abdf40.fill(0);state.cam_files_at_3abdf28.fill(0);
        state.entry_words_at_3d08428.fill(0);
    }
};
void emit(const char *name, const sadx::ResetLeafResult &r, Fixture &f,
          const std::vector<sadx::ResetLeafCall> &calls) {
    std::cout << name << ' ' << static_cast<int>(r.outcome) << ' ' << r.source_boundary
              << ' ' << r.eax.value_or(123) << ' ' << static_cast<int>(r.nested.state)
              << ' ' << r.nested.source_boundary;
    const auto word=[](const auto &v){ std::cout << ' ' << (v ? std::to_string(*v) : "unknown"); };
    for (const auto &v:f.records) word(v);
    for (const auto &v:f.state.sound_record_flags_at_3b29304) word(v);
    for (const auto &v:f.state.words_at_3b29b90) word(v);
    for (const auto &v:f.state.set_files_at_3abdf40) word(v);
    for (const auto &v:f.state.cam_files_at_3abdf28) word(v);
    word(f.state.word_at_3b11190); word(f.timing); word(f.state.callback_at_3b291b4);
    word(f.state.player_at_3abdf94); word(f.state.player_at_3abdf9c);
    word(f.state.word_at_3abdfa0); word(f.state.word_at_3abdf98); word(f.state.word_at_3abdfa8);
    word(f.state.word_at_3c4e458); word(f.state.word_at_3c4e45c);
    for (auto c:calls) std::cout << " call:" << c.source_call << ':' << c.callee << ':'
                               << c.this_object.value_or(0) << ':' << c.argument.value_or(0);
    std::cout << '\n';
}
}
int main() {
    for (unsigned n=0;n<14;++n) {
        Fixture f;std::vector<sadx::ResetLeafCall> calls;
        auto callee=0x424460U;std::optional<std::uint32_t> arg=0;
        if(n==0) f.state.sound_record_flags_at_3b29304[0].reset();
        if(n==1) f.ready.reset();
        if(n==2) f.ready=1;
        if(n==3) f.state.sound_record_flags_at_3b29304[1].reset();
        if(n>=4&&n<=5) {callee=0x421fb0;if(n==4) f.state.word_at_3c5e8b8.reset();}
        if(n>=6&&n<=8) {callee=0x40cff0;arg=n==8?1:0;}
        if(n>=9&&n<=12) {callee=0x4228f0;arg=n==12?5:0;
            f.state.callback_at_3b291b4=n==12?0x592370U:99;
            f.state.set_files_at_3abdf40[0]=42;f.state.set_files_at_3abdf40[1]=42;
            if(n==11) f.state.callback_at_3b291b4.reset();}
        if(n==13) {callee=0x785330;f.state.word_at_389d608.reset();}
        auto r=sadx::run_reset_leaf(callee,arg,f.state,[&](const auto &c){
            calls.push_back(c);
            if(n==6) f.state.player_at_3abdf9c=27;
            if(n==7) f.state.player_at_3abdf9c.reset();
            if(n==8) f.state.player_at_3abdf94=0;
            if(n==9) {f.state.set_files_at_3abdf40[1]=43;f.state.callback_at_3b291b4=98;}
            return sadx::GameLogicOwnerResult{n==10?sadx::GameLogicOwnerState::failed:
                                              sadx::GameLogicOwnerState::completed,0,{}};
        });
        emit(std::to_string(n).c_str(),r,f,calls);
    }
    for (unsigned n=0;n<40;++n) {
        std::optional<std::uint32_t> frame=7;sadx::CrtRandState rng;
        sadx::GameResetState s{frame,65535,n==0?std::optional<std::uint32_t>{}:42,42,rng};
        std::vector<sadx::GameResetCall> calls;
        sadx::GameResetRuntime rt{s,[&](const auto &c){
            calls.push_back(c);
            if(c.callee==0x780fb0&&n==1) s.heap_at_3b0e330=43;
            return sadx::GameLogicOwnerResult{calls.size()==(n==2?1:n==3?3:n==4?4:n==5?26:n==6?27:n>=10?n-9:999)?
                sadx::GameLogicOwnerState::failed:sadx::GameLogicOwnerState::completed,123,{}};
        }};
        if(n==7) rt.invoke={};
        if(n==8||n==9) rt.invoke=[&](const auto &c){calls.push_back(c);return sadx::GameLogicOwnerResult{
            n==8?sadx::GameLogicOwnerState::cancelled:sadx::GameLogicOwnerState::unavailable,456,{}};};
        const auto r=sadx::run_game_reset_40bf40(rt);
        std::cout<<"reset"<<n<<' '<<static_cast<int>(r.failure)<<' '<<r.source_boundary<<' '
                 <<r.nested_source_boundary<<' '<<frame.value_or(123)<<' '<<s.word_at_3b29d08.value_or(123)
                 <<' '<<s.heap_at_3b290d0.value_or(123)<<' '<<s.heap_at_3b0e330.value_or(123)
                 <<' '<<rng.seed_at_2bcbdf8.value_or(123)<<' '<<bool(r.pending_call);
        for(auto c:calls) std::cout<<" call:"<<c.source_call<<':'<<c.callee<<':'<<c.argument.value_or(0);
        std::cout<<'\n';
    }
    // POD native order has no unavailable flags. Callback observes both preceding stores.
    std::array<sadx::SoundRecordInitialValues,36> records{};
    std::array<sadx::TaskHandle,36> tasks{};
    sadx::cleanup::AudioQueueView audio{records,tasks};records[0].timer=99;records[0].flags=2;
    unsigned visits=0;
    sadx::cleanup::stop_all_sounds(audio,[&](auto){if(records[0].timer||records[0].flags!=18)throw std::runtime_error("order");++visits;});
    std::cout<<"native-sounds "<<visits<<'\n';
}

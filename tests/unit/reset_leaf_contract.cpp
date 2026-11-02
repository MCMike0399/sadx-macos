#if __has_include("guest/game_reset_leaves_reference.hpp")
#include "guest/game_reset_leaves_reference.hpp"
#else
#include "game/game_reset_leaves.hpp"
#endif
#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>
namespace {
struct Fixture {
    std::optional<std::uint32_t> depth=9, queue=8, ready=1;
    std::optional<std::int32_t> timing=7;
    std::array<std::optional<std::uint32_t>,36> records{};
    sadx::ResetLeafState state{5,6,depth,queue,4,3,timing,0,records,{},{},ready,
                              10,11,12,13,14,15,1,1,16,{},17,18,19,{},{},{}};
    Fixture() { state.entry_words_at_3d08428.fill(0); }
};
void word(const std::optional<std::uint32_t> &v) {
    std::cout << ' ' << bool(v) << ':' << v.value_or(0);
}
}
int main() {
    using namespace sadx;
    for (unsigned n=0;n<8;++n) {
        Fixture f;
        if(n==0) f.ready=0;
        if(n==1) f.ready.reset();
        if(n==2) f.state.config_at_3a7cf3c.reset();
        if(n==3) f.state.config_at_38f6ec0.reset();
        std::vector<ResetLeafCall> calls;
        const auto result=run_reset_leaf(0x4253c0,{},f.state,[&](const auto &call) {
            calls.push_back(call);
            if(n==4) f.ready.reset();
            if(n==5) f.state.config_at_38f6ec0=0;
            return GameLogicOwnerResult{n==6?GameLogicOwnerState::failed:GameLogicOwnerState::completed,123,{}};
        });
        if(n==7) {
            assert(result.outcome==ResetLeafOutcome::completed);
            assert(!f.state.player_at_3abdf94 && !f.state.player_at_3abdf9c);
            assert(f.state.word_at_3abdfa0==0 && f.state.word_at_3abdfa4==0 &&
                   f.state.word_at_3abdf98==0 && f.state.word_at_3abdfa8==0);
        }
        std::cout << "init " << n << ' ' << static_cast<unsigned>(result.outcome) << ' '
                  << result.source_boundary << ' ' << result.nested.source_boundary;
        for(auto *v:{&f.state.player_at_3abdf94,&f.state.player_at_3abdf9c,&f.state.word_at_3abdfa0,
                     &f.state.word_at_3abdfa4,&f.state.word_at_3abdf98,&f.state.word_at_3abdfa8})word(*v);
        for(const auto &c:calls)std::cout << " call:" << c.source_call << ':' << c.callee << ':'
                                       << bool(c.argument) << ':' << c.argument.value_or(0);
        std::cout << '\n';
    }
    for(std::uint32_t entry:{0U,1U,2U,3U,4294967295U}) {
        Fixture f;f.state.entry_words_at_3d08428[0]=entry;
        const auto r=run_reset_leaf(0x785330,4294967295U,f.state,{});
        assert(r.outcome==ResetLeafOutcome::completed);
        std::cout << "vibration " << entry << ' ' << r.eax.value_or(123);
        word(f.state.entry_words_at_3d08428[0]);std::cout << '\n';
    }
    Fixture f;
    assert(reset_leaf_print_debug_401000("literal\n",f.state));
    assert(!reset_leaf_print_debug_401000("%d",f.state));
    std::cout << "debug " << std::quoted(f.state.debug_text_at_38f6d08) << '\n';
    const auto missing=run_reset_leaf(0x401410,{},f.state,{});
    assert(missing.outcome==ResetLeafOutcome::unknown_state && f.depth==9);
    const auto written=run_reset_leaf(0x401410,4294967295U,f.state,{});
    assert(written.outcome==ResetLeafOutcome::completed && f.depth==4294967295U);
}

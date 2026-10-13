#include "game/game_reset.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    {sadx::CrtRandState rng{1};require(sadx::crt_rand_6443bf(rng)==41u&&rng.seed_at_2bcbdf8==0x00269ec3u+0x343fdu,"MSVC rand(1) first value 41");
     sadx::crt_srand_6443b5(rng,0xdead0cab);const auto a=*sadx::crt_rand_6443bf(rng);sadx::crt_srand_6443b5(rng,0xdead0cab);require(sadx::crt_rand_6443bf(rng)==a,"reseed repeats the sequence");
     sadx::CrtRandState unknown;require(!sadx::crt_rand_6443bf(unknown)&&!unknown.seed_at_2bcbdf8,"unknown seed stores nothing");}
    {std::optional<std::uint32_t> w80=5;sadx::CrtRandState rng{7};sadx::GameResetState s{w80,9,0x1000,0,rng};std::vector<sadx::GameResetCall> calls;
     sadx::GameResetRuntime rt{s,[&](const sadx::GameResetCall& c){calls.push_back(c);return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0,{}};}};
     const auto r=sadx::run_game_reset_40bf40(rt);
     require(r&&w80==0u&&s.word_at_3b29d08==0&&s.heap_at_3b290d0==0u&&rng.seed_at_2bcbdf8==0xdead0cabu&&calls.size()==27&&calls[25].callee==0x780fb0&&calls[25].argument==0x1000u,"reset stores, one free, reseed");}
    {std::optional<std::uint32_t> w80=5;sadx::CrtRandState rng{7};sadx::GameResetState s{w80,9,{},0,rng};sadx::GameResetRuntime rt{s,[](const auto&){return sadx::GameLogicOwnerResult{sadx::GameLogicOwnerState::completed,0,{}};}};
     const auto r=sadx::run_game_reset_40bf40(rt);require(r.failure==sadx::GameResetFailure::unknown_state&&r.source_boundary==0x40bfb9&&rng.seed_at_2bcbdf8==7u,"unknown heap pointer stops before the reseed");}
    std::cout<<"Game reset: CRT rand/srand and 40bf40 stores, frees, reseed and boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

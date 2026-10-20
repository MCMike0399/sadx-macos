#include "guest/game_library_init_reference.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
// Owner semantics the oracle does not compare: unknown config/viewport facts,
// partial stores before a stop, the retry exhaustion path and the success path.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    GameLibraryInitState s;X87Context fp(0x7f);std::vector<std::uint32_t> calls;std::uint32_t result=0;
    GameLibraryInitRuntime rt{s,0x892940,[](std::uint32_t a)->std::optional<std::uint32_t>{return a==0x89298c?0u:a;},
        [](std::uint32_t a)->std::optional<std::uint32_t>{return a==0x3d12788?640u:480u;},fp,
        [&](const GameLibraryCall& c)->GameLibraryOwnerResult{calls.push_back(c.callee);return {{GameLogicOwnerState::completed,0,{}},c.callee==0x788000?std::optional<std::uint32_t>{result}:std::nullopt};}};
    auto r=run_library_init_781070(rt);
    require(r&&r.tries_left==8u&&s.aspect_at_3d0f9c0==0.75F&&s.scale_at_3d0f800==1.0F&&s.words_at_389d864[0]==0x892968u,"success on the first 788000 try");
    require(calls.front()==0x793ef0&&calls.back()==0x789c50&&std::count(calls.begin(),calls.end(),0x788000)==1,"no 794050 when +0x4c is 0");
    result=0xffffffe9u;calls.clear();r=run_library_init_781070(rt);
    require(r&&r.tries_left==0u&&std::count(calls.begin(),calls.end(),0x788000)==8&&std::count(calls.begin(),calls.end(),0x7881a0)==9,"eight failing tries, eight cleanups plus the final one");
    rt.read_viewport=[](std::uint32_t)->std::optional<std::uint32_t>{return std::nullopt;};s.scale_at_3d0f800.reset();result=0;
    r=run_library_init_781070(rt);require(r.required_fact==0x3d1278cu&&!s.scale_at_3d0f800,"unknown height stops before the 3d0f800 store");
    std::cout<<"Library init 781070: success path, retry exhaustion with cleanups, unknown viewport and store order passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

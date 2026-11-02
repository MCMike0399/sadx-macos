#include "game/game_reset.hpp"

namespace sadx {
namespace {struct Stop{};}
GameResetResult run_game_reset_40bf40(GameResetRuntime& rt){
    GameResetResult r;auto& s=rt.state;
    const auto fail=[&](GameResetFailure f,std::uint32_t at){r.failure=f;r.source_boundary=at;throw Stop{};};
    const auto call=[&](std::uint32_t site,std::uint32_t callee,std::optional<std::uint32_t> argument={}){
        r.pending_call=GameResetCall{site,callee,argument};
        if(!rt.invoke)fail(GameResetFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);r.nested_source_boundary=receipt.source_boundary;
        if(receipt.state!=GameLogicOwnerState::completed)fail(receipt.state==GameLogicOwnerState::cancelled?GameResetFailure::cancelled:
            receipt.state==GameLogicOwnerState::failed?GameResetFailure::owner_failed:GameResetFailure::missing_owner,site);
        r.pending_call.reset();r.nested_source_boundary=0;
    };
    const auto release=[&](std::optional<std::uint32_t>& pointer,std::uint32_t load,std::uint32_t site){
        if(!pointer)fail(GameResetFailure::unknown_state,load);
        if(*pointer!=0){call(site,0x780fb0,*pointer);pointer=0;}
    };
    try{
        call(0x40bf42,0x40cd10);call(0x40bf49,0x401410,1);call(0x40bf51,0x785330,0);
        s.word_at_3abdf80=0;s.word_at_3b29d08=0;
        call(0x40bf63,0x403e60);call(0x40bf68,0x4257c0);call(0x40bf6d,0x4257e0);call(0x40bf72,0x424460);call(0x40bf79,0x410100,0xffffffffU);
        call(0x40bf7f,0x40cff0,0);call(0x40bf86,0x40cff0,1);call(0x40bf8b,0x4208e0);call(0x40bf91,0x40d490,0);call(0x40bf99,0x40b1a0);call(0x40bf9e,0x40b460);
        for(std::uint32_t i=0;i<=9;++i)call(0x40bfa6,0x4228f0,i);
        call(0x40bfb4,0x421fb0);
        release(s.heap_at_3b290d0,0x40bfb9,0x40bfc3);release(s.heap_at_3b0e330,0x40bfd1,0x40bfdb);
        // 51f680 x4 (-1, 0xff12, 3abdd00, 3abdd00) is a verified RET.
        call(0x40c00e,0x411af0);
        crt_srand_6443b5(s.rng,0xdead0cabU);
        r.completed=true;
    }catch(Stop){}
    return r;
}
} // namespace sadx

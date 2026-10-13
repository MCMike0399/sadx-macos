#include "game/game_winmain.hpp"
#include "game/win32_profile.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
// Owner semantics the oracle does not compare: the profile composition over the
// pinned sonicDX.ini, unknown facts, a missing profile owner and a null
// ImmDisableIME procedure.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(){try{
    const std::string ini="[sonicDX]\r\nframerate=1\r\nfogemulation=1\r\nsound3d=1\r\nscreensize=2\r\ncliplevel=0\r\nsevoice=1\r\nbgm=1\r\nscreen=0\r\nmousemode=0\r\nbgmv=30\r\nvoicev=70\r\n";
    const Win32Profile profile(std::vector<std::uint8_t>(ini.begin(),ini.end()));
    GameWinMainState s;s.media_at_3ab918c=1;s.media_at_3ab9190=1;s.message_text_at_8929b0=0;s.message_caption_at_8929c4=0;
    for(auto& w:s.pad_bits_at_8929d8)w=0;
    std::optional<std::int32_t> render=3;std::optional<std::uint8_t> flags=0;std::optional<std::uint32_t> sevoice=0,voice=0,bgm=0,voice_copy=0;
    std::vector<std::uint32_t> calls;
    GameWinMainRuntime rt{s,{render,flags,sevoice,voice,bgm,voice_copy},{1,1},
        [&](const WinMainProfileRead& r)->std::optional<std::int32_t>{return profile.get_int(r.section,r.key,r.fallback);},
        [&](const WinMainProfileRead& r)->std::optional<std::string>{return profile.get_string(r.section,r.key,r.string_fallback,r.size);},
        [&](const WinMainCall& c)->WinMainOwnerResult{calls.push_back(c.callee);return {{GameLogicOwnerState::completed,0,{}},c.callee==0x7db040?std::optional<std::uint32_t>{0x10000000}:c.callee==0x7db044?std::optional<std::uint32_t>{0x20000000}:std::nullopt};}};
    require(bool(run_winmain_settings(rt)),"settings span over the profile");
    require(render==1&&flags==1&&s.clip_level_at_3abdcf0==0u&&sevoice==1u&&bgm==1u&&s.screen_at_892950==1u,"framerate 1, fog emulation, cliplevel 0, sevoice/bgm on, screen 0");
    require(s.sound3d_at_3b0ef28==1u&&voice_copy==voice&&s.mouse_mode_at_3b0eae0==0u,"40ff20, 40ccf0 and 40e8a0");
    require(voice==static_cast<std::uint32_t>((70*97/100-100)*100)&&s.bgm_volume_at_909f28==static_cast<std::uint32_t>((30*89/100-100)*100),"volume conversions");
    require(calls==std::vector<std::uint32_t>{0x7db040,0x7db044,0x20000000,0x401a20},"nested owners in order (no MessageBoxA, no ShowCursor)");
    s.media_at_3ab918c.reset();require(run_winmain_settings(rt).required_fact==0x3ab918cu,"unknown media object member");s.media_at_3ab918c=1;
    rt.profile_int=nullptr;require(run_winmain_settings(rt).failure==GameWinMainFailure::profile_unavailable,"no profile owner");
    rt.profile_int=[&](const WinMainProfileRead& r)->std::optional<std::int32_t>{return profile.get_int(r.section,r.key,r.fallback);};
    rt.invoke=[&](const WinMainCall& c)->WinMainOwnerResult{return {{GameLogicOwnerState::completed,0,{}},c.callee==0x7db040?std::optional<std::uint32_t>{1}:std::optional<std::uint32_t>{0}};};
    require(run_winmain_settings(rt).source_boundary==0x4025e8u,"a null ImmDisableIME pointer faults at the call");
    std::cout<<"WinMain settings: profile composition, volumes, nested owners, unknown facts and the null procedure passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

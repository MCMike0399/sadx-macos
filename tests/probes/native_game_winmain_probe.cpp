#include "game/game_winmain.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <string>
#include <vector>
// Native WinMain settings span with the layout of tools/verify_original_game_winmain.py.
// in: [0] fail-at nested call [1] LoadLibraryA result [2] GetProcAddress result [3] ShowCursor start count
//     [4] voice language [5] text language [6] 909eb4 [7] 38f6ec0 [8] 3abdfac [9] 3a7cf3c [10] 89295c
//     [11] 3ab918c [12] 3ab9190 [13] 8929b0 [14] 8929c4 [15..31] 8929d8 table [32..44] 389d634 words
//     [45] 3b0ef38 [46] 3b0f0e8 [47] 3abdcf0 [48] 909f28 [49] 892950 [50] 3b0ef28 [51] 3abdf90 [52] 3b0eae0
// ini: "section\1key\1value\n" lines (the scripted profile; absent keys return the fallback).
// out: [0] completed [1] imm32 module [6..14] the in[6..14] words after [32..44] words [45..49]
//      [60] nested count [61..] nested (callee, site, a0..a3) x 24; trace: "site|section|key|fallback\n" lines.
namespace {
std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
}
extern "C" int sadx_native_game_winmain(const std::uint32_t* in,const char* ini,std::uint32_t* out,char* trace,std::size_t trace_size){
    using namespace sadx;
    std::map<std::pair<std::string,std::string>,std::string> profile;
    {std::string line;for(const char* p=ini;;++p){if(*p=='\n'||!*p){const auto a=line.find('\1'),b=line.find('\1',a+1);
        if(a!=std::string::npos&&b!=std::string::npos)profile[{lower(line.substr(0,a)),lower(line.substr(a+1,b-a-1))}]=line.substr(b+1);line.clear();if(!*p)break;}else line.push_back(*p);}}
    GameWinMainState s;
    for(int i=0;i<17;++i)s.pad_bits_at_8929d8[i]=static_cast<std::int32_t>(in[15+i]);
    for(int i=0;i<13;++i)s.pad_buttons_at_389d634[i]=static_cast<std::uint16_t>(in[32+i]);
    s.voice_language_at_3b0ef38=in[45];s.text_language_at_3b0f0e8=in[46];s.clip_level_at_3abdcf0=in[47];s.bgm_volume_at_909f28=in[48];s.screen_at_892950=in[49];
    s.media_at_3ab918c=in[11];s.media_at_3ab9190=in[12];s.message_text_at_8929b0=in[13];s.message_caption_at_8929c4=in[14];
    std::optional<std::int32_t> render=static_cast<std::int32_t>(in[10]);std::optional<std::uint8_t> flags=static_cast<std::uint8_t>(in[6]);
    std::optional<std::uint32_t> sevoice=in[7],voice=in[8],bgm=in[9],voice_copy=in[51];s.sound3d_at_3b0ef28=in[50];s.mouse_mode_at_3b0eae0=in[52];
    std::string text;std::vector<std::array<std::uint32_t,6>> nested;std::int32_t cursor=static_cast<std::int32_t>(in[3]);
    GameWinMainRuntime rt{s,{render,flags,sevoice,voice,bgm,voice_copy},{static_cast<std::int32_t>(in[4]),static_cast<std::int32_t>(in[5])},
        [&](const WinMainProfileRead& r)->std::optional<std::int32_t>{
            text+=std::to_string(r.source_call)+"|"+r.section+"|"+r.key+"|"+std::to_string(r.fallback)+"\n";
            const auto found=profile.find({lower(r.section),lower(r.key)});if(found==profile.end())return r.fallback;
            return static_cast<std::int32_t>(std::stol(found->second));},
        [&](const WinMainProfileRead& r)->std::optional<std::string>{
            text+=std::to_string(r.source_call)+"|"+r.section+"|"+r.key+"|"+r.string_fallback+"\n";
            const auto found=profile.find({lower(r.section),lower(r.key)});auto v=found==profile.end()?r.string_fallback:found->second;
            if(v.size()>r.size-1)v.resize(r.size-1);return v;},
        [&](const WinMainCall& c)->WinMainOwnerResult{
            nested.push_back({c.callee,c.source_call,c.arguments[0].value_or(0xdeadbeef),c.arguments[1].value_or(0xdeadbeef),c.arguments[2].value_or(0xdeadbeef),c.arguments[3].value_or(0xdeadbeef)});
            if(in[0]&&nested.size()==in[0])return {{GameLogicOwnerState::unavailable,c.callee,{}},std::nullopt};
            std::optional<std::uint32_t> eax;
            if(c.callee==0x7db040)eax=in[1];else if(c.callee==0x7db044)eax=in[2];else if(c.callee==0x7db1dc){--cursor;eax=static_cast<std::uint32_t>(cursor);}
            return {{GameLogicOwnerState::completed,0,{}},eax};}};
    const auto r=run_winmain_settings(rt);
    std::fill(out,out+256,0);out[0]=r?1U:0U;out[1]=r.imm32_module.value_or(0xdeadbeef);
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    out[6]=v(flags);out[7]=v(sevoice);out[8]=v(voice);out[9]=v(bgm);out[10]=static_cast<std::uint32_t>(render.value_or(0x7eadbeef));
    for(int i=0;i<13;++i)out[32+i]=v(s.pad_buttons_at_389d634[i]);
    out[45]=v(s.voice_language_at_3b0ef38);out[46]=v(s.text_language_at_3b0f0e8);out[47]=v(s.clip_level_at_3abdcf0);out[48]=v(s.bgm_volume_at_909f28);out[49]=v(s.screen_at_892950);out[50]=v(s.sound3d_at_3b0ef28);out[51]=v(voice_copy);out[52]=v(s.mouse_mode_at_3b0eae0);
    out[60]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<24;++i)std::copy(nested[i].begin(),nested[i].end(),out+61+6*i);
    std::snprintf(trace,trace_size,"%s",text.c_str());
    return 1;
}

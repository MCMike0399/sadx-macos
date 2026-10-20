#include "guest/game_library_init_reference.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 781070 with the layout of tools/verify_original_library_init.py.
// in: [0] fail-at nested call [1] CW [2..24] 892940..89299b [25] 3d12788 [26] 3d1278c [27..34] 788000 results per try
//     [35..48] initial 389d864..884, 38a5dc4/c8/cc, 3d0f800, 3d0f9c0
// out: [0] completed [1] tries left [2..10] 389d864.. [11..13] 38a5dc4/c8/cc [14] 3d0f800 [15] 3d0f9c0
//      [20] nested count [21..] nested (callee, site, a0..a3) x 40
extern "C" int sadx_native_library_init(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    GameLibraryInitState s;X87Context fp(static_cast<std::uint16_t>(in[1]));
    const auto as_float=[](std::uint32_t b){float x;std::memcpy(&x,&b,4);return x;};
    for(int i=0;i<9;++i)s.words_at_389d864[i]=in[35+i];
    s.word_at_38a5dc4=in[44];s.word_at_38a5dc8=in[45];s.word_at_38a5dcc=in[46];s.scale_at_3d0f800=as_float(in[47]);s.aspect_at_3d0f9c0=as_float(in[48]);
    std::vector<std::array<std::uint32_t,6>> nested;std::uint32_t tries=0;
    GameLibraryInitRuntime rt{s,0x892940,
        [&](std::uint32_t a)->std::optional<std::uint32_t>{if(a<0x892940||a>=0x89299c)return std::nullopt;return in[2+(a-0x892940)/4];},
        [&](std::uint32_t a)->std::optional<std::uint32_t>{if(a==0x3d12788)return in[25];if(a==0x3d1278c)return in[26];return std::nullopt;},fp,
        [&](const GameLibraryCall& c)->GameLibraryOwnerResult{
            nested.push_back({c.callee,c.source_call,c.arguments[0].value_or(0xdeadbeef),c.arguments[1].value_or(0xdeadbeef),c.arguments[2].value_or(0xdeadbeef),c.arguments[3].value_or(0xdeadbeef)});
            if(in[0]&&nested.size()==in[0])return {{GameLogicOwnerState::unavailable,c.callee,{}},std::nullopt};
            std::optional<std::uint32_t> eax;if(c.callee==0x788000)eax=in[27+(tries++<8?tries-1:7)];
            return {{GameLogicOwnerState::completed,0,{}},eax};}};
    const auto r=run_library_init_781070(rt);
    std::fill(out,out+300,0);out[0]=r?1U:0U;out[1]=r.tries_left;
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    for(int i=0;i<9;++i)out[2+i]=v(s.words_at_389d864[i]);
    out[11]=v(s.word_at_38a5dc4);out[12]=v(s.word_at_38a5dc8);out[13]=v(s.word_at_38a5dcc);
    const auto f=[](const std::optional<float>& x){std::uint32_t b=0xdeadbeef;if(x)std::memcpy(&b,&*x,4);return b;};
    out[14]=f(s.scale_at_3d0f800);out[15]=f(s.aspect_at_3d0f9c0);
    out[20]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<40;++i)std::copy(nested[i].begin(),nested[i].end(),out+21+6*i);
    return 1;
}

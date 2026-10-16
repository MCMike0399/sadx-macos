#include "guest/game_mode_reference.hpp"

namespace sadx {
namespace {
struct Stop{};
std::int32_t s32(std::uint32_t v){return static_cast<std::int32_t>(v);}
std::int32_t s16(std::uint32_t v){return static_cast<std::int16_t>(static_cast<std::uint16_t>(v));} // MOVSX AX
}
// Labels carry the source address of the instruction they stand for.
GameModeResult run_game_mode_40c090(GameModeRuntime& rt){
    GameModeResult result;auto& st=rt.state;auto& loop=rt.loop;auto& mode=rt.mode_at_3abdc7c;
    const auto fail=[&](GameModeFailure f,std::uint32_t at)->void{result.failure=f;result.source_boundary=at;throw Stop{};};
    const auto need=[&](const auto& v,std::uint32_t at){if(!v)fail(GameModeFailure::unknown_state,at);return *v;};
    const auto call=[&](std::uint32_t site,std::uint32_t callee,std::optional<std::uint32_t> a={},std::optional<std::uint32_t> b={},std::optional<std::uint32_t> c={},bool returns=false)->std::uint32_t{
        result.pending_call=GameModeCall{site,callee,{a,b,c}};
        if(!rt.invoke)fail(GameModeFailure::missing_owner,site);
        const auto receipt=rt.invoke(*result.pending_call);result.nested_source_boundary=receipt.owner.source_boundary;
        if(receipt.owner.state!=GameLogicOwnerState::completed)
            fail(receipt.owner.state==GameLogicOwnerState::cancelled?GameModeFailure::cancelled:
                 receipt.owner.state==GameLogicOwnerState::failed?GameModeFailure::owner_failed:GameModeFailure::missing_owner,site);
        if(returns&&!receipt.return_value)fail(GameModeFailure::missing_return,site);
        result.pending_call.reset();result.nested_source_boundary=0;
        return receipt.return_value.value_or(0);
    };
    const auto ret=[&](std::uint32_t site,std::uint32_t callee,std::optional<std::uint32_t> a={},std::optional<std::uint32_t> b={}){return call(site,callee,a,b,{},true);};
    const auto level=[&](std::uint32_t dereference){return static_cast<std::uint32_t>(static_cast<std::int32_t>(need(st.level_word_via_3b0ef4c,dereference)));};
    std::uint32_t r=0,x=0,m=0;
    try{
        m=static_cast<std::uint32_t>(need(mode,0x40c091));
        if(m>0x1a)goto tail; // JA: unsigned
        switch(m){
        case 0x00:
            call(0x40c0b4,0x415000);call(0x40c0b9,0x42cea0);
            if(ret(0x40c0be,0x421f70)!=0)goto L106;
            if(s32(ret(0x40c0c7,0x421f80))>=1)goto L106;
            mode=0xd;goto tail;
        case 0x0d:
            call(0x40c0e6,0x4221e0,0x7dcc20,0);call(0x40c0ee,0x4f88a0); // EAX only reaches the dead local
            [[fallthrough]];
        case 0x0e:
            mode=0x17;goto tail;
        case 0x17:
            goto L106;
        case 0x18:
            call(0x40c115,0x42c7b0);mode=1;
            [[fallthrough]];
        case 0x01:
            r=ret(0x40c124,0x42cc70);
            if(r==1){call(0x40c130,0x411e30,1,1);goto L3bc;}
            if(r!=3)goto tail;
            call(0x40c14a,0x411e30,1,1);st.word_at_3b2c45c=0;mode=8;goto tail;
        case 0x02:
            loop.mode_at_3b28114=0;r=ret(0x40c16d,0x42e890);
            if(r==1){loop.mode_at_3b28114=1;call(0x40c17d,0x4212e0,0);mode=ret(0x40c185,0x413c00)==1?5:4;}
            else if(r==0xffffffffU){mode=3;call(0x40c1bd,0x42f590);}
            call(r==1?0x40c19b:0x40c1c2,0x40fdc0);call(r==1?0x40c1a0:0x40c1c7,0x40ef40);goto tail;
        case 0x03:
            if(s16(ret(0x40c1d1,0x42f240))!=1)goto tail;
            goto L67d;
        case 0x04:
            if(need(st.byte_at_3abdf74,0x40c1e7)!=0){
                if(need(loop.save_phase_at_3b291a4,0x40c1ef)!=0)goto tail;
                call(0x40c1fb,0x40bf30);call(0x40c200,0x40bf40);mode=0x15;st.byte_at_3abdf74=0;goto tail;
            }
            r=static_cast<std::uint32_t>(s16(ret(0x40c21a,0x416610)));
            if(s32(r)==-2)goto L3a8;
            if(r==1){mode=5;goto tail;}
            if(s32(r)==-1){call(0x40c244,0x40bf30);call(0x40c249,0x40bf40);call(0x40c252,0x42f080);x=ret(0x40c257,0x42caa0);
                if(x==0)goto L3bc;mode=s32(x);goto tail;}
            if(r==10)st.byte_at_3abdf74=1;
            goto tail;
        case 0x05:
            r=static_cast<std::uint32_t>(s16(ret(0x40c279,0x4173b0)));
            if(s32(r)==-2)goto L3a8;
            if(r==2){mode=4;goto tail;}
            if(s32(r)==-1){call(0x40c2a3,0x40bf30);call(0x40c2a8,0x40bf40);x=ret(0x40c2b1,0x42caa0);
                if(x==0)goto L3bc;mode=s32(x);goto tail;}
            if(r!=10)goto tail;
            call(0x40c2d0,0x40bf30);call(0x40c2d5,0x40bf40);mode=0x15;goto tail;
        case 0x08:
            if(ret(0x40c2ef,0x42f020,need(st.word_at_3b2c45c,0x40c2e9))!=0)goto L3ae;
            call(0x40c2ff,0x40bf30);call(0x40c304,0x40bf40);loop.mode_at_3b28114=1;goto L67d;
        case 0x09:
            r=static_cast<std::uint32_t>(s16(ret(0x40c318,0x416610)));
            if(s32(r)==-2){loop.request_at_3b291ad=0;call(0x40c32b,0x40bf30);call(0x40c330,0x40bf40);call(0x40c33b,0x40ef70,2);goto L3bc;}
            if(r==1){call(0x40c350,0x40ef70,2);mode=0xb;st.byte_at_3abdcfc=2;goto tail;}
            if(s32(r)!=-1)goto tail;
            mode=0xb;st.byte_at_3abdcfc=2;call(0x40c38e,0x40ef70,2);goto tail;
        case 0x0a:
            r=static_cast<std::uint32_t>(s16(ret(0x40c39b,0x415af0)));
            if(s32(r)==-2)goto L3a8;
            if(r==1){mode=10;goto tail;}
            if(s32(r)!=-1)goto L880;
            call(0x40c3ef,0x40bf30);call(0x40c3f4,0x40bf40);mode=0xb;st.byte_at_3abdcfc=1;call(0x40c410,0x40ef70,2);goto tail;
        case 0x0b:
            if(need(st.byte_at_3abdcfd,0x40c439)==2)goto L4cd;
            {static constexpr std::array<std::array<std::uint32_t,2>,7> stages{{{0x40c448,4},{0x40c44e,0},{0x40c455,5},{0x40c45c,7},{0x40c463,6},{0x40c46a,8},{0x40c471,9}}};for(const auto& e:stages)call(e[0],0x4228f0,e[1]);}
            x=need(loop.word_at_3abdcf8,0x40c476);
            if(x!=0){call(0x40c483,0x780fb0,x);loop.word_at_3abdcf8=0;}
            call(0x40c491,0x420f40);call(0x40c496,0x420ef0);call(0x40c49b,0x406f00);call(0x40c4a0,0x5034a0);call(0x40c4ab,0x4221e0,0x7dcc10,0);
            if(need(st.byte_at_3abdcfd,0x40c4b0)!=1)goto L4cd;
            call(0x40c4bc,0x5033f0);st.byte_at_3abdcfd=2;goto tail;
        case 0x0c:
            goto L51a;
        case 0x11:
            call(0x40c6a3,0x4221e0,0x7dcc04,0);
            if(ret(0x40c6a9,0x642640,0)!=0xffffffffU){mode=need(mode,0x40c733)+1;goto L739;}
            mode=5;call(0x40c6c4,0x427ee0);x=ret(0x40c6c9,0x4144e0);call(0x40c6d0,0x42d770);
            if(x==3&&ret(0x40c6dc,0x412d20,0x4a)==0&&s32(ret(0x40c6ea,0x42f020,5))<0)goto L93c;
            call(0x40c6fa,0x412ac0);call(0x40c700,0x414470,x);call(0x40c706,0x413380,x);call(0x40c716,0x4133e0,level(0x40c711));
            loop.mode_at_3b28114=1;call(0x40c726,0x4212e0,0);goto tail;
        case 0x12:
            goto L739;
        case 0x13:
            call(0x40c7c5,0x4221e0,0x7dcc04,0);call(0x40c7cb,0x642170,0);mode=need(mode,0x40c7d0)+1;
            [[fallthrough]];
        case 0x14:
            r=ret(0x40c7df,0x641c60,0);
            if(r==1){
                if(need(st.byte_at_3b2a2fa,0x40c7f0)!=2){mode=0x11;goto tail;}
                call(0x40c808,0x42c630);loop.mode_at_3b28114=1;x=ret(0x40c817,0x4144e0);call(0x40c81e,0x42d770);call(0x40c823,0x412ac0);
                call(0x40c829,0x414470,x);call(0x40c82f,0x413380,x);call(0x40c83e,0x4133e0,level(0x40c839));goto L653;
            }
            if(r==0xffffffffU){st.byte_at_3abdcfc=1;st.byte_at_3abdcfd=1;goto L951;}
            goto L8a3;
        case 0x15:
            call(0x40c8b7,0x4221e0,0x7dcc04,0);call(0x40c8bd,0x641a80,0);m=static_cast<std::uint32_t>(need(mode,0x40c8c2));
            loop.mode_at_3b28114=0;mode=s32(m+1);call(0x40c8d9,0x411e30,1,1);call(0x40c8e0,0x77ed40,1);
            [[fallthrough]];
        case 0x16:
            r=ret(0x40c8e9,0x640f20,0);
            if(s32(r)==-2){call(0x40c934,0x411e30,1,1);goto L93c;}
            if(s32(r)==-1){call(0x40c915,0x411e30,1,1);mode=0xb;st.byte_at_3abdcfc=1;goto tail;}
            if(r==1){call(0x40c900,0x42c730);mode=1;}
            goto tail;
        case 0x1a:
            call(0x40c42c,0x402f10,0xff000022U,0xff660000U,0xff000000U);goto L3b3;
        default: // 6, 7, 0xf, 0x10, 0x19
            goto tail;
        }
    L106:
        mode=0x18;goto tail;
    L3a8:
        loop.request_at_3b291ad=0;
    L3ae:
        call(0x40c3ae,0x40bf30);
    L3b3:
        call(0x40c3b3,0x40bf40);
    L3bc:
        call(0x40c3bc,0x42ca10);mode=0xb;st.byte_at_3abdcfc=0;goto tail;
    L4cd:
        call(0x40c4d2,0x5032e0,0x3abdcfc); // EAX only reaches the dead local
        call(0x40c4e1,0x77ed40,1);mode=need(mode,0x40c4e6)+1;st.byte_at_3b18db4=0;call(0x40c4fc,0x424c80);
        if(need(st.byte_at_3abdcfd,0x40c501)==2){st.byte_at_3abdcfd=0;goto tail;}
    L51a:
        r=ret(0x40c51f,0x5033b0,0x3abdcfc);
        if(r+2U>0x6dU)goto tail;
        switch(s32(r)){
        case -2:goto L93c;
        case -1:call(0x40c542,0x503380,0);mode=2;call(0x40c554,0x42e780);goto tail;
        case 1:call(0x40c5a2,0x503380,0);loop.mode_at_3b28114=1;call(0x40c5ae,0x4212e0,0);call(0x40c5b6,0x413c00);mode=9;goto tail;
        case 100:
            call(0x40c55f,0x503380,0);call(0x40c564,0x42c630);loop.mode_at_3b28114=1;call(0x40c570,0x4212e0,0);
            x=ret(0x40c575,0x4144e0);call(0x40c57b,0x414470,x);call(0x40c583,0x413c00);call(0x40c588,0x427ee0);call(0x40c58d,0x427ec0);mode=9;goto tail;
        case 101:call(0x40c5cb,0x503380,0);mode=0x11;goto tail;
        case 102:
            call(0x40c613,0x503380,0);call(0x40c618,0x42c630);loop.mode_at_3b28114=1;x=ret(0x40c623,0x4144e0);call(0x40c62a,0x42d770);call(0x40c62f,0x412ac0);
            call(0x40c635,0x414470,x);call(0x40c63b,0x413380,x);call(0x40c64b,0x4133e0,level(0x40c646));goto L653;
        case 103:call(0x40c5fb,0x503380,0);mode=0x15;goto tail;
        case 104:call(0x40c5e3,0x503380,0);mode=0x13;goto tail;
        case 107:call(0x40c678,0x42c820);goto L67d;
        default:goto tail;
        }
    L653:
        r=ret(0x40c653,0x4144e0);
        if(s32(r)==2||(s32(r)>4&&s32(r)<=6))call(0x40c84e,0x414570,0x21,0);
        else call(0x40c84e,0x414570,0x1a,3);
        call(0x40c857,0x4147f0,0);call(0x40c85d,0x412c00,0);call(0x40c863,0x4212e0,0);x=ret(0x40c868,0x4144e0);call(0x40c86e,0x414470,x);
        call(0x40c876,0x413c00);call(0x40c87b,0x427ee0);
    L880:
        mode=10;goto tail;
    L67d:
        call(0x40c67e,0x4212e0,0);mode=ret(0x40c686,0x413c00)==1?5:4;goto tail;
    L739:
        r=ret(0x40c73a,0x6428d0,0);
        if(r==1){
            x=ret(0x40c74b,0x4144e0);call(0x40c752,0x427ee0);call(0x40c757,0x42d770);call(0x40c75c,0x412ac0);call(0x40c762,0x414470,x);call(0x40c768,0x413380,x);
            call(0x40c777,0x4133e0,level(0x40c772));loop.mode_at_3b28114=1;call(0x40c787,0x4212e0,0);mode=ret(0x40c78f,0x413bc0)!=2?5:4;goto tail;
        }
        if(r==0xffffffffU){st.byte_at_3abdcfc=1;goto L951;}
    L8a3:
        if(s32(r)!=-2)goto tail;
    L93c:
        call(0x40c93c,0x40bf30);call(0x40c941,0x40bf40);call(0x40c946,0x42ca10);st.byte_at_3abdcfc=0;
    L951:
        mode=0xb;
    tail:
        if(need(st.word_at_3b290e4,0x40c95f)!=0&&need(st.byte_at_3b22e1e,0x40c967)!=0&&need(st.byte_at_3abdf78,0x40c96f)==0&&need(loop.save_phase_at_3b291a4,0x40c977)==0){
            call(0x40c980,0x785330,0);call(0x40c988,0x424320);
            if(need(mode,0x40c98d)==10&&ret(0x40c995,0x590650)!=0){call(0x40c99e,0x590660);call(0x40c9a4,0x40d490,0);}
        }
        call(0x40c9ac,0x42c4f0);call(0x40c9b1,0x426050);
        result.completed=true;
    }catch(Stop){}
    return result;
}
} // namespace sadx

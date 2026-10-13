#include "game/game_library_init.hpp"
#include <cstring>
namespace sadx {
namespace {
struct Stop{};
struct Operation {
    GameLibraryInitRuntime& rt;GameLibraryInitResult r;
    [[noreturn]]void fail(GameLibraryInitFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    std::uint32_t config(std::uint32_t offset,std::uint32_t at){
        const auto address=rt.config+offset;const auto v=rt.read?rt.read(address):std::nullopt;
        if(!v)fail(GameLibraryInitFailure::unknown_state,at,address);return *v;}
    std::optional<std::uint32_t> call(std::uint32_t site,std::uint32_t callee,std::array<std::optional<std::uint32_t>,4> arguments={}){
        r.pending_call=GameLibraryCall{site,callee,arguments};
        if(!rt.invoke)fail(GameLibraryInitFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);
        if(receipt.owner.state!=GameLogicOwnerState::completed)fail(receipt.owner.state==GameLogicOwnerState::cancelled?GameLibraryInitFailure::cancelled:
            receipt.owner.state==GameLogicOwnerState::failed?GameLibraryInitFailure::owner_failed:GameLibraryInitFailure::missing_owner,site);
        r.pending_call.reset();return receipt.eax;}
    // FILD m32, then FADD 2^32 (the 0x4f800000 constant) when the integer is negative: an unsigned load.
    X87Value unsigned_load(std::uint32_t address,std::uint32_t at){
        const auto v=rt.read_viewport?rt.read_viewport(address):std::nullopt;if(!v)fail(GameLibraryInitFailure::unknown_state,at,address);
        auto x=rt.fp.from_int64(static_cast<std::int32_t>(*v));
        if(static_cast<std::int32_t>(*v)<0)x=rt.fp.add_float_memory(x,4294967296.0F);
        return x;}
    void cleanup(std::uint32_t base){ // 7881a0, 793e80, 77f220, 780e70, 780f40, 793d90, 789d30, 78c5a0 (RET)
        const std::uint32_t callees[7]={0x7881a0,0x793e80,0x77f220,0x780e70,0x780f40,0x793d90,0x789d30};
        for(std::uint32_t i=0;i<7;++i)call(base+5*i,callees[i]);}
    void run(){
        auto& s=rt.state;
        // 781076..7810fd: the window-rect words to a local, config words to globals (instruction order).
        const auto w4=config(4,0x78107a),w0=config(0,0x78107d),w8=config(8,0x78107f),wc=config(0xc,0x781086);
        const std::uint32_t sites[9]={0x78108d,0x78109a,0x7810a7,0x7810af,0x7810b8,0x7810c1,0x7810c9,0x7810d2,0x7810db};
        for(std::uint32_t i=0;i<9;++i)s.words_at_389d864[i]=config(0x28+4*i,sites[i]);
        s.word_at_38a5dc4=config(0x10,0x7810e3);s.word_at_38a5dc8=config(0x50,0x7810ec);s.word_at_38a5dcc=config(0x54,0x7810f5);
        if(config(0x4c,0x7810fd)==1)call(0x781103,0x794050);
        call(0x78110d,0x793ef0,{w0,w4,w8,wc});   // window creation over the local rect (the loader's CreateSADXWindow behind 789e50)
        call(0x781112,0x789f80);
        // 781119: 78c5a0(0) is a RET.
        call(0x781120,0x77ff40,{0u});
        call(0x781125,0x793d80);                 // 791610 then 7800a0
        call(0x78112e,0x780f20,{0u,0u});
        {const auto a1c=config(0x1c,0x781133),a18=config(0x18,0x781136),a14=config(0x14,0x781139);call(0x78113f,0x780c30,{a14,a18,a1c});}
        // 781144: 78c5a0 is a RET.
        call(0x781155,0x77f150,{0u,0x3cfc400u,0x3ce8550u});
        call(0x78115d,0x793e70);
        std::uint32_t tries=8;
        for(;;){
            const auto result=call(0x781189,0x788000,{8u,0x3ce71c0u,0x2c8u,0x3cf4580u});
            if(!result)fail(GameLibraryInitFailure::owner_failed,0x781191);
            if(*result==0xffffffe9u||*result==0xffffffdfu)cleanup(0x7811a1); // then 78c5a0
            else if(*result==0)break;
            --tries;if(static_cast<std::int32_t>(tries)<=0)break;
        }
        r.tries_left=tries;
        if(tries==0)cleanup(0x7811d5);
        const auto height=unsigned_load(0x3d1278c,0x7811fd);
        s.scale_at_3d0f800=1.0F;                 // 78120b, between the two loads
        const auto width=unsigned_load(0x3d12788,0x78121d);
        s.aspect_at_3d0f9c0=rt.fp.to_float(rt.fp.divide(height,width));
        call(0x78123a,0x789c50);
    }
};
}
GameLibraryInitResult run_library_init_781070(GameLibraryInitRuntime& rt){
    Operation op{rt,{}};
    try{op.run();}catch(Stop){}catch(const X87Error&){op.r.failure=GameLibraryInitFailure::fp_failed;op.r.source_boundary=0x781232;}
    return op.r;
}
} // namespace sadx

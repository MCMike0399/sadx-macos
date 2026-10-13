#include "game/sound_manager.hpp"
namespace sadx {
namespace {
struct Stop{};
struct SkipRecord{};
struct Operation {
    SoundManagerRuntime& rt;SoundManagerResult r;
    [[noreturn]]void fail(SoundManagerFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(SoundManagerFailure::unknown_state,at,fact);return *v;}
    std::optional<std::uint32_t> call(std::uint32_t site,std::uint32_t callee,std::array<std::optional<std::uint32_t>,2> arguments,bool returns=false){
        r.pending_call=SoundManagerCall{site,callee,arguments};
        if(!rt.invoke)fail(SoundManagerFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);r.nested_source_boundary=receipt.owner.source_boundary;
        if(receipt.owner.state!=GameLogicOwnerState::completed)fail(receipt.owner.state==GameLogicOwnerState::failed?SoundManagerFailure::owner_failed:SoundManagerFailure::missing_owner,site);
        r.pending_call.reset();r.nested_source_boundary=0;
        if(returns&&!receipt.eax)throw SkipRecord{};
        return receipt.eax;}
    void record(std::uint32_t i){
        auto& f=rt.fields;auto& flags=rt.flags_at_3b29304[i];auto& timer=rt.audio.record_words_at_3b292fc[i];
        const std::uint32_t at=static_cast<std::uint32_t>(0x3b292fc+0x38*i);
        const auto flag_bits=need(flags,0x425120,at+8);
        if(flag_bits&1){
            const auto id=static_cast<std::int32_t>(need(f.ids_at_3b29308[i],0x42512a,at+0xc));
            if(id>0x568){
                call(0x42513c,0x425710,{static_cast<std::uint32_t>(id+0x159)});
                const auto later=need(flags,0x425141,at+8);flags=later&~0x10U;flags=(later&~0x10U)&~1U;timer=0;return;}
            call(0x425176,0x4246c0,{static_cast<std::uint32_t>(id)},true); // the active body continues through unported owners
            return;
        }
        auto t=static_cast<std::int32_t>(need(timer,0x4252a8,at));
        if(t>0){
            auto bits=need(flags,0x4252b1,at+8);
            if(!(bits&0x4000)){
                if(bits&0x22){
                    if(bits&0x100){
                        const auto v=*call(0x4252c1,0x4244a0,{i},true);
                        if(need(f.current_volume_at_3b29c28[i],0x4252cc,0x3b29c28+4*i)!=v){call(0x4252d3,0x423c20,{v,i});f.current_volume_at_3b29c28[i]=v;}
                    }
                    flags=need(flags,0x4252de,at+8)&~2U;
                }
                if(need(flags,0x4252e1,at+8)&0x200){
                    const auto v=need(f.volume_at_3b29310[i],0x4252ec,at+0x14);
                    if(need(f.current_volume_at_3b29c28[i],0x4252ef,0x3b29c28+4*i)!=v){call(0x4252f6,0x423c20,{v,i});f.current_volume_at_3b29c28[i]=v;}
                }
            }
            if(need(flags,0x425301,at+8)&0x800){
                auto v=static_cast<std::int32_t>(static_cast<std::uint32_t>(need(f.frequency_at_3b2930c[i],0x425308,at+0x10))*0x4fU);
                if(v<-10000)v=-10000;else if(v>10000)v=10000;
                call(0x42532a,0x410260,{i,static_cast<std::uint32_t>(v)});flags=need(flags,0x42532f,at+8)&~0x800U;
            }
            if(need(flags,0x42533b,at+8)&0x2000){
                call(0x425347,0x423cc0,{need(f.pan_at_3b29318[i],0x425342,at+0x1c),i});flags=need(flags,0x42534c,at+8)&~0x2000U;
            }
            timer=static_cast<std::uint32_t>(static_cast<std::int32_t>(need(timer,0x425358,at))-1);
        }
        if(need(timer,0x42535d,at)!=0)return;
        if(need(flags,0x425362,at+8)&0x10)call(0x425368,0x410100,{i});
        timer=0;f.words_at_3b29300[i]=0;flags=0;f.ids_at_3b29308[i]=0xffffffffU;rt.entities_at_3b29b90[i]=0;
    }
};
}
SoundManagerResult run_sound_manager_4250d0(SoundManagerRuntime& rt){
    Operation op{rt,{}};auto& a=rt.audio;
    try{
        if(op.need(a.word_at_3b29ce0,0x4250d3,0x3b29ce0)!=0)return op.r;
        const auto frame=op.need(a.byte_at_3b29ce8,0x4250eb,0x3b29ce8);a.byte_at_3b29ce8=static_cast<std::uint8_t>(frame+1);
        if(frame>3){for(auto& w:a.words_at_3b292a8)w=0;a.byte_at_3b29ce8=0;}
        for(std::uint32_t i=0;i<36;++i){try{op.record(i);}catch(SkipRecord){}}
        if(op.need(a.word_at_3b29ce4,0x4253a2,0x3b29ce4)!=0)return op.r;
        // 424b10: 3D positions only with a listener.
        if(op.need(rt.fields.listener_at_3b2cbb0,0x424b10,0x3b2cbb0)==0)return op.r;
        op.call(0x4253b1,0x424b10,{});
    }catch(Stop){}
    return op.r;
}
} // namespace sadx

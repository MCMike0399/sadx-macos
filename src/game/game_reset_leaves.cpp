#include "game/game_reset_leaves.hpp"

namespace sadx {
namespace {
struct Stop{ResetLeafOutcome outcome;std::uint32_t at;GameLogicOwnerResult nested;};
}
bool reset_leaf_print_debug_401000(const std::string& format,ResetLeafState& s){
    if(format.find('%')!=std::string::npos)return false;s.debug_text_at_38f6d08=format;return true;
}
ResetLeafResult run_reset_leaf(std::uint32_t callee,std::optional<std::uint32_t> argument,ResetLeafState& s,const ResetLeafNested& nested){
    ResetLeafResult r;r.outcome=ResetLeafOutcome::completed;
    const auto need=[&](const auto& v,std::uint32_t at){if(!v)throw Stop{ResetLeafOutcome::unknown_state,at,{}};return *v;};
    const auto call=[&](std::uint32_t site,std::uint32_t target,std::optional<std::uint32_t> object={},std::optional<std::uint32_t> arg={}){
        const auto receipt=nested?nested({site,target,object,arg}):GameLogicOwnerResult{GameLogicOwnerState::unavailable,target,{}};
        if(receipt.state!=GameLogicOwnerState::completed)throw Stop{ResetLeafOutcome::nested_failed,site,receipt};};
    const auto arg=[&](std::uint32_t at){return need(argument,at);};
    try{
        switch(callee){
        case 0x40cd10:s.word_at_3abdfac=need(s.word_at_3abdf90,0x40cd10);break;
        case 0x401410:s.depth_mode_at_8928c8=arg(0x401410);break;
        case 0x403e60:s.queue_mode_at_3ab98ac=2;break;
        case 0x4257c0:s.word_at_912698=0xffffffffU;s.word_at_3b29cf8=0;s.word_at_91269c=0xffffffffU;s.word_at_9126a8=0xffffffffU;break;
        case 0x4257e0:s.word_at_912694=0xffffffffU;s.word_at_3b29cf4=0;break;
        case 0x421fb0:{const auto fx=need(s.word_at_3c5e8b8,0x421fb0);s.word_at_3b11190=0;s.timing_word_at_3b1118c=0;if(fx!=0)call(0x421fc6,0x504ed0);break;}
        case 0x4228f0:{const auto n=arg(0x4228f0);
            if(n==0)for(const std::uint32_t inner:{0x422440U,0x4224b0U}){
                const auto leaf=run_reset_leaf(inner,{},s,nested);
                if(leaf.outcome!=ResetLeafOutcome::completed)throw Stop{leaf.outcome,leaf.source_boundary,leaf.nested};}
            else if(n==5&&need(s.callback_at_3b291b4,0x422909)==0x592370U)break;
            const auto callback=need(s.callback_at_3b291b4,0x422915);if(callback!=0)call(0x42291e,callback);
            s.callback_at_3b291b4=0;break;}
        case 0x410100:{ // only the audio-uninitialized early return is ported
            if(need(s.audio_initialized_at_3b0ef2c,0x410100)!=0)throw Stop{ResetLeafOutcome::nested_failed,0x41010d,{GameLogicOwnerState::unavailable,0x41010d,{}}};
            break;}
        case 0x424460:
            for(std::uint32_t i=0;i<36;++i){
                const auto flags=need(s.sound_record_flags_at_3b29304[i],0x424470);
                s.sound_record_words_at_3b292fc[i]=0;s.sound_record_flags_at_3b29304[i]=flags|0x10U;
                const auto inner=run_reset_leaf(0x410100,i,s,nested);
                if(inner.outcome!=ResetLeafOutcome::completed)throw Stop{inner.outcome,inner.source_boundary,inner.nested}; // innermost boundary
                s.words_at_3b29b90[i]=0;
            }
            r.eax=0;break;
        case 0x40cff0:{const auto n=arg(0x40cff0);
            if(n==0){if(need(s.word_at_3abdfa0,0x40d020)!=0){const auto p=need(s.player_at_3abdf9c,0x40d028);call(0x40d02e,0x411760,p);call(0x40d039,0x411720,need(s.player_at_3abdf9c,0x40d033));s.word_at_3abdfa0=0;s.word_at_3abdf98=0;}}
            else if(n==1){const auto p=need(s.player_at_3abdf94,0x40cffe);if(p!=0){call(0x40d008,0x411760,p);call(0x40d013,0x411720,need(s.player_at_3abdf94,0x40d00d));}}
            s.word_at_3abdfa8=0;break;}
        case 0x422440:case 0x4224b0:{
            const bool set=callee==0x422440;
            if(!reset_leaf_print_debug_401000(set?"ReleaseSetFile\n":"ReleaseCamFile\n",s))throw Stop{ResetLeafOutcome::nested_failed,set?0x422447u:0x4224b6u,{GameLogicOwnerState::unavailable,0x643dd0,{}}};
            if(set){s.word_at_3c4e458=0;s.word_at_3c4e45c=0;}
            auto& files=set?s.set_files_at_3abdf40:s.cam_files_at_3abdf28;
            for(std::size_t i=0;i<files.size();++i){const auto p=need(files[i],set?0x422460:0x4224c0);if(p!=0){call(set?0x42246b:0x4224cb,0x780fb0,{},p);files[i]=0;}}
            break;}
        case 0x4253c0:case 0x40d0d0:{
            // A created player is a source object identity this port does not
            // own yet; after a completed survey owner it stays unknown.
            const auto create=[&](std::uint32_t site,std::optional<std::uint32_t>& player){call(site,0x6443e7,{},0x50);player.reset();};
            if(need(s.audio_initialized_at_3b0ef2c,0x40ff10)!=0&&need(s.config_at_3a7cf3c,0x40d0f3)!=0)create(0x40d0fd,s.player_at_3abdf9c);
            else s.player_at_3abdf9c=0;
            s.word_at_3abdfa0=0;s.word_at_3abdfa4=0;s.word_at_3abdf98=0;s.word_at_3abdfa8=0;
            if(need(s.audio_initialized_at_3b0ef2c,0x40ff10)!=0&&need(s.config_at_38f6ec0,0x40d175)!=0)create(0x40d17f,s.player_at_3abdf94);
            else s.player_at_3abdf94=0;
            break;}
        case 0x40d490:if(arg(0x40d490)!=0)throw Stop{ResetLeafOutcome::nested_failed,0x40d49f,{GameLogicOwnerState::unavailable,0x40d49f,{}}};s.word_at_3b0e338=0;break;
        case 0x785330:{const auto x=arg(0x785330);(void)need(s.word_at_389d608,0x785334);(void)x; // the loop never advances: entry 0 always
            const auto e=need(s.entry_words_at_3d08428[0],0x78534f);if(e<3){s.entry_words_at_3d08428[0]=3;r.eax=0;}else r.eax=0xffffffffU;break;}
        default:r.outcome=ResetLeafOutcome::not_a_leaf;break;
        }
    }catch(const Stop& stop){r.outcome=stop.outcome;r.source_boundary=stop.at;r.nested=stop.nested;}
    return r;
}
} // namespace sadx

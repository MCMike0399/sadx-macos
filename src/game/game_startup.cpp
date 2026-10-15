#include "game/game_startup.hpp"

namespace sadx {
void clear_input_edges_40f350(GameLogicEdgeState& e,GameFrameInputState& input,GameControllerInitGlobals& g){
    e.bytes_at_3b0e3e0={0,0};
    for(std::size_t row=0;row<e.raw_at_3b0e3e2.size();++row){
        e.raw_at_3b0e3e2[row][0]=0;
        if(row<255){e.raw_at_3b0e3e2[row][1]=0;e.raw_at_3b0e3e2[row][2]=0;} // the clear ends at 3b0e6df
    }
    e.special_at_3b0e3ba=0;e.byte_at_3b0e3b9=0;input.reset_input_mask=0;
    for(auto& w:g.words_at_3b0e990)w=0;
}
ExitCallbackResult register_exit_callback_789930(std::uint32_t function,ExitCallbackList& list){
    ExitCallbackResult r;
    for(auto& slot:list.slots_at_3d084b0){
        if(!slot){r.source_boundary=0x789932;return r;}
        if(*slot==0){slot=function;r.returned=1;return r;}
    }
    r.returned=0;return r;
}
bool reset_menu_block_40a8a0(MenuBlockState& m){
    if(!m.block_at_900e88[0])return false;
    const bool copy=*m.block_at_900e88[0]==0;m.byte_at_3abdbc1=1;
    if(copy){m.block_at_9008b4=m.block_at_900e88;m.block_at_9008b4[0]=0;m.block_at_9008b4[1]=0;m.block_at_9008b4[2]=0;m.block_at_9008b4[3]=1;}
    m.block_at_900e88[0]=0;m.byte_at_903e58=0xff;return true;
}
StartupResult run_startup_40c030(StartupRuntime& rt){
    StartupResult r;auto& o=rt.owners;
    const auto call=[&](StartupOperation op,std::uint32_t site,std::uint32_t callee,std::optional<std::uint32_t> a={},std::optional<std::uint32_t> b={}){
        r.pending_call=StartupCall{op,site,callee,{a,b}};
        if(!rt.invoke){r.failure=StartupFailure::missing_owner;r.source_boundary=site;return false;}
        const auto receipt=rt.invoke(*r.pending_call);r.nested_source_boundary=receipt.source_boundary;
        if(receipt.state!=GameLogicOwnerState::completed){r.failure=receipt.state==GameLogicOwnerState::cancelled?StartupFailure::cancelled:
            receipt.state==GameLogicOwnerState::failed?StartupFailure::owner_failed:StartupFailure::missing_owner;r.source_boundary=site;return false;}
        r.pending_call.reset();r.nested_source_boundary=0;return true;
    };
    o.word_at_3abdc80=0;o.word_at_3abdc78=0;
    if(!call(StartupOperation::queue_408630,0x420810,0x408630))return r; // 420810: 408630 then tail-jump 40f350
    clear_input_edges_40f350(o.edges,o.input,o.controller_globals);
    if(!call(StartupOperation::ret_78c5a0,0x40c045,0x78c5a0,0))return r;
    if(!call(StartupOperation::set_mode_411e30,0x40c04e,0x411e30,1,1))return r;
    // 424c20
    o.audio.word_at_3b29ce0=0;o.audio.word_at_3b29ce4=0;
    if(!call(StartupOperation::audio_410510,0x424c30,0x410510))return r;
    o.audio.word_at_3b29b7c=0;for(auto& w:o.audio.record_words_at_3b292fc)w=0;
    if(!o.audio.byte_at_3b29ce8){r.failure=StartupFailure::unknown_state;r.source_boundary=0x424c4c;return r;}
    {const auto prior=*o.audio.byte_at_3b29ce8;o.audio.byte_at_3b29ce8=static_cast<std::uint8_t>(prior+1);if(prior>3)o.audio.byte_at_3b29ce8=0;}
    for(auto& w:o.audio.words_at_3b292a8)w=0;
    if(!call(StartupOperation::reset_40bf40,0x40c058,0x40bf40))return r;
    if(!init_controllers_40f3a0(o.controller_template_at_3b0eaa8,o.controllers_raw_at_3b0e9c8,o.controller_pointers_at_3b0e77c,o.controller_globals)){
        r.failure=StartupFailure::controller_record_missing;r.source_boundary=0x40f3cd;return r;}
    if(o.input_mod_raw_input_at_1000e748)input_mod_init_raw_controllers(*o.input_mod_raw_input_at_1000e748,o.controller_pointers_at_3b0e77c);
    o.mode_at_3abdc7c=0;
    if(!call(StartupOperation::audio_objects_40d0d0,0x40c068,0x4253c0))return r; // thunk to 40d0d0
    o.word_at_3abdc90=0;
    if(!reset_menu_block_40a8a0(o.menu)){r.failure=StartupFailure::unknown_state;r.source_boundary=0x40a8a0;return r;}
    const auto registered=register_exit_callback_789930(0x410630,o.exit_callbacks); // via 781000; EAX discarded
    if(!registered.returned){r.failure=StartupFailure::unknown_state;r.source_boundary=registered.source_boundary;return r;}
    r.exit_callback_registered=*registered.returned==1;r.completed=true;return r;
}
} // namespace sadx

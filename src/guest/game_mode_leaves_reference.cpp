#include "guest/game_mode_leaves_reference.hpp"
#include <cstring>
namespace sadx {
namespace {
struct Stop{GameModeLeafOutcome outcome;std::uint32_t at;std::optional<std::uint32_t> fact;GameLogicOwnerResult nested;};
struct Operation {
    GameModeLeafState& s;const GameModeLeafNested& nested;
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)throw Stop{GameModeLeafOutcome::unknown_state,at,fact,{}};return *v;}
    template<class B>std::uint64_t need(const B& block,std::uint32_t address,std::size_t width,std::uint32_t at){
        const auto v=block.read(address,width);if(!v)throw Stop{GameModeLeafOutcome::unknown_state,at,address,{}};return *v;}
    template<class B>void write(B& block,std::uint32_t address,std::uint64_t value,std::size_t width,std::uint32_t at){
        if(!block.write(address,value,width))throw Stop{GameModeLeafOutcome::unknown_state,at,address,{}};}
    template<class B>void zero(B& block,std::uint32_t address,std::size_t count,std::uint32_t at){
        if(!block.fill(address,count,0))throw Stop{GameModeLeafOutcome::unknown_state,at,address,{}};}
    void call(std::uint32_t site,std::uint32_t callee,std::array<std::optional<std::uint32_t>,7> arguments={}){
        const auto receipt=nested?nested({site,callee,arguments}):GameLogicOwnerResult{GameLogicOwnerState::unavailable,callee,{}};
        if(receipt.state!=GameLogicOwnerState::completed)throw Stop{GameModeLeafOutcome::nested_failed,site,{},receipt};}
    std::uint32_t character(std::uint32_t at){return need(s.character_at_3b22dc0,at,0x3b22dc0);}
    // 425fe0 returns MOVSX AX (only AX is defined); callers compare AX.
    std::int16_t sign_byte_3b0ef34(std::uint32_t at){return static_cast<std::int16_t>(static_cast<std::int8_t>(need(s.byte_at_3b0ef34,at,0x3b0ef34)));}
    // Actual 422680: per-character motion words from the bound character data.
    std::uint32_t leaf_422680(std::uint32_t c,std::uint32_t site){
        if(!s.model_bindings||!s.model_dlls){call(site,0x422680,{c});return 0;}
        const auto table=[&](std::uint32_t global,std::uint32_t at){const auto p=need(s.model_bindings->value_at(global),at,global);
            if(p==0)throw Stop{GameModeLeafOutcome::unknown_state,at+5,global,{}}; // null data pointer: the source faults
            return p;};
        const auto field=[&](std::uint32_t p,std::uint32_t offset,std::uint32_t at){return need(s.model_dlls->u32(p+offset),at,p+offset);};
        switch(c){
        case 0:case 2:{const auto p=table(0x38f6e7c,0x494820);s.motion_at_3c566c0=field(p,0x190,0x494825);s.motion_at_3c566d0=field(p,0x194,0x494831);return p;}
        case 7:{const auto p=table(0x3aad104,0x48d1c0);s.motion_at_3c559c0=field(p,0x10,0x48d1c5);s.motion_at_3c559d0=field(p,0x10,0x48d1ce);
            s.motion_at_3c559e0=field(p,0x74,0x48d1d7);const auto d=field(p,0x78,0x48d1e0);s.motion_at_3c55a00=d;s.motion_at_3c559f0=d;return d;}
        case 3:{const auto p=table(0x3a7cf28,0x4767f0);s.motion_at_3c53510=field(p,0xa0,0x4767f5);s.motion_at_3c53520=field(p,0xa4,0x476801);return p;}
        case 5:case 6:return c; // the "?MOTREL" print is the 51f680 RET
        default:throw Stop{GameModeLeafOutcome::nonterminating,0x422700,{},{}}; // PrintDebug then JMP 422700, forever
        }}
    // Actual 42c4f0: when the fade alpha 3b29d5f is set, blends (0,8)/(1,6) and a
    // screen-sized 4073b0 rectangle at depth 0x46ac3dfb in the 3b29d5c colour.
    void leaf_42c4f0(){
        const auto color=need(s.color_at_3b29d5c,0x42c4f0,0x3b29d5f);if(!(color>>24))return;
        call(0x42c4fd,0x4030d0,{0u,8u});call(0x42c506,0x4030d0,{1u,6u});
        if(!s.fp)throw Stop{GameModeLeafOutcome::unknown_state,0x42c50b,{},{}};
        auto& fp=*s.fp;const auto f32=[](float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;};
        const auto& sh=s.screen_height_at_8928c4?*s.screen_height_at_8928c4:std::optional<float>{};
        const auto h=fp.to_float(fp.multiply_float_memory(fp.from_float(need(sh,0x42c50b,0x8928c4)),480.0F));
        const auto& sw=s.screen_width_at_8928c0?*s.screen_width_at_8928c0:std::optional<float>{};
        const auto w=fp.to_float(fp.multiply_float_memory(fp.from_float(need(sw,0x42c54f,0x8928c0)),640.0F));
        call(0x42c562,0x4073b0,{0u,0u,f32(w),f32(h),0x46ac3dfbu,color,1u});}
    void leaf_421f90(){s.byte_at_3b290d8=1;s.byte_at_3b290e0=0;s.mode.word_at_3b290e4=0;s.byte_at_3b290e8=0;s.word_at_3b290dc=0;}
    void leaf_427ee0(){s.half_at_3b0f0e4=0;s.word_at_3b0ef44=0;}
    void leaf_43b490(){s.word_at_3b0f0f8=0;s.word_at_3b0f0fc=0xbf800000U;s.word_at_3b0f100=0;s.word_at_3b0f10c=0;s.word_at_3b0f0f4=0;}
    void leaf_42f5d0(){zero(s.block_at_3b18250,0x3b18608,0x200,0x42f5dd);s.word_at_3b2c560=0;s.event_task_at_3b2c578=TaskHandle{};
        s.byte_at_3b2c565=0;s.byte_at_3abe134=1;s.byte_at_3b2c574=0;s.word_at_914788=0xffffffffU;}
    void leaf_413300(){
        auto& b=s.block_at_3b18250;
        zero(b,0x3b183a8,0x60,0x41330d);zero(b,0x3b18808,0x200,0x413319);zero(b,0x3b18250,0x158,0x413325);zero(b,0x3b18a0c,0x60,0x413331);
        zero(b,0x3b18a58,0x200,0x41333d);zero(b,0x3b18c58,0x158,0x413349);zero(b,0x3b18408,0x200,0x413355);
        s.mode.level_word_via_3b0ef4c.reset(); // 3b0ef4c = 0: a null level pointer (the deref faults)
        write(b,0x3b18db0,0,4,0x41335c);write(b,0x3b18849,1,1,0x413361);
        leaf_42f5d0();s.word_at_3b18244=0;}
    // Actual 432ea0: -1 in each 0x18 record from 3b2c604 below 3b2c664 (signed JL), then fixed words.
    void leaf_432ea0(){
        auto& b=s.block_at_3b2c5e8;
        for(std::uint32_t a=0x3b2c604;static_cast<std::int32_t>(a)<0x3b2c664;a+=0x18)write(b,a,0xffffffffU,4,0x432eb0);
        for(auto [a,v]:{std::pair{0x3b2c5fcu,0u},{0x3b2c600u,0u},{0x3b2c614u,0u},{0x3b2c618u,0x3f800000u},{0x3b2c62cu,0x3f800000u},{0x3b2c630u,0u},{0x3b2c644u,0x3f800000u},
            {0x3b2c648u,0x3f800000u},{0x3b2c658u,0xffffffffu},{0x3b2c650u,0u},{0x3b2c65cu,1u},{0x3b2c654u,0u},{0x3b2c5ecu,0x3f800000u},{0x3b2c5e8u,0u}})write(b,a,v,4,0x432ec2);}
    // Actual 42c730 (SEGA logo setup).
    void leaf_42c730(){
        call(0x42c732,0x40b1a0);call(0x42c737,0x40b460);
        s.word_at_3b2c420=0;                                       // 42e470(0)
        if(s.clear_colors_at_3ce7174)for(auto& w:*s.clear_colors_at_3ce7174)w=0xff000000u; // 402f10(0,0,0) -> 780c00 (alpha ORed in)
        else call(0x42c747,0x402f10,{0u,0u,0u});
        call(0x42c756,0x421180,{0x7df3d4u,0x913a48u});             // LoadPVM("SEGALOGO_E", 913a48): the loader's LoadPVM_r in the profile
        s.state_at_3b29d6c=1;s.word_at_3b29d68=0;
        call(0x42c76f,0x411e30,{1u,1u});call(0x42c776,0x77ed40,{1u});
        s.color_at_3b29d5c=0;                                      // 42c4e0(&four zero bytes)
        leaf_432ea0();                                             // then 78c5a0(3): RET
    }
    std::uint32_t leaf_427f10(){
        s.word_at_3b0f13c=0;s.byte_at_3b0ef35=0;s.byte_at_3b0f128=0;s.byte_at_3b0ef48=0;
        const auto c=character(0x427f2b);s.word_at_3b0f108=0;
        if(c==6){s.byte_at_3b0ef48=3;s.byte_at_3b0f128=0;}
        return c;}
    void leaf_420e90(std::uint32_t c){
        if(c>=8)throw Stop{GameModeLeafOutcome::unknown_state,0x420e95,static_cast<std::uint32_t>(0x90ed54+4*c),{}};
        const auto list=need(s.block_at_90ed54,0x90ed54+4*c,4,0x420e95);if(list==0)return;
        const auto count=need(s.block_at_3b290c4,0x3b290c4+c,1,0x420ea0);
        if(count==1)call(0x420ebe,0x403290,{c}); // release walk over the character's texture lists
        const auto again=static_cast<std::int8_t>(need(s.block_at_3b290c4,0x3b290c4+c,1,0x420ed1));
        if(again>0)write(s.block_at_3b290c4,0x3b290c4+c,static_cast<std::uint8_t>(again-1),1,0x420edd);}
    void tail_46b230(){
        auto& b=s.block_at_3c4ae08;
        for(std::uint32_t a=0x3c4ae0c;a<0x3c4ae84;a+=0xc){write(b,a-4,0xff,1,0x46b240);write(b,a,0x3f800000U,4,0x46b244);}
        write(b,0x3c4b00c,0xffffffffU,4,0x46b257);write(b,0x3c4b010,0xffffffffU,4,0x46b25d);write(b,0x3c4b014,0xffff,2,0x46b264);
        zero(b,0x3c4b040,0x3400,0x46b276);
        write(b,0x3c4b034,0,1,0x46b284);write(b,0x3c4ae80,0,1,0x46b28a);write(b,0x3c4b030,0,4,0x46b290);write(b,0x3c4b02c,1,1,0x46b296);
        for(std::uint32_t a=0x3c4b040;a<0x3c4e440;a+=0x200)write(b,a,8,1,0x46b2a3);
        write(b,0x3c4b008,0xff,1,0x46b2b2);write(b,0x3c4b024,0,4,0x46b2b9);}
    // Actual 413c00: returns 2 or 1 by the signed stage word.
    std::uint32_t leaf_413c00(){
        s.game_mode_at_3b22de4=0x15;s.half_at_3b22e1c=0;s.mode.word_at_3b2c45c=0;leaf_43b490();
        const auto flags=static_cast<std::uint8_t>(need(s.block_at_3b2b5f9,0x3b2b5f9,1,0x413c1e));
        s.half_at_3b18db8=0;s.word_at_3b22e20=0;s.byte_at_3abe134=static_cast<std::uint8_t>(~(flags>>1)&1);
        if(sign_byte_3b0ef34(0x413c3b)<4)s.byte_at_3b0ef34=4; // 425af0
        call(0x413c4c,0x785330,{0u});
        s.callback_at_3d08420=0x40a340; // 783260: the material callback's source identity
        const auto stage=static_cast<std::int16_t>(need(s.stage_at_3b22dcc,0x413c5b,0x3b22dcc));
        if(stage<=0x19)return 2;if(stage<=0x22)return 1;return stage<=0x26?2:1;}
    void leaf_414470(std::uint32_t argument){
        leaf_420e90(character(0x414470));
        const auto next=static_cast<std::uint16_t>(argument);s.character_at_3b22dc0=next;
        leaf_422680(next,0x41448c);
        auto index=character(0x414491);if(index>4)--index;if(index>1)--index;
        s.byte_at_3b0ef34=static_cast<std::uint8_t>(need(s.block_at_3b2b5f9,0x3b2b5fa+index,1,0x4144a7));
        if(sign_byte_3b0ef34(0x4144b3)<4)s.byte_at_3b0ef34=4; // 425af0
        tail_46b230();}
};
}
GameModeLeafResult run_game_mode_leaf(std::uint32_t callee,std::optional<std::uint32_t> argument,GameModeLeafState& s,const GameModeLeafNested& nested){
    GameModeLeafResult r;r.outcome=GameModeLeafOutcome::completed;Operation op{s,nested};
    const auto arg=[&](std::uint32_t at){if(!argument)throw Stop{GameModeLeafOutcome::unknown_state,at,{},{}};return *argument;};
    try{
        switch(callee){
        case 0x4144e0:r.eax=op.character(0x4144e0);break;
        case 0x421f70:r.eax=0xf;break;
        case 0x421f80:r.eax=1;break;
        case 0x4f88a0:r.eax=0;break;
        case 0x4221e0:r.eax=0;break; // the "FCLoadFileModule" print is the 51f680 RET
        case 0x426050:{const auto d=static_cast<std::uint32_t>(op.need(s.timing.logic_increment,0x426056,0x3b11178));
            s.word_at_3b0f13c=static_cast<std::uint32_t>(op.need(s.word_at_3b0f13c,0x426050,0x3b0f13c)+d);r.eax=d;break;}
        case 0x42cea0:s.word_at_3b2a2f0=0;s.word_at_3b2a2f4=0;s.byte_at_3b2a2f8=0;s.byte_at_3b2a2f9=0;s.mode.byte_at_3b2a2fa=0;s.byte_at_3b2a2fb=0;
            s.word_at_3b2a2fc=0;s.half_at_3b2a300=0;s.word_at_3b2b358=0;r.eax=0;break;
        case 0x421f90:op.leaf_421f90();r.eax=0;break;
        case 0x427ee0:op.leaf_427ee0();r.eax=0;break;
        case 0x427ec0:op.zero(s.block_at_3b0ef9c,0x3b0ef9c,0x3c,0x427ec7);r.eax=0x3b0efdc;break;
        case 0x425af0:s.byte_at_3b0ef34=4;break;
        case 0x43b490:op.leaf_43b490();r.eax=0;break;
        case 0x42f5d0:op.leaf_42f5d0();r.eax=0;break;
        case 0x413300:op.leaf_413300();r.eax=0;break;
        case 0x427f10:r.eax=op.leaf_427f10();break;
        case 0x420e90:op.leaf_420e90(arg(0x420e91));break;
        case 0x422680:r.eax=op.leaf_422680(arg(0x422680),0x422680);break;
        case 0x414470:op.leaf_414470(arg(0x41447d));r.eax=0x3c4e440;break;
        case 0x415000:
            op.leaf_421f90();op.leaf_414470(0);op.leaf_427f10();op.leaf_427ee0();op.leaf_413300();
            s.byte_at_3abe134=0;op.leaf_413c00();s.byte_at_3b0f0e0=0;
            crt_srand_6443b5(s.rng,0xdead0cabU);break;
        case 0x413c00:r.eax=op.leaf_413c00();break;
        case 0x783260:s.callback_at_3d08420=arg(0x783260);r.eax=*s.callback_at_3d08420;break;
        case 0x42c4f0:op.leaf_42c4f0();break;
        case 0x42c730:op.leaf_42c730();r.eax=0;break; // EAX: 432ea0's XOR survives 78c5a0's RET
        case 0x42c7b0:
            op.leaf_42c730();s.word_at_3b2c47c=0;s.timing.word_at_3b2a2e4=0;s.demo_cutscene_at_3b2a2e8=0;s.word_at_3b2919c=0;
            op.call(0x42c7cb,0x42d3b0);break;
        default:r.outcome=GameModeLeafOutcome::not_a_leaf;break;
        }
    }catch(const Stop& stop){r.outcome=stop.outcome;r.source_boundary=stop.at;r.required_fact=stop.fact;r.nested=stop.nested;}
    return r;
}
} // namespace sadx

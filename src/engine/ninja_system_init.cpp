#include "engine/ninja_system_init.hpp"
namespace sadx {
namespace {
struct Stop{};
struct Operation {
    NinjaSystemInitRuntime& rt;NinjaSystemInitResult r;
    [[noreturn]]void fail(NinjaSystemInitFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(NinjaSystemInitFailure::unknown_state,at,fact);return *v;}
    std::optional<std::uint32_t> call(std::uint32_t site,std::uint32_t callee,std::optional<std::uint32_t> ecx={},std::optional<std::uint32_t> edx={},std::array<std::optional<std::uint32_t>,4> arguments={}){
        r.pending_call=NinjaSystemCall{site,callee,ecx,edx,arguments};
        if(!rt.invoke)fail(NinjaSystemInitFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);
        if(receipt.owner.state!=GameLogicOwnerState::completed)fail(receipt.owner.state==GameLogicOwnerState::cancelled?NinjaSystemInitFailure::cancelled:
            receipt.owner.state==GameLogicOwnerState::failed?NinjaSystemInitFailure::owner_failed:NinjaSystemInitFailure::missing_owner,site);
        r.pending_call.reset();return receipt.eax;}
    std::uint32_t malloc_780f60(std::uint32_t size,std::uint32_t site){const auto m=nj_malloc_780f60(rt.borrowed.memory,rt.borrowed.heap,size);if(!m)fail(NinjaSystemInitFailure::memory_failed,site);return m.eax;}
    template<class B>void put(B& block,std::uint32_t address,std::uint64_t value,std::size_t width,std::uint32_t at){if(!block.write(address,value,width))fail(NinjaSystemInitFailure::store_outside_owner,at,address);}
    // 780ea0(a0..a4): free the old pool, clamp negatives to 0, 3ce71a4 = 4 * sum, njMalloc it when nonzero.
    void pools_780ea0(std::array<std::int32_t,5> a){
        auto& s=rt.state;
        if(const auto pool=need(s.pool_at_3ce71b0,0x780ea0,0x3ce71b0);pool!=0){
            if(!nj_free_780fb0(rt.borrowed.memory,rt.borrowed.heap,pool))fail(NinjaSystemInitFailure::memory_failed,0x780eaa);s.pool_at_3ce71b0=0;}
        std::uint32_t sum=0;for(auto v:a)sum+=static_cast<std::uint32_t>(v<0?0:v);
        const auto size=sum<<2;s.size_at_3ce71a4=size;
        if(size!=0)s.pool_at_3ce71b0=malloc_780f60(size,0x780f07);}
    // 78e720(count 0x20, size, base): 3d127a0 = base; 3d128ac = malloc(size * 36) (CRT 645002);
    // 794540 per 3d0fe00 record with the 389d864/389d888/389d8ac/389d8d0 words; 3d0fae0..cdf = 0.
    void buffers_78e720(std::uint32_t size,std::uint32_t base){
        auto& s=rt.state;s.word_at_3d127a0=base;
        rt.borrowed.frame.vertex_buffer_at_3d128ac=rt.borrowed.crt_heap.alloc(rt.borrowed.crt_heap.create(),size*36U).value_or(0);
        for(std::uint32_t i=0;i<9;++i){
            const auto e=need(s.tables_at_389d888[18+i],0x78e750,0x389d8d0+4*i),f=need(s.tables_at_389d888[9+i],0x78e756,0x389d8ac+4*i),g=need(s.tables_at_389d888[i],0x78e75c,0x389d888+4*i);
            const auto d=need(rt.borrowed.sizes_at_389d864[i],0x78e763,0x389d864+4*i);
            call(0x78e76d,0x794540,0x3d0fe00+0x24*i,d,{g,f,e});}
        if(!rt.borrowed.block_at_3d0fae0.fill(0x3d0fae0,0x200,0))fail(NinjaSystemInitFailure::store_outside_owner,0x78e78c,0x3d0fae0);}
    // 77f150: zero the 4 x 0x78 records at 3ce6f48 and the 4 x 0x34 records at 3ce6e78 (+0x30 = &3ce6f48[i]);
    // the 7928e0 call at 77f205 is NOP'd by the Input mod; register 77f110; tail-jump 785240.
    void controllers_77f150(){
        auto& s=rt.state;
        for(std::uint32_t i=0;i<4;++i){const auto at=0x3ce6f48+0x78*i;if(!s.records_at_3ce6f48.fill(at,0x78,0))fail(NinjaSystemInitFailure::store_outside_owner,0x77f160,at);}
        for(std::uint32_t i=0;i<4;++i){const auto at=0x3ce6e78+0x34*i;
            put(s.records_at_3ce6e78,at+0x30,0x3ce6f48+0x78*i,4,0x77f1c0);
            if(!s.records_at_3ce6e78.fill(at,0x30,0))fail(NinjaSystemInitFailure::store_outside_owner,0x77f1c3,at);}
        if(!ninja_callback_register_77ffc0(rt.borrowed.callbacks,0x77f110))fail(NinjaSystemInitFailure::unknown_state,0x77f20f,0x3ce712c);
        for(auto& w:rt.borrowed.entry_words_at_3d08428)w=0;                       // 785240 (tail jump)
        if(!ninja_callback_register_77ffc0(rt.borrowed.callbacks,0x785190))fail(NinjaSystemInitFailure::unknown_state,0x785261,0x3ce712c);}
    // 77efd0: zero the 3ce6d80 records (with their +0x16..+0x1a spill) and the 3ce6df0 records; the
    // 791d60 call at 77f03e is NOP'd by the Input mod; register 77efc0.
    void mouse_77efd0(){
        auto& b=rt.state.block_at_3ce6d80;
        for(std::uint32_t i=0;i<4;++i){
            const auto at=0x3ce6d80+0x18*i;if(!b.fill(at,0x16,0)||!b.fill(at+0x16,5,0))fail(NinjaSystemInitFailure::store_outside_owner,0x77efe9,at);
            const auto r12=0x3ce6df0+0x12*i;if(!b.fill(r12,0x12,0))fail(NinjaSystemInitFailure::store_outside_owner,0x77f002,r12);}
        if(!ninja_callback_register_77ffc0(rt.borrowed.callbacks,0x77efc0))fail(NinjaSystemInitFailure::unknown_state,0x77f048,0x3ce712c);}
    // 40bb20(instance, window): DirectInput8Create into 3abdc10, the mouse CreateDevice into 3abdc14,
    // SetDataFormat(c_dfDIMouse), SetCooperativeLevel(window, 6), Acquire. HRESULTs are ignored.
    void direct_input_40bb20(std::uint32_t instance,std::uint32_t window){
        auto& s=rt.state;
        s.direct_input_at_3abdc10=call(0x40bb36,0x77dbb0,{},{},{instance,0x800u,0x88ad34u,0x3abdc10u}).value_or(0); // DirectInput8Create
        if(need(s.direct_input_at_3abdc10,0x40bb3b,0x3abdc10)==0)fail(NinjaSystemInitFailure::owner_failed,0x40bb40); // [eax] on a null interface
        s.mouse_at_3abdc14=call(0x40bb4f,0x40bb4f,{},{},{*s.direct_input_at_3abdc10,0x88ac94u}).value_or(0);         // CreateDevice(GUID_SysMouse)
        if(*s.mouse_at_3abdc14==0)fail(NinjaSystemInitFailure::owner_failed,0x40bb57);
        call(0x40bb5f,0x40bb5f,{},{},{*s.mouse_at_3abdc14,0x88ac0cu});      // SetDataFormat(c_dfDIMouse)
        call(0x40bb71,0x40bb71,{},{},{*s.mouse_at_3abdc14,window,6u});      // SetCooperativeLevel
        call(0x40bb7c,0x40bb7c,{},{},{*s.mouse_at_3abdc14});                // Acquire
    }
    void run(){
        auto& s=rt.state;auto& b=rt.borrowed;
        s.word_at_3d0f83c=0x38fcf08;s.word_at_3d0f810=0x40000U>>11<<11;                     // 77f240
        pools_780ea0({-32,0,0x160000,0,0x38000});
        s.color_at_3ce715c=0xffbfbfbfu;s.scale_at_3ce7154=8.0F;                              // 780870
        {const auto buffer=malloc_780f60(0x1000,0x780889);s.buffer_at_3ce7158=buffer;call(0x7808a2,0x793c70,buffer,0x80,{0x800u});}
        s.scale_at_3ce7154=rt.fp.to_float(rt.fp.from_int64(0xc));                          // 7808c0(0xc): FILD (u16) then FSTP m32
        buffers_78e720(0x6000,0x38fcf08);                                                  // 780850 -> 78e720(0x20, 0x6000, 38fcf08)
        s.word_at_3d0f740=0x38f6ef8;                                                        // 7803b0 (ECX)
        call(0x40188e,0x780180,0x3a7cf48,0x80,{0u});                                        // njInitMatrix
        {const auto t=nj_init_texture_77f610(b.textures,b.selection,0x3a8b0a0,0x800);if(!t)fail(NinjaSystemInitFailure::store_outside_owner,t.source_boundary,t.required_fact);}
        s.flag_at_3ce7150=1;                                                                // 780140
        {const auto now=rt.counter?rt.counter():std::nullopt;if(!now)fail(NinjaSystemInitFailure::missing_owner,0x78014f);
            s.counter_low_at_3ce7130=static_cast<std::uint32_t>(*now);s.counter_high_at_3ce7134=static_cast<std::uint32_t>(static_cast<std::uint64_t>(*now)>>32);}
        b.word_at_3d0f9c8=8;                                                                // 77ff60(8)
        if(b.clear_colors_at_3ce7174)for(auto& w:*b.clear_colors_at_3ce7174)w=0xff000000u;  // 402f10(0,0,0) -> 780c00
        else call(0x4018b4,0x402f10,{},{},{0u,0u,0u});
        controllers_77f150();                                                              // then 78c5a0(1) is a RET before it
        call(0x4018d1,0x77f0a0);                                                            // keyboard init (the runtime's owner, profile 77f0d7)
        mouse_77efd0();
        {const auto window=need(b.window_at_3d0fd30,0x4018db,0x3d0fd30),instance=need(b.instance_at_3d0fd34,0x4018e0,0x3d0fd34);direct_input_40bb20(instance,window);}
    }
};
}
NinjaSystemInitResult run_controllers_77f150(NinjaSystemInitRuntime& rt){
    Operation op{rt,{}};
    try{op.controllers_77f150();}catch(Stop){}
    return op.r;
}
NinjaSystemInitResult run_system_init_401830(NinjaSystemInitRuntime& rt){
    Operation op{rt,{}};
    try{op.run();}catch(Stop){}catch(const X87Error&){op.r.failure=NinjaSystemInitFailure::fp_failed;op.r.source_boundary=0x7808c9;}
    return op.r;
}
} // namespace sadx

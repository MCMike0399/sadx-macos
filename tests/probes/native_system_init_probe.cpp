#include "ninja/ninja_system_init.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
// Native 401830 with the layout of tools/verify_original_system_init.py.
// in: [0] fail-at nested call [1] CW [2] 3ce71b0 before [3] 389d628 before [4] callback count before [5..12] slots before
//     [13] 3d0fd30 [14] 3d0fd34 [15] QPC low [16] QPC high [17..25] 389d864 [26..52] 389d888.. tables
// out: see FIELDS in the oracle; pool: 0x22000 bytes; blocks: 0x1e0 + 0xd0 + 0xb8 + 0x200; text: heap traces.
extern "C" int sadx_native_system_init(const std::uint32_t* in,std::uint32_t* out,std::uint8_t* pool_out,std::uint8_t* blocks_out,char* text,std::size_t text_size){
    using namespace sadx;
    NinjaSystemInitState s;
    s.word_at_3d0f83c=s.word_at_3d0f810=s.size_at_3ce71a4=s.color_at_3ce715c=s.buffer_at_3ce7158=s.word_at_3d127a0=s.word_at_3d0f740=s.flag_at_3ce7150=0u;
    s.counter_low_at_3ce7130=s.counter_high_at_3ce7134=s.direct_input_at_3abdc10=s.mouse_at_3abdc14=0u;s.scale_at_3ce7154=0.0F;s.pool_at_3ce71b0=in[2];
    for(auto& b:s.records_at_3ce6f48.bytes)b=0x11;for(auto& b:s.records_at_3ce6e78.bytes)b=0x22;for(auto& b:s.block_at_3ce6d80.bytes)b=0x33;
    for(int i=0;i<27;++i)s.tables_at_389d888[i]=in[26+i];
    NinjaMemoryState memory;memory.heap_at_389d628=in[3];GuestHeap heap,crt(0x00520000u,0x60000000u);
    NinjaFrameCallState frame;frame.vertex_buffer_at_3d128ac=0;SourceBytes<0x3d0fae0,0x200> block;for(auto& b:block.bytes)b=0x44;
    std::array<std::optional<std::uint32_t>,9> sizes;for(int i=0;i<9;++i)sizes[i]=in[17+i];
    NinjaTexturePoolState pool;for(auto& b:pool.records_at_3a8b0a0.bytes)b=0x55;pool.base_at_3d0f844=pool.count_at_3d0f80c=pool.word_at_3d0f830=pool.word_at_3d0fa14=pool.word_at_3d0f960=0u;pool.byte_at_3d0f860=0;for(auto& w:pool.words_at_3d0f7c0)w=0u;
    NinjaTextureSelectionState sel;sel.current_list_at_3d0fa24=std::shared_ptr<NinjaTextureListOwner>{};sel.requested_memory_at_3ce7128=std::shared_ptr<const NinjaTextureMemoryReference>{};NinjaCallbackList callbacks;callbacks.count_at_3ce712c=in[4];std::optional<std::uint32_t> word_3d0f9c8=0u;std::array<std::optional<std::uint32_t>,4> entry_words{1u,2u,3u,4u};for(int i=0;i<8;++i)callbacks.slots_at_3d0fa40.push_back(in[5+i]);
    std::array<std::optional<std::uint32_t>,4> clear{0u,0u,0u,0u};std::optional<std::uint32_t> window=in[13],instance=in[14];X87Context fp(static_cast<std::uint16_t>(in[1]));
    std::vector<std::array<std::uint32_t,8>> nested;
    NinjaSystemInitRuntime rt{s,{memory,heap,crt,frame,block,sizes,pool,sel,callbacks,word_3d0f9c8,entry_words,&clear,window,instance},fp,
        [&](const NinjaSystemCall& c)->NinjaSystemOwnerResult{
            const auto v=[](const std::optional<std::uint32_t>& x){return x.value_or(0xdeadbeef);};
            nested.push_back({c.callee,c.source_call,v(c.ecx),v(c.edx),v(c.arguments[0]),v(c.arguments[1]),v(c.arguments[2]),v(c.arguments[3])});
            if(in[0]&&nested.size()==in[0])return {{GameLogicOwnerState::unavailable,c.callee,{}},std::nullopt};
            std::optional<std::uint32_t> eax;if(c.callee==0x77dbb0)eax=0x11000800u;else if(c.callee==0x40bb4f)eax=0x11000900u;
            return {{GameLogicOwnerState::completed,0,{}},eax};},
        [&]()->std::optional<std::int64_t>{return static_cast<std::int64_t>((static_cast<std::uint64_t>(in[16])<<32)|in[15]);}};
    const auto r=run_system_init_401830(rt);
    std::fill(out,out+320,0);out[0]=r?1U:0U;
    const auto v=[](const auto& o){return static_cast<std::uint32_t>(o.value_or(0xdeadbeef));};
    const auto f=[](const std::optional<float>& x){std::uint32_t b=0xdeadbeef;if(x)std::memcpy(&b,&*x,4);return b;};
    const std::uint32_t fields[]={v(s.word_at_3d0f83c),v(s.word_at_3d0f810),v(s.pool_at_3ce71b0),v(s.size_at_3ce71a4),v(s.color_at_3ce715c),f(s.scale_at_3ce7154),v(s.buffer_at_3ce7158),
        v(s.word_at_3d127a0),v(s.word_at_3d0f740),v(s.flag_at_3ce7150),v(s.counter_low_at_3ce7130),v(s.counter_high_at_3ce7134),v(word_3d0f9c8),v(s.direct_input_at_3abdc10),v(s.mouse_at_3abdc14),
        v(frame.vertex_buffer_at_3d128ac),v(pool.base_at_3d0f844),v(pool.count_at_3d0f80c),v(pool.word_at_3d0f830),v(pool.word_at_3d0fa14),v(pool.word_at_3d0f960),v(pool.byte_at_3d0f860),v(memory.heap_at_389d628)};
    std::copy(std::begin(fields),std::end(fields),out+1);
    for(int i=0;i<8;++i)out[30+i]=v(pool.words_at_3d0f7c0[i]);
    out[38]=sel.current_list_at_3d0fa24&&!*sel.current_list_at_3d0fa24?0U:0xdeadbeef;out[39]=sel.requested_memory_at_3ce7128&&!*sel.requested_memory_at_3ce7128?0U:0xdeadbeef;
    for(int i=0;i<4;++i)out[40+i]=v(clear[i]);
    out[44]=v(callbacks.count_at_3ce712c);for(std::size_t i=0;i<callbacks.slots_at_3d0fa40.size()&&i<16;++i)out[45+i]=v(callbacks.slots_at_3d0fa40[i]);
    for(int i=0;i<4;++i)out[66+i]=v(entry_words[i]);
    out[70]=static_cast<std::uint32_t>(nested.size());for(std::size_t i=0;i<nested.size()&&i<28;++i)std::copy(nested[i].begin(),nested[i].end(),out+71+8*i);
    for(std::size_t i=0;i<pool.records_at_3a8b0a0.size;++i)pool_out[i]=pool.records_at_3a8b0a0.bytes[i].value_or(0xef);
    std::size_t o=0;
    for(const auto& x:s.records_at_3ce6f48.bytes)blocks_out[o++]=x.value_or(0xef);for(const auto& x:s.records_at_3ce6e78.bytes)blocks_out[o++]=x.value_or(0xef);
    for(const auto& x:s.block_at_3ce6d80.bytes)blocks_out[o++]=x.value_or(0xef);for(const auto& x:block.bytes)blocks_out[o++]=x.value_or(0xef);
    std::string t;for(const auto* h:{&heap,&crt})for(const auto& c:h->trace)t+=std::string("H|")+c.kind+"|"+std::to_string(c.a)+"|"+std::to_string(c.b)+"\n";
    std::snprintf(text,text_size,"%s",t.c_str());
    return 1;
}

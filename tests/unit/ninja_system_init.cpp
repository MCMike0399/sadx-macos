#include "ninja/ninja_system_init.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
// 401830 owner semantics: the Input mod profile path, the two callback registrations, the pools,
// the CRT vertex block, unknown window facts and a null DirectInput interface.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    NinjaSystemInitState s;s.pool_at_3ce71b0=0;for(auto& w:s.tables_at_389d888)w=1;
    NinjaMemoryState memory;memory.heap_at_389d628=0xffffffffu;GuestHeap heap,crt(0x00520000u,0x60000000u);NinjaFrameCallState frame;SourceBytes<0x3d0fae0,0x200> block;
    std::array<std::optional<std::uint32_t>,9> sizes;for(auto& w:sizes)w=0x100;NinjaTexturePoolState pool;NinjaTextureSelectionState sel;NinjaCallbackList callbacks;callbacks.count_at_3ce712c=0;std::optional<std::uint32_t> word_3d0f9c8;std::array<std::optional<std::uint32_t>,4> entry_words{1u,2u,3u,4u};
    std::array<std::optional<std::uint32_t>,4> clear{};std::optional<std::uint32_t> window=0x1234,instance=0x400000;X87Context fp(0x7f);std::vector<std::uint32_t> calls;
    NinjaSystemInitRuntime rt{s,{memory,heap,crt,frame,block,sizes,pool,sel,callbacks,word_3d0f9c8,entry_words,&clear,window,instance},fp,
        [&](const NinjaSystemCall& c)->NinjaSystemOwnerResult{calls.push_back(c.callee);
            std::optional<std::uint32_t> eax;if(c.callee==0x77dbb0)eax=0x1000u;else if(c.callee==0x40bb4f)eax=0x2000u;return {{GameLogicOwnerState::completed,0,{}},eax};},
        []()->std::optional<std::int64_t>{return 0x123456789LL;}};
    auto r=run_system_init_401830(rt);
    require(r&&s.size_at_3ce71a4==0x660000u&&s.pool_at_3ce71b0&&*s.pool_at_3ce71b0!=0&&s.scale_at_3ce7154==12.0F&&s.word_at_3d0f810==0x40000u,"77f240, 780ea0, 780870/7808c0");
    require(frame.vertex_buffer_at_3d128ac==0x60000000u&&s.counter_high_at_3ce7134==1u&&clear[3]==0xff000000u&&callbacks.slots_at_3d0fa40.size()==3&&callbacks.slots_at_3d0fa40[0]==0x77f110u&&callbacks.slots_at_3d0fa40[1]==0x785190u&&callbacks.slots_at_3d0fa40[2]==0x77efc0u&&entry_words[3]==0u,"78e720, 780140, 780c00, 785240 and the 77f110/785190/77efc0 registrations");
    require(s.direct_input_at_3abdc10==0x1000u&&s.mouse_at_3abdc14==0x2000u&&pool.count_at_3d0f80c==0x800u,"40bb20 and 77f610");
    require(std::count(calls.begin(),calls.end(),0x794540)==9&&std::count(calls.begin(),calls.end(),0x7928e0)==0&&std::count(calls.begin(),calls.end(),0x791d60)==0,"nine 794540 calls; the Input mod NOPs remove 7928e0 and 791d60");
    window.reset();require(run_system_init_401830(rt).required_fact==0x3d0fd30u,"unknown window");window=1;
    rt.invoke=[&](const NinjaSystemCall&)->NinjaSystemOwnerResult{return {{GameLogicOwnerState::completed,0,{}},0u};};
    require(run_system_init_401830(rt).source_boundary==0x40bb40u,"a null DirectInput interface faults at the vtable load");
    std::cout<<"System init 401830: profile path, pools, registrations, CRT block, unknown window and null DirectInput passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

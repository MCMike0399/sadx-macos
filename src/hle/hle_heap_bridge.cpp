#include "hle/hle_heap_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/ninja_memory.hpp"
#include <array>
#include <cstdio>
namespace sadx {
namespace {
std::string hex(std::uint32_t at){char b[16];std::snprintf(b,sizeof b,"%x",at);return b;}
}
std::optional<std::uint32_t> invoke_heap_method(HleProcess& p,const char* symbol,std::uint32_t slot,std::uint32_t site,const std::array<std::uint32_t,3>& args){
    auto& g=p.guest();const auto* method=p.hle().find("KERNEL32.dll",symbol);const auto target=g.u32(slot),expected=p.hle().trap(g,"KERNEL32.dll",symbol);
    if(!method||method->argument_bytes!=12||!target||!expected||*target!=*expected){g.stop(std::string("heap bridge: non-HLE ")+symbol+" target at "+hex(site));return std::nullopt;}
    const auto frame=g.esp()-16;std::array<std::uint32_t,4> old{};
    if(!g.read(frame,old.data(),sizeof old)){g.stop("heap bridge: missing call frame at "+hex(site));return std::nullopt;}
    const std::array<std::uint32_t,4> words={site+6,args[0],args[1],args[2]};
    if(!g.write(frame,words.data(),sizeof words)){g.stop("heap bridge: unwritable call frame at "+hex(site));return std::nullopt;}
    HleCall call(g,frame);const auto receipt=method->body(call);g.write(frame,old.data(),sizeof old);
    p.hle().record({"KERNEL32.dll",symbol,site+6,{args[0],args[1],args[2]},receipt});g.set_eax(receipt);
    return g.stop_reason()?std::nullopt:std::optional<std::uint32_t>(receipt);
}
std::vector<OverrideEntry> heap_overrides(HleProcess& p){
    std::vector<OverrideEntry> out;
    for(const auto entry:{0x780f60U,0x780fb0U})out.push_back({entry,entry==0x780f60?"ninja_malloc":"ninja_free",{{0x389d628,4}},[&p,entry](X86Guest& g){
        NinjaMemoryState state{g.u32(0x389d628)};const auto previous=state.heap_at_389d628;
        NinjaMemoryMethods methods{
            [&]{return invoke_heap_method(p,"HeapCreate",0x7db078,0x780f73,{0,0x800000,0});},
            [&](std::uint32_t handle,std::uint32_t size){
                // Source 780f79 stores HeapCreate's result before calling HeapAlloc.
                if(previous==0xffffffffU&&state.heap_at_389d628)g.set_u32(0x389d628,*state.heap_at_389d628);
                return invoke_heap_method(p,"HeapAlloc",0x7db074,0x780f86,{handle,0,size});
            },
            [&](std::uint32_t handle,std::uint32_t address){return invoke_heap_method(p,"HeapFree",0x7db0f8,0x780fc1,{handle,0,address});}};
        const auto arg=g.u32(g.esp()+4);
        if(!arg){g.stop("heap bridge: missing argument at "+hex(entry));return 0U;}
        const auto result=entry==0x780f60?nj_malloc_780f60(state,methods,*arg):nj_free_780fb0(state,methods,*arg);
        if(!result){if(!g.stop_reason())g.stop("heap bridge: native boundary "+hex(result.source_boundary));}
        else g.set_eax(result.eax);
        return 0U;
    }});
    return out;
}
} // namespace sadx

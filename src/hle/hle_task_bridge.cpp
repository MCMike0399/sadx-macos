#include "hle/hle_task_bridge.hpp"
#include "hle/hle_heap_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/task_work.hpp"
#include <unicorn/unicorn.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <map>
namespace sadx {
namespace {
std::string hex(std::uint32_t value){char b[16];std::snprintf(b,sizeof b,"%x",value);return b;}
struct MetadataUnavailable{};
struct TaskBridge {
    HleProcess& process;std::shared_ptr<TaskWorkArena> arena;
    std::map<std::uint32_t,TaskWorkHandle> allocations;
    std::uint32_t base=0,entry=0;
    explicit TaskBridge(HleProcess& p):process(p){}
    void bind(std::uint32_t address){
        base=address;allocations.clear();arena=std::make_shared<TaskWorkArena>(base);
        arena->set_metadata_writer([this](std::uint32_t offset,std::optional<std::uint32_t> value){
            if(!value||!process.guest().set_u32(base+offset,*value)){
                process.guest().stop("task bridge: unavailable metadata at "+hex(entry)+" ("+hex(base+offset)+")");throw MetadataUnavailable{};
            }
        });
    }
    void initialize(){
        auto& g=process.guest();entry=0x40b1a0;const auto current=g.u32(0x3abdbf0);
        if(!current){g.stop("task bridge: missing arena global at 40b1a5");return;}
        auto address=*current;
        if(!address){
            const auto heap=invoke_heap_method(process,"HeapCreate",0x7db078,0x40b1b2,{0,0,0});if(!heap)return;
            const auto block=invoke_heap_method(process,"HeapAlloc",0x7db074,0x40b1c0,{*heap,8,TaskWorkArena::usable_bytes+32});if(!block)return;
            address=*block;g.set_u32(0x3abdbf0,address);
        }
        g.set_eax(0xffffffffU);g.set_reg(UC_X86_REG_EDX,address);
        if(!address||std::uint64_t(address)+TaskWorkArena::usable_bytes+32>0x100000000ULL){g.stop("task bridge: unavailable arena at 40b1e0");return;}
        bind(address);
        // REP STOSD writes exactly 16 MiB; only the first 16 sentinel bytes
        // are initialized after it. Reinitialization preserves sentinel padding.
        if(!g.write(base,arena->source_bytes().data(),TaskWorkArena::usable_bytes)||
            !g.write(base+TaskWorkArena::usable_bytes,arena->source_bytes().data()+TaskWorkArena::usable_bytes,16)){
            g.stop("task bridge: unmapped arena at 40b1e0");return;
        }
        g.set_u32(0x3abdbe8,base);g.set_reg(UC_X86_REG_ECX,0);
    }
    bool ready(){
        auto& g=process.guest();const auto current=g.u32(0x3abdbf0),head=g.u32(0x3abdbe8);
        if(!current||!head||!*current){g.stop("task bridge: missing initialized arena at "+hex(entry));return false;}
        if(!arena||base!=*current){g.stop("task bridge: arena initialized outside native owner at "+hex(entry));return false;}
        const auto expected=arena->free_head()?base+*arena->free_head():0;
        if(*head!=expected){g.stop("task bridge: changed free head at "+hex(entry));return false;}
        for(const auto& block:arena->blocks()){
            std::array<std::uint8_t,16> bytes{};
            if(!g.read(base+block.offset,bytes.data(),bytes.size())||std::memcmp(bytes.data(),arena->source_bytes().data()+block.offset,bytes.size())){
                g.stop("task bridge: changed metadata at "+hex(entry)+" ("+hex(base+block.offset)+")");return false;
            }
        }
        return true;
    }
    std::uint32_t run(X86Guest& g,std::uint32_t at){
        entry=at;const auto value=g.u32(g.esp()+4);if(!value){g.stop("task bridge: missing argument at "+hex(at));return 0;}
        if(!*value){g.set_eax(0);return 0;}
        if(!ready())return 0;
        try{
            TaskWorkResult result;
            if(at==0x40b220){
                result=arena->allocate(*value);
                if(result.failure==TaskWorkFailure::exhausted){g.set_eax(0);return 0;}
                if(result){const auto address=base+result.work->data_offset();allocations[address]=result.work;g.set_eax(address);}
            }else{
                const auto found=allocations.find(*value);
                if(found==allocations.end()){g.stop("task bridge: unowned work at 40b31c");return 0;}
                result=arena->release(found->second);if(result)allocations.erase(found);
            }
            if(!result){g.stop("task bridge: native failure "+std::to_string(static_cast<unsigned>(result.failure))+" at "+hex(at));return 0;}
            g.set_u32(0x3abdbe8,arena->free_head()?base+*arena->free_head():0);
        }catch(const MetadataUnavailable&){}
        return 0;
    }
};
}
std::vector<OverrideEntry> task_overrides(HleProcess& p){
    auto state=std::make_shared<TaskBridge>(p);std::vector<OverrideEntry> out;
    out.push_back({0x40b1a0,"task_work_initialize",{{0x3abdbf0,4},{0x3abdbe8,4},{0x3c839fc,8}},[state](X86Guest& g){
        // Execute the retained CRT prerequisite at the original CALL depth.
        g.redirect(0x64fc70,{},0,[state](X86Guest&){state->initialize();},4);return 0U;
    }});
    for(const auto entry:{0x40b220U,0x40b310U})out.push_back({entry,entry==0x40b220?"task_work_allocate":"task_work_release",{{0x3abdbe8,4}},[state,entry](X86Guest& g){return state->run(g,entry);}});
    return out;
}
} // namespace sadx

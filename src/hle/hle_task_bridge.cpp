#include "hle/hle_task_bridge.hpp"
#include "hle/hle_heap_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/task_work.hpp"
#include "engine/task.hpp"
#include <algorithm>
#include <set>
#include <utility>
#include <unicorn/unicorn.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <map>
namespace sadx {
namespace {
std::string hex(std::uint32_t value){char b[16];std::snprintf(b,sizeof b,"%x",value);return b;}
struct MetadataUnavailable{};
struct CallbackUnavailable {unsigned token;TaskHandle task;};
template<std::size_t N>void opaque_callback(TaskScheduler&,const TaskHandle& task){throw CallbackUnavailable{N,task};}
template<std::size_t... N>auto callback_bank(std::index_sequence<N...>){return std::array<TaskCallback,sizeof...(N)>{&opaque_callback<N>...};}
const auto callbacks=callback_bank(std::make_index_sequence<1024>{});
struct NativeResource {std::uint64_t identity;};
struct TaskBridge {
    HleProcess& process;std::shared_ptr<TaskWorkArena> arena;
    std::map<std::uint32_t,TaskWorkHandle> allocations;
    std::uint32_t base=0,entry=0,pool=0;
    std::unique_ptr<TaskScheduler> scheduler;
    std::map<std::uint32_t,TaskCallback> functions;
    std::map<TaskCallback,std::uint32_t> source_functions;
    std::map<std::uint32_t,std::shared_ptr<void>> resources;
    std::map<const void*,std::uint32_t> source_resources;
    std::map<const Task::ObjectControl*,std::uint32_t> controls;
    std::vector<std::array<std::uint32_t,13>> serialized;
    TaskCallback callback(std::uint32_t address){
        if(!address)return nullptr;
        if(const auto i=functions.find(address);i!=functions.end())return i->second;
        TaskCallback fn=nullptr;
        if(address==0x51f680)fn=&TaskScheduler::source_no_op;else if(address==0x40b570)fn=&TaskScheduler::destroy_callback;else if(address==0x40b7e0)fn=&TaskScheduler::free_children_callback;
        else {unsigned n=0;for(const auto& [f,raw]:source_functions){(void)raw;if(f!=&TaskScheduler::source_no_op&&f!=&TaskScheduler::destroy_callback&&f!=&TaskScheduler::free_children_callback)++n;}if(n>=callbacks.size()){process.guest().stop("task bridge: callback identities exhausted at "+hex(entry));throw MetadataUnavailable{};}fn=callbacks[n];}
        functions[address]=fn;source_functions[fn]=address;return fn;
    }
    std::uint32_t raw(TaskCallback fn){if(!fn)return 0;if(fn==&TaskScheduler::source_no_op)return 0x51f680;if(fn==&TaskScheduler::destroy_callback)return 0x40b570;if(fn==&TaskScheduler::free_children_callback)return 0x40b7e0;return source_functions.at(fn);}
    std::shared_ptr<void> resource(std::uint32_t value){if(!value)return {};if(!resources.count(value)){auto r=std::make_shared<NativeResource>(NativeResource{resources.size()+1});resources[value]=r;source_resources[r.get()]=value;}return resources[value];}
    std::uint32_t raw(const std::shared_ptr<void>& value){return value?source_resources.at(value.get()):0;}
    std::uint32_t address(std::optional<std::uint16_t> slot)const{return slot?pool+52U*(*slot):0;}
    std::optional<std::uint16_t> slot(std::uint32_t address_value){if(!address_value)return {};if(address_value<pool||(address_value-pool)%52||(address_value-pool)/52>=4096){process.guest().stop("task bridge: non-pool task at "+hex(entry));throw MetadataUnavailable{};}return static_cast<std::uint16_t>((address_value-pool)/52);}
    TaskWorkHandle work(std::uint32_t address_value){if(!address_value)return {};const auto i=allocations.find(address_value);if(i==allocations.end()||i->second->released()){process.guest().stop("task bridge: unowned work field at "+hex(entry));throw MetadataUnavailable{};}return i->second;}
    std::uint32_t address(const TaskWorkHandle& w){return w?base+w->data_offset():0;}
    std::uint32_t word(std::uint32_t at){const auto v=process.guest().u32(at);if(!v){process.guest().stop("task bridge: missing field "+hex(at)+" at "+hex(entry));throw MetadataUnavailable{};}return *v;}

    explicit TaskBridge(HleProcess& p):process(p){}
    void bind(std::uint32_t address){
        base=address;pool=0;scheduler.reset();allocations.clear();arena=std::make_shared<TaskWorkArena>(base);
        arena->set_metadata_writer([this](std::uint32_t offset,std::optional<std::uint32_t> value){
            if(!value||!process.guest().set_u32(base+offset,*value)){
                process.guest().stop("task bridge: unavailable metadata at "+hex(entry)+" ("+hex(base+offset)+")");throw MetadataUnavailable{};
            }
        });
        arena->set_payload_transport([this](std::uint32_t at,void* out,std::uint32_t bytes){return process.guest().read(base+at,out,bytes);},[this](std::uint32_t at,const void* in,std::uint32_t bytes){return process.guest().write(base+at,in,bytes);});
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
    void flush_work(){process.guest().set_u32(0x3abdbe8,arena->free_head()?base+*arena->free_head():0);}
    void initialize_pool(){
        entry=0x40b460;if(!ready())return;
        try{scheduler=std::make_unique<TaskScheduler>();const auto result=scheduler->initialize_source_pool(arena,resource(word(0x38f6e08)));
            if(!result){if(result.work_failure==TaskWorkFailure::exhausted){pool=0;process.guest().set_u32(0x3abdbe4,0);process.guest().set_u32(0x3abdbec,0);process.guest().set_u32(0x3abdbf8,0);process.guest().set_eax(0);flush_work();scheduler.reset();return;}process.guest().stop("task bridge: pool native boundary "+hex(result.source_boundary));return;}
            pool=address(scheduler->source_pool_work());process.guest().set_u32(0x3abdbe4,pool);allocations[pool]=scheduler->source_pool_work();serialized.assign(4096,{});
            const auto current=word(0x3abdbf4);export_graph();process.guest().set_u32(0x3abdbf4,current);process.guest().set_eax(0);process.guest().set_reg(UC_X86_REG_EDX,0);process.guest().set_reg(UC_X86_REG_ECX,0);flush_work();
        }catch(const MetadataUnavailable&){}
    }
    void import_graph(){
        if(!scheduler||!pool){process.guest().stop("task bridge: pool unavailable at "+hex(entry));throw MetadataUnavailable{};}
        auto& g=process.guest();if(!g.read(pool,serialized.data(),4096*52)){g.stop("task bridge: unreadable pool at "+hex(entry));throw MetadataUnavailable{};}
        TaskGraphState state;std::set<std::uint16_t> free;auto next=slot(word(0x3abdbec));std::optional<std::uint16_t> previous;
        while(next){if(!free.insert(*next).second||slot(serialized[*next][1])!=previous){g.stop("task bridge: corrupt task free list at "+hex(entry));throw MetadataUnavailable{};}state.free_slots.push_back(*next);previous=next;next=slot(serialized[*next][0]);}
        std::reverse(state.free_slots.begin(),state.free_slots.end());
        for(unsigned k=0;k<8;++k)state.heads[k]=slot(word(0x3abdbc4+4*k));
        std::set<std::uint16_t> linked;
        std::function<void(std::optional<std::uint16_t>)> visit=[&](std::optional<std::uint16_t> h){while(h){if(free.count(*h)||!linked.insert(*h).second){g.stop("task bridge: corrupt active graph at "+hex(entry));throw MetadataUnavailable{};}visit(slot(serialized[*h][3]));h=slot(serialized[*h][0]);}};
        for(const auto& h:state.heads)visit(h);
        for(unsigned k=0;k<4096;++k)if(!free.count(k)&&!linked.count(k)&&!serialized[k][2])visit(slot(serialized[k][3]));
        controls.clear();std::vector<std::pair<std::shared_ptr<Task::ObjectControl>,std::optional<std::uint16_t>>> control_tasks;
        for(unsigned k=0;k<4096;++k)if(!free.count(k)){
            const auto& words=serialized[k];TaskGraphNode n{};n.slot=k;n.linked=linked.count(k)!=0;n.next=slot(words[0]);n.previous=slot(words[1]);n.parent=slot(words[2]);n.child=slot(words[3]);
            n.exec=callback(words[4]);n.display=callback(words[5]);n.destructor=callback(words[6]);
            n.primary=work(words[8]);n.motion=work(words[9]);n.force=work(words[10]);n.auxiliary=work(words[11]);n.field30=resource(words[12]);
            if(words[7]){auto control=std::make_shared<Task::ObjectControl>();if(!g.read(words[7]+2,&control->flags_at_2,1)){g.stop("task bridge: missing control flag at "+hex(entry));throw MetadataUnavailable{};}control_tasks.push_back({control,slot(word(words[7]+4))});controls[control.get()]=words[7];n.control=control;}
            state.nodes.push_back(std::move(n));
        }
        state.context.current_function=callback(word(0x3abdbf4));state.context.field30=resource(word(0x3abdbf8));
        scheduler->set_default_field30(resource(word(0x38f6e08)));
        const auto result=scheduler->restore_source_graph(state);if(!result){g.stop("task bridge: invalid native graph at "+hex(entry));throw MetadataUnavailable{};}
        for(auto& [control,target]:control_tasks){control->task_at_4=target?scheduler->source_task(*target):TaskHandle{};if(target&&!control->task_at_4){g.stop("task bridge: unbound control task at "+hex(entry));throw MetadataUnavailable{};}}
    }
    void export_graph(){
        auto& g=process.guest();const auto state=scheduler->source_graph();if(state.failure!=TaskFailure::none){g.stop("task bridge: incomplete native graph at "+hex(entry));throw MetadataUnavailable{};}
        for(const auto& n:state.nodes){auto& words=serialized[n.slot];words[0]=address(n.next);words[1]=address(n.previous);words[2]=address(n.parent);words[3]=address(n.child);
            words[4]=raw(n.exec);words[5]=raw(n.display);words[6]=raw(n.destructor);words[7]=n.control?controls.at(n.control.get()):0;
            const TaskWorkHandle handles[4]={n.primary,n.motion,n.force,n.auxiliary};for(unsigned k=0;k<4;++k){words[8+k]=address(handles[k]);if(handles[k])allocations[words[8+k]]=handles[k];}words[12]=raw(n.field30);
        }
        for(unsigned k=0;k<state.free_slots.size();++k){auto& words=serialized[state.free_slots[k]];words[0]=k?address(state.free_slots[k-1]):0;words[1]=k+1<state.free_slots.size()?address(state.free_slots[k+1]):0;}
        // Serialize only changed words so guest-owned fields/partial callback stores
        // are not overwritten by an unchanged native snapshot.
        for(unsigned k=0;k<4096;++k)for(unsigned f=0;f<13;++f){const auto at=pool+52*k+4*f;if(g.u32(at)!=serialized[k][f])g.set_u32(at,serialized[k][f]);}
        for(unsigned k=0;k<8;++k)g.set_u32(0x3abdbc4+4*k,address(state.heads[k]));
        g.set_u32(0x3abdbec,state.free_slots.empty()?0:address(state.free_slots.back()));
        g.set_u32(0x3abdbf4,raw(state.context.current_function));g.set_u32(0x3abdbf8,raw(state.context.field30));
    }
    std::uint32_t graph(X86Guest& g,std::uint32_t at){
        entry=at;
        try{
        const auto level=at==0x40b030?static_cast<int>(g.reg(UC_X86_REG_EDX)):at==0x40b860?static_cast<int>(word(g.esp()+8)):7;
        if(at==0x40b030||at==0x40b860){const auto head=g.u32(0x3abdbec);if((head&&!*head)||level<0||level>=9){g.set_eax(0);return 0;}}
        if(!ready())return 0;
        import_graph();TaskResult result;
            if(at==0x40b6c0){const auto target=word(g.esp()+4);if(!target){g.set_eax(0);return 0;}if(target<pool||(target-pool)%52||(target-pool)/52>=4096){g.stop("task bridge: unobserved task at 40b6c6");return 0;}}
            if(at==0x40b030)result=scheduler->allocate_task(static_cast<int>(g.reg(UC_X86_REG_EDX)),callback(g.reg(UC_X86_REG_EDI)));
            else if(at==0x40b860)result=scheduler->create_element(word(g.esp()+4),static_cast<int>(word(g.esp()+8)),callback(word(g.esp()+12)));
            else {const auto target_word=word(g.esp()+(at==0x40b940?12:4));const bool in_pool=target_word>=pool&&(target_word-pool)%52==0&&(target_word-pool)/52<4096;const auto target=in_pool?slot(target_word):std::nullopt;const auto parent=target?scheduler->source_task(*target):TaskHandle{};
                if(at==0x40b940){if(!parent){result=scheduler->create_element(word(g.esp()+4),7,callback(word(g.esp()+8)));export_graph();flush_work();if(result){g.set_eax(address(result.task->source_slot()));g.stop("task bridge: unobserved parent at 40b95c");}else if(result.failure==TaskFailure::source_pool_exhausted)g.set_eax(0);else g.stop("task bridge: graph native boundary "+hex(result.source_boundary));return 0;}result=scheduler->create_child(word(g.esp()+4),callback(word(g.esp()+8)),parent);}
                else {const bool active=scheduler->active_member(parent);g.set_eax(active?1:0);if(active)result=scheduler->free_task(parent);}}
            const bool exhausted=!result&&result.work_failure==TaskWorkFailure::exhausted&&result.task&&result.task->retired();
            if(exhausted){auto& words=serialized[*result.task->source_slot()];for(unsigned f=2;f<12;++f)words[f]=0;words[12]=raw(result.task->field30);}
            export_graph();flush_work();if(exhausted){g.set_eax(0);return 0;}if(!result){if(result.failure==TaskFailure::source_pool_exhausted||result.failure==TaskFailure::invalid_list_index){g.set_eax(0);return 0;}g.stop("task bridge: graph native boundary "+hex(result.source_boundary));return 0;}
            if(at!=0x40b6c0)g.set_eax(result.task?address(result.task->source_slot()):0);
        }catch(const CallbackUnavailable& call){export_graph();flush_work();g.stop("task bridge: unported callback "+hex(raw(callbacks[call.token]))+" at 40b590");}
        catch(const MetadataUnavailable&){}
        return 0;
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
    out.push_back({0x40b460,"task_pool_initialize",{{0x3abdbe4,4},{0x3abdbec,4},{0x3abdbc4,32},{0x3abdbe8,4}},[state](X86Guest& g){g.redirect(0x51a740,{},0,[state](X86Guest&){state->initialize_pool();},8);return 0U;}});
    for(const auto at:{0x40b030U,0x40b860U,0x40b940U,0x40b6c0U})out.push_back({at,at==0x40b030?"task_allocate":at==0x40b860?"task_create_element":at==0x40b940?"task_create_child":"task_free",{{0x3abdbec,4},{0x3abdbc4,32},{0x3abdbe8,4}},[state,at](X86Guest& g){return state->graph(g,at);}});
    for(const auto entry:{0x40b220U,0x40b310U})out.push_back({entry,entry==0x40b220?"task_work_allocate":"task_work_release",{{0x3abdbe8,4}},[state,entry](X86Guest& g){return state->run(g,entry);}});
    return out;
}
} // namespace sadx

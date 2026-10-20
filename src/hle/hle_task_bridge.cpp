#include "hle/hle_task_bridge.hpp"
#include "hle/hle_heap_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/task_work.hpp"
#include "engine/task.hpp"
#include <algorithm>
#include <bitset>
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
    // The pinned input mod owns this CRT allocation. Its destructor clears the
    // task's UnknownA_ptr before deleting the timer; it is not arena force work.
    struct InputRumbleTimer { std::uint32_t address; };
    std::map<unsigned, InputRumbleTimer> input_rumble_timers;
    bool is_input_rumble_timer(const std::array<std::uint32_t,13>& words) const {
        const auto* input = process.modules().by_name("sadx-input-mod.dll");
        return input && words[6] == input->base + 0x8f20 &&
               (words[4] == input->base + 0x8e00 || words[4] == 0x40b570);
    }
    bool valid_input_rumble_timer(std::uint32_t pointer) const {
        const auto bytes = process.memory().size(pointer);
        std::array<std::uint8_t,8> timer{};
        return bytes && *bytes >= timer.size() &&
               process.guest().read(pointer,timer.data(),timer.size()) &&
               timer[0] <= 1 && timer[1] < 8 && (timer[2] == 1 || timer[2] == 2);
    }
    std::unique_ptr<TaskScheduler> scheduler;
    std::map<std::uint32_t,TaskCallback> functions;
    std::map<TaskCallback,std::uint32_t> source_functions;
    std::map<std::uint32_t,std::shared_ptr<void>> resources;
    std::map<const void*,std::uint32_t> source_resources;
    std::map<const Task::ObjectControl*,std::uint32_t> controls;
    std::vector<std::array<std::uint32_t,13>> serialized;
    std::vector<std::array<std::uint32_t,13>> guest_snapshot;
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
    TaskWorkHandle work(std::uint32_t address_value, unsigned task_slot, unsigned field) {
        if (!address_value)
            return {};
        const auto found = allocations.find(address_value);
        if (found == allocations.end() || found->second->released()) {
            process.guest().stop(
                "task bridge: unowned work field " + hex(address_value) + " at " + hex(entry) +
                " (arena " + hex(base) + ", slot " + std::to_string(task_slot) + ", field +" +
                hex(field) + ", exec " + hex(serialized[task_slot][4]) + ", current " +
                hex(process.guest().u32(0x3abdbf4).value_or(0)) + ")");
            throw MetadataUnavailable{};
        }
        return found->second;
    }
    std::uint32_t address(const TaskWorkHandle& w){return w?base+w->data_offset():0;}
    std::uint32_t word(std::uint32_t at){const auto v=process.guest().u32(at);if(!v){process.guest().stop("task bridge: missing field "+hex(at)+" at "+hex(entry));throw MetadataUnavailable{};}return *v;}

    explicit TaskBridge(HleProcess& p):process(p){}
    void bind(std::uint32_t address){
        base=address;pool=0;scheduler.reset();allocations.clear();input_rumble_timers.clear();arena=std::make_shared<TaskWorkArena>(base);
        arena->set_metadata_writer([this](std::uint32_t offset,std::optional<std::uint32_t> value){
            if(!value||!process.guest().set_u32(base+offset,*value)){
                process.guest().stop("task bridge: unavailable metadata at "+hex(entry)+" ("+hex(base+offset)+")");throw MetadataUnavailable{};
            }
        });
        arena->set_payload_transport([this](std::uint32_t at,void* out,std::uint32_t bytes){return process.guest().read(base+at,out,bytes);},[this](std::uint32_t at,const void* in,std::uint32_t bytes){return process.guest().write(base+at,in,bytes);});
        arena->set_reference_resolver([this](std::uint32_t value){const auto i=allocations.find(value);if(i==allocations.end())return TaskWorkReferenceResult{TaskWorkFailure::unbound_reference,{},0,{}};return TaskWorkReferenceResult{TaskWorkFailure::none,i->second,0,i->second->data_offset()};});
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
        try{input_rumble_timers.clear();scheduler=std::make_unique<TaskScheduler>();const auto result=scheduler->initialize_source_pool(arena,resource(word(0x38f6e08)));
            if(!result){if(result.work_failure==TaskWorkFailure::exhausted){pool=0;process.guest().set_u32(0x3abdbe4,0);process.guest().set_u32(0x3abdbec,0);process.guest().set_u32(0x3abdbf8,0);process.guest().set_eax(0);flush_work();scheduler.reset();return;}process.guest().stop("task bridge: pool native boundary "+hex(result.source_boundary));return;}
            pool=address(scheduler->source_pool_work());process.guest().set_u32(0x3abdbe4,pool);allocations[pool]=scheduler->source_pool_work();serialized.assign(4096,{});
            const auto current=word(0x3abdbf4);export_graph();process.guest().set_u32(0x3abdbf4,current);process.guest().set_eax(0);process.guest().set_reg(UC_X86_REG_EDX,0);process.guest().set_reg(UC_X86_REG_ECX,0);flush_work();
        }catch(const MetadataUnavailable&){}
    }
    void import_graph(){
        if(!scheduler||!pool){process.guest().stop("task bridge: pool unavailable at "+hex(entry));throw MetadataUnavailable{};}
        auto& g=process.guest();if(!g.read(pool,serialized.data(),4096*52)){g.stop("task bridge: unreadable pool at "+hex(entry));throw MetadataUnavailable{};}
        TaskGraphState state;std::bitset<4096> free;auto next=slot(word(0x3abdbec));std::optional<std::uint16_t> previous;
        while(next){if(free.test(*next)||slot(serialized[*next][1])!=previous){g.stop("task bridge: corrupt task free list at "+hex(entry));throw MetadataUnavailable{};}free.set(*next);state.free_slots.push_back(*next);previous=next;next=slot(serialized[*next][0]);}
        std::reverse(state.free_slots.begin(),state.free_slots.end());
        for(unsigned k=0;k<8;++k)state.heads[k]=slot(word(0x3abdbc4+4*k));
        // Membership is by bounded pool slot; traversal order stays in free_slots.
        std::bitset<4096> linked;
        std::function<void(std::optional<std::uint16_t>)> visit=[&](std::optional<std::uint16_t> h){while(h){if(free.test(*h)||linked.test(*h)){g.stop("task bridge: corrupt active graph at "+hex(entry));throw MetadataUnavailable{};}linked.set(*h);visit(slot(serialized[*h][3]));h=slot(serialized[*h][0]);}};
        for(const auto& h:state.heads)visit(h);
        for(unsigned k=0;k<4096;++k)if(!free.test(k)&&!linked.test(k)&&!serialized[k][2])visit(slot(serialized[k][3]));
        std::map<unsigned, InputRumbleTimer> imported_rumble_timers;
        controls.clear();std::vector<std::pair<std::shared_ptr<Task::ObjectControl>,std::optional<std::uint16_t>>> control_tasks;
        for(unsigned k=0;k<4096;++k)if(!free.test(k)){
            const auto& words=serialized[k];TaskGraphNode n{};n.slot=k;n.linked=linked.test(k)!=0;n.next=slot(words[0]);n.previous=slot(words[1]);n.parent=slot(words[2]);n.child=slot(words[3]);
            n.exec=callback(words[4]);n.display=callback(words[5]);n.destructor=callback(words[6]);
            n.primary=work(words[8],k,32);n.motion=work(words[9],k,36);
            if (words[10] && is_input_rumble_timer(words) && valid_input_rumble_timer(words[10]))
                imported_rumble_timers.emplace(k,InputRumbleTimer{words[10]});
            else
                n.force=work(words[10],k,40);
            n.auxiliary=work(words[11],k,44);n.field30=resource(words[12]);
            if(words[7]){auto control=std::make_shared<Task::ObjectControl>();if(!g.read(words[7]+2,&control->flags_at_2,1)){g.stop("task bridge: missing control flag at "+hex(entry));throw MetadataUnavailable{};}control_tasks.push_back({control,slot(word(words[7]+4))});controls[control.get()]=words[7];n.control=control;}
            state.nodes.push_back(std::move(n));
        }
        state.context.current_function=callback(word(0x3abdbf4));state.context.field30=resource(word(0x3abdbf8));
        scheduler->set_default_field30(resource(word(0x38f6e08)));
        const auto result=scheduler->restore_source_graph(state);if(!result){g.stop("task bridge: invalid native graph at "+hex(entry));throw MetadataUnavailable{};}
        input_rumble_timers=std::move(imported_rumble_timers);
        for(auto& [control,target]:control_tasks){control->task_at_4=target?scheduler->source_task(*target):TaskHandle{};if(target&&!control->task_at_4){g.stop("task bridge: unbound control task at "+hex(entry));throw MetadataUnavailable{};}}
    }
    void export_graph(){
        auto& g=process.guest();const auto state=scheduler->source_graph();if(state.failure!=TaskFailure::none){g.stop("task bridge: incomplete native graph at "+hex(entry));throw MetadataUnavailable{};}
        for(const auto& n:state.nodes){auto& words=serialized[n.slot];words[0]=address(n.next);words[1]=address(n.previous);words[2]=address(n.parent);words[3]=address(n.child);
            words[4]=raw(n.exec);words[5]=raw(n.display);words[6]=raw(n.destructor);words[7]=n.control?controls.at(n.control.get()):0;
            const TaskWorkHandle handles[4]={n.primary,n.motion,n.force,n.auxiliary};for(unsigned k=0;k<4;++k){words[8+k]=address(handles[k]);if(handles[k])allocations[words[8+k]]=handles[k];}
            if (const auto timer=input_rumble_timers.find(n.slot); timer!=input_rumble_timers.end())
                words[10]=timer->second.address;
            words[12]=raw(n.field30);
        }
        for(unsigned k=0;k<state.free_slots.size();++k){auto& words=serialized[state.free_slots[k]];words[0]=k?address(state.free_slots[k-1]):0;words[1]=k+1<state.free_slots.size()?address(state.free_slots[k+1]):0;}
        // Compare the pool in one read: no guest code runs between these independent
        // word stores. Retain scalar writes and their order so unchanged guest-owned
        // fields and partial callback stores are never republished from a stale snapshot.
        guest_snapshot.resize(serialized.size());
        const bool complete=g.read(pool,guest_snapshot.data(),guest_snapshot.size()*sizeof(guest_snapshot.front()));
        for(unsigned k=0;k<4096;++k){
            // Most callbacks leave most rows unchanged. Compare all their bytes
            // together, retaining scalar field order for every changed row.
            if (complete && std::memcmp(guest_snapshot[k].data(), serialized[k].data(),
                                        sizeof(serialized[k])) == 0)
                continue;
            for(unsigned f=0;f<13;++f){
            const auto at=pool+52*k+4*f;
            // An incomplete read keeps the old scalar path and its failure behavior.
            const bool changed=complete?guest_snapshot[k][f]!=serialized[k][f]:g.u32(at)!=serialized[k][f];
            if(changed)g.set_u32(at,serialized[k][f]);
            }
        }
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
    struct LifetimeRun {
        TaskLifetimeState cursor;
        std::uint32_t owner=0,source_pool=0;
        std::array<std::uint32_t,4> saved{}; // EBP, ESI, EBX, EDI
        bool children=false;
        std::optional<unsigned> input_rumble_destructor;
    };
    void lifetime_next(const std::shared_ptr<LifetimeRun>& run,bool returned=false){
        auto& g=process.guest();entry=run->owner;
        try{
            if (returned && pool == run->source_pool && run->input_rumble_destructor) {
                const auto slot_index=*run->input_rumble_destructor;
                if (word(pool+52*slot_index+40)!=0) {
                    g.stop("task bridge: input rumble destructor did not clear work at 40b596");
                    return;
                }
                run->input_rumble_destructor.reset();
            }
            if(returned){if(pool!=run->source_pool){g.stop("task bridge: changed lifetime pool at 40b596");return;}if(!ready())return;import_graph();}
            auto before=scheduler->source_graph();std::vector<std::pair<std::shared_ptr<Task::ObjectControl>,std::uint32_t>> borrowed;
            for(const auto& n:before.nodes)if(n.control)borrowed.push_back({n.control,controls.at(n.control.get())});
            const auto call=scheduler->advance_lifetime(run->cursor);
            if(call.task&&call.task->retired()){auto& row=serialized[*call.task->source_slot()];for(unsigned f=2;f<12;++f)row[f]=0;row[12]=raw(call.task->field30);}
            for(const auto& [control,address_value]:borrowed){g.write(address_value+2,&control->flags_at_2,1);g.set_u32(address_value+4,control->task_at_4?address(control->task_at_4->source_slot()):0);}
            export_graph();flush_work();
            if(!call.result){g.stop("task bridge: lifetime boundary "+hex(call.result.source_boundary?call.result.source_boundary:run->owner));return;}
            if(call.completed){g.set_reg(UC_X86_REG_ESI,run->saved[1]);if(!run->children){g.set_reg(UC_X86_REG_EBP,run->saved[0]);g.set_reg(UC_X86_REG_EBX,run->saved[2]);g.set_reg(UC_X86_REG_EDI,run->saved[3]);}return;}
            const auto task=address(call.task->source_slot());std::uint32_t target=0,back=0,reserve=0;const auto sp=g.esp();
            if(run->children){target=call.callee==TaskLifetimeCallee::run_children?0x40b420:0x40b570;back=call.callee==TaskLifetimeCallee::run_children?0x40b7f4:0x40b826;reserve=8;g.set_u32(sp-4,run->saved[1]);g.set_reg(UC_X86_REG_ESI,address(run->cursor.task->source_slot()));g.set_eax(task);}
            else {
                g.set_u32(sp-4,run->saved[1]);g.set_u32(sp-8,run->saved[0]);g.set_reg(UC_X86_REG_ESI,address(run->cursor.task->source_slot()));g.set_reg(UC_X86_REG_EBP,0);
                if(call.callee==TaskLifetimeCallee::destructor){
                    if (input_rumble_timers.count(*call.task->source_slot()))
                        run->input_rumble_destructor=*call.task->source_slot();
                    target=raw(call.callback);back=0x40b593;reserve=12;g.set_eax(target);}
                else {target=0x40b7e0;back=0x40b5b1;reserve=20;g.set_u32(sp-12,run->saved[2]);g.set_u32(sp-16,run->saved[3]);g.set_reg(UC_X86_REG_EBX,run->cursor.previous?address(run->cursor.previous->source_slot()):0);g.set_reg(UC_X86_REG_EDI,run->cursor.next?address(run->cursor.next->source_slot()):0);}
            }
            g.redirect(target,{task},0,[this,run](X86Guest&){lifetime_next(run,true);},reserve,back);
        }catch(const MetadataUnavailable&){}
    }
    std::uint32_t lifetime(X86Guest& g,std::uint32_t at){
        entry=at;if(at==0x40b570&&g.u32(g.esp()+4)==0){g.set_eax(0);return 0;}if(!ready())return 0;
        try{import_graph();const auto id=slot(word(g.esp()+4));const auto task=id?scheduler->source_task(*id):TaskHandle{};
            // DestroyTask calls CheckTask before reading the destructor or work.
            // Known free pool rows and detached tasks are inactive, so retained
            // cleanup pointers after another owner destroyed a task are no-ops.
            // slot() still rejects foreign identities before this guard.
            if(at==0x40b570&&!scheduler->active_member(task)){g.set_eax(0);return 0;}
            if(!task){g.stop("task bridge: unobserved lifetime task "+hex(word(g.esp()+4))+" at "+hex(at));return 0;}
            auto run=std::make_shared<LifetimeRun>();run->owner=at;run->source_pool=pool;run->children=at==0x40b7e0;
            const int regs[4]={UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EBX,UC_X86_REG_EDI};for(unsigned k=0;k<4;++k)run->saved[k]=g.reg(regs[k]);
            run->cursor=run->children?scheduler->begin_free_children(task):scheduler->begin_destruction(task);lifetime_next(run);
        }catch(const MetadataUnavailable&){}
        return 0;
    }
    struct DispatchRun {
        TaskDispatchState cursor;
        std::uint32_t owner=0,source_pool=0;
        std::array<std::uint32_t,4> saved{}; // EBP, ESI, EBX, EDI
        int position=0;
        bool all=false,children=false;
    };
    void dispatch_next(const std::shared_ptr<DispatchRun>& run,bool returned=false){
        auto& g=process.guest();entry=run->owner;
        try{
            if(returned){if(pool!=run->source_pool){g.stop("task bridge: changed snapshot pool at "+hex(run->children?0x40b433:0x40b0e0));return;}if(!ready())return;import_graph();}
            for(;;){
                const auto call=scheduler->advance_dispatch(run->cursor);export_graph();
                if(!call.result){g.stop("task bridge: dispatch boundary "+hex(call.result.source_boundary?call.result.source_boundary:run->children?0x40b43cU:0x40b0e0U));return;}
                if(call.completed){
                    if(run->all&&++run->position<8){run->cursor=scheduler->begin_index_dispatch(run->position-1);continue;}
                    g.set_reg(UC_X86_REG_ESI,run->saved[1]);if(!run->children){g.set_reg(UC_X86_REG_EBP,run->saved[0]);g.set_reg(UC_X86_REG_EBX,run->saved[2]);g.set_reg(UC_X86_REG_EDI,run->saved[3]);}
                    return;
                }
                const auto task=address(call.task->source_slot()),next=run->cursor.next?address(run->cursor.next->source_slot()):0,fn=raw(call.callback),field30=raw(call.task->field30);
                const auto sp=g.esp();unsigned reserve=run->children?8U:run->all?0x20U:0x14U;
                if(run->children){g.set_u32(sp-4,run->saved[1]);g.set_eax(task);g.set_reg(UC_X86_REG_ECX,field30);g.set_reg(UC_X86_REG_ESI,next);}
                else {
                    auto top=sp;
                    if(run->all){g.set_u32(sp-4,run->saved[1]);g.set_u32(sp-8,run->position==0?8U:static_cast<unsigned>(run->position-1));g.set_u32(sp-12,run->position==0?0x40b178:0x40b186);top-=12;}
                    g.set_u32(top-4,run->saved[0]);g.set_u32(top-8,run->all&&run->position?static_cast<unsigned>(run->position-1):run->saved[1]);g.set_u32(top-12,run->saved[2]);g.set_u32(top-16,run->saved[3]);
                    g.set_reg(UC_X86_REG_EBP,0);g.set_reg(UC_X86_REG_ESI,task);g.set_reg(UC_X86_REG_EBX,next);g.set_reg(UC_X86_REG_EDI,fn);g.set_eax(field30);
                }
                g.redirect(fn,{task},0,[this,run](X86Guest&){dispatch_next(run,true);},reserve,run->children?0x40b43fU:0x40b10bU);
                return;
            }
        }catch(const MetadataUnavailable&){}
    }
    std::uint32_t dispatch(X86Guest& g,std::uint32_t at){
        entry=at;if(!ready())return 0;
        try{import_graph();auto run=std::make_shared<DispatchRun>();run->owner=at;run->source_pool=pool;run->all=at==0x40b170;run->children=at==0x40b420;
            const int regs[4]={UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EBX,UC_X86_REG_EDI};for(unsigned k=0;k<4;++k)run->saved[k]=g.reg(regs[k]);
            if(run->children){const auto parent=slot(word(g.esp()+4));const auto handle=parent?scheduler->source_task(*parent):TaskHandle{};if(!handle){g.stop("task bridge: unobserved parent at 40b424");return 0;}run->cursor=scheduler->begin_child_dispatch(handle);}
            else run->cursor=scheduler->begin_index_dispatch(run->all?8:static_cast<int>(word(g.esp()+4)));
            dispatch_next(run);
        }catch(const MetadataUnavailable&){}
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
    for(const auto at:{0x40b570U,0x40b7e0U})out.push_back({at,at==0x40b570?"task_destroy":"task_free_children",{{0x3abdbec,4},{0x3abdbc4,32},{0x3abdbe8,4}},[state,at](X86Guest& g){return state->lifetime(g,at);}});
    for(const auto at:{0x40b0c0U,0x40b170U,0x40b420U})out.push_back({at,at==0x40b0c0?"task_dispatch_index":at==0x40b170?"task_dispatch_all":"task_dispatch_children",{{0x3abdbf4,8}},[state,at](X86Guest& g){return state->dispatch(g,at);}});
    for(const auto entry:{0x40b220U,0x40b310U})out.push_back({entry,entry==0x40b220?"task_work_allocate":"task_work_release",{{0x3abdbe8,4}},[state,entry](X86Guest& g){return state->run(g,entry);}});
    return out;
}
} // namespace sadx
